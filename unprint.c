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

#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<string.h>

#undef DEBUG_CANON

typedef struct {
  unsigned char unidirectional;
  unsigned char microweave;
  int page_management_units; /* dpi */
  int relative_horizontal_units; /* dpi */
  int absolute_horizontal_units; /* dpi, assumed to be >= relative */
  int relative_vertical_units; /* dpi */
  int absolute_vertical_units; /* dpi, assumed to be >= relative */
  int top_margin; /* dots */
  int bottom_margin; /* dots */
  int page_length; /* dots */
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
 * and don't require that the allocated length be full page width.  This
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
  

void *mycalloc(size_t count,size_t size){
  void *p;
  if ((p=calloc(count,size))) {
    return(p);
  }

  fprintf(stderr,"Buy some RAM, dude!");
  exit(-1);
}

void *mymalloc(size_t size){
  void *p;
  if ((p=malloc(size))) {
    return(p);
  }

  fprintf(stderr,"Buy some RAM, dude!");
  exit(-1);
}

void *myrealloc(void *ptr, size_t size){
  void *p;
  
  if ((p=realloc(ptr,size))||(size==0)) {
    return(p);
  }

  fprintf(stderr,"Buy some RAM, dude!");
  exit(-1);
}

int get_bits(unsigned char *p,int index) {

  /* p is a pointer to a bit stream, ordered MSb first.  Extract the
   * indexth bpp bit width field and return that value.  Ignore byte
   * boundries.
   */

  int value,b;
  
  value=0;
  for (b=0;b<pstate.bpp;b++) {
    value*=2;
    value|=(p[(index*pstate.bpp+b)/8]>>(7-((index*pstate.bpp+b)%8)))&1;
  }
  return(value);
}

void set_bits(unsigned char *p,int index,int value) {

  /* p is a pointer to a bit stream, ordered MSb first.  Set the
   * indexth bpp bit width field to value value.  Ignore byte
   * boundries.
   */

  int b;
  
  for (b=pstate.bpp-1;b>=0;b--) {
    if (value&1) {
      p[(index*pstate.bpp+b)/8]|=1<<(7-((index*pstate.bpp+b)%8));
    } else {
      p[(index*pstate.bpp+b)/8]&=~(1<<(7-((index*pstate.bpp+b)%8)));
    }
    value/=2;
  }
}

void mix_ink(ppmpixel p, int c, unsigned int a) {

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

void merge_line(line_type *p, unsigned char *l, int startl, int stopl, int color){

  int i;
  int temp,shift,length,lvalue,pvalue,oldstop;
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
  length=stopl-startl+1;
  
  oldstop=p->stopx[color];
  p->stopx[color]=(stopl>p->stopx[color])?stopl:p->stopx[color];
  p->line[color]=myrealloc(p->line[color],((p->stopx[color]-p->startx[color]+1)*pstate.bpp+7)/8);
  memset(p->line[color]+((oldstop-p->startx[color]+1)*pstate.bpp+7)/8,0,
          ((p->stopx[color]-p->startx[color]+1)*pstate.bpp+7)/8-
          ((oldstop-p->startx[color]+1)*pstate.bpp+7)/8);
  for (i=0;i<length;i++) {
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

void expand_line (unsigned char *src, unsigned char *dst, int length, int skip) {

  /* src is a pointer to a bit stream which is composed of fields of length
   * bpp starting with the most significant bit of the first byte and
   * proceding from there with no regard to byte boundaries.  For the
   * existing Epson printers, bpp is 1 or 2, which means fields will never
   * cross byte boundaries.  However, if bpp were 3, this would undoubtedly
   * happen.  This routine will make no assumptions about bpp, and handle each
   * bit individually.  It's slow, but, it's the only way that will work in
   * the general case of arbitrary bpp.
   *
   * We want to copy each field from the src to the dst, spacing the fields
   * out every skip fields.
   */

  int i;

  if (skip==1) { /* the trivial case, this should be faster */
    memcpy(dst,src,(length*pstate.bpp+7)/8);
    return;
  }

  for (i=0;i<length;i++) {
    set_bits(dst,i*skip,get_bits(src,i));
  }

}

void write_output(FILE *fp_w) {
  int c,l,p,left,right,first,last,width,height,sp,sl;
  unsigned int amount;
  ppmpixel pixel;

  fprintf(stderr,"Margins: top: %d bottom: top+%d\n",pstate.top_margin,
          pstate.bottom_margin);
  for (first=0;(first<pstate.bottom_margin)&&(!page[first]);
       first++);
  for (last=pstate.bottom_margin-1;(last>first)&&
       (!page[last]);last--);
  if ((first<pstate.bottom_margin)&&(page[first])) {
    height = last-first+1;
  } else {
    height = 0;
  }
  left=INT_MAX;
  right=0;
  for (l=first;l<=last;l++) {
    if (page[l]) {
      for (c=0;c<MAX_INKS;c++) {
	/* try to cut left and right border: */
	sl= page[l]->stopx[c]-page[l]->startx[c];
	for (sp= 0; sp<sl && !get_bits(page[l]->line[c],sp); sp++);
	if (sp<sl && page[l]->startx[c]+sp<left) 
	  left= page[l]->startx[c]+sp;
  
	for (sp= 0; sp<sl && !get_bits(page[l]->line[c],sl-sp); sp++);
	if (sp<sl && page[l]->stopx[c]-sp>right)
	  right= page[l]->stopx[c]-sp;
	
      }
    }
  }
  fprintf(stderr,"Image from (%d,%d) to (%d,%d).\n",left,first,right,last);
  width=right-left+1;
  if (width<0) {
    width=0;
  }
  fprintf(stderr,"Writing output...\n");
  /* write out the PPM header */
  fprintf(fp_w,"P6\n");
  fprintf(fp_w,"%d %d\n",width,height);
  fprintf(fp_w,"255\n");
  for (l=first;l<=last;l++) {
    for (p=left;p<=right;p++) {
      memset(pixel,255,3); /* start with white, add inks */
      for (c=0;c<MAX_INKS;c++) {
        if ((page[l])&&(page[l]->line[c])&&
	    (page[l]->startx[c]<=p)&&
	    (page[l]->stopx[c]>=p)) {
          amount=get_bits(page[l]->line[c],p-page[l]->startx[c]);
          mix_ink(pixel,c,amount);
        }
      }
      fwrite(pixel,sizeof(pixel),1,fp_w);
    }
  }
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
		int density         /* I - vertical density in dpi  */
		) {

  int y,skip,oldstart,oldstop,mi;
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
    page=(line_type **)mycalloc(pstate.bottom_margin, sizeof(line_type *));
  }
  for (mi=0,y=pstate.yposition;
       y<pstate.yposition+m*(pstate.microweave?1:pstate.nozzle_separation);
       y+=(pstate.microweave?1:pstate.nozzle_separation),mi++) {
    if (y>=pstate.bottom_margin) {
      fprintf(stderr,"Warning. Unprinter out of unpaper.\n");
      return(1);
    }
    if (!(page[y])) {
      page[y]=(line_type *) mycalloc(sizeof(line_type),1);
    }
    if (page[y]->line[color]) {
       oldline=page[y]->line[color];
       oldstart=page[y]->startx[color];
       oldstop=page[y]->stopx[color];
    } else {
      oldline=NULL;
      oldstart = -1;
      oldstop = -1;
    }
    page[y]->line[color]=
      (unsigned char *) mycalloc(sizeof(unsigned char),
				 (((n-1)*skip+1)*pstate.bpp+7)/8);
    page[y]->startx[color]=pstate.xposition;
    page[y]->stopx[color]=pstate.xposition+((n-1)*skip);
    expand_line(buf+mi*((n*pstate.bpp+7)/8),
                   page[y]->line[color],n,skip);
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



void parse_escp2(FILE *fp_r){

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
      if (ch!=0x1b) {
        fprintf(stderr,"Corrupt file?  No ESC found.  Found: %02X at 0x%08X\n",ch,counter-1);
        continue;
      }
      get1("Corrupt file.  No command found.\n");
      /* fprintf(stderr,"Got a %X.\n",ch); */
      switch (ch) {
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
                pstate.page_length=22*360; /* 22 inches is default ??? */
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
            density=pstate.relative_horizontal_units;
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
              case 1:  /* run length encoding */
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
        case 0x5C: /* set relative horizontal position */
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
              pstate.current_color=ch;
            } else {
              fprintf(stderr,"Invalid color %d.\n",ch);
            }
            break;
        case '(': /* commands with a payload */
            get1("Corrupt file.  Incomplete extended command.\n");
            if (ch=='R') { /* "remote mode" */
              fprintf(stderr,"Warning!  Commands in remote mode ignored.\n");
              do {
                while((!eject)&&(ch!=0x1b)) {
                  get1("Error in remote mode.\n");
                }
                get1("Error reading remote mode terminator\n");
              } while ((!eject)&&(ch!=0));
              continue;
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
                    pstate.absolute_vertical_units=3600/buf[0];
                    break;
                  case 5:
                    pstate.page_management_units=(buf[4]*256+buf[3])/buf[0];
                    pstate.relative_vertical_units=
                    pstate.absolute_vertical_units=(buf[4]*256+buf[3])/buf[1];
                    pstate.relative_horizontal_units=
                    pstate.absolute_horizontal_units=(buf[4]*256+buf[3])/buf[2];
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
                if ((bufsize!=2)||(buf[0]!=0)||((buf[1]>4)&&(buf[1]!=0x10))) {
                  fprintf(stderr,"Malformed dotsize setting command.\n");
                } else {
                  if (got_graphics) {
                    fprintf(stderr,"Changing dotsize while printing not supported.\n");
                  } else {
                    pstate.dotsize=buf[1];
                    if (pstate.dotsize&0x16) {
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
                       pstate.page_length) {
                    pstate.page_length=pstate.top_margin+pstate.bottom_margin;
                  }
                  page=(line_type **)mycalloc(pstate.bottom_margin,
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
              case 0x5C: /* set relative horizontal position 700/EX */
              case '/': /* set relative horizontal position  740/750/1200 */
                i=(buf[3]<<24)|(buf[2]<<16)|(buf[1]<<8)|buf[0];
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
                break;
              case 'C': /* set page length */
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
 * run-length-decodes a given buffer of length "n" 
 * and stores the result in the same buffer 
 * not exceeding a size of "max" bytes.
 */
int rle_decode(unsigned char *inbuf, int n, int max)
{
  unsigned char outbuf[1440*3];
  char *ib= (char *)inbuf;
  char cnt;
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

void parse_canon(FILE *fp_r){

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
     fgets(buf,256*256,fp_r);
     counter+= strlen(buf);
     if (!strncmp(buf,"JLSTART",7)) {
       while (strncmp(buf,"BJLEND",6)) {
	 fgets(buf,256*256,fp_r);
	 counter+= strlen(buf);
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
	   pstate.page_length=22*360; /* 22 inches is default ??? */
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
     case 'A': /* 0x41 - transfer graphics data */
       switch (*buf) { 
       case 'C': currentcolor= 2; currentdelay= delay_C;
	 break;
       case 'M': currentcolor= 1; currentdelay= delay_M;
	 break;
       case 'Y': currentcolor= 3; currentdelay= delay_Y;
	 break;
       case 'K': currentcolor= 0; currentdelay= delay_K;
	 break;
       case 'c': currentcolor= 5; currentdelay= delay_c;
	 break;
       case 'm': currentcolor= 4; currentdelay= delay_m;
	 break;
       case 'y': currentcolor= 6; currentdelay= delay_y;
	 break;
       default:
	 fprintf(stderr,"Error: unsupported color type 0x%02x.\n",*buf);
	 /* exit(-1); */
       }
       pstate.current_color= currentcolor;
       m= rle_decode(buf+1,bufsize-1,256*256-1);
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
       /* FIXME: replace with real page length */
       fprintf(stderr,"canon: res is %d x %d dpi\n",
	       pstate.relative_horizontal_units,
	       pstate.relative_vertical_units);

       page=(line_type **)mycalloc(pstate.bottom_margin,sizeof(line_type *));
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






int main(int argc,char *argv[]){

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
    if (!strcmp(UNPRINT,"canon")) {
      pstate.extraskip=1;
      parse_canon(fp_r);
    } else {
      pstate.extraskip=2;
      parse_escp2(fp_r);
    }
    fprintf(stderr,"Done reading.\n");
    write_output(fp_w);
    fclose(fp_w);
    fprintf(stderr,"Segmentation integrity.  (core contained)\n");

    return(0);
}
