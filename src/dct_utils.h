/*--------------------------------------------------------------------------/
File:   dct_utils.h
Date:   2021/12/29
Author: kaiyen
---------------------------------------------------------------------------*/
#ifndef DCT_UTILS_H
#define DCT_UTILS_H

#include "huffman.h"

void init_inverse_dct_table(unsigned char precision);

// Converts a raw binary block into an organized dct block, stored in the provided scratch_block.
// Note: It's up to the caller to provide the scratch_block buffer. Assumes non-NULL.
unsigned bits_to_dct_block(unsigned char*const block, const huff_node_t** huff_tables, unsigned* scratch_block, unsigned* prev_dc_val);

#endif

