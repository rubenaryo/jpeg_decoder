/*--------------------------------------------------------------------------
File:   print_utils.cpp
Date:   2022/01/08
Author: kaiyen
---------------------------------------------------------------------------*/

#include "print_utils.h"

#include "decoder.h"

#include <stdlib.h>
#include <stdio.h>

void print_jpeg_header(decode_context_t* ctx, unsigned char jfif_major, unsigned char jfif_minor)
{
  if (ctx == NULL)
    return;

  char buf[16];
  snprintf(buf, 16, "%d.%d", jfif_major, jfif_minor);

  printf("+-------------------------+\n");
  printf("| JPEG Header Information |\n");
  printf("+-------------------------+\n");
  printf("| JFIF %-19s|\n", buf);

  snprintf(buf, 16, "%1d", ctx->density_units);
  printf("| Density Units: %-9s|\n", buf);

  snprintf(buf, 16, "%dx%d", ctx->x_density, ctx->y_density);
  printf("| Density: %-15s|\n", buf);
  printf("+-------------------------+\n");
}

void print_quant_tables(decode_context_t* ctx)
{
  if (ctx == NULL)
    return;

  printf("+---------------------------------+   +---------------------------------+\n");
  printf("|              LUMA               |   |              CHROMA             |\n");
  printf("+---------------------------------+   +---------------------------------+\n");
  for (unsigned char i = 0; i != 8; ++i)
  {
    const unsigned char offset = i * 8;
    printf("| ");
    for (unsigned char j = 0; j != 8; ++j)
    {
        printf("%03d ", ctx->luma_q_table[offset + j]);
    }

    printf("|   | ");

    for (unsigned char j = 0; j != 8; ++j)
    {
        printf("%03d ", ctx->chrm_q_table[offset + j]);
    }
    printf("|\n");
  }
  printf("+---------------------------------+   +---------------------------------+\n");
}

void print_huffman_info(unsigned char ht_header, unsigned char ht_count, unsigned char ht_type, unsigned char* ht_lengths, unsigned char* ht_items, unsigned char ht_items_count)
{
  if (ht_lengths == NULL || ht_items == NULL)
    return;

  printf("ht_header:\t0x%02x\nht_count:\t0x%02x\nht_type:\t0x%02x\n", ht_header, ht_count, ht_type);

  // Lengths
  printf("ht_lengths:\t{ ");
  for (unsigned char i = 0; i != 16; ++i)
    printf("%d ", ht_lengths[i]);

  printf("}\n");


  // Items
  printf("ht_items(%d):\t{ ", ht_items_count);
  for (unsigned i = 0; i != ht_items_count; ++i)
  {
    printf("%d ", ht_items[i]);
  }
  printf("}\n");

}
