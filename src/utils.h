/*--------------------------------------------------------------------------/
File:   utils.h
Date:   2021/12/30
Author: kaiyen
---------------------------------------------------------------------------*/
#ifndef UTILS_H
#define UTILS_H

// Stream reader shamelessly copied from: https://github.com/sakhnik/cpp-sandbox/blob/737132386dc20009be2f1997cbd62302118a7125/BitStream.cpp
// NOTE: Probably not very performant for this use case and probably doesn't guard too well against T overflow.
int read_stream(const unsigned char *src, unsigned offset, unsigned count, unsigned accum = 0);

// See Table 5 in https://www.impulseadventure.com/photo/jpeg-huffman-coding.html
int dc_ac_value_decode(int read_bits, unsigned bit_count);

// Widely accessible table for zig zag indices.
unsigned short get_zig_zagged_index(unsigned char idx);
#endif
