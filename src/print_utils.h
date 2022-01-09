/*--------------------------------------------------------------------------/
File:   print_utils.h
Date:   2022/01/08
Author: kaiyen
---------------------------------------------------------------------------*/
#ifndef PRINT_UTILS_H
#define PRINT_UTILS_H

#include <stddef.h>

typedef enum _print
{
  PT_BYTE,
  PT_SHORT,
  PT_FLOAT,
  PT_INT,
  PT_COUNT
} print_t;

struct _decode_context;
struct _jfif_component;

// Prints the jpeg header information obtained from APP0
void print_jpeg_header(struct _decode_context* ctx, unsigned char jfif_major, unsigned char jfif_minor);

// Prints the quantization tables held in the decoder context
void print_quant_tables(struct _decode_context* ctx, unsigned char qt_info, unsigned char qt_precision);

// Prints the huffman table header, lengths, and items.
void print_huffman_info(unsigned char ht_header, unsigned char ht_count, unsigned char ht_type, unsigned char* ht_lengths, unsigned char* ht_items, unsigned char ht_items_count);

// Prints the component information for a single jpeg channel
void print_component_info(struct _jfif_component* component, unsigned char component_counter, unsigned char component_id);

// Prints a single block to the console, and a caller specified header string
void print_block(const char* header, const char* elem_fmt, void* block, size_t block_side_len, print_t type);

#endif
