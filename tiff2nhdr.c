#include <stdio.h>
#include <stdlib.h>
#include "tiffio.h"
#include <string.h>

//done: sizes, dimensions, encoding, offset, byte_order
//need: type, multiDirectory
int main(int argc, char* argv[]) {
  TIFF* tif = TIFFOpen(argv[1], "r");
  FILE *f = fopen(argv[1], "r");

  if (tif) {
    uint32 *width, *length;
    short *depth;
    long  **offset;
    short *sample_format;
    int dimensions = 2;
    short encoding = 0, bps = 0;
    char *byte_order = (char*) malloc(2 *sizeof(char));
    fread(byte_order, sizeof(char), 2, f);
    fclose(f);
    
    width = (uint32*)malloc(sizeof(uint32));
    length = (uint32*)malloc(sizeof(uint32));
    depth = (short*)malloc(sizeof(short));
    
    offset = (long**) malloc(sizeof(long*));
    offset[0] = (long*) malloc(sizeof(long));
    sample_format = (short*)malloc(sizeof(short));

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, length);
    TIFFGetField(tif, TIFFTAG_IMAGEDEPTH, depth);

    TIFFGetField(tif, TIFFTAG_STRIPOFFSETS, offset);
    TIFFGetField(tif, TIFFTAG_COMPRESSION, &encoding);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, sample_format);

    if (encoding != 1) {
      printf("Error: Not Raw data\n");
      return 0;
    }
    if (depth > 0) {
      dimensions  = 3;
    }
    
    printf("Width: %d\n", *width);
    printf("Length: %d\n", *length);
    printf("Depth: %hu\n", *depth);
    printf("Offset: %ld\n", *offset[0]);
    printf("Byte Order: %c%c\n", byte_order[0], byte_order[1]);
    printf("Bits per sample: %d\n", bps);
    printf("Data Type: %hu\n", *sample_format);
    
    FILE *output = fopen("test.txt", "w");
    
    
    fclose(output);
    TIFFClose(tif);
  }
  
  return 0;
}
