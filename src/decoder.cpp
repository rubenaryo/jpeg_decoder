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

void populate_maps(void)
{
  s_stage_map[JFIF_SOI] = {"Start of Image", process_func_start_of_image};
  s_stage_map[JFIF_AP0] = {"Application Segment 0", process_func_app_segment_0};
  s_stage_map[JFIF_DQT] = {"Quantization Table", process_func_quant_table};
  s_stage_map[JFIF_SOF] = {"Start of Frame", process_func_start_of_frame};
  s_stage_map[JFIF_DHT] = {"Huffman Table", process_func_huffman_table};
  s_stage_map[JFIF_SOS] = {"Start of Scan", process_func_start_of_scan};
  s_stage_map[JFIF_EOI] = {"End of Image", process_func_end_of_image};
}

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

static unsigned short get_stage_length(unsigned char* img_buf)
{
  return ((unsigned short)img_buf[0] << 8) | img_buf[1];
}

unsigned short process_func_start_of_image(unsigned char* img_buf)
{
  // The start of image marker doesn't have a length after it and is 0 length anyway. No-op.
  return 0;
}

unsigned short process_func_app_segment_0(unsigned char* img_buf)
{
  unsigned short stage_len = get_stage_length(img_buf);
  
  return stage_len;
}

unsigned short process_func_quant_table(unsigned char* img_buf)
{
  unsigned short stage_len = get_stage_length(img_buf);
  
  return stage_len;
}

unsigned short process_func_start_of_frame(unsigned char* img_buf)
{
  unsigned short stage_len = get_stage_length(img_buf);
  
  return stage_len;
}

unsigned short process_func_huffman_table(unsigned char* img_buf)
{
  unsigned short stage_len = get_stage_length(img_buf);
  
  return stage_len;
}

unsigned short process_func_start_of_scan(unsigned char* img_buf)
{
  unsigned short stage_len = get_stage_length(img_buf);
  
  return stage_len;
}

unsigned short process_func_end_of_image(unsigned char* img_buf)
{
  unsigned short stage_len = get_stage_length(img_buf);
  
  return stage_len;
}
