#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define HSIZE 199
#define VSIZE 199
#define HSIZE2 (HSIZE/2)
#define VSIZE2 (VSIZE/2)
#define TRUE 1
#define FALSE 0
/* X1, Y1, etc are initial bits to be set in the matrix. Choose well!! */
#define X1 0
#define Y1 0
#define X2 150
#define Y2 50
#define X3 75
#define Y3 124
#define X4 112
#define Y4 162

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
        total += drand48()/hsearch;
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
