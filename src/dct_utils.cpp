/*--------------------------------------------------------------------------
File:   dct_utils.cpp
Date:   2021/12/29
Author: kaiyen
---------------------------------------------------------------------------*/
#include "dct_utils.h"
#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static float* s_inverse_dct_table = NULL;

void init_inverse_dct_table(unsigned char precision)
{
  s_inverse_dct_table = (float*)malloc(sizeof(float) * precision * precision);
  if (s_inverse_dct_table == NULL)
  {
    printf("ERROR: Failed to allocate inverse DCT Table!");
    return;
  }

  unsigned char i = 0, j;
  unsigned offset;
  float coeff = (1.0f / M_SQRT2) / 16.0f;

  for (j = 0; j != precision; ++j)
  {
    s_inverse_dct_table[j] = coeff; // We can bake in the coefficient, since cos(0) is 1.
  }

  coeff = 1.0f;

  for (i = 1; i != precision; ++i)
  {
    offset = i * precision;
    for (j = 0; j != precision; ++j)
    {
      s_inverse_dct_table[offset + j] = coeff * cosf(((2.0f * (float)j + 1) * i * M_PI) / 16.0f);
    }
  }

  printf("\nInverse DCT Table:\n");
  for (i = 0; i != precision; ++i)
  {
    putchar(' ');
    offset = i * precision;
    for (j = 0; j != precision; ++j)
    {
      printf("%+02.2f ", s_inverse_dct_table[offset + j]);
    }
    putchar('\n');
  }

}

// huff_tables is assumed to be a non-null array of 2 huffman table pointers.
// scratch_block is assumed to be a buffer provided by the caller.
unsigned bits_to_dct_block(unsigned char*const block, const huff_node_t** huff_tables, unsigned* scratch_block, unsigned* prev_dc_val)
{
  const huff_node_t* huff_table_ptr = huff_tables[0];
  // read one bit at a time.
  unsigned cur_pos = 0;
  unsigned char bits_to_read = huff_table_lookup(huff_table_ptr, block, &cur_pos);

  if (bits_to_read == 0x0) // 0 is the EOB indicator
  {
    printf("DC EOB INDICATOR! cur_pos: %d\n", cur_pos);
  }
  else
  {
    // Huff table returns num bits to read from the stream.
    // Bits read need to be decoded into a usable value, and THAT goes into the scratch table.
    int read_bits = read_stream(block, cur_pos, bits_to_read);
    int decoded_dc_val = dc_ac_value_decode(read_bits, bits_to_read);

    printf("DC Read returned: %d.\n", decoded_dc_val);

    cur_pos += bits_to_read;

    scratch_block[0] = decoded_dc_val + *prev_dc_val;
    *prev_dc_val = decoded_dc_val;
  }

  //We've read the DC value, now time for the 63 AC values.
  huff_table_ptr = huff_tables[1];

  if (huff_table_ptr == NULL)
  {
    printf("FUCK!\n");
  }

  // This could be a while(1) but theoretically we should never be able to read any more than 63 discrete AC values.
  for (unsigned char i = 0; i != 63; ++i)
  {
    bits_to_read = huff_table_lookup(huff_table_ptr, block, &cur_pos);

    if (bits_to_read == 0x0) // 0 is the EOB indicator
    {
      printf("AC EOB INDICATOR! cur_pos:%d i:%d\n", cur_pos, i);
      break;
    }

    int read_bits = read_stream(block, cur_pos, bits_to_read);
    int decoded_ac_val = dc_ac_value_decode(read_bits, bits_to_read);
    printf("AC Read returned: %d.\n", decoded_ac_val);
    cur_pos += bits_to_read;

    scratch_block[get_zig_zagged_index(i)] = decoded_ac_val;
  }

  // NOw,the table should be filled. We can do inverse DCT.
//  for (unsigned char i = 0; i != 64; ++i)
  //{
    //scratch_block[i] *= ctx->luma_q_table[i];// * (unsigned)(s_inverse_dct_table[i] * 255.0f);
 // }

  printf("\nFinal DCT Block::\n");
  for (unsigned char i = 0; i != 8; ++i)
  {
    putchar(' ');
    unsigned offset = i * 8;
    for (unsigned char j = 0; j != 8; ++j)
    {
      printf("%+04d ", scratch_block[offset + j]);
    }
    putchar('\n');
  }
  putchar('\n');

  return cur_pos;
}
