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

// Pointer to function that processes a single byte from a jpeg image.
// Swapped around according to what section of the image we're in.
typedef void (*process_func_t)(unsigned char);

typedef struct _jfif_stage
{
  const char* name;
  process_func_t process_func;
} jfif_stage_t;

// Fills the name and process function maps at init time.
void populate_maps(void);

// Accesses the table and returns a stage, or NULL if not found
bool get_stage(unsigned char marker, jfif_stage_t* out_stage); 

// Process Function Signatures
void process_func_start_of_image(unsigned char);
void process_func_app_segment_0(unsigned char);
void process_func_quant_table(unsigned char);
void process_func_start_of_frame(unsigned char);
void process_func_huffman_table(unsigned char);
void process_func_start_of_scan(unsigned char);
void process_func_end_of_image(unsigned char);

#endif
