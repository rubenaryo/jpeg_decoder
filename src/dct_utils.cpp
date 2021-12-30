/*--------------------------------------------------------------------------
File:   dct_utils.cpp
Date:   2021/12/29
Author: kaiyen
---------------------------------------------------------------------------*/
#include "dct_utils.h"

#include <math.h>
#include <stdio.h>

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

void do_inverse_dct(unsigned char* block, decode_context_t* ctx)
{
  
}
