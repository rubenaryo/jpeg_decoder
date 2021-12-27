/*--------------------------------------------------------------------------
File:   decoder.cpp
Date:   2021/12/25
Author: kaiyen
---------------------------------------------------------------------------*/
#include "decoder.h"

#include <stdio.h>
#include <string.h>
#include <unordered_map>

// Static mappings from JFIF stage to its name and associated process functoin.
static std::unordered_map<unsigned char, jfif_stage_t> s_stage_map;

// Decode Context. Holds State.
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
    dest_table = ctx.luma_q_table;
  else
    dest_table = ctx.chrm_q_table;
  
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

  return stage_len;
}

static unsigned short process_func_huffman_table(unsigned char* img_buf)
{
  static const unsigned char HT_COUNT_MASK = 0xE0;
  static const unsigned char HT_TYPE_MASK  = 0010;
  
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);

  img_buf += sizeof(unsigned short);

  unsigned char ht_byte = *img_buf;

  unsigned char ht_count = ((ht_byte & HT_COUNT_MASK));
  unsigned char ht_type  = ((ht_byte & HT_TYPE_MASK));

  printf("\nht_byte:\t%02x\nht_count:\t%x\nht_type:\t%x\n", ht_byte, ht_count, ht_type);
  
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
  
  return stage_len;
}

static void callback_app_segment_0(void)
{
  char buf[16];
  snprintf(buf, 16, "%d.%d", ctx.jfif_ver.major, ctx.jfif_ver.minor);
  
  printf("+-------------------------+\n");
  printf("| JPEG Image Information  |\n");
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
