/*--------------------------------------------------------------------------/
File:   decoder.h
Date:   2021/12/25
Author: kaiyen
---------------------------------------------------------------------------*/
#ifndef DECODER_H
#define DECODER_H

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

typedef struct _jfif_ext_data
{
  void* thumbnail_data;
  unsigned char thumbnail_format;
  unsigned char thumbnail_len_x;
  unsigned char thumbnail_len_y;
  
} extension_data;

/*
----------------
Decode Context: 
----------------
This is mostly responsible for holding the state.
Each stage's information will be organized and stuffed into this thing.
*/
typedef struct _decode_ctx
{
  extension_data* ext_data;

  unsigned short luma_q_table[64];
  unsigned short chrm_q_table[64];
  
  unsigned short x_density;
  unsigned short y_density;

  struct
  {
    unsigned char major;
    unsigned char minor;
  } jfif_ver;

  unsigned char density_units;
  
} decode_ctx;

#endif
