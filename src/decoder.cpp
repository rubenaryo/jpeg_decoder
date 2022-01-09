/*--------------------------------------------------------------------------
File:   decoder.cpp
Date:   2021/12/25
Author: kaiyen
---------------------------------------------------------------------------*/
#include "decoder.h"

#include "dct_utils.h"
#include "huffman.h"
#include "print_utils.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Decode Context. Holds working state and algorithm params.
static decode_context_t ctx;

// JFIF Version
static struct
{
  unsigned char major;
  unsigned char minor;
} jfif_ver;

static void init_decode_ctx()
{
#if 1
  ctx.extension_data = NULL;

  memset(&ctx.huffman_tables_luma, 0, sizeof(void*)*HUFF_TABLES_PER_CHANNEL_TYPE);
  memset(&ctx.huffman_tables_chroma, 0, sizeof(void*)*HUFF_TABLES_PER_CHANNEL_TYPE);

  ctx.components = NULL;

  memset(&ctx.luma_q_table, 0, sizeof(unsigned short)*QUANT_TABLE_SIZE);
  memset(&ctx.chrm_q_table, 0, sizeof(unsigned short)*QUANT_TABLE_SIZE);

  ctx.x_length = ctx.y_length = 0;

  ctx.x_density = ctx.y_density = 0;

  ctx.density_units = ctx.bits_per_sample = ctx.num_components = 0;
#else
  memset(&ctx, 0, sizeof(decode_context_t));
#endif
}

// Returns the segment length from the buffer's next two bytes and prints it out.
unsigned short get_segment_len(unsigned char* img_buf)
{
  if (img_buf == NULL)
    return 0;

  const unsigned short segment_len = get_short(img_buf);
  printf("(Segment Length: %d)...\n", segment_len);

  return segment_len;
}

static unsigned short process_func_start_of_image(unsigned char* img_buf)
{
  // The start of image marker doesn't have a length after it and is 0 length anyway. No-op.
  printf("(Segment Length: 0)...\n");
  return 0;
}

static unsigned short process_func_app_segment_0(unsigned char* img_buf)
{
  unsigned short segment_len = get_segment_len(img_buf);

  // Initialize the decode context.
  init_decode_ctx();

  // App0 offsets
  static const unsigned char VERSION_MAJOR = sizeof(unsigned short) + (sizeof(unsigned char) * 5);
  static const unsigned char VERSION_MINOR = VERSION_MAJOR + sizeof(unsigned char);
  static const unsigned char DENSITY_UNITS = VERSION_MINOR + sizeof(unsigned char);
  static const unsigned char DENSITY_DIM_X = DENSITY_UNITS + sizeof(unsigned char);
  static const unsigned char DENSITY_DIM_Y = DENSITY_DIM_X + sizeof(unsigned short);

  jfif_ver.major = img_buf[VERSION_MAJOR];
  jfif_ver.minor = img_buf[VERSION_MINOR];

  ctx.density_units = img_buf[DENSITY_UNITS];

  ctx.x_density = get_short(&img_buf[DENSITY_DIM_X]);
  ctx.y_density = get_short(&img_buf[DENSITY_DIM_Y]);

  print_jpeg_header(&ctx, jfif_ver.major, jfif_ver.minor);

  // Ignore thumbnail crap for now.
  return segment_len;
}

static unsigned short process_func_quant_table(unsigned char* img_buf)
{
  unsigned short segment_len = get_segment_len(img_buf);

  // Advance past the length.
  img_buf += sizeof(unsigned short);

  // Read destination and advance past it.
  unsigned char qt_info = *img_buf++;
  unsigned short* dest_table = NULL;

  static const unsigned char QT_ID_MASK = 0x0F;
  static const unsigned char QT_PRECISION_MASK = 0xF0;

  unsigned char dest = qt_info & QT_ID_MASK;
  unsigned char precision = (qt_info & QT_PRECISION_MASK) >> 4;

  printf("qt_info:\t0x%02X\n", qt_info);
  if (dest == 0x0)
  {
    dest_table = ctx.luma_q_table;
  }
  else
  {
    dest_table = ctx.chrm_q_table;
  }

  // Quantized tables are encoded according to a zig zag pattern.
  if (precision == 0)
  {
    printf("Each element in the quant table is 1 byte.\n");

    for (unsigned char i = 0; i != 64; ++i)
    {
      dest_table[get_zig_zagged_index(i)] = img_buf[i];
    }
  }
  else
  {
    printf("Each element in the quant table is 2 bytes.\n");

    for (unsigned char i = 0; i != 64; ++i, img_buf += sizeof(unsigned short))
    {
      dest_table[get_zig_zagged_index(i)] = get_short(img_buf);
    }
  }

  print_quant_tables(&ctx);

  return segment_len;
}

static unsigned short process_func_start_of_frame(unsigned char* img_buf)
{
  unsigned short segment_len = get_segment_len(img_buf);

  img_buf += sizeof(unsigned short);

  unsigned char precision = *img_buf;
  ++img_buf;

  printf("Image Bits/Sample: %d\n", precision);
  ctx.bits_per_sample = precision;

  unsigned short img_width = get_short(img_buf);
  img_buf += sizeof(unsigned short);
  unsigned short img_height = get_short(img_buf);
  img_buf += sizeof(unsigned short);

  printf("Image Dimensions: %dx%d\n", img_width, img_height);
  ctx.x_length = img_width;
  ctx.y_length = img_height;

  unsigned char num_components = *img_buf++;

  if (!(num_components == 1 || num_components == 3))
  {
    printf("WARNING: Weird number of components: %d\n", num_components);
  }

  ctx.components = (jfif_component_t*)malloc(sizeof(jfif_component_t) * num_components);
  jfif_component_t* component_it;
  for (unsigned char component_id, sample_factors, q_table_id, i = 0; i != num_components; ++i)
  {
    // Sample factor masks
    static const unsigned char SF_VERT_MASK  = 0x0F; // 0b00001111
    static const unsigned char SF_HORIZ_MASK = 0xF0; // 0b11110000

    component_id   = *img_buf++;
    sample_factors = *img_buf++;
    q_table_id     = *img_buf++;

    component_it = &ctx.components[i];
    component_it->quant_table_id = q_table_id;
    component_it->sample_factor_vert  = (sample_factors & SF_VERT_MASK );
    component_it->sample_factor_horiz = (sample_factors & SF_HORIZ_MASK) >> 4;

    print_component_info(component_it, i, component_id);
  }

  init_inverse_dct_table(ctx.bits_per_sample);

  return segment_len;
}

static unsigned short process_func_huffman_table(unsigned char* img_buf)
{
  // HT Header Masks
  static const unsigned char HT_COUNT_MASK = 0x0F;
  static const unsigned char HT_TYPE_MASK  = 0x10; // Bits 5-7 Unused

  unsigned segment_len = (unsigned)get_segment_len(img_buf);

  img_buf += sizeof(unsigned short);

  // Header information
  unsigned char ht_header = *img_buf++;

  unsigned char ht_count = ((ht_header & HT_COUNT_MASK));
  unsigned char ht_type  = ((ht_header & HT_TYPE_MASK) >> 4);

  unsigned char ht_lengths[16];
  memcpy(&ht_lengths, img_buf, 16);

  // Extract all the huff table items
  img_buf += 16;

  unsigned ht_lengths_sum = 0;
  for (unsigned i = 0; i != 16; ++i)
    ht_lengths_sum += ht_lengths[i];

  unsigned char* ht_items = (unsigned char*)malloc(ht_lengths_sum);
  for (unsigned ht_length_temp, j = 0, i = 0; i != 16; ++i)
  {
    ht_length_temp = ht_lengths[i];
    if (ht_length_temp == 0)
      continue;

    // Length isn't zero. Read that many items from the image and put it into the items array.
    memcpy(&ht_items[j], &img_buf[j], ht_length_temp);
    j += ht_length_temp;
  }

  print_huffman_info(ht_header, ht_count, ht_type, ht_lengths, ht_items, ht_lengths_sum);

  huff_node_t* true_root = (huff_node_t*)malloc(sizeof(huff_node_t));
  huff_node_init(true_root, INTERMEDIATE_NODE_VAL);

  unsigned item_counter = 0;
  for (unsigned i = 0; i != 16; ++i)
  {
    const unsigned char code_len = i + 1;
    for (unsigned k = 0; k != ht_lengths[i]; ++k)
    {
      if (!huff_table_insert(&true_root, code_len, 0, ht_items[item_counter++]))
      {
        printf("ERROR: Failed to build huff table. val:%d\n", ht_items[item_counter++]);
      }
    }
  }

  if (ht_count == 0x0) // Luma
  {
    printf("Storing Luma Huff Table %d into the Decoder Context.\n", ht_type);
    ctx.huffman_tables_luma[ht_type] = true_root;
  }
  else
  {
    printf("Storing Chroma Huff Table %d into the Decoder Context.\n", ht_type);
    ctx.huffman_tables_chroma[ht_type] = true_root;
  }

  free(ht_items);

  return segment_len;
}

static unsigned short process_func_start_of_scan(unsigned char* img_buf)
{
  unsigned short sos_header_len = get_short(img_buf);
  printf("(Header Size: %d, ", sos_header_len);

  // Process SOS header: Selectors and Tables

  if (sos_header_len != 12)
  {
    printf("\nWARNING: Something weird is going on. %d\n", sos_header_len);
    return sos_header_len;
  };

  img_buf += sos_header_len;

  // offsets holds the position of the 0xFF.
    size_t temp_buffer_size = ctx.x_length * ctx.y_length; // TODO(kaiyen): Maybe store the buffer length in the context and use it for this?
  unsigned* offsets = (unsigned*)malloc(sizeof(unsigned)*temp_buffer_size);
  unsigned segment_len, offsets_counter = 0;
  unsigned i = 0;
  while (offsets_counter < temp_buffer_size) // We shouldn't really ever reach this point.
  {
    if (img_buf[i] == 0xFF)
    {
      if(img_buf[i+1] != 0x00)
      {
        // Reached the end of the section. This means we're at the marker for EOI.
        segment_len = i;

        // add it to the end of the array, but without incrementing the counter.
        // by doing this we can copy the final span of the image too. from the last real offset to the EOI marker.
        offsets[offsets_counter] = i;
        break;
      }
      offsets[offsets_counter++] = i;
    }
    ++i;
  }

  printf("Image Size: %d)...", segment_len);

  // Now that we know the true size of the section. We can cleanse the image data of all 0x00's.
  for(i = 0; i != offsets_counter; ++i)
  {
    // TODO(kaiyen): Maybe better to just alloc a temporary buffer and then memcpy?
    unsigned offset = offsets[i];
    memmove(&img_buf[offset+1], &img_buf[offset+2], offsets[i+1] - offset-1);
  }

  // Scratch block for DCT block building
  const size_t block_size = sizeof(unsigned) * QUANT_TABLE_SIZE;
  unsigned* scratch_block = (unsigned*)malloc(block_size);
  memset(scratch_block, 0, block_size);

  unsigned luma_dc_val = 0, chroma_dc_val = 0;
  unsigned x, y, x_blocks = ctx.x_length/8, y_blocks = ctx.y_length/8;
  unsigned offset;
  printf("\n%d x %d pixels being divided into %d x %d blocks.\n", ctx.x_length, ctx.y_length, x_blocks, y_blocks);
  for (y = 0; y != y_blocks; ++y)
  {
    offset = y*ctx.y_length*8;
    for (x = 0; x != x_blocks; ++x)
    {
      // Luminance
      unsigned char*const block = &img_buf[offset + x*8];
      unsigned chm_offset = bits_to_dct_block(block, (const huff_node_t**)ctx.huffman_tables_luma, scratch_block, &luma_dc_val);

      // TODO: IDCT (DCT #3)

      memset(scratch_block, 0, block_size);

      // Chrominance
      unsigned char*const block2 = block + chm_offset;
      unsigned unused = bits_to_dct_block(block2, (const huff_node_t**)ctx.huffman_tables_chroma, scratch_block, &chroma_dc_val);
      unused++;

      memset(scratch_block, 0, block_size);
    }
  }

  free(scratch_block);
  free(offsets);
  return segment_len+sos_header_len;
}

static unsigned short process_func_end_of_image(unsigned char* img_buf)
{
  unsigned short segment_len = get_segment_len(img_buf);

  // Cleanup the decode context
  for (unsigned char i = 0; i != 2; ++i)
  {
    if (ctx.huffman_tables_luma[i])
      huff_table_cleanup(ctx.huffman_tables_luma[i]);

    if (ctx.huffman_tables_chroma[i])
      huff_table_cleanup(ctx.huffman_tables_chroma[i]);
  }

  return segment_len;
}

bool get_segment_process_func(unsigned char marker, process_func_t* out_process_func, char* out_segment_name)
{
  switch (marker)
  {
    case JFIF_SOI:
      *out_process_func = process_func_start_of_image;
      strcpy(out_segment_name, "Start of Image");
      break;
    case JFIF_AP0:
      *out_process_func = process_func_app_segment_0;
      strcpy(out_segment_name, "App Segment 0");
      break;
    case JFIF_DQT:
      *out_process_func = process_func_quant_table;
      strcpy(out_segment_name, "Quantization Table");
      break;
    case JFIF_SOF:
      *out_process_func = process_func_start_of_frame;
      strcpy(out_segment_name, "Start of Frame");
      break;
    case JFIF_DHT:
      *out_process_func = process_func_huffman_table;
      strcpy(out_segment_name, "Huffman Table");
      break;
    case JFIF_SOS:
      *out_process_func = process_func_start_of_scan;
      strcpy(out_segment_name, "Start of Scan");
      break;
    case JFIF_EOI:
      *out_process_func = process_func_end_of_image;
      strcpy(out_segment_name, "End of Image");
      break;
    default:
      return false;
  }
  return true;
}

static unsigned short process_func_default(unsigned char* img_buf)
{
  unsigned short segment_len = get_segment_len(img_buf);

  return segment_len;
}

void get_default_stage(unsigned char marker, process_func_t* out_process_func, char* out_segment_name)
{
  *out_process_func = process_func_default;
  sprintf(out_segment_name, "Unsupported Stage: 0xFF%X", marker);
}
