/* $Id$ */
/*
 * Attempt to simulate a printer to facilitate driver testing.  Is this
 * useful?
 *
 * Copyright 2000 Eric Sharkey <sharkey@superk.physics.sunysb.edu>
 *                Andy Thaller <thaller@ph.tum.de>
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "../lib/libprintut.h"
#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>

#ifdef __GNUC__
#define inline __inline__
#endif

#undef DEBUG_CANON

typedef struct {
  unsigned char unidirectional;
  unsigned char microweave;
  int page_management_units; /* dpi */
  int relative_horizontal_units; /* dpi */
  int absolute_horizontal_units; /* dpi, assumed to be >= relative */
  int relative_vertical_units; /* dpi */
  int absolute_vertical_units; /* dpi, assumed to be >= relative */
  int horizontal_spacing;	/* Horizontal dot spacing */
  int top_margin; /* dots */
  int bottom_margin; /* dots */
  int page_height; /* dots */
  int dotsize;
  int bpp; /* bits per pixel */
  int current_color;
  int xposition; /* dots */
  int yposition; /* dots */
  int monomode;
  int nozzle_separation;
  int nozzles;
  int extraskip;
} pstate_t;

/* We'd need about a gigabyte of ram to hold a ppm file of an 8.5 x 11
 * 1440 x 720 dpi page.  That's more than I have in my laptop, so, let's
 * play some games to reduce memory.  Allocate each scan line separately,
 * and don't require that the allocated height be full page width.  This
 * way, if we only want to print a 2x2 image, we only need to allocate the
 * ram that we need.  We'll build up the printed image in ram at low
 * color depth, KCMYcm color basis, and then write out the RGB ppm file
 * as output.  This way we never need to have the full data in RAM at any
 * time.  2 bits per color of KCMYcm is half the size of 8 bits per color
 * of RBG.
 */
#define MAX_INKS 7
typedef struct {
   unsigned char *line[MAX_INKS];
   int startx[MAX_INKS];
   int stopx[MAX_INKS];
} line_type;

typedef unsigned char ppmpixel[3];

unsigned char buf[256*256];
unsigned char minibuf[256];
unsigned short bufsize;
unsigned char ch;
unsigned short sh;

pstate_t pstate;
int unweave;


line_type **page=NULL;


/* Color Codes:
   color    Epson1  Epson2   Sequential
   Black    0       0        0
   Magenta  1       1        1
   Cyan     2       2        2
   Yellow   4       4        3
   L.Mag.   17      257      4
   L.Cyan   18      258      5
   L.Yellow NA      NA       6
 */

/* convert either Epson1 or Epson2 color encoding into a sequential encoding */
#define seqcolor(c) (((c)&3)+(((c)&276)?3:0))  /* Intuitive, huh? */
/* sequential to Epson1 */
#define ep1color(c)  ({0,1,2,4,17,18}[c])
/* sequential to Epson2 */
#define ep2color(c)  ({0,1,2,4,257,258}[c])

void merge_line (line_type *p, unsigned char *l, int startl, int stopl, 
                 int color);
void expand_line (unsigned char *src, unsigned char *dst, int height,
                  int skip, int left_ignore);
void write_output (FILE *fp_w);
void find_white (unsigned char *buf,int npix, int *left, int *right);
int update_page (unsigned char *buf, int bufsize, int m, int n, int color,
                 int density);
void parse_escp2 (FILE *fp_r);
void reverse_bit_order (unsigned char *buf, int n);
int rle_decode (unsigned char *inbuf, int n, int max);
void parse_canon (FILE *fp_r);
     

static inline int 
get_bits(unsigned char *p,int index) 
{

  /* p is a pointer to a bit stream, ordered MSb first.  Extract the
   * indexth bpp bit width field and return that value.  Ignore byte
   * boundries.
   */

  int value,b;
  unsigned addr;
  switch (pstate.bpp)
    {
    case 1:
      return (p[index >> 3] >> (7 - (index & 7))) & 1;
    case 2:
      return (p[index >> 2] >> ((3 - (index & 3)) << 1)) & 3;
    case 4:
      return (p[index >> 1] >> ((1 - (index & 1)) << 2)) & 0xf;
    case 8:
      return p[index];
    default:
      addr = (index*pstate.bpp);
      value=0;
      for (b=0;b<pstate.bpp;b++) {
	value*=2;
	value|=(p[(addr + b) >> 3] >> (7-((addr + b) & 7)))&1;
      }
      return(value);
    }
}

static inline void 
set_bits(unsigned char *p,int index,int value) 
{

  /* p is a pointer to a bit stream, ordered MSb first.  Set the
   * indexth bpp bit width field to value value.  Ignore byte
   * boundries.
   */

  int b;

  switch (pstate.bpp)
    {
    case 1:
      p[index >> 3] &= ~(1 << (7 - (index & 7)));
      p[index >> 3] |= value << (7 - (index & 7));
      break;
    case 2:
      p[index >> 2] &= ~(3 << ((3 - (index & 3)) << 1));
      p[index >> 2] |= value << ((3 - (index & 3)) << 1);
      break;
    case 4:
      p[index >> 1] &= ~(0xf << ((1 - (index & 1)) << 2));
      p[index >> 1] |= value << ((1 - (index & 1)) << 2);
      break;
    case 8:
      p[index] = value;
      break;
    default:
      for (b=pstate.bpp-1;b>=0;b--) {
	if (value&1) {
	  p[(index*pstate.bpp+b)/8]|=1<<(7-((index*pstate.bpp+b)%8));
	} else {
	  p[(index*pstate.bpp+b)/8]&=~(1<<(7-((index*pstate.bpp+b)%8)));
	}
	value/=2;
      }
    }
}

static inline void 
mix_ink(ppmpixel p, int c, unsigned int a) 
{

  /* this is pretty crude */

  int i;
  float ink[3];
  float size;

  size=(float)a/((float)((1<<pstate.bpp)-1));
  if (a) {
    switch (c) {
      case 0: ink[0]=ink[1]=ink[2]=0;break; /* black */
      case 1: ink[0]=1; ink[1]=0; ink[2]=1;break; /* magenta */
      case 2: ink[0]=0; ink[1]=ink[2]=1;break; /* cyan */
      case 3: ink[0]=ink[1]=1; ink[2]=0;break; /* yellow */
      case 4: ink[0]=1; ink[1]=0.5; ink[2]=1;break; /* lmagenta */
      case 5: ink[0]=0.5; ink[1]=ink[2]=1;break; /* lcyan */
      case 6: ink[0]=ink[1]=1; ink[2]=0.5;break; /* lyellow */
      default:fprintf(stderr,"unknown ink %d\n",c);return;
    }
    for (i=0;i<3;i++) {
      ink[i]=(1-size)+size*ink[i];
      p[i]*=ink[i];
    }
  }
}

void 
merge_line(line_type *p, unsigned char *l, int startl, int stopl, int color)
{

  int i;
  int temp,shift,height,lvalue,pvalue,oldstop;
  unsigned char *tempp;

  if (startl<p->startx[color]) { /* l should be to the right of p */
    temp=p->startx[color];
    p->startx[color]=startl;
    startl=temp;
    temp=p->stopx[color];
    p->stopx[color]=stopl;
    stopl=temp;
    tempp=p->line[color];
    p->line[color]=l;
    l=tempp;
  }
  shift=startl-p->startx[color];
  height=stopl-startl+1;

  oldstop=p->stopx[color];
  p->stopx[color]=(stopl>p->stopx[color])?stopl:p->stopx[color];
  p->line[color]=xrealloc(p->line[color],((p->stopx[color]-p->startx[color]+1)*pstate.bpp+7)/8);
  memset(p->line[color]+((oldstop-p->startx[color]+1)*pstate.bpp+7)/8,0,
          ((p->stopx[color]-p->startx[color]+1)*pstate.bpp+7)/8-
          ((oldstop-p->startx[color]+1)*pstate.bpp+7)/8);
  for (i=0;i<height;i++) {
    lvalue=get_bits(l,i);
    pvalue=get_bits(p->line[color],i+shift);
    if (0&&pvalue&&lvalue) {
      fprintf(stderr,"Warning!  Double printing detected at x,y=%d!\n",p->startx[color]+i);
    } else {
    pvalue+=lvalue;
    if (pvalue>(1<<pstate.bpp)-1) {
/*      fprintf(stderr,"Warning!  Clipping at x=%d!\n",p->startx[color]+i); */
      pvalue=(1<<pstate.bpp)-1;
    }
    set_bits(p->line[color],i+shift,pvalue);
    }
  }
}

void expand_line (unsigned char *src, unsigned char *dst, int height, int skip,
                  int left_ignore) 
{

  /* src is a pointer to a bit stream which is composed of fields of height
   * bpp starting with the most significant bit of the first byte and
   * proceding from there with no regard to byte boundaries.  For the
   * existing Epson printers, bpp is 1 or 2, which means fields will never
   * cross byte boundaries.  However, if bpp were 3, this would undoubtedly
   * happen.  This routine will make no assumptions about bpp, and handle each
   * bit individually.  It's slow, but, it's the only way that will work in
   * the general case of arbitrary bpp.
   *
   * We want to copy each field from the src to the dst, spacing the fields
   * out every skip fields.  We should ignore the first left_ignore fields
   * pointed to by src, though.
   */

  int i;

  if ((skip==1)&&!(left_ignore*pstate.bpp%8)) {
    /* the trivial case, this should be faster */
    memcpy(dst,src+left_ignore*pstate.bpp/8,(height*pstate.bpp+7)/8);
    return;
  }

  for (i=0;i<height;i++) {
    set_bits(dst,i*skip,get_bits(src,i+left_ignore));
  }
}

void write_output(FILE *fp_w) 
{
  int c,l,p,left,right,first,last,width,height,i;
  unsigned int amount;
  ppmpixel *out_row;
  int oversample = pstate.absolute_horizontal_units /
    pstate.absolute_vertical_units;
  if (oversample == 0)
    oversample = 1;

  fprintf(stderr,"Margins: top: %d bottom: top+%d\n",pstate.top_margin,
          pstate.bottom_margin);
  for (first=0;(first<pstate.bottom_margin)&&(!page[first]);
       first++);
  for (last=pstate.bottom_margin-1;(last>first)&&
       (!page[last]);last--);
  if ((first<pstate.bottom_margin)&&(page[first])) {
    height = oversample * (last-first+1);
  } else {
    height = 0;
  }
  left=INT_MAX;
  right=0;
  for (l=first;l<=last;l++) {
    if (page[l]) {
      for (c=0;c<MAX_INKS;c++) {
        if (page[l]->line[c]) {
          left=(page[l]->startx[c]<left)?page[l]->startx[c]:left;
          right=(page[l]->stopx[c]>right)?page[l]->stopx[c]:right;
        }
      }
    }
  }
  fprintf(stderr,"Image from (%d,%d) to (%d,%d).\n",left,first,right,last);
  width=right-left+1;
  if (width<0) {
    width=0;
  }
  out_row = malloc(sizeof(ppmpixel) * width);
  /* start with white, add inks */
  fprintf(stderr,"Writing output...\n");
  /* write out the PPM header */
  fprintf(fp_w,"P6\n");
  fprintf(fp_w,"%d %d\n",width,height);
  fprintf(fp_w,"255\n");
  for (l=first;l<=last;l++) {
    memset(out_row, 255, (sizeof(ppmpixel) * width));
    for (p=left;p<=right;p++) {
      for (c=0;c<MAX_INKS;c++) {
	if ((page[l])&&(page[l]->line[c])&&
	    (page[l]->startx[c]<=p)&&
	    (page[l]->stopx[c]>=p)) {
	  amount=get_bits(page[l]->line[c],p-page[l]->startx[c]);
	  mix_ink(out_row[p - left],c,amount);
	}
      }
    }
    for (i = 0; i < oversample; i++)
      fwrite(out_row, sizeof(ppmpixel), width, fp_w);
  }
  free(out_row);
}

#if 0
int num_bits_zero_lsb(int i,int max) 
{

  int n;

  for (n=0;(n<max)&&!(i&1);n++,i>>1);
  return n;

}

int num_bits_zero_msb(int i, int max) 
{

  int n;

  for (n=0;(n<max)&&i;n++,i>>1);
  return(max-n);

}
#endif

void find_white(unsigned char *buf,int npix, int *left, int *right) 
{

/* If a line has white borders on either side, count the number of
 * pixels and fill that info into left and right.
 */

 int i,j,max;
 int words,bytes,bits;

 *left=*right=0;
 bits=npix*pstate.bpp;
 bytes=bits/8;
 words=bytes/sizeof(long);

  /* left side */
  max=words;
  for(i=0;(i<max)&&(((long *)buf)[i]==0);i++);
  max=(i<words)?(i+1)*sizeof(long):bytes;
  for(i*=sizeof(long);(i<max)&&(buf[i]==0);i++);
  max=(i<bytes)?8:bits%8;
  for(j=0;(j<max)&&!(buf[i]&(1<<(7-j)));j++);
  *left=(i*8+j)/pstate.bpp;
  *right=0;

  /* if left is everything, then right is nothing */
  if (*left==npix) {
    return;
  }

  /* right side, this is a little trickier */
  for (i=0;(i<bits%8)&&!(buf[bytes]&(1<<(i+8-bits%8)));i++);
  if (i<bits%8) {
    *right=i/pstate.bpp;
    return;
  }
  *right=bits%8; /*temporarily store right in bits to avoid rounding error*/

  for (i=0;(i<bytes%sizeof(long))&&!(buf[bytes-1-i]);i++);
  if (i<bytes%sizeof(long)) {
    for (j=0;(j<8)&&!(buf[bytes-1-i]&(1<<j));j++);
    *right=(*right+i*8+j)/pstate.bpp;
    return;
  }
  *right+=i*8;

  for (i=0;(i<words)&&!(((int *)buf)[words-1-i]);i++);

  if (i<words) {
    *right+=i*sizeof(long)*8;
    for(j=0;(j<sizeof(long))&&!(buf[(words-i)*sizeof(long)-1-j]);j++);
    if (j<sizeof(long)) {
      *right+=j*8;
      max=(words-i)*sizeof(long)-1-j;
      for (j=0;(j<8)&&!(buf[max]&(1<<j));j++);
      if (j<8) {
        *right=(*right+j)/pstate.bpp;
        return;
      }
    }
  }

  fprintf(stderr,"Warning: Reality failure.  The impossible happened.\n");

}

/* 'update_page'
 *
 *
 *
 *
 */
int update_page(unsigned char *buf, /* I - pixel data               */
		int bufsize,        /* I - size of buf in bytes     */
		int m,              /* I - height of area in pixels */
		int n,              /* I - width of area in pixels  */
		int color,          /* I - color of pixel data      */
		int density         /* I - horizontal density in dpi  */
		) 
{

  int y,skip,oldstart,oldstop,mi;
  int left_white,right_white;
  unsigned char *oldline;

  if ((n==0)||(m==0)) {
    return(0);  /* shouldn't happen */
  }

  skip=pstate.relative_horizontal_units/density;
  skip*=pstate.extraskip;

  if (skip==0) {
    fprintf(stderr,"Warning!  Attempting to print at %d DPI but units are set "
	    "to %d DPI.\n",density,pstate.relative_horizontal_units);
    return(0);
  }

  if (!page) {
    fprintf(stderr,"Warning!  Attempting to print before setting up page!\n");
    /* Let's hope that we've at least initialized the printer with
     * with an ESC @ and allocate the default page.  Otherwise, we'll
     * have unpredictable results.  But, that's a pretty acurate statement
     * for a real printer, too!  */
    page=(line_type **)xcalloc(pstate.bottom_margin, sizeof(line_type *));
  }
  for (mi=0,y=pstate.yposition;
       y<pstate.yposition+m*(pstate.microweave?1:pstate.nozzle_separation);
       y+=(pstate.microweave?1:pstate.nozzle_separation),mi++) {
    if (y>=pstate.bottom_margin) {
      fprintf(stderr,"Warning. Unprinter out of unpaper.\n");
      return(1);
    }
    find_white(buf+mi*((n*pstate.bpp+7)/8),n,&left_white,&right_white);
    if (left_white==n)
      continue; /* ignore blank lines */
    if (!(page[y])) {
      page[y]=(line_type *) xcalloc(sizeof(line_type),1);
    }
    if ((left_white*pstate.bpp<8)&&(skip==1)) {
      left_white=0; /* if it's just a few bits, don't bother cropping */
    }               /* unless we need to expand the line anyway       */
    if (page[y]->line[color]) {
       oldline=page[y]->line[color];
       oldstart=page[y]->startx[color];
       oldstop=page[y]->stopx[color];
    } else {
      oldline=NULL;
      oldstart = -1;
      oldstop = -1;
    }
    page[y]->startx[color]=pstate.xposition+left_white*skip;
    page[y]->stopx[color]=pstate.xposition+((n-1-right_white)*skip);
    page[y]->line[color]=(unsigned char *) xcalloc(sizeof(unsigned char),
     (((page[y]->stopx[color]-page[y]->startx[color])*skip+1)*pstate.bpp+7)/8);
    expand_line(buf+mi*((n*pstate.bpp+7)/8),page[y]->line[color],
                page[y]->stopx[color]-page[y]->startx[color]+1,skip,left_white);
    if (oldline) {
      merge_line(page[y],oldline,oldstart,oldstop,color);
    }
  }
  pstate.xposition+=n?(n-1)*skip+1:0;
  return(0);
}



#define get1(error) if (!(count=fread(&ch,1,1,fp_r)))\
{fprintf(stderr,error);eject=1;continue;} else counter+=count;

#define get2(error) {if(!(count=fread(minibuf,1,2,fp_r))){\
fprintf(stderr,error);eject=1;continue;} else counter+=count;\
sh=minibuf[0]+minibuf[1]*256;}

#define getn(n,error) if (!(count=fread(buf,1,n,fp_r)))\
{fprintf(stderr,error);eject=1;continue;} else counter+=count;

#define getnoff(n,offset,error) if (!(count=fread(buf+offset,1,n,fp_r))){\
fprintf(stderr,error);eject=1;continue;} else counter+=count;

void parse_escp2(FILE *fp_r)
{

  int i,m=0,n=0,c=0;
  int currentcolor,currentbpp,density,eject,got_graphics;
  int count,counter;

  counter=0;
  eject=got_graphics=currentbpp=currentcolor=density=0;

    while ((!eject)&&(fread(&ch,1,1,fp_r))){
      counter++;
      if (ch==0xd) { /* carriage return */
        pstate.xposition=0;
        continue;
      }
      if (ch==0xc) { /* form feed */
        eject=1;
        continue;
      }
      if (ch==0x0) { /* NUL */
	fprintf(stderr, "Ignoring NUL character at 0x%08X\n", counter-1);
	continue;
      }
      if (ch!=0x1b) {
        fprintf(stderr,"Corrupt file?  No ESC found.  Found: %02X at 0x%08X\n",ch,counter-1);
        continue;
      }
    nextcommand:
      get1("Corrupt file.  No command found.\n");
      /* fprintf(stderr,"Got a %X.\n",ch); */
      switch (ch) {
	case 1: /* Magic EJL stuff to get USB port working */
	    fprintf(stderr,"Ignoring EJL commands.\n");
	    do {
	      get1("Error reading EJL commands.\n");
	    } while (!eject && ch != 0x1b);
	    if (ch==0x1b)
	      goto nextcommand;
	    break;
        case '@': /* initialize printer */
            if (page) {
              eject=1;
              continue;
            } else {
              pstate.unidirectional=0;
              pstate.microweave=0;
              pstate.dotsize=0;
              pstate.bpp=1;
              pstate.page_management_units=360;
              pstate.relative_horizontal_units=180;
              pstate.absolute_horizontal_units=60;
              pstate.relative_vertical_units=360;
              pstate.absolute_vertical_units=360;
              pstate.top_margin=120;
              pstate.bottom_margin=
                pstate.page_height=22*360; /* 22 inches is default ??? */
              pstate.monomode=0;
            }
            break;
        case 'U': /* turn unidirectional mode on/off */
            get1("Error reading unidirectionality.\n");
            if ((ch<=2)||((ch>=0x30)&&(ch<=0x32))) {
              pstate.unidirectional=ch;
            }
            break;
        case 'i': /* transfer raster image */
            get1("Error reading color.\n");
            currentcolor=seqcolor(ch);
            get1("Error reading compression mode!\n");
            c=ch;
            get1("Error reading bpp!\n");
            if (ch!=pstate.bpp) {
              fprintf(stderr,"Warning!  Color depth altered by ESC i.  This could be very very bad.\n");
              pstate.bpp=ch;
            }
            if (pstate.bpp>2) {
              fprintf(stderr,"Warning! Excessively deep color detected.\n");
            }
            if (pstate.bpp==0) {
              fprintf(stderr,"Warning! 0 bit pixels are far too Zen for this software.\n");
            }
            get2("Error reading number of horizontal dots!\n");
            n=sh * 8 / pstate.bpp;
            get2("Error reading number of vertical dots!\n");
            m=sh;
            density=pstate.horizontal_spacing;
            ch=0; /* make sure ch!='.' and fall through */
        case '.': /* transfer raster image */
            got_graphics=1;
            if (ch=='.') {
              get1("Error reading compression mode!\n");
              c=ch;
              if (c>2) {
                fprintf(stderr,"Warning!  Unknown compression mode.\n");
                break;
              }
              get1("Error reading vertical density!\n");
              get1("Error reading horizontal density!\n");
              density=3600/ch;
              get1("Error reading number of vertical dots!\n");
              m=ch;
              get2("Error reading number of horizontal dots!\n");
              n=sh;
              currentcolor=pstate.current_color;
            }
            switch (c) {
              case 0:  /* uncompressed */
                bufsize=m*((n*pstate.bpp+7)/8);
                getn(bufsize,"Error reading raster data!\n");
                update_page(buf,bufsize,m,n,currentcolor,density);
                break;
              case 1:  /* run height encoding */
                for (i=0;(!eject)&&(i<(m*((n*pstate.bpp+7)/8)));) {
                  get1("Error reading counter!\n");
                  if (ch<128) {
                    bufsize=ch+1;
                    getnoff(bufsize,i,"Error reading RLE raster data!\n");
                  } else {
                    bufsize=257-(unsigned int)ch;
                    get1("Error reading compressed RLE raster data!\n");
                    memset(buf+i,ch,bufsize);
                  }
                  i+=bufsize;
                }
                if (i!=(m*((n*pstate.bpp+7)/8))) {
                  fprintf(stderr,"Error decoding RLE data.\n");
                  fprintf(stderr,"Total bufsize %d, expected %d\n",i,
                        (m*((n*pstate.bpp+7)/8)));
                  eject=1;
                  continue;
                }
                update_page(buf,i,m,n,currentcolor,density);
                break;
              case 2: /* TIFF compression */
                fprintf(stderr,"TIFF mode not yet supported!\n");
                /* FIXME: write TIFF stuff */
                break;
              default: /* unknown */
                fprintf(stderr,"Unknown compression mode.\n");
                break;
            }
            break;
        case '\\': /* set relative horizontal position */
            get2("Error reading relative horizontal position.\n");
            if (pstate.xposition+(signed short)sh<0) {
              fprintf(stderr,"Warning! Attempt to move to -X region ignored.\n");
              fprintf(stderr,"   Command:  ESC %c %X %X   Original Position: %d\n",ch,minibuf[0],minibuf[1],pstate.xposition);
            } else  /* FIXME: Where is the right margin??? */
              pstate.xposition+=(signed short)sh;
            break;
        case '$': /* set absolute horizontal position */
            get2("Error reading absolute horizontal position.\n");
            pstate.xposition=sh*(pstate.relative_horizontal_units/
                                pstate.absolute_horizontal_units);
            break;
        case 0x6: /* flush buffers */
            /* Woosh.  Consider them flushed. */
            break;
        case 0x19: /* control paper loading */
            get1("Error reading paper control byte.\n");
            /* paper? */
            break;
        case 'r': /* select printing color */
            get1("Error reading color.\n");
            if ((ch<=4)&&(ch!=3)) {
              pstate.current_color=seqcolor(ch);
            } else {
              fprintf(stderr,"Invalid color %d.\n",ch);
            }
            break;
        case '(': /* commands with a payload */
            get1("Corrupt file.  Incomplete extended command.\n");
            if (ch=='R') { /* "remote mode" */
              get2("Corrupt file.  Error reading buffer size.\n");
	      bufsize=sh;
	      getn(bufsize,"Corrupt file.  Error reading remote mode name.\n");
	      if (bufsize==8 && memcmp(buf, "\0REMOTE1", 8)==0) {
		int rc1=0, rc2=0;
		/* Remote mode 1 */
		do {
		  get1("Corrupt file.  Error in remote mode.\n");
		  rc1=ch;
		  get1("Corrupt file.  Error reading remote mode command.\n");
		  rc2=ch;
		  get2("Corrupt file.  Error reading remote mode command size.\n");
		  bufsize=sh;
		  if (bufsize) {
		    getn(bufsize, "Corrupt file.  Error reading remote mode command parameters.\n");
		  }
		  if (rc1==0x1b && rc2==0) {
		    /* ignore quietly */
		  } else if (rc1=='L' && rc2=='D') {
		    fprintf(stderr, "Load settings from NVRAM command ignored.\n");
		  } else if (rc1=='N' && rc2=='C') {
		    fprintf(stderr, "Nozzle check command ignored.\n");
		  } else if (rc1=='V' && rc2=='I') {
		    fprintf(stderr, "Print version information command ignored.\n");
		  } else if (rc1=='A' && rc2=='I') {
		    fprintf(stderr, "Print printer ID command ignored.\n");
		  } else if (rc1=='C' && rc2=='H') {
		    fprintf(stderr, "Remote head cleaning command ignored.\n");
		  } else if (rc1=='D' && rc2=='T') {
		    fprintf(stderr, "Print alignment pattern command ignored.\n");
		  } else if (rc1=='D' && rc2=='A') {
		    fprintf(stderr, "Alignment results command ignored.\n");
		  } else if (rc1=='S' && rc2=='V') {
		    fprintf(stderr, "Alignment save command ignored.\n");
		  } else if (rc1=='R' && rc2=='S') {
		    fprintf(stderr, "Remote mode reset command ignored.\n");
		  } else if (rc1=='I' && rc2=='Q') {
		    fprintf(stderr, "Fetch ink quantity command ignored.\n");
		  } else {
		    fprintf(stderr, "Remote mode command `%c%c' ignored.\n",
			    rc1,rc2);
		  }
		} while (!eject && !(rc1==0x1b && rc2==0));
	      } else {
                fprintf(stderr,"Warning!  Commands in unrecognised remote mode ignored.\n");
                do {
                  while((!eject)&&(ch!=0x1b)) {
                    get1("Error in remote mode.\n");
                  }
                  get1("Error reading remote mode terminator\n");
                } while ((!eject)&&(ch!=0));
	      }
	      break;
            }
            get2("Corrupt file.  Error reading buffer size.\n");
            bufsize=sh;
            /* fprintf(stderr,"Command %X bufsize %d.\n",ch,bufsize); */
            getn(bufsize,"Corrupt file.  Error reading data buffer.\n");
            switch (ch) {
              case 'G': /* select graphics mode */
                /* FIXME: this is supposed to have more side effects */
                pstate.microweave=0;
                pstate.dotsize=0;
                pstate.bpp=1;
                break;
              case 'U': /* set page units */
                switch (bufsize) {
                  case 1:
                    pstate.page_management_units=
                    pstate.absolute_horizontal_units=
                    pstate.relative_horizontal_units=
                    pstate.relative_vertical_units=
		    pstate.horizontal_spacing=
                    pstate.absolute_vertical_units=3600/buf[0];
		    fprintf(stderr, "Setting units to 1/%d\n",
			    pstate.absolute_horizontal_units);
                    break;
                  case 5:
		    pstate.extraskip=1;
                    pstate.page_management_units=(buf[4]*256+buf[3])/buf[0];
                    pstate.relative_vertical_units=
                    pstate.absolute_vertical_units=(buf[4]*256+buf[3])/buf[1];
                    pstate.relative_horizontal_units=
		    pstate.horizontal_spacing=
                    pstate.absolute_horizontal_units=(buf[4]*256+buf[3])/buf[2];
		    fprintf(stderr, "Setting page management to 1/%d\n",
			    pstate.page_management_units);
		    fprintf(stderr, "Setting vertical to 1/%d\n",
			    pstate.relative_vertical_units);
		    fprintf(stderr, "Setting horizontal to 1/%d\n",
			    pstate.relative_horizontal_units);
                    break;
                }
                break;
              case 'i': /* set MicroWeave mode */
                if (bufsize!=1) {
                  fprintf(stderr,"Malformed microweave setting command.\n");
                } else {
                  switch (buf[0]) {
                    case 0x00:
                    case 0x30:pstate.microweave=0;
                        break;
                    case 0x01:
                    case 0x31:pstate.microweave=1;
                         break;
                    default:fprintf(stderr,"Unknown Microweave mode 0x%X.\n",
                                    buf[0]);

                  }
                }
                break;
              case 'e': /* set dot size */
                if ((bufsize!=2)||(buf[0]!=0)) {
                  fprintf(stderr,"Malformed dotsize setting command.\n");
                } else {
                  if (got_graphics) {
                    fprintf(stderr,"Changing dotsize while printing not supported.\n");
                  } else {
                    pstate.dotsize=buf[1];
                    if (pstate.dotsize&0x10) {
                      pstate.bpp=2;
                    } else {
                      pstate.bpp=1;
                    }
                  }
                }
                break;
              case 'c': /* set page format */
                if (page) {
                  fprintf(stderr,"Setting the page format in the middle of printing a page is not supported.\n");
                  exit(-1);
                }
                switch (bufsize) {
                  case 4:
                    pstate.top_margin=buf[1]*256+buf[0];
                    pstate.bottom_margin=buf[3]*256+buf[2];
                    break;
                  case 8:
                    fprintf(stderr,"Warning!  Using undocumented 8 byte page format command.\n");
                    pstate.top_margin=buf[3]<<24|buf[2]<<16|buf[1]<<8|buf[0];
                    pstate.bottom_margin=buf[7]<<24|buf[6]<<16|buf[5]<<8|buf[4];
                    break;
                  default:
                    fprintf(stderr,"Malformed page format.  Ignored.\n");
                }
                if ((bufsize==4)||(bufsize==8)) {
                  pstate.yposition=0;
                  if (pstate.top_margin+pstate.bottom_margin>
                       pstate.page_height) {
                    pstate.page_height=pstate.top_margin+pstate.bottom_margin;
                  }
                  page=(line_type **)xcalloc(pstate.bottom_margin,
                                  sizeof(line_type *));
                  /* FIXME: what is cut sheet paper??? */
                }
                break;
              case 'V': /* set absolute vertical position */
                i=0;
                switch (bufsize) {
                    case 4:i=(buf[2]<<16)+(buf[3]<<24);
                    case 2:i+=(buf[0])+(256*buf[1]);
                    if (i*(pstate.relative_vertical_units/
                            pstate.absolute_vertical_units)>=pstate.yposition) {
                      pstate.yposition=i*(pstate.relative_vertical_units/
                            pstate.absolute_vertical_units);
                    } else {
                       fprintf(stderr,"Warning: Setting Y position in negative direction ignored\n");
                    }
                    break;
                  default:
                    fprintf(stderr,"Malformed absolute vertical position set.\n");
                }
                if (pstate.yposition>pstate.bottom_margin) {
                  fprintf(stderr,"Warning! Printer head moved past bottom margin.  Dumping output and exiting.\n");
                  eject=1;
                }
                break;
              case 'v': /* set relative vertical position */
                i=0;
                switch (bufsize) {
                    case 4:i=(buf[2]<<16)+(buf[3]<<24);
                    case 2:i+=(buf[0])+(256*buf[1]);
                      if (unweave) {
                        i=pstate.nozzles;
                      }
                      pstate.yposition+=i;
                    break;
                  default:
                    fprintf(stderr,"Malformed relative vertical position set.\n");
                }
                if (pstate.yposition>pstate.bottom_margin) {
                  fprintf(stderr,"Warning! Printer head moved past bottom margin.  Dumping output and exiting.\n");
                  eject=1;
                }
                break;
              case 'K':
                if (bufsize!=2) {
                  fprintf(stderr,"Malformed monochrome/color mode selection.\n");
                } else {
                  if (buf[0]) {
                    fprintf(stderr,"Non-zero first byte in monochrome selection command. Ignored.\n");
                  } else if (buf[0]>0x02) {
                    fprintf(stderr,"Unknown color mode 0x%X.\n",buf[1]);
                  } else
                    pstate.monomode=buf[1];
                }
                break;
              case 'S': /* set paper dimensions */
                break;
	      case 'D':
		if (bufsize != 4)
		  {
		    fprintf(stderr, "Malformed set resolution request.\n");
		  }
		else
		  {
		    int res_base = (256 * buf[1]) + buf[0];
		    pstate.nozzle_separation =
		      pstate.absolute_vertical_units / (res_base / buf[2]);
		    pstate.horizontal_spacing = res_base / buf[3];
		    fprintf(stderr, "Setting vertical spacing to %d\n",
			    pstate.nozzle_separation);
		    fprintf(stderr, "Setting horizontal spacing to %d\n",
			    pstate.horizontal_spacing);
		  }
		break;
              case 'r': /* select color */
                if (bufsize!=2) {
                  fprintf(stderr,"Malformed color selection request.\n");
                } else {
                  sh=256*buf[0]+buf[1];
                  if ((buf[1]>4)||(buf[1]==3)||(buf[0]>1)||
                      (buf[0]&&((buf[1]==0)||(buf[1]==4)))) {
                    fprintf(stderr,"Invalid color 0x%X.\n",sh);
                  } else {
                    pstate.current_color=seqcolor(sh);
                  }
                }
                break;
              case '\\': /* set relative horizontal position 700/EX */
              case '/': /* set relative horizontal position  740/750/1200 */
                i=(buf[3]<<8)|buf[2];
                if (pstate.xposition+i<0) {
                  fprintf(stderr,"Warning! Attempt to move to -X region ignored.\n");
                  fprintf(stderr,"   Command:  ESC ( %c %X %X %X %X  Original position: %d\n",ch,buf[0],buf[1],buf[2],buf[3],pstate.xposition);
                } else  /* FIXME: Where is the right margin??? */
                  pstate.xposition+=i;
                break;
              case '$': /* set absolute horizontal position */
                i=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
                pstate.xposition=i*(pstate.relative_horizontal_units/
                                     pstate.absolute_horizontal_units);
                break;
              case 'C': /* set page height */
                break;
              default:
                fprintf(stderr,"Warning: Unknown command ESC ( 0x%X at 0x%08X.\n",ch,counter-5-bufsize);
            }
            break;
          default:
            fprintf(stderr,"Warning: Unknown command ESC 0x%X at 0x%08X.\n",ch,counter-2);
      }
    }
}


/* 'reverse_bit_order'
 *
 * reverse the bit order in an array of bytes - does not reverse byte order!
 */
void reverse_bit_order(unsigned char *buf, int n)
{
  int i;
  unsigned char a;
  if (!n) return; /* nothing to do */

  for (i= 0; i<n; i++) {
    a= buf[i];
    buf[i]=
      (a & 0x01) << 7 |
      (a & 0x02) << 5 |
      (a & 0x04) << 3 |
      (a & 0x08) << 1 |
      (a & 0x10) >> 1 |
      (a & 0x20) >> 3 |
      (a & 0x40) >> 5 |
      (a & 0x80) >> 7;
  }
}

/* 'rle_decode'
 *
 * run-height-decodes a given buffer of height "n"
 * and stores the result in the same buffer
 * not exceeding a size of "max" bytes.
 */
int rle_decode(unsigned char *inbuf, int n, int max)
{
  unsigned char outbuf[1440*3];
  signed char *ib= (signed char *)inbuf;
  signed char cnt;
  int num;
  int i= 0, j;
  int o= 0;

#ifdef DEBUG_RLE
  fprintf(stderr,"input: %d\n",n);
#endif
  if (n<=0) return 0;
  if (max>1440*3) max= 1440*3; /* FIXME: this can be done much better! */

  while (i<n && o<max) {
    cnt= ib[i];
    if (cnt<0) {
      /* cnt identical bytes */
      /* fprintf(stderr,"rle 0x%02x = %4d = %4d\n",cnt&0xff,cnt,1-cnt); */
      num= 1-cnt;
      /* fprintf (stderr,"+%6d ",num); */
      for (j=0; j<num && o+j<max; j++) outbuf[o+j]= inbuf[i+1];
      o+= num;
      i+= 2;
    } else {
      /* cnt individual bytes */
      /* fprintf(stderr,"raw 0x%02x = %4d = %4d\n",cnt&0xff,cnt,cnt + 1); */
      num= cnt+1;
      /* fprintf (stderr,"*%6d ",num); */
      for (j=0; j<num && o+j<max; j++) outbuf[o+j]= inbuf[i+j+1];
      o+= num;
      i+= num+1;
    }
  }
  if (o>=max) {
    fprintf(stderr,"Warning: rle decompression exceeds output buffer.\n");
    return 0;
  }
  /* copy decompressed data to inbuf: */
  memset(inbuf,0,max-1);
  memcpy(inbuf,outbuf,o);
#ifdef DEBUG_RLE
   fprintf(stderr,"output: %d\n",o);
#endif
  return o;
}

void parse_canon(FILE *fp_r)
{

  int m=0;
  int currentcolor,currentbpp,density,eject,got_graphics;
  int count,counter,cmdcounter;
  int delay_c=0, delay_m=0, delay_y=0, delay_C=0,
    delay_M=0, delay_Y=0, delay_K=0, currentdelay=0;

  counter=0;

  page= 0;
  eject=got_graphics=currentbpp=currentcolor=density=0;
  while ((!eject)&&(fread(&ch,1,1,fp_r))){
    counter++;
   if (ch==0xd) { /* carriage return */
     pstate.xposition=0;
#ifdef DEBUG_CANON
     fprintf(stderr,"<  ");
#endif
     continue;
   }
   if (ch==0xc) { /* form feed */
     eject=1;
     continue;
   }
   if (ch=='B') {
     fgets((char *)buf,sizeof(buf),fp_r);
     counter+= strlen((char *)buf);
     if (!strncmp((char *)buf,"JLSTART",7)) {
       while (strncmp((char *)buf,"BJLEND",6)) {
	 fgets((char *)buf,sizeof(buf),fp_r);
	 counter+= strlen((char *)buf);
	 fprintf(stderr,"got BJL-plaintext-command %s",buf);
       }
     } else {
       fprintf(stderr,"Error: expected BJLSTART but got B%s",buf);
     }
     counter= ftell(fp_r);
     continue;
   }
   if (ch!=0x1b) {
     fprintf(stderr,"Corrupt file?  No ESC found.  Found: %02X at 0x%08X\n",
	     ch,counter-1);
     continue;
   }
   get1("Corrupt file.  No command found.\n");
   /* fprintf(stderr,"Got a %X.\n",ch); */
   switch (ch) {
   case '[': /* 0x5b initialize printer */
     get1("Error reading CEM-code.\n");
     cmdcounter= counter;
     get2("Error reading CEM-data size.\n");
     getn(sh,"Error reading CEM-data.\n");

     if (ch=='K') /* 0x4b */ {
       if (sh!=2 || buf[0]!=0x00 ) {
	 fprintf(stderr,"Error initializing printer with ESC [ K\n");
	 eject=1;
	 continue;
       }
       if (page) {
	 eject=1;
	 continue;
       } else {
	 pstate.unidirectional=0;
	 pstate.microweave=0;
	 pstate.dotsize=0;
	 pstate.bpp=1;
	 pstate.page_management_units=360;
	 pstate.relative_horizontal_units=180;
	 pstate.absolute_horizontal_units=60;
	 pstate.relative_vertical_units=360;
	 pstate.absolute_vertical_units=360;
	 pstate.top_margin=120;
	 pstate.bottom_margin=
	   pstate.page_height=22*360; /* 22 inches is default ??? */
	 pstate.monomode=0;
	 pstate.xposition= 0;
	 pstate.yposition= 0;
	 fprintf(stderr,"canon: init printer\n");
       }
     } else {
       fprintf(stderr,"Warning: Unknown command ESC %c 0x%X at 0x%08X.\n",
	       0x5b,ch,cmdcounter);
     }
     break;

   case '@': /* 0x40 */
     eject=1;
     break;

   case '(': /* 0x28 */
     get1("Corrupt file.  Incomplete extended command.\n");
     cmdcounter= counter;
     get2("Corrupt file.  Error reading buffer size.\n");
     bufsize=sh;
     getn(bufsize,"Corrupt file.  Error reading data buffer.\n");

     switch(ch) {
/* Color Codes:
   color    Epson1  Epson2   Sequential
   Black    0       0        0 K
   Magenta  1       1        1 M
   Cyan     2       2        2 C
   Yellow   4       4        3 Y
   L.Mag.   17      257      4 m
   L.Cyan   18      258      5 c
   L.Yellow NA      NA       6 y
 */
     case 'A': /* 0x41 - transfer graphics data */
       switch (*buf) {
       case 'K': currentcolor= 0; currentdelay= delay_K; break;
       case 'M': currentcolor= 1; currentdelay= delay_M; break;
       case 'C': currentcolor= 2; currentdelay= delay_C; break;
       case 'Y': currentcolor= 3; currentdelay= delay_Y; break;
       case 'm': currentcolor= 4; currentdelay= delay_m; break;
       case 'c': currentcolor= 5; currentdelay= delay_c; break;
       case 'y': currentcolor= 6; currentdelay= delay_y; break;
       default:
	 fprintf(stderr,"Error: unsupported color type 0x%02x.\n",*buf);
	 /* exit(-1); */
       }
       pstate.current_color= currentcolor;
       m= rle_decode(buf+1,bufsize-1,sizeof(buf)-1);
       /* reverse_bit_order(buf+1,m); */
       pstate.yposition+= currentdelay;
       if (m) update_page(buf+1,m,1,(m*8)/pstate.bpp,currentcolor,
			  pstate.absolute_horizontal_units);
       pstate.yposition-= currentdelay;
#ifdef DEBUG_CANON
       fprintf(stderr,"%c:%d>%d  ",*buf,sh-1,m);
#endif
       break;
     case 'a': /* 0x61 - turn something on/off */
       break;
     case 'b': /* 0x62 - turn something else on/off */
       break;
     case 'c': /* 0x63 - some information about the print job */
       break;
     case 'd': /* 0x64 - set resolution */
       if (page) {
	 fprintf(stderr,"Setting the page format in the middle of printing "
		 "a page is not supported.\n");
	 exit(-1);
       }
       pstate.relative_vertical_units=
	 pstate.absolute_vertical_units=
	 buf[1]+256*buf[0];
       pstate.relative_horizontal_units=
	 pstate.absolute_horizontal_units=
	 buf[3]+256*buf[2];
       pstate.bottom_margin= pstate.relative_vertical_units* 22;
       /* FIXME: replace with real page height */
       fprintf(stderr,"canon: res is %d x %d dpi\n",
	       pstate.relative_horizontal_units,
	       pstate.relative_vertical_units);

       page=(line_type **)xcalloc(pstate.bottom_margin,sizeof(line_type *));
       break;
     case 'e': /* 0x65 - vertical head movement */
       pstate.yposition+= (buf[1]+256*buf[0]);
#ifdef DEBUG_CANON
       fprintf(stderr,"\n");
#endif
       break;
     case 'l': /* 0x6c - some more information about the print job*/
       break;
     case 'm': /* 0x6d - used printheads and other things */
       break;
     case 'p': /* 0x70 - set printable area */
       break;
     case 'q': /* 0x71 - turn yet something else on/off */
       break;
     case 't': /* 0x74 - contains bpp and line delaying*/
       pstate.bpp= buf[0];
       fprintf(stderr,"canon: using %d bpp\n",pstate.bpp);
       if (buf[1]&0x04) {
	 delay_y= 0;
	 delay_m= 0;
	 delay_c= 0;
	 delay_Y= 0;
	 delay_M= delay_Y+112;
	 delay_C= delay_M+112;
	 delay_K= delay_C+112;
	 fprintf(stderr,"canon: using line delay code\n");
       }
       break;

     default:
       fprintf(stderr,"Warning: Unknown command ESC ( 0x%X at 0x%08X.\n",
	       ch,cmdcounter);
     }
     break;

   default:
     fprintf(stderr,"Warning: Unknown command ESC 0x%X at 0x%08X.\n",
	     ch,counter-2);
   }
 }
}

int main(int argc,char *argv[])
{

  int arg;
  char *s;
  char *UNPRINT;
  FILE *fp_r,*fp_w;

    unweave=0;
    pstate.nozzle_separation=6;
    fp_r = fp_w = NULL;
    for (arg=1;arg<argc;arg++) {
      if (argv[arg][0]=='-') {
        switch (argv[arg][1]) {
          case 0:if (fp_r)
                   fp_w=stdout;
                 else
                   fp_r=stdin;
                 break;
          case 'n':if (argv[arg][2]) {
                     s=argv[arg]+2;
                   } else {
                     if (argc<=arg+1) {
                       fprintf(stderr,"Missing nozzle separation\n");
                       exit(-1);
                     } else {
                       s=argv[++arg];
                     }
                   }
                   if (!sscanf(s,"%d",&pstate.nozzle_separation)) {
                     fprintf(stderr,"Error parsing nozzle separation\n");
                     exit(-1);
                   }
                  break;
          case 'u':unweave=1;
                 break;
        }
      } else {
        if (fp_r) {
          if (!(fp_w = fopen(argv[arg],"w"))) {
            perror("Error opening ouput file");
            exit(-1);
          }
        } else {
          if (!(fp_r = fopen(argv[arg],"r"))) {
            perror("Error opening input file");
            exit(-1);
          }
        }
      }
    }
    if (!fp_r)
      fp_r=stdin;
    if (!fp_w)
      fp_w=stdout;

    if (unweave) {
      pstate.nozzle_separation=1;
    }
    pstate.nozzles=96;

    UNPRINT= getenv("UNPRINT");
    if ((UNPRINT)&&(!strcmp(UNPRINT,"canon"))) {
      pstate.extraskip=1;
      parse_canon(fp_r);
    } else {
      pstate.extraskip=2; 
      parse_escp2(fp_r);
    }
    fprintf(stderr,"Done reading.\n");
    write_output(fp_w);
    fclose(fp_w);
    fprintf(stderr,"Image dump complete.\n");

    return(0);
}
