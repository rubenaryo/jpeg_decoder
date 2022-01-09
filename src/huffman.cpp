/*--------------------------------------------------------------------------
File:   huffman.cpp
Date:   2021/12/28
Author: kaiyen
---------------------------------------------------------------------------*/
#include "huffman.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

#define ENABLE_HT_LOG 0

#if ENABLE_HT_LOG
#define HT_LOG(...) printf(__VA_ARGS__)
#else
#define HT_LOG(...)
#endif

static void alloc_and_init_huff_node(huff_node_t** root, unsigned char val)
{
  *root = (huff_node_t*)malloc(sizeof(huff_node_t));
  huff_node_init(*root, val);
}

void huff_node_init(huff_node_t* node, unsigned char val)
{
  node->left = node->right = NULL;
  node->val = val;
}

bool huff_table_insert(huff_node_t** root, const unsigned char code_len, unsigned char cur_pos, const unsigned char val)
{
  if (*root == NULL)
  {
    if (cur_pos < code_len) // We need to go deeper
    {
      HT_LOG("Created an intermediate node at lvl: %d\n", cur_pos);
      alloc_and_init_huff_node(root, INTERMEDIATE_NODE_VAL);
    }
    else if (cur_pos == code_len) // Success. Early out.
    {
      // TODO(kaiyen): I know how many items there are, so
      // it might make more sense to just alloc all those huff leaf nodes at once in a single array.
      HT_LOG("Inserted %d at pos:%d.\n", val, cur_pos);
      alloc_and_init_huff_node(root, val);
      return true;
    }
    else // We went too far.
    {
      HT_LOG("Went too far! val:%d cur_pos:%d code_len:%d\n", val, cur_pos, code_len);
      return false;
    }
  }
  else if (cur_pos != 0 && (*root)->left == NULL && (*root)->right == NULL)
  {
    HT_LOG("Cannot insert %d. Found a leaf node with val: %d. cur_pos:%d.\n", val, (*root)->val, cur_pos);
    return false;
  }

  huff_node_t** left_child  = &(*root)->left;
  huff_node_t** right_child = &(*root)->right;

  HT_LOG("Attempting insert left for %d. cur_pos:%d. code_len:%d.\n", val, cur_pos, code_len);
  if (huff_table_insert(left_child, code_len, cur_pos+1, val))
  {
    return true;
  }

  HT_LOG("Attempting insert right for %d. cur_pos:%d. code_len:%d.\n", val, cur_pos, code_len);
  if (huff_table_insert(right_child, code_len, cur_pos+1, val))
  {
    return true;
  }

  return false;
}

#if 0
unsigned char huff_table_lookup(huff_node_t* root, const unsigned code, const unsigned code_len, const unsigned cur_shift)
{
  if (root == NULL)
  {
    printf("ERROR! Root is NULL! code:%d, cur_shift:%d\n", code, cur_shift);
    return INTERMEDIATE_NODE_VAL;
  }

  if (cur_shift == code_len) // We're at the right level. This should be the final stop.
  {
    return root->val;
  }

  // TODO(kaiyen): Can this be done better?
  const unsigned char bit = (code >> (code_len - 1 - cur_shift)) & 1;

  if (bit == 0)
  {
    return huff_table_lookup(root->left, code, code_len, cur_shift + 1);
  }
  else
  {
    return huff_table_lookup(root->right, code, code_len, cur_shift + 1);
  }
}
#endif

unsigned char huff_table_lookup(const huff_node_t* root, const unsigned char* const block, unsigned* offset)
{
  if (root == NULL)
  {
    printf("FOUND NULL. offset: %d\n", *offset);
    return 0x0;
  }

  // Success case
  if (root && root->left == NULL && root->right == NULL)
  {
    printf("SUCCESS: val:0x%X, offset: %d\n", root->val, *offset);
    return root->val;
  }

  int bit = read_stream(block, *offset, 1);
  (*offset)++;

  if (bit == 0)
  {
    printf("Going Left. offset:%d\n", *offset);
    return huff_table_lookup(root->left, block, offset);
  }
  else
  {
    printf("Going Right. offset:%d\n", *offset);
    return huff_table_lookup(root->right, block, offset);
  }
}

void huff_table_cleanup(huff_node_t* root)
{
  if (root == NULL)
  {
    return;
  }

  huff_table_cleanup(root->left);
  huff_table_cleanup(root->right);

  root->left = root->right = NULL;

  free(root);
}
