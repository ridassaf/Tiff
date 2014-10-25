#include <stdio.h>
#include <stdlib.h>
#include "tiffio.h"
#include <string.h>

//done: sizes, dimensions, encoding
//need: type, offset
int main(int argc, char* argv[]) {
	TIFF* tif = TIFFOpen(argv[1], "r");		
	if (tif) {
	  uint32 *width, *length;
		short *depth;
		char *doc_name;
		long int *offset;
        int dimensions = 2;
        short encoding = 0;

	width = (uint32*)malloc(sizeof(uint32));
	length = (uint32*)malloc(sizeof(uint32));
	offset = (long*)malloc(sizeof(long));
	depth = (short*)malloc(sizeof(short));

		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, width);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, length);
		TIFFGetField(tif, TIFFTAG_IMAGEDEPTH, depth);
		TIFFGetField(tif, TIFFTAG_STRIPOFFSETS, offset);
        TIFFGetField(tif, TIFFTAG_COMPRESSION, &encoding);
//		TIFFGetField(tif, TIFFTAG_DOCUMENTNAME, &doc_name);
        
        if (encoding != 1) {
            printf("Error: Not Raw data\n");
            return 0;
        }
        if (depth > 0) {
            dimensions  = 3;
        }
		
		printf("1 Width: %d\n", *width);
		printf("1 Length: %d\n", *length);
		//printf("1 Depth: %hu\n", *depth);
		printf("1 Offset: %ld\n", *offset);
//		printf("Doc Name: %s\n", doc_name);
		
		FILE *output = fopen("test.txt", "w");
		
		TIFFClose(tif);
	}
	
	return 0;
}
