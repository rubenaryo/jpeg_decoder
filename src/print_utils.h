/*--------------------------------------------------------------------------/
File:   print_utils.h
Date:   2022/01/08
Author: kaiyen
---------------------------------------------------------------------------*/
#ifndef PRINT_UTILS_H
#define PRINT_UTILS_H

struct _decode_context;

// Prints the jpeg header information obtained from APP0
void print_jpeg_header(struct _decode_context* ctx, unsigned char jfif_major, unsigned char jfif_minor);

// Prints the quantization tables held in the decoder context
void print_quant_tables(struct _decode_context* ctx);

// Returns the segment length from the buffer's next two bytes and prints it out.
unsigned short get_segment_len(unsigned char* img_buf);

#endif