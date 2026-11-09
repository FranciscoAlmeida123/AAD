//
// Tomás Oliveira e Silva,  October 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// display the chunks of a PNG file; see https://www.libpng.org/pub/png/spec/1.2/PNG-Contents.html for details
//

#include <stdio.h>
#include "aad_png_crc.h"

int main(int argc,char **argv)
{
  for(int argi = 1;argi < argc;argi++)
  {
#   define MAX_FILE_SIZE (1 << 25)
    static unsigned char file_contents[MAX_FILE_SIZE]; // we could use malloc() to adjust the array to the file size, but it is too much trouble
    FILE *fp = fopen(argv[argi],"r");
    if(fp == NULL)
    {
      fprintf(stderr,"unable to open file %s\n",argv[argi]);
      continue;
    }
    int file_size = fread((void *)file_contents,(size_t)1,(size_t)MAX_FILE_SIZE,fp);
    fclose(fp);
    if(file_size < 8 || file_contents[0] != 137 || file_contents[1] !=  80 || file_contents[2] !=  78 || file_contents[3] !=  71
                     || file_contents[4] !=  13 || file_contents[5] !=  10 || file_contents[6] !=  26 || file_contents[7] !=  10)
    {
      fprintf(stderr,"%s is not a PNG file\n",argv[argi]);
      continue;
    }
    printf("%s\n",argv[argi]);
    printf("  %d byte%s\n",file_size,(file_size == 1) ? "" : "s");
    printf("  PNG signature at index 0\n");
    for(int idx = 8; idx < file_size;)
    {
      if(idx + 12 > file_size)
      {
        printf("  incomplete last chunk\n");
        idx = file_size;
      }
      int length;
      length  = (int)file_contents[idx + 0] << 24;
      length += (int)file_contents[idx + 1] << 16;
      length += (int)file_contents[idx + 2] << 8;
      length += (int)file_contents[idx + 3] << 0;
      printf("  chunk %c%c%c%c at index %d with data length %d",file_contents[idx + 4],file_contents[idx + 5],file_contents[idx + 6],file_contents[idx + 7],idx,length);
      unsigned int crc = compute_png_crc((const char *)&file_contents[idx + 4],length + 4);
#if 1
      int chunk_id;
      chunk_id  = (int)file_contents[idx + 4] << 24;
      chunk_id += (int)file_contents[idx + 5] << 16;
      chunk_id += (int)file_contents[idx + 6] << 8;
      chunk_id += (int)file_contents[idx + 7] << 0;
      //printf(" <%08X>",chunk_id);
      if(chunk_id == 0x74455874u)
      {
        printf(" [");
        for(int i = 0;i < length;i++)
          if(file_contents[idx + 8 + i] == 0)
            printf("|");
          else
            printf("%c",file_contents[idx + 8 + i]);
        printf("]");
      }
#endif
      idx += 8 + length;
      unsigned int chunk_crc;
      chunk_crc  = (unsigned int)file_contents[idx + 0] << 24;
      chunk_crc += (unsigned int)file_contents[idx + 1] << 16;
      chunk_crc += (unsigned int)file_contents[idx + 2] << 8;
      chunk_crc += (unsigned int)file_contents[idx + 3] << 0;
      idx += 4;
      printf(" (%s crc)\n",(crc == chunk_crc) ? "good" : "bad");
    }
#   undef MAX_FILE_SIZE
  }
  return 0;
}
