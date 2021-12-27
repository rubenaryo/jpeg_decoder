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
  printf("(Stage Size: 0)");
  return 0;
}

static unsigned short process_func_app_segment_0(unsigned char* img_buf)
{
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);

  // App0 offsets
  static const unsigned char VERSION_MAJOR = sizeof(unsigned short) + (sizeof(unsigned char) * 5);
  static const unsigned char VERSION_MINOR = VERSION_MAJOR + 1;
  static const unsigned char DENSITY_UNITS = VERSION_MINOR + 1;
  static const unsigned char DENSITY_DIM_X = DENSITY_UNITS + 1;
  static const unsigned char DENSITY_DIM_Y = DENSITY_DIM_X + 2;

  ctx.jfif_ver.major = img_buf[VERSION_MAJOR];
  ctx.jfif_ver.minor = img_buf[VERSION_MINOR];
  
  ctx.density_units = img_buf[DENSITY_UNITS];

  ctx.x_density = get_short(&img_buf[DENSITY_DIM_X]);
  ctx.y_density = get_short(&img_buf[DENSITY_DIM_Y]);

  // Ignore thumbnail crap for now.
  
  return stage_len;
}

static unsigned short process_func_quant_table(unsigned char* img_buf)
{
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);
    
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
  unsigned short stage_len = get_short(img_buf);
  printf("(Stage Size: %d)...", stage_len);
  
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
  char buf[64];
  sprintf(buf, "%d.%d", ctx.jfif_ver.major, ctx.jfif_ver.minor);
  
  printf("+-------------------------+\n");
  printf("| JPEG Image Information  |\n");
  printf("+-------------------------+\n");
  printf("| JFIF %-19s|\n", buf);

  sprintf(buf, "%1d", ctx.density_units);
  printf("| Density Units: %-9s|\n", buf);

  sprintf(buf, "%dx%d", ctx.x_density, ctx.y_density);
  printf("| Density: %-15s|\n", buf);
  printf("+-------------------------+\n");
}

void populate_stage_map(void)
{
  s_stage_map[JFIF_SOI] = {"Start of Image", process_func_start_of_image, NULL};
  s_stage_map[JFIF_AP0] = {"Application Segment 0", process_func_app_segment_0, callback_app_segment_0};
  s_stage_map[JFIF_DQT] = {"Quantization Table", process_func_quant_table, NULL};
  s_stage_map[JFIF_SOF] = {"Start of Frame", process_func_start_of_frame, NULL};
  s_stage_map[JFIF_DHT] = {"Huffman Table", process_func_huffman_table, NULL};
  s_stage_map[JFIF_SOS] = {"Start of Scan", process_func_start_of_scan, NULL};
  s_stage_map[JFIF_EOI] = {"End of Image", process_func_end_of_image, NULL};
}
