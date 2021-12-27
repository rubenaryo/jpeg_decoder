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

  populate_stage_map();
  
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

  jfif_stage_t cur_stage;
  if (!get_stage(JFIF_SOI, &cur_stage))
  {
    printf("Failed to get initial stage.\n");
    free(img_buf);
    return EXIT_FAILURE;
  }

  // TODO(kaiyen): Maintain iterators instead of using a counter.
  for (unsigned short s = 0; s < byte_size;)
  {
    if (img_buf[s] == JFIF_MFF && get_stage(img_buf[s+1], &cur_stage))
    {
      printf("Processing %s ", cur_stage.name);
      s += sizeof(unsigned short);
    }
    
    unsigned short stage_len = cur_stage.process_func(&img_buf[s]);

    printf(" DONE\n");
    if (cur_stage.callback_func)
      cur_stage.callback_func();
    
    s += stage_len;
  }

  free(img_buf);
  return EXIT_SUCCESS;
}
