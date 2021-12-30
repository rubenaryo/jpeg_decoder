/*--------------------------------------------------------------------------/
File:   dct_utils.h
Date:   2021/12/29
Author: kaiyen
---------------------------------------------------------------------------*/
#ifndef DCT_UTILS_H
#define DCT_UTILS_H

#include "decoder.h"

void init_inverse_dct_table(unsigned char precision);

// Does inverse DCT on block, which is the start of an 8x8 block of cosine-ified image data.
void do_inverse_dct(unsigned char* block, decode_context_t* ctx);

#endif
