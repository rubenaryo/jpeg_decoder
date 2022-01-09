/*--------------------------------------------------------------------------
File:   print_utils.cpp
Date:   2022/01/08
Author: kaiyen
---------------------------------------------------------------------------*/

#include "print_utils.h"

#include "decoder.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

void print_quant_tables(decode_context_t* ctx, unsigned char qt_info, unsigned char qt_precision)
{
  if (ctx == NULL)
    return;

  printf("qt_info:\t0x%03X\n", qt_info);

  if (qt_precision == 0)
  {
    printf("Each element in the quant table is 1 byte.\n");
  }
  else
  {
    printf("Each element in the quant table is 2 bytes.\n");
  }

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

void print_component_info(struct _jfif_component* component, unsigned char component_counter, unsigned char component_id)
{
  if (component == NULL)
    return;

  static const char* COMP_ID_TO_NAME[] = {"Undefined", "Y", "Cb", "Cr", "I", "Q"};

  printf("Component %d:\n", component_counter);
  printf("  Component:\t\t\t%s\n", COMP_ID_TO_NAME[component_id]);
  printf("  Quantization Table ID:\t%d\n", component->quant_table_id);
  printf("  Vertical Sample Factor:\t%d\n", component->sample_factor_vert);
  printf("  Horizontal Sample Factor:\t%d\n", component->sample_factor_horiz);
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


static void print_byte(const char* elem_fmt, const void* item) { printf(elem_fmt, *(const unsigned char*)item);}
static void print_short(const char* elem_fmt, const void* item) { printf(elem_fmt, *(const short*)item);}
static void print_float(const char* elem_fmt, const void* item) { printf(elem_fmt, *(const float*)item);}
static void print_int(const char* elem_fmt, const void* item) { printf(elem_fmt, *(const int*)item);}

void print_block(const char* header, const char* elem_fmt, void* block, size_t block_side_len, enum print_t type)
{
  typedef void (*print_elem_func_t)(const char*, const void*);
  static print_elem_func_t PT_FUNCS[PT_COUNT] =
  {
    print_byte,
    print_short,
    print_float,
    print_int
  };

  static size_t PT_SIZES[PT_COUNT] =
  {
    sizeof(unsigned char),
    sizeof(short),
    sizeof(float),
    sizeof(int)
  };

  if (header == NULL || elem_fmt == NULL || block == NULL || block_side_len == 0 || type < 0 || type > PT_COUNT)
    return;

  print_elem_func_t print_func = PT_FUNCS[type];
  size_t elem_size = PT_SIZES[type];

  const void* block_it = NULL;

  printf("\n%s:\n", header);
  for (size_t offset, j, i = 0; i != block_side_len; ++i)
  {
    putchar(' ');
    offset = i * block_side_len;
    for (j = 0; j != block_side_len; ++j)
    {
      block_it = (const unsigned char*)block + (elem_size * (offset + j));
      print_func(elem_fmt, block_it);
    }
    putchar('\n');
  }

}
