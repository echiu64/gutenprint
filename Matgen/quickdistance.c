/* This program is written 2000 by Thomas Tonino. */
/* It is placed in the public domain.             */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define TRUE 1
#define FALSE 0

/* 199 size matrix designed to be re-used shifted 100 pixels
 * shifted right or down
 */
#if 0
#define HSIZE 199
#define VSIZE 199
#define HSIZE2 (HSIZE/2)
#define VSIZE2 (VSIZE/2)
#define X1 0
#define Y1 0
#define X2 150
#define Y2 50
#define X3 75
#define Y3 124
#define X4 112
#define Y4 162
#endif

/* Another 199 size matrix designed to be re-used shifted 100 pixels
 * shifted right or down - should not interfere with the one above.
 */
#if 0
#define HSIZE 199
#define VSIZE 199
#define HSIZE2 (HSIZE/2)
#define VSIZE2 (VSIZE/2)
#define X1 38
#define Y1 38
#define X2 88
#define Y2 188
#define X3 162
#define Y3 113
#define X4 1
#define Y4 150
#endif

/*
/* 257x257  matrix designed to be shifted 128 pixels right or down
 */
#if 1
#define HSIZE 257
#define VSIZE 257
#define HSIZE2 (HSIZE/2)
#define VSIZE2 (VSIZE/2)
#define X1 0
#define Y1 0
#define X2 194
#define Y2 65
#define X3 96
#define Y3 160
#define X4 145
#define Y4 209
#endif
int main(void)
{
int bitcount, x, y, initial[HSIZE*VSIZE], result[HSIZE*VSIZE], hsearch, tmp, vsearch;
int i, j, xp, yp, largetotal;
double distance, total, maximum;
srand48(time(NULL));
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
initial[X4+Y4*HSIZE]=TRUE;
result[X4+Y4*HSIZE]=bitcount++;

while (bitcount < VSIZE*HSIZE)
  {
/* 6 seems to give good results for multimplier, but smaller values are faster */
  hsearch=3*sqrt(VSIZE*HSIZE)/sqrt(bitcount);
  tmp=3*sqrt(VSIZE*HSIZE)/sqrt((VSIZE*HSIZE+1)-bitcount);
  if (tmp > hsearch)
    hsearch=tmp;
  vsearch=hsearch;
  if ( hsearch > HSIZE2 )
    hsearch = HSIZE2;
  if ( vsearch > VSIZE2 )
    vsearch = VSIZE2;
  maximum=0;
/* Start looking for the largest hole */
  for ( i=0; i<HSIZE; ++i )
    for ( j=0; j<VSIZE; ++j )
      {
      total=0;
/* check only if it is a hole right now */
      if (initial[i+j*HSIZE]==FALSE)
        {
        for ( x=-hsearch; x<hsearch; ++x )
          for ( y=-vsearch; y<vsearch; ++y )
/* scan all positions, in distance v/hsearch from (i.j), munge and add */
            {
            xp = x + i;
            yp = y + j;
            if (xp >= HSIZE)
              xp = xp - HSIZE;
            if (xp < 0)
              xp = xp + HSIZE;
            if (yp >= VSIZE)
              yp = yp - VSIZE;
            if (yp < 0)
              yp = yp + VSIZE;
            if (initial[(xp+yp*HSIZE)]==TRUE)
              {
              distance = (x*x+y*y);
              total+=(1-1/distance);
              }
            else
              total+=1;
            }
// The below value seems slightly noisy in darker tones, try adding less noise
        total += drand48()/(hsearch);
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
  if ( bitcount % 100 == 0 )
    {
    printf("%d of %d, scanning a %d size block\n",bitcount,VSIZE*HSIZE,hsearch);
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
  }
/* print result */
printf("\n\n");
for (j=0; j<HSIZE*VSIZE; ++j)
  printf("%d, ",result[j]);
printf("\n\n");
}
