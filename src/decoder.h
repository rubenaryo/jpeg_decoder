/*--------------------------------------------------------------------------/
File:   decoder.h
Date:   2021/12/25
Author: kaiyen
---------------------------------------------------------------------------*/
#ifndef DECODER_H
#define DECODER_H

#include "huffman.h"

enum : unsigned char
{
  JFIF_MFF = 0xFF, // Marker Byte
  JFIF_SOI = 0xD8, // Start of Image
  JFIF_AP0 = 0xE0, // Application Segment 0
  JFIF_DQT = 0xDB, // Define Quantization Table
  JFIF_SOF = 0xC0, // Start of Frame
  JFIF_DHT = 0xC4, // Define Huffman Table
  JFIF_SOS = 0xDA, // Start of Scan
  JFIF_EOI = 0xD9  // End of Image
};

typedef unsigned short (*process_func_t)(unsigned char*);
typedef void (*callback_func_t)(void);

typedef struct _jfif_stage
{
  const char* name;
  process_func_t process_func;
  callback_func_t callback_func;
} jfif_stage_t;

// Fills the name and process function maps at init time.
void populate_stage_map(void);

// Accesses the table and returns a stage, or NULL if not found
bool get_stage(unsigned char marker, jfif_stage_t* out_stage);

jfif_stage_t get_default_stage(unsigned char marker);

typedef struct _jfif_component
{
  unsigned char quant_table_id;
  unsigned char sample_factor_vert;
  unsigned char sample_factor_horiz;
} jfif_component_t;

// TODO(kaiyen): I don't really give a shit about thumbnails right now
typedef struct _jfif_ext_data
{
  void* thumbnail_data;
  unsigned char thumbnail_format;
  unsigned char thumbnail_len_x;
  unsigned char thumbnail_len_y;

} extension_data_t;

/*
----------------
Decode Context:
----------------
This is mostly responsible for holding the state.
Each stage's information will be organized and stuffed into this thing.
*/
static const unsigned char HUFF_TABLES_PER_CHANNEL_TYPE = 2;
static const unsigned char QUANT_TABLE_SIZE = 64;
typedef struct _decode_context
{
  extension_data_t* extension_data;

  // By convention: Index 0 is DC, Index 1 is AC
  // chroma tables are NULL for greyscale images.
  huff_node_t* huffman_tables_luma[HUFF_TABLES_PER_CHANNEL_TYPE];
  huff_node_t* huffman_tables_chroma[HUFF_TABLES_PER_CHANNEL_TYPE];
  jfif_component_t* components;

  unsigned short luma_q_table[QUANT_TABLE_SIZE];
  unsigned short chrm_q_table[QUANT_TABLE_SIZE];

  unsigned short x_length;
  unsigned short y_length;

  unsigned short x_density;
  unsigned short y_density;

  unsigned char density_units;
  unsigned char bits_per_sample;
  unsigned char num_components;

} decode_context_t;

#endif
