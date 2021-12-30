/*--------------------------------------------------------------------------
File:   decoder.cpp
Date:   2021/12/25
Author: kaiyen
---------------------------------------------------------------------------*/
#include "decoder.h"
#include "huffman.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unordered_map>

// Static mappings from JFIF stage to its name and associated process function.
static std::unordered_map<unsigned char, jfif_stage_t> s_stage_map;

// Decode Context. Holds working state and algorithm params.
static decode_ctx ctx;

bool get_stage(unsigned char marker, jfif_stage_t* out_stage)
{
  auto itStage = s_stage_map.find(marker);

  if (itStage != s_stage_map.end())
  {
    memcpy(out_stage, &(itStage->second), sizeof(jfif_stage_t));
    return true;
  }

  return false;
}

static unsigned short get_short(unsigned char* img_buf)
{
  return ((unsigned short)img_buf[0] << 8) | img_buf[1];
}

static unsigned short process_func_start_of_image(unsigned char* img_buf)
{
  // The start of image marker doesn't have a length after it and is 0 length anyway. No-op.
  printf("(Stage Size: 0)...");
  return 0;
}

static unsigned short process_func_app_segment_0(unsigned char* img_buf)
{
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);

  // App0 offsets
  static const unsigned char VERSION_MAJOR = sizeof(unsigned short) + (sizeof(unsigned char) * 5);
  static const unsigned char VERSION_MINOR = VERSION_MAJOR + sizeof(unsigned char);
  static const unsigned char DENSITY_UNITS = VERSION_MINOR + sizeof(unsigned char);
  static const unsigned char DENSITY_DIM_X = DENSITY_UNITS + sizeof(unsigned char);
  static const unsigned char DENSITY_DIM_Y = DENSITY_DIM_X + sizeof(unsigned short);

  ctx.jfif_ver.major = img_buf[VERSION_MAJOR];
  ctx.jfif_ver.minor = img_buf[VERSION_MINOR];

  ctx.density_units = img_buf[DENSITY_UNITS];

  ctx.x_density = get_short(&img_buf[DENSITY_DIM_X]);
  ctx.y_density = get_short(&img_buf[DENSITY_DIM_Y]);

  // Ignore thumbnail crap for now.

  return stage_len;
}

static const unsigned short ZIG_ZAG_INDEX_TABLE[64] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
};

static unsigned short process_func_quant_table(unsigned char* img_buf)
{
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);

  // Advance past the length.
  img_buf += sizeof(unsigned short);

  // Read destination and advance past it.
  unsigned char dest = *img_buf++;
  unsigned short* dest_table = NULL;

  if (dest == 0x0)
  {
    dest_table = ctx.luma_q_table;
  }
  else
  {
    dest_table = ctx.chrm_q_table;
  }

  // Quantized tables are encoded according to a zig zag pattern.
  for (unsigned char i = 0; i != 64; ++i)
  {
    dest_table[ZIG_ZAG_INDEX_TABLE[i]] = img_buf[i];
  }

  return stage_len;
}

static unsigned short process_func_start_of_frame(unsigned char* img_buf)
{
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);

  img_buf += sizeof(unsigned short);

  unsigned char precision = *img_buf;
  ++img_buf;

  printf("\nImage Bits/Sample: %d\n", precision);
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
    printf("WARNING:Weird number of components: %d\n", num_components);
  }

  static const char* COMP_ID_TO_NAME[] = {"Undefined", "Y", "Cb", "Cr", "I", "Q"};

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

    printf("Component %d:\n", i);
    printf("  Component:\t\t\t%s\n", COMP_ID_TO_NAME[component_id]);
    printf("  Quantization Table ID:\t%d\n", component_it->quant_table_id);
    printf("  Vertical Sample Factor:\t%d\n", component_it->sample_factor_vert);
    printf("  Horizontal Sample Factor:\t%d\n", component_it->sample_factor_horiz);
  }

  return stage_len;
}

static unsigned short process_func_huffman_table(unsigned char* img_buf)
{
  static const unsigned char HT_COUNT_MASK = 0x0F;
  static const unsigned char HT_TYPE_MASK  = 0x10;

  unsigned stage_len = (unsigned)get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);

  img_buf += sizeof(unsigned short);

  // Header information
  unsigned char ht_header = *img_buf;

  unsigned char ht_count = ((ht_header & HT_COUNT_MASK));
  unsigned char ht_type  = ((ht_header & HT_TYPE_MASK) >> 4);

  printf("\nht_header:\t0x%02x\nht_count:\t0x%02x\nht_type:\t0x%02x\n", ht_header, ht_count, ht_type);

  ++img_buf;

  unsigned char ht_lengths[16];
  memcpy(&ht_lengths, img_buf, 16);

  printf("ht_lengths:\t{ ");

  // Extract all the huff table items
  img_buf += 16;

  unsigned ht_lengths_sum = 0;

  for (unsigned ht_length_temp, i = 0; i != 16; ++i)
  {
    ht_length_temp = ht_lengths[i];
    ht_lengths_sum += ht_length_temp;
    printf("%d ", ht_length_temp);
  }
  printf("}\n");

  unsigned char* ht_items_arr = (unsigned char*)malloc(ht_lengths_sum);
  for (unsigned ht_length_temp, i = 0, j = 0; i != 16; ++i)
  {
    ht_length_temp = ht_lengths[i];
    if (ht_length_temp == 0)
      continue;

    // Length isn't zero. Read that many items from the image and put it into the items array.
    memcpy(&ht_items_arr[j], &img_buf[j], ht_length_temp);
    j += ht_length_temp;
  }

  printf("ht_items(%d):\t{ ", ht_lengths_sum);
  for (unsigned i = 0; i != ht_lengths_sum; ++i)
  {
    printf("%d ", ht_items_arr[i]);
  }
  printf("}\n");

  huff_node_t* true_root = (huff_node_t*)malloc(sizeof(huff_node_t));
  huff_node_init(true_root, INTERMEDIATE_NODE_VAL);

  unsigned item_counter = 0;
  for (unsigned i = 0; i != 16; ++i)
  {
    const unsigned char code_len = i + 1;
    for (unsigned j = 0; j != ht_lengths[i]; ++j)
    {
      if (!huff_table_insert(&true_root, code_len, 0, ht_items_arr[item_counter++]))
      {
        printf("ERROR: Failed to build huff table. val:%d\n", ht_items_arr[item_counter++]);
      }
    }
  }

  ctx.huffman_tables[ht_header] = true_root;
  free(ht_items_arr);

  return stage_len;
}

static unsigned short process_func_start_of_scan(unsigned char* img_buf)
{
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);

  return stage_len;
}

static unsigned short process_func_end_of_image(unsigned char* img_buf)
{
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);

  // Cleanup the decode context
  for (unsigned char i = 0; i != 4; ++i)
  {
    huff_table_cleanup(ctx.huffman_tables[i]);
  }

  return stage_len;
}

static void callback_app_segment_0(void)
{
  char buf[16];
  snprintf(buf, 16, "%d.%d", ctx.jfif_ver.major, ctx.jfif_ver.minor);

  printf("+-------------------------+\n");
  printf("| JPEG Header Information |\n");
  printf("+-------------------------+\n");
  printf("| JFIF %-19s|\n", buf);

  snprintf(buf, 16, "%1d", ctx.density_units);
  printf("| Density Units: %-9s|\n", buf);

  snprintf(buf, 16, "%dx%d", ctx.x_density, ctx.y_density);
  printf("| Density: %-15s|\n", buf);
  printf("+-------------------------+\n");
}

static void callback_quant_table(void)
{
  printf("+---------------------------------+   +---------------------------------+\n");
  printf("|              LUMA               |   |              CHROMA             |\n");
  printf("+---------------------------------+   +---------------------------------+\n");
  for (unsigned char i = 0; i != 8; ++i)
  {
    printf("| ");
    for (unsigned char j = 0; j != 8; ++j)
    {
        printf("%03d ", ctx.luma_q_table[i*8 + j]);
    }

    printf("|   | ");

    for (unsigned char j = 0; j != 8; ++j)
    {
        printf("%03d ", ctx.chrm_q_table[i*8 + j]);
    }
    printf("|\n");
  }
  printf("+---------------------------------+   +---------------------------------+\n");
}

void populate_stage_map(void)
{
  s_stage_map[JFIF_SOI] = {"Start of Image", process_func_start_of_image, NULL};
  s_stage_map[JFIF_AP0] = {"Application Segment 0", process_func_app_segment_0, callback_app_segment_0};
  s_stage_map[JFIF_DQT] = {"Quantization Table", process_func_quant_table, callback_quant_table};
  s_stage_map[JFIF_SOF] = {"Start of Frame", process_func_start_of_frame, NULL};
  s_stage_map[JFIF_DHT] = {"Huffman Table", process_func_huffman_table, NULL};
  s_stage_map[JFIF_SOS] = {"Start of Scan", process_func_start_of_scan, NULL};
  s_stage_map[JFIF_EOI] = {"End of Image", process_func_end_of_image, NULL};
}
