#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "tiffio.h"

void spec(int min, int size, int *hist, int *oldHist, unsigned short **matched){
  int start = 0;
  int minDist;
  for (int i = 0; i < size; i++){
    minDist = 65536;
    int flag = 0;
    for (int j = start; j < 65536; j++){
      if (abs(hist[i] - oldHist[j]) < minDist){
	minDist = abs(hist[i] - oldHist[j]);
	(*matched)[i] = j;
	start = j;
      }
      else if (minDist < 65536 && abs(hist[i] - oldHist[j]) == minDist){
	if (flag == 0)
	  start = j;
	flag = 1;
      }
      else if (minDist < 65536 && abs(hist[i] - oldHist[j]) > minDist){
	break;
      }
    }
  }
}

void spec_int(int min, int size, int *hist, int *oldHist, unsigned short **matched, int interval){
  int start = 0;
  int minDist;
  for (int i = 0; i < size; i++){
    minDist = 65536;
    int flag = 0;
    for (int j = start; j < 65536; j++){
      if (abs(hist[i] - oldHist[j]) < minDist){
	minDist = abs(hist[i] - oldHist[j]);
	(*matched)[i] = (unsigned short)(j*interval);
	start = j;
      }
      else if (minDist < 65536 && abs(hist[i] - oldHist[j]) == minDist){
	if (flag == 0)
	  start = j;
	flag = 1;
      }
      else if (minDist < 65536 && abs(hist[i] - oldHist[j]) >= minDist){
	break;
      }
    }
  }
}

int main(int argc, char* argv[]){
  TIFFSetWarningHandler(NULL);
  //make sure inputs correct
  if (argc != 3 && !(argc == 4 && strncmp(argv[3], "-l", 3) == 0)) {
    printf("Correct usage: <input file> <interval / numBins if log> <-l>\n");
    return -1;
  }
    
  int logFlag = 0;
  if (argc == 4){
    logFlag = 1;
  }

  //open and process header file to get arguments
  TIFF * in = TIFFOpen(argv[1], "r");
  if (in == NULL){
    printf("Header file cannot be opened\n");
    return -1;
  }

  int rows, cols;  
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, &rows);
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &cols);
  int numFiles = 0;
  int interval, numBins;
  if (logFlag == 0){
    interval = atoi(argv[2]);
  }
  else{
    numBins = atoi(argv[2]);
  }

  do{
    numFiles++; 
  }while(TIFFReadDirectory(in));

  //set variables and allocate memory
  TIFFSetDirectory(in, 0); 
  unsigned short minOrg;
  unsigned short maxOrg;
  double ratio;
  int dir = 0;
  int *oHist, *ocHist;
  unsigned short *endPointsO;
  long double minLog;
  long double maxLog;
  if (logFlag == 0){
    oHist = calloc((int)(65536/interval), sizeof(int));
    ocHist = calloc((int)(65536/interval), sizeof(int));    
  }
  else {
    oHist = (int*)calloc(numBins, sizeof(int));
    ocHist = (int *)calloc(numBins, sizeof(int));
    endPointsO = (unsigned short*)malloc(numBins*sizeof(unsigned short));
  }
  float *alphas = (float *)malloc(sizeof(float)*numFiles);
  float *betasI = (float *)malloc(sizeof(float)*numFiles);
  float *betas = (float *)malloc(sizeof(float)*numFiles);

  char out_img[256];
  char prefix[256];
  strncpy(prefix, argv[1], strlen(argv[1]) - 5);
  if (logFlag == 0)
    sprintf(out_img, "%s-adj%d.tiff", prefix, atoi(argv[2]));
  else
    sprintf(out_img, "%s-adjlog%d.tiff", prefix, atoi(argv[2]));
  TIFF* out = TIFFOpen(out_img, "w");
  
  if (out == NULL){
    printf("Output file %s cannot be opened\n", out_img);
    return -1;
  }

  do{
    printf("%i\n", dir);
    //set up histogram variables
    unsigned short minBin = 65535;
    unsigned short maxBin = 0;
    unsigned short *array = (unsigned short *)calloc(rows*cols, sizeof(unsigned short));
    unsigned short *adj_array = (unsigned short*)calloc(rows*cols, sizeof(unsigned short));
    unsigned short *endPoint;
    int *record;

    //get data from input file
    for (int row = 0; row < rows; row++){
      TIFFReadScanline(in, &array[row*cols], row, 0);
    }

    //find the min and the max
    for (int i =0; i < rows*cols; i++) {
      if (array[i] < minBin)
	minBin = array[i];
      if (array[i] > maxBin)
	maxBin = array[i];
    }

    // make the histograms
    if (logFlag == 0){
      numBins = (int)((maxBin - minBin)/(interval)) + 1;
    }
    else{
      record = calloc(rows*cols, sizeof(int));
      minLog = log1p(minBin);
      maxLog = log1p(maxBin);
      endPoint = (unsigned short*)malloc(sizeof(unsigned short)*(numBins));
      for (int i = 1; i <= numBins; i++){
	endPoint[i - 1] = (unsigned short)(expm1(minLog + (maxLog - minLog)*i/numBins));
	if (dir == 0)
	  endPointsO[i - 1] = (unsigned short)(expm1(minLog + (maxLog - minLog)*i/numBins));
      }
    }

    int *hist = calloc(numBins, sizeof(int));
    int *cHist = calloc(numBins, sizeof(int));
    if (dir == 0) {
      minOrg = minBin;
      maxOrg = maxBin;
      for (int i = 0; i < rows*cols; i++){
	if (logFlag == 0){
	  int bin = (int)((array[i]-minBin)/interval);
	  if (bin < numBins)
	    oHist[bin]++;
	  else if (bin == numBins)
	    oHist[bin - 1]++;
	}
	else {
	  for (int j = 0; j < numBins; j++){
	    if (array[i] <= endPoint[j]){
	      oHist[j]++;
	      break;
	    }  
	  }
	}
      }
      int total = 0;
      for (int i = 0; i < numBins; i++){
	total += oHist[i];
	ocHist[i] = total;
      }
    }

    //initial histogram
    for (int i = 0; i < rows*cols; i++){
      if(logFlag == 0){
	int bin = (int)((array[i]-minBin)/interval);
	if (bin < numBins)
	  hist[bin]++;
	else if (bin == numBins)
	  hist[bin - 1]++;
      }
      else {
	for (int j = 0; j < numBins; j++){
	  if (array[i] <= endPoint[j]){
	    hist[j]++;
	    record[i] = j;
	    break;
	  }
	}
      }
    }  

    //cummulative histogram
    int total = 0;
    for (int i = 0; i < numBins; i++){
      total += hist[i];
      cHist[i] = total;
    }
    free(hist);
	 
    //get the new values and histogram bins
    unsigned short *matching = (unsigned short*)malloc(numBins*sizeof(unsigned short));
    if (logFlag == 0){
      spec_int(minBin, numBins, cHist, ocHist, &matching, interval);
    }
    else{
      spec(minBin, numBins, cHist, ocHist, &matching);
    }

    free(cHist);

    float x_avg = 0;
    float y_avg = 0;
    float xy_avg = 0;
    float x2_avg = 0;
    
    for (int i = 0; i < rows*cols; i++) {
      if (logFlag == 0){
	int mod = (array[i] - minBin) % interval;
	if (mod == 0){
	  adj_array[i] = (unsigned short)matching[(int)((array[i] - minBin)/interval)];	
	}
	else {
	  int x_0 = array[i] - mod;
	  int x_1 = x_0 + interval;
	  adj_array[i] = (unsigned short)(matching[(int)((x_0 - minBin)/interval)] + (matching[(int)((x_1 - minBin)/interval)] - matching[(int)((x_0 - minBin)/interval)])*(array[i] - x_0)/(x_1 - x_0)); 
	  if (adj_array[i] >= 65535){
	    adj_array[i] = 65534;
	  }
	}
      }
      else{
	int bin = record[i];
	unsigned short x_0, y_0;
	if (bin == 0){
	  x_0 = minBin;
	}
	else{
	  x_0 = endPoint[bin - 1];
	}
	if (matching[bin] == 0){
	  y_0 = minOrg;
	}
	else{
	  y_0 = endPointsO[matching[bin] - 1];
	}
	unsigned short x_1 = endPoint[bin];
	unsigned short y_1 = endPointsO[matching[bin]];
	adj_array[i] = (unsigned short)(y_0 + (y_1 - y_0)*(array[i] - x_0)/(x_1 - x_0));
      }
      //OLS information
      y_avg += adj_array[i];
      x_avg += array[i];
      xy_avg += adj_array[i]*array[i];
      x2_avg += adj_array[i]*adj_array[i];	  
    }

    if (logFlag == 1){
      free(record);
      free(endPoint);
    }
    free(array);
    //OLS imformation
    y_avg = y_avg/(rows*cols);
    x_avg = x_avg/(rows*cols);
    xy_avg = xy_avg/(rows*cols);
    x2_avg = x2_avg/(rows*cols);
    betas[dir] = (xy_avg - x_avg*y_avg)/(x2_avg - x_avg*x_avg);
    alphas[dir] = y_avg - betas[dir]*x_avg;
    betasI[dir] = 1/betas[dir];

    //write the TIF file
    int temp;
    TIFFSetField(out, TIFFTAG_IMAGEWIDTH, cols); 
    TIFFSetField(out, TIFFTAG_IMAGELENGTH, rows);
    TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &temp);
    TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, temp);
    temp = 0;
    TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &temp);
    TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, temp);
    temp = 0;
    TIFFGetField(in, TIFFTAG_ORIENTATION, &temp);
    TIFFSetField(out, TIFFTAG_ORIENTATION, temp);
    TIFFGetField(in, TIFFTAG_PLANARCONFIG, &temp);
    TIFFSetField(out, TIFFTAG_PLANARCONFIG, temp);
    TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &temp);
    TIFFSetField(out, TIFFTAG_PHOTOMETRIC, temp);
    temp = 0;
    TIFFGetField(in, TIFFTAG_ROWSPERSTRIP, &temp);
    TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, temp);
    temp = 0;
    TIFFGetField(in, TIFFTAG_THRESHHOLDING, &temp);
    TIFFSetField(out, TIFFTAG_THRESHHOLDING, temp);
    temp = 0;
    TIFFGetField(in, TIFFTAG_XRESOLUTION, &temp);
    TIFFSetField(out, TIFFTAG_XRESOLUTION, temp);
    temp = 0;
    TIFFGetField(in, TIFFTAG_YRESOLUTION, &temp);
    TIFFSetField(out, TIFFTAG_YRESOLUTION, temp);
    temp = 0;
    TIFFGetField(in, TIFFTAG_RESOLUTIONUNIT, &temp);
    TIFFSetField(out, TIFFTAG_RESOLUTIONUNIT, 1);
    TIFFSetField(out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
    TIFFSetField(out, TIFFTAG_PAGENUMBER, dir, numFiles);

    unsigned short* buf = _TIFFmalloc(cols*sizeof(unsigned short));
    for (int row = 0; row < rows; row++) {
      memcpy(buf, &adj_array[row*cols], cols*sizeof(unsigned short));
      TIFFWriteScanline(out, buf, row, 0);    
    }
    TIFFWriteDirectory(out);
    dir++;

    //free allocated memory
    free(adj_array);
    _TIFFfree(buf);
    free(matching);


  } while(TIFFReadDirectory(in));

  //close files
  TIFFClose(in);
  TIFFClose(out);

  //plot the affine transformation
  FILE *gnuplot = popen("gnuplot", "w");
  fprintf(gnuplot, "set term png large\n");
  if (logFlag == 0){
    fprintf(gnuplot, "set output 'alphas%s.png'\n", argv[2]);
  }
  else {
    fprintf(gnuplot, "set output 'alphasLog%s.png'\n", argv[2]);
  }
  fprintf(gnuplot, "plot '-' \n");
  for (int i = 0; i < numFiles; i++) {
    fprintf(gnuplot, "%i %f\n", i, alphas[i]);
  }
  fprintf(gnuplot, "e\n");

  fprintf(gnuplot, "set term png large\n");
  if (logFlag == 0){
    fprintf(gnuplot, "set output 'betas%s.png'\n", argv[2]);
  }
  else{
    fprintf(gnuplot, "set output 'betasLog%s.png'\n", argv[2]);  
  }
  fprintf(gnuplot, "plot '-' \n");
  for (int i = 0; i < numFiles; i++) {
    fprintf(gnuplot, "%i %f\n", i, betas[i]);
  }
  fprintf(gnuplot, "e\n");

  fprintf(gnuplot, "set term png large\n");
  if (logFlag == 0){
    fprintf(gnuplot, "set output 'betasInverse%s.png'\n", argv[2]);
  }
  else{
    fprintf(gnuplot, "set output 'betasInverseLog%s.png'\n", argv[2]);
  }
  fprintf(gnuplot, "set yrange [0:20]\n");
  fprintf(gnuplot, "plot '-' \n");
  for (int i = 0; i < numFiles; i++) {
    fprintf(gnuplot, "%i %f\n", i, betasI[i]);
  }
  fprintf(gnuplot, "e\n");
  fflush(gnuplot);

  //free rest of memory
  if (logFlag == 1){
    free(endPointsO);
  }
  free(alphas);
  free(betasI);
  free(betas);
}
