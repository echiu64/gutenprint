/* Dither matrix generation                       */
/* This program is written 2000 by Thomas Tonino. */
/* It is placed in the public domain.             */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define HSIZE 73
#define VSIZE 73
#define HSIZE2 (HSIZE/2)
#define VSIZE2 (VSIZE/2)
#define TRUE 1
#define FALSE 0
/* X1, Y1, etc are initial bits to be set in the matrix. Choose well!! */
#define X1 2
#define Y1 1
#define X2 48
#define Y2 24
#define X3 24
#define Y3 49

int main(void)
{
int x, y, xd, yd, i, j, bitcount, largehole, largetotal, result[HSIZE*VSIZE];
float distance, thishole, maximum, total;
int initial[HSIZE*VSIZE];

/* Set up array with zeroes and a number of 'ones' */
bitcount=0;
for ( x=0; x<HSIZE; ++x )
  for ( y=0; y<VSIZE; ++y )
    {
    initial[x+y*HSIZE]=FALSE;
    result[x+y*HSIZE]=0;
    }

initial[X1+Y1*HSIZE]=TRUE;
result[X1+Y1*HSIZE]=bitcount++;
initial[X2+Y2*HSIZE]=TRUE;
result[X2+Y2*HSIZE]=bitcount++;
initial[X3+Y3*HSIZE]=TRUE;
result[X3+Y3*HSIZE]=bitcount++;

while (bitcount < VSIZE*HSIZE)
  {
  maximum=0;
/* Start looking for the largest hole */
  for ( i=0; i<HSIZE; ++i )
    for ( j=0; j<VSIZE; ++j )
      {
      total=0;
/* check only if it is a hole right now */
      if (initial[i+j*HSIZE]==FALSE)
        {
        for ( x=0; x<HSIZE; ++x )
          for ( y=0; y<VSIZE; ++y )
/* scan all positions, find distance, munge and add */
            if (initial[x+y*HSIZE]==TRUE)
              {
              xd = abs(x - i);
              yd = abs(y - j);
              if (xd > HSIZE2)
                xd = HSIZE - xd;
              if (yd > VSIZE2)
                yd = VSIZE - yd;
              distance = sqrt(xd*xd+yd*yd);
              total+=(1-1/distance);
              }
        if (total > maximum)
          {
/* it is the largest so far */
          largetotal = i+j*HSIZE;
          maximum = total;
          }
	}
      }
/* put a "1" in the largest hole */
  initial[largetotal] = TRUE;
  result[largetotal]=bitcount++;

/* print the result so far */
  for ( i=0; i<HSIZE; ++i )
    {
    for ( j=0; j<VSIZE; ++j )
      if (initial[i+j*HSIZE])
        printf("@");
      else
        printf("-");

    printf("\n");
    }
  }
/* print result */
printf("\n\n");
for (j=0; j<HSIZE*VSIZE; ++j)
  printf("%d, ",result[j]);
printf("\n\n");
}
