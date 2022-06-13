#include <string.h>
#include <stdio.h>
#include "fitsio.h"
#include <emscripten.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>

const char *inputFitsFile = "temp.fits";

long flag_and_rank_ts(double *xtin, float *ytin, double *xt, float *yt, float *qt, double *st, long n, int binFAC) {
  // Want to set data with no finite time to -1
  // set data with no finite pdc to -2
  // Also bin valid data by binFAC
  size_t i;
  double xmin=DBL_MAX, xmax=DBL_MIN;
  float ymin=FLT_MAX, ymax=FLT_MIN;
  double ysum=0.0, ysq_sum=0.0, ymean, yvariance, curxsum, curysum;
  double dblbinFAC;
  float fltbinFAC;
  long yusen=0, binpos, curcnt;

  dblbinFAC = (double) binFAC;
  binpos = 0; // binpos will keep track of the binned index
  curcnt = 0; // curcnt will keep track of the current number of measurements in the bin
              // when it reaches binFAC close off bin and start new one
  curxsum = 0.0; // current sum of measurements in the current bin will be used to calcule mean over bin
  curysum = 0.0;
  for (i=0; i<n; i++) {
    if (isfinite(xtin[i]) && isfinite(ytin[i])) {
      // both x and y data are valid add to bin accumulation
      curxsum += xtin[i];
      curysum += (double)ytin[i];
      curcnt += 1;
      if (curcnt == binFAC) { // reached end of bin do all the means and stats
        xt[binpos] = curxsum / dblbinFAC;
        yt[binpos] = (float)(curysum / dblbinFAC);
        if (xt[binpos] < xmin) xmin = xt[binpos];
        if (xt[binpos] > xmax) xmax = xt[binpos];
        if (yt[binpos] < ymin) ymin = yt[binpos];
        if (yt[binpos] > ymax) ymax = yt[binpos];
        yusen += 1;
        ysum += (double)yt[binpos];
        ysq_sum += ((double)yt[binpos]) * ((double)yt[binpos]);
        // Now reset things for a new bin
        binpos++;
        curcnt = 0;
        curxsum = 0.0;
        curysum = 0.0;
      }
    }
  }
  ymean = ysum / ((float) yusen);
  yvariance = ysq_sum / ((float) yusen) - ymean*ymean;
  st[0] = (float) xmin;
  st[1] = (float) xmax;
  st[2] = ymin;
  st[3] = ymax;
  st[4] = ymean;
  st[5] = sqrt(yvariance);
  return binpos;
}

EMSCRIPTEN_KEEPALIVE long tess_fitslc_getnrows(long* idptr) {
  // Get the number of rows in the fits lightcurve files
  // Assumes the fits file of interest has been copied over to the
  // wasm MEMFS as temp.fits
  fitsfile *fptr;
  long nrows;
  int hdutype, status=0;
  char card[FLEN_CARD];   /* Standard string lengths defined in fitsio.h */
  char headval[FLEN_VALUE], comment[FLEN_COMMENT];
  char *eptr;

  if (!fits_open_file(&fptr, inputFitsFile, READONLY, &status)) {
    // Get number of reads in first extension header
    fits_movabs_hdu(fptr, 2, &hdutype, &status);
    fits_read_card(fptr, "NUM_FRM", card, &status);
    fits_parse_value(card, headval, comment, &status);
    idptr[1]=strtol(headval, &eptr, 10);

    // Get number of nrows
    fits_get_num_rows(fptr, &nrows, &status);
    idptr[0] = nrows;
    fits_close_file(fptr, &status);
    return 0;
  } else {
    fits_close_file(fptr, &status);
    return -1;
  }

}

EMSCRIPTEN_KEEPALIVE int anyfits_getheader(){
  // Write out the headers for any fits file
  // Uses the emscripten FS memfs to write file to header.txt
  fitsfile *fptr;         /* FITS file pointer, defined in fitsio.h */
  char card[FLEN_CARD];   /* Standard string lengths defined in fitsio.h */
  int status = 0;   /* CFITSIO status value MUST be initialized to zero! */
  int nkeys, ii, hdupos;
  const char *outputHeaderFile = "header.txt";

  FILE *fpo = fopen(outputHeaderFile, "w");
  if (fpo == NULL) return -1;

  if (!fits_open_file(&fptr, inputFitsFile, READONLY, &status))
  {
    // Iterate through the headers and write them out to
    fits_get_hdu_num(fptr, &hdupos);  /* Get the current HDU position */
    for (; !status; hdupos++)  /* Main loop through each extension */
    {
      fits_get_hdrspace(fptr, &nkeys, NULL, &status); /* get # of keywords */

      fprintf(fpo,"Header listing for HDU #%d:\n", hdupos);
      for (ii = 1; ii <= nkeys; ii++) { /* Read and print each keywords */
         if (fits_read_record(fptr, ii, card, &status))break;
         fprintf(fpo,"%s\n", card);
      }
      fprintf(fpo,"END\n\n");  /* terminate listing with END */

      fits_movrel_hdu(fptr, 1, NULL, &status);  /* try to move to next HDU */
    }

    if (status == END_OF_FILE)  status = 0; /* Reset after normal error */
  }
  fclose(fpo);
  fits_close_file(fptr, &status);
  if (status) {
    fits_report_error(stderr, status); /* print any error message */
  }
  return(status);
}

EMSCRIPTEN_KEEPALIVE int tess_fitslc_export(double *dptr, float *fltptr, float *qptr, double *stptr, long* idptr, int binFAC, int scaleByMean) {
    fitsfile *fptr;         /* FITS file pointer, defined in fitsio.h */
    char card[FLEN_CARD];   /* Standard string lengths defined in fitsio.h */
    char headval[FLEN_VALUE], comment[FLEN_COMMENT];
    int status = 0;   /* CFITSIO status value MUST be initialized to zero! */
    int single = 0, hdupos, nkeys, ii, anynul, hdutype, cnt;
    long nrows, jj, useN;
    char *val, value[1000], nullstr[]="-1", *eptr; // buffers for reading in data to strings
    double *tmpdptr;
    float *tmpfptr;
    double dnul=0.0;
    float fnul=0.0;


    if (!fits_open_file(&fptr, inputFitsFile, READONLY, &status))
    {

      fits_get_hdu_num(fptr, &hdupos);  /* Get the current HDU position */
      // Read in sector from primary header
      fits_read_card(fptr, "SECTOR", card, &status);
      fits_parse_value(card, headval, comment, &status);
      idptr[1]=strtol(headval, &eptr, 10);

      // Now go to first extension to find the data sets
      fits_movabs_hdu(fptr, 2, &hdutype, &status);
      // Get number of nrows
      fits_get_num_rows(fptr, &nrows, &status);
      // Get some values from header
      fits_read_card(fptr, "TICID", card, &status);
      fits_parse_value(card, headval, comment, &status);
      idptr[0]=strtol(headval, &eptr, 10);
      fits_read_card(fptr, "NUM_FRM", card, &status);
      fits_parse_value(card, headval, comment, &status);
      idptr[2]=strtol(headval, &eptr, 10);

      // Read data from fits to data array
      // Need to allocate space for the entire data series
      tmpdptr = (double*)malloc(nrows * sizeof(double));
      if (tmpdptr == NULL) {
        printf("Memory to time array not allocated");
        return(-1);
      }
      tmpfptr = (float*)malloc(nrows * sizeof(float));
      if (tmpfptr == NULL) {
        printf("Memory for pdc array not allocated");
        return(-1);
      }
      fits_read_col(fptr, TDOUBLE, 1, 1, 1, nrows, &dnul, tmpdptr, &anynul, &status);
      fits_read_col(fptr, TFLOAT, 8, 1, 1, nrows, &fnul, tmpfptr, &anynul, &status);
      // Peform data binning and stats calculations
      useN = flag_and_rank_ts(tmpdptr, tmpfptr, dptr, fltptr, qptr, stptr, nrows, binFAC);
      free(tmpdptr);
      free(tmpfptr);
      idptr[3] = useN; // useN is number of valid data points in final binned result
      fnul = stptr[4];
      if (scaleByMean) {
        for (jj=0; jj<useN; jj++) {
          fltptr[jj] = ((fltptr[jj] / fnul) - 1.0) * 1.0e3 ;
        }
      }
    } else {
      printf("Initial open failure %d\n", status);
    }
    fits_close_file(fptr, &status);
    if (status) {
      fits_report_error(stderr, status); /* print any error message */
    }
    return(status);
}

int main(int argc, char *argv[]) {
  return 0;
}
