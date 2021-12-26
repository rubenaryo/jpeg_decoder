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
    printf("no args.\n");
    return EXIT_FAILURE;
  }
  
  FILE* jpeg = fopen(argv[1], "rb");

  if (jpeg == NULL)
  {
    printf("Failed to open '%s'\n", argv[1]);
    return EXIT_FAILURE;
  }

  populate_maps();
  
  fseek(jpeg, 0, SEEK_END);
  int byte_size = (int) ftell(jpeg);
  printf("ftell %d bytes\n", byte_size); 

  unsigned char* img_buf = (unsigned char*)malloc(byte_size);
  if (img_buf == NULL)
  {
    printf("Failed to allocate buffer for image.\n");
    return EXIT_FAILURE;
  }
  
  fseek(jpeg, 0, SEEK_SET);

  byte_size = (int)fread(img_buf, sizeof(unsigned char), byte_size, jpeg);
  printf("fread %d bytes \n", byte_size); 

  jfif_stage_t cur_stage;
  if (!get_stage(JFIF_SOI, &cur_stage))
  {
    printf("Failed to get initial stage.\n");
    free(img_buf);
    return EXIT_FAILURE;
  }
  
  for (int s = 0; s < byte_size; ++s)
  {
    if (img_buf[s] == JFIF_MFF && get_stage(img_buf[s+1], &cur_stage))
    {
      printf("Processing %s...\n", cur_stage.name);
      s = s + 2;
      continue;
    }

    cur_stage.process_func(img_buf[s]);
  }

  free(img_buf);
  return EXIT_SUCCESS;
}
