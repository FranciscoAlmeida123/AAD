//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// custom functions to read and write PPM files
//

#ifndef AAD_PPM
#define AAD_PPM

//
// data type
//
// binary PPM image file format with 8 bits RGB color components (https://netpbm.sourceforge.net/doc/ppm.html)
//   ascii header data: "P6\nWIDTH HEIGHT\n255\n" (WIDTH and HEIGHT are numbers, we enforce the new lines)
//   binary data (after the header): 3*width*height bytes, organized by rows, columns, (r,g,b) pixels
//

#ifndef PPM_MAX_WIDTH
# define PPM_MAX_WIDTH  3840
#endif
#ifndef PPM_MAX_HEIGHT
# define PPM_MAX_HEIGHT  2160
#endif

typedef struct
{
  int width;               // width of the image
  int height;              // height of the image
  int header_size;         // the number of bytes of the PPM header
  int file_size;           // the PPM file size (header_size + rgb_size)
  int rgb_size;            // RGB data size: 3*width*height
  int data_size;           // the number of bytes allocated (PPM header + RGB data + SHA1 trailer, a multiple of 64)
  unsigned char *data;     // all data (PPM header + RGB data + SHA1 trailer)
  unsigned char *rgb_data; // pointer to the start of the RGB data
}
ppm_t;


//
// create a P6 PPM image data structure and fill its RGB data with all zeros, append a SHA1 trailer at the end
//
// return value: 0 (success), -1 (failure)
// irrespective of the return value, if ppm->data != NULL after the function call, then to avoid a memory leak use free(ppm->data) to free it
//

static int create_ppm_image(int width,int height,ppm_t *ppm)
{
  char ppm_header[32];
  int i;

  ppm->data = NULL;
  if(width < 1 || width > PPM_MAX_WIDTH || height < 1 || height > PPM_MAX_HEIGHT)
  {
    fprintf(stderr,"create_ppm_image: bad width or height\n");
    return -1;
  }
  // create header
  ppm->width = width;
  ppm->height = height;
  ppm->header_size = (int)snprintf(ppm_header,sizeof(ppm_header),"P6\n%d %d\n255\n",width,height);
  if(ppm->header_size >= (int)sizeof(ppm_header))
  {
    fprintf(stderr,"create_ppm_image: ppm_header[] overflow!\n");
    return -1;
  }
  ppm->rgb_size = 3 * width * height;
  // allocate memory
  ppm->file_size = ppm->header_size + ppm->rgb_size;
  ppm->data_size = ppm->file_size + 9; // the smallest SHA1 trailer is 9 bytes
  if(ppm->data_size % 64 != 0)
    ppm->data_size += 64 - ppm->data_size % 64; // make sure that the entire data size is a multiple of 64
  ppm->data = (unsigned char *)calloc((size_t)ppm->data_size,sizeof(unsigned char));
  if(ppm->data == NULL)
  {
    fprintf(stderr,"create_ppm_image: out of memory\n");
    return -1;
  }
  ppm->rgb_data = &ppm->data[ppm->header_size];
  // store PPM header
  for(i = 0;i < ppm->header_size;i++)
    ppm->data[i] = (unsigned char)ppm_header[i];
  // append SHA1 trailer (see aad_sha1_cpu_tests.c)
  i = ppm->header_size + ppm->rgb_size;
  ppm->data[i++] = (unsigned char)0x80; // padding of one 1 bit followed by seven 0 bits
  while(i % 64 != 56)
    ppm->data[i++] = (unsigned char)0x00; // zero padding --- not really needed because of the calloc(), but we do it anyway
  unsigned long n_bits = 8ul * (unsigned long)ppm->file_size;
  for(int shift = 56;shift >= 0;shift -= 8)
    ppm->data[i++] = (unsigned char)(n_bits >> shift); // append the number of bits of the message
  if(i != ppm->data_size)
  {
    fprintf(stderr,"create_ppm_image): internal error\n");
    return -1;
  }
  // done
  return 0; // all is well
}


//
// read a P6 PPM file and append a SHA1 trailer
//
// return value: 0 (success), -1 (failure)
// irrespective of the return value, if ppm->data != NULL after the function call, then to avoid a memory leak use free(ppm->data) to free it
//

int read_ppm_file(const char *file_name,ppm_t *ppm)
{
  int width,height,max;
  FILE *fp;

  ppm->data = NULL;
  // open file
  fp = fopen(file_name,"rb");
  if(fp == NULL)
  {
    fprintf(stderr,"read_ppm_file(\"%s\"): unable to open file\n",file_name);
    return -1;
  }
  // read and validate header
  if(fscanf(fp,"P6 %d%d%d",&width,&height,&max) != 3 || width < 1 || width > PPM_MAX_WIDTH || height < 1 || height > PPM_MAX_HEIGHT || max != 255 || fgetc(fp) != '\n')
  {
    fclose(fp);
    fprintf(stderr,"read_ppm_file(\"%s\"): bad header\n",file_name);
    return -1;
  }
  // create image
  if(create_ppm_image(width, height,ppm) != 0)
  {
    fclose(fp);
    fprintf(stderr,"read_ppm_file(\"%s\"): failed\n",file_name);
    return -1;
  }
  // read and save RGB data
  if(fread((void *)&ppm->data[ppm->header_size],(size_t)1,(size_t)ppm->rgb_size,fp) != (size_t)ppm->rgb_size)
  {
    fclose(fp);
    fprintf(stderr,"read_ppm_file(\"%s\"): incomplete RGB data\n",file_name);
    return -1;
  }
  // done
  fclose(fp);
  return 0; // all is well
}


//
// write a P6 PPM file
//
// return value: 0 (success), -1 (failure)
//   on error, the file may have been created; if so, you will need to delete it using unlink(file_name) or remove(file_name)
//

int write_ppm_file(const char *file_name,ppm_t *ppm)
{
  FILE *fp;

  // open file
  fp = fopen(file_name,"wb");
  if(fp == NULL)
  {
    fprintf(stderr,"write_ppm_file(\"%s\"): unable to open file\n",file_name);
    return -1;
  }
  // write everything
  if(fwrite((void *)ppm->data,(size_t)1,(size_t)ppm->file_size,fp) != (size_t)ppm->file_size || fflush(fp) != 0 || fclose(fp) != 0)
  {
    fclose(fp);
    fprintf(stderr,"write_ppm_file(\"%s\"): write error\n",file_name);
    return -1;
  }
  // done
  return 0; // all is well
}

#endif
