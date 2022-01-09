/*--------------------------------------------------------------------------
File:   main.cpp
Date:   2021/12/23
Author: kaiyen
---------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>

#include "decoder.h"

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    printf("Missing argument <jpeg file>.\n");
    return EXIT_FAILURE;
  }

  FILE* jpeg = fopen(argv[1], "rb");

  if (jpeg == NULL)
  {
    printf("Failed to open '%s'\n", argv[1]);
    return EXIT_FAILURE;
  }

  fseek(jpeg, 0, SEEK_END);
  int byte_size = (int) ftell(jpeg);

  unsigned char* img_buf = (unsigned char*)malloc(byte_size);
  if (img_buf == NULL)
  {
    printf("Failed to allocate buffer for image.\n");
    return EXIT_FAILURE;
  }

  fseek(jpeg, 0, SEEK_SET);

  byte_size = (int)fread(img_buf, sizeof(unsigned char), byte_size, jpeg);

  fclose(jpeg);

  process_func_t process_func = NULL;
  char segment_name_buf[64];
  if (!get_segment_process_func(JFIF_SOI, &process_func, segment_name_buf))
  {
    printf("Failed to get initial stage.\n");
    free(img_buf);
    return EXIT_FAILURE;
  }

  // TODO(kaiyen): Maintain iterators instead of using a counter.
  for (unsigned short s = 0; s < byte_size;)
  {
    if (img_buf[s] == JFIF_MFF)
    {
      if (get_segment_process_func(img_buf[s+1], &process_func, segment_name_buf))
      {
        printf("> Processing %s ", segment_name_buf);
      }
      else
      {
        get_default_stage(img_buf[s+1], &process_func, segment_name_buf);
        printf("> Skipping %s ", segment_name_buf);
      }

      s += sizeof(unsigned short);
    }

    unsigned short stage_len = process_func(&img_buf[s]);

    s += stage_len;
  }

  free(img_buf);
  return EXIT_SUCCESS;
}
