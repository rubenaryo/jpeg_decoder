/*--------------------------------------------------------------------------/
File:   huffman.h
Date:   2021/12/28
Author: kaiyen
---------------------------------------------------------------------------*/
#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdlib.h>

typedef struct _huff_node huff_node_t;

struct _huff_node
{
  huff_node_t* left;
  huff_node_t* right;

  unsigned char val;
};

void huff_node_init(huff_node_t* node, unsigned char val);

bool huff_table_insert(huff_node_t** root, const unsigned char code_len, unsigned char cur_pos, const unsigned char val);

unsigned char huff_table_lookup(huff_node_t* root, const unsigned code, const unsigned code_len, const unsigned cur_shift);

void huff_table_cleanup(huff_node_t* root);

static const unsigned char INTERMEDIATE_NODE_VAL = 0xcd;

#endif
