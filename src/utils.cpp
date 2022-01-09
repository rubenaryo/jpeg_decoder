/*--------------------------------------------------------------------------
File:   utils.cpp
Date:   2022/01/05
Author: kaiyen
---------------------------------------------------------------------------*/

#include "utils.h"

int read_stream(const unsigned char *src, unsigned offset, unsigned count, unsigned accum /*= 0*/)
{
// Base of recursion
if (count == 0)
return accum;

// Look at the source, how many bits there left in the current byte
const unsigned char *cur_src = src + offset / 8;
const unsigned bits_left = 8 - offset % 8;
const unsigned char cur_data = *cur_src << (8 - bits_left);

// How many bits we need and can write now
const unsigned bits_to_use = bits_left < count ? bits_left : count;

// Write the desired bits to the accumulator
accum <<= bits_to_use;
unsigned char mask = (1 << bits_to_use) - 1;
const unsigned off = 8 - bits_to_use;
accum |= (cur_data & (mask << off)) >> off;

// Tail-recurse into the rest of required bits
return read_stream(src, offset + bits_to_use, count - bits_to_use, accum);
}

// See Table 5 in https://www.impulseadventure.com/photo/jpeg-huffman-coding.html
int dc_ac_value_decode(int read_bits, unsigned bit_count)
{
  const int sign = 1 << (bit_count - 1);
  return read_bits & sign ? read_bits : (read_bits - (2 * sign) + 1);
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

unsigned short get_zig_zagged_index(unsigned char idx)
{
  return ZIG_ZAG_INDEX_TABLE[idx];
}

unsigned short get_short(unsigned char* img_buf)
{
  return ((unsigned short)img_buf[0] << 8) | img_buf[1];
}
