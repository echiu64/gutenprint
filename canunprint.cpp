/*  very quick and the more dirty unprint tool for the 
 *  Canon BJC-6000 and alike
 *
 *  to compile, use "gcc -Wall -o canunprint canunprint.cpp"
 *
 *  Features:
 *  - no error checking is done in initialization codes
 *  - leading empty lines are ingnored
 *  - extremely large files might be created
 *  - produces .xbm files can be viewed but not used in a C-source
 *
 *  Usage:
 *  canunprint <INFILE> <OUTFILE>
 *  
 *  will read a canon-printfile from INFILE and write the colorcomponents
 *  CcMmYyK to OUTFILE_X.xbm, X being the component
 *
 *  if OUTFILE is omitted, INFILE is used as the filename-base
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>


class cmykSave {

  char *outfilename;
  char *efnlc,*efnlm,*efnly,*efnc,*efnm,*efny,*efnk;

  int lc_cnt,lm_cnt,ly_cnt,c_cnt,m_cnt,y_cnt,k_cnt;

  FILE *outfilelc;
  FILE *outfilelm;
  FILE *outfilely;
  FILE *outfilec;
  FILE *outfilem;
  FILE *outfiley;
  FILE *outfilek;

  int xsize,ysize,pixcnt;
  
public:
  cmykSave(char *fn= 0) {
    outfilelc= outfilelm= outfilely=
      outfilec= outfilem= outfiley= outfilek= 0;
    xsize= ysize=0;
    if (fn) to(fn);
  }

  ~cmykSave() { 
    if (outfilec) close(); 
  }

  void delete_if_empty(char *name,int bytes)
  {
    if (!bytes) {
      fprintf(stderr,"file %s is empty - unlinking\n",name);
      unlink(name);
    }
  }

  void to (char *f) { 
    outfilename= (f) ? strdup(f) : 0;
    efnlc= new char[strlen(outfilename)+100];
    efnlm= new char[strlen(outfilename)+100];
    efnly= new char[strlen(outfilename)+100];
    efnc= new char[strlen(outfilename)+100];
    efnm= new char[strlen(outfilename)+100];
    efny= new char[strlen(outfilename)+100];
    efnk= new char[strlen(outfilename)+100];
    sprintf(efnc,outfilename,'C');
    if (strcmp(efnc,outfilename)) {
      outfilec= fopen(efnc,"w");
      sprintf(efnm,outfilename,'M');
      outfilem= fopen(efnm,"w");
      sprintf(efny,outfilename,'Y');
      outfiley= fopen(efny,"w");
      sprintf(efnk,outfilename,'K');
      outfilek= fopen(efnk,"w");
      sprintf(efnly,outfilename,'y');
      outfilely= fopen(efnly,"w");
      sprintf(efnlm,outfilename,'m');
      outfilelm= fopen(efnlm,"w");
      sprintf(efnlc,outfilename,'c');
      outfilelc= fopen(efnlc,"w");
    } else {
      sprintf(efnlc,"%s_c.xbm",outfilename);
      outfilelc= fopen(efnlc,"w");
      sprintf(efnlm,"%s_m.xbm",outfilename);
      outfilelm= fopen(efnlm,"w");
      sprintf(efnly,"%s_y.xbm",outfilename);
      outfilely= fopen(efnly,"w");
      sprintf(efnc,"%s_C.xbm",outfilename);
      outfilec= fopen(efnc,"w");
      sprintf(efnm,"%s_M.xbm",outfilename);
      outfilem= fopen(efnm,"w");
      sprintf(efny,"%s_Y.xbm",outfilename);
      outfiley= fopen(efny,"w");
      sprintf(efnk,"%s_K.xbm",outfilename);
      outfilek= fopen(efnk,"w");     
    }
  }

#define HEADER "\
#define %s_width %d
#define %s_height %d
static char %s_bits[] = {"

  void open(int x, int y) { 
    if (xsize) return;
    xsize=x; ysize=y; 
    fprintf(stderr,"output is %d x %d\n",xsize,ysize);
    fprintf(outfilelc,HEADER,efnlc,xsize,efnlc,ysize,efnlc);
    fprintf(outfilelm,HEADER,efnlm,xsize,efnlm,ysize,efnlm);
    fprintf(outfilely,HEADER,efnly,xsize,efnly,ysize,efnly);
    fprintf(outfilec, HEADER,efnc, xsize,efnc, ysize,efnc);
    fprintf(outfilem, HEADER,efnm, xsize,efnm, ysize,efnm);
    fprintf(outfiley, HEADER,efny, xsize,efny, ysize,efny);
    fprintf(outfilek, HEADER,efnk, xsize,efnk, ysize,efnk);
    lc_cnt=lm_cnt=ly_cnt=c_cnt=m_cnt=y_cnt=k_cnt= 0;

    pixcnt=0;
  }

  char conv(char i) {
    char o= 0;
    if (i & 0x80) o|=0x01;
    if (i & 0x40) o|=0x02;
    if (i & 0x20) o|=0x04;
    if (i & 0x10) o|=0x08;
    if (i & 0x08) o|=0x10;
    if (i & 0x04) o|=0x20;
    if (i & 0x02) o|=0x40;
    if (i & 0x01) o|=0x80;
    return o;
  }

  void emptyline()
  {
    for (int i=0; i<(xsize)/8; i++) {
      if (!(i%10)) {
	fprintf(outfilelc,"\n");
	fprintf(outfilelm,"\n");
	fprintf(outfilely,"\n");
	fprintf(outfilec,"\n");
	fprintf(outfilem,"\n");
	fprintf(outfiley,"\n");
	fprintf(outfilek,"\n");
      }
      fprintf(outfilelc," 0x%02x,",0);
      fprintf(outfilelm," 0x%02x,",0);
      fprintf(outfilely," 0x%02x,",0);
      fprintf(outfilec," 0x%02x,", 0);
      fprintf(outfilem," 0x%02x,", 0);
      fprintf(outfiley," 0x%02x,", 0);
      fprintf(outfilek," 0x%02x,", 0);
      pixcnt++;
    }
  }

  void saveline(unsigned char *c, unsigned char *m, 
		unsigned char* y, unsigned char *k,
		unsigned char *lc,unsigned char *lm,unsigned char *ly) {
    // fprintf(stderr,"chk %d\n",__LINE__);
    for (int i=0; i<(xsize)/8; i++) {
      if (!(i%10)) {
	fprintf(outfilelc,"\n");
	fprintf(outfilelm,"\n");
	fprintf(outfilely,"\n");
	fprintf(outfilec,"\n");
	fprintf(outfilem,"\n");
	fprintf(outfiley,"\n");
	fprintf(outfilek,"\n");
      }
      fprintf(outfilelc," 0x%02x,",conv(lc[i])&0xff);
      fprintf(outfilelm," 0x%02x,",conv(lm[i])&0xff);
      fprintf(outfilely," 0x%02x,",conv(ly[i])&0xff);
      fprintf(outfilec," 0x%02x,", conv(c[i])&0xff);
      fprintf(outfilem," 0x%02x,", conv(m[i])&0xff);
      fprintf(outfiley," 0x%02x,", conv(y[i])&0xff);
      fprintf(outfilek," 0x%02x,", conv(k[i])&0xff);
      if (lc[i]) lc_cnt++;
      if (lm[i]) lm_cnt++;
      if (ly[i]) ly_cnt++;
      if (c[i])  c_cnt++;
      if (m[i])  m_cnt++;
      if (y[i])  y_cnt++;
      if (k[i])  k_cnt++;
      pixcnt++;
    }
  }

  void close() {
    if (!xsize) return;
    int miss= int(xsize/8)*ysize - pixcnt;
    for (int i=1; i<miss; i++) {
      if (!(i%10)) {
	fprintf(outfilelc,"\n");
	fprintf(outfilelm,"\n");
	fprintf(outfilely,"\n");
	fprintf(outfilec,"\n");
	fprintf(outfilem,"\n");
	fprintf(outfiley,"\n");
	fprintf(outfilek,"\n");
      }
      fprintf(outfilelc," 0x%02x,",0);
      fprintf(outfilelm," 0x%02x,",0);
      fprintf(outfilely," 0x%02x,",0);
      fprintf(outfilec," 0x%02x,",0);
      fprintf(outfilem," 0x%02x,",0);
      fprintf(outfiley," 0x%02x,",0);
      fprintf(outfilek," 0x%02x,",0);
      pixcnt++;
    }
    fprintf(outfilelc," 0x00 };\n");
    fprintf(outfilelm," 0x00 };\n");
    fprintf(outfilely," 0x00 };\n");
    fprintf(outfilec," 0x00 };\n");
    fprintf(outfilem," 0x00 };\n");
    fprintf(outfiley," 0x00 };\n");
    fprintf(outfilek," 0x00 };\n");
    fclose(outfilelc);
    fclose(outfilelm);
    fclose(outfilely);
    fclose(outfilec);
    fclose(outfilem);
    fclose(outfiley);
    fclose(outfilek);
    delete_if_empty(efnlc,lc_cnt);
    delete_if_empty(efnlm,lm_cnt);
    delete_if_empty(efnly,ly_cnt);
    delete_if_empty(efnc, c_cnt);
    delete_if_empty(efnm, m_cnt);
    delete_if_empty(efny, y_cnt);
    delete_if_empty(efnk, k_cnt);
    fprintf(stderr,"wrote %d of %d blocks\n",pixcnt+1,int(xsize/8)*ysize);
    outfilec= 0;
  }

};


class bjcFile {

  char bf[9];

  char *infilename;

  FILE *infile;
  cmykSave *saver;

  unsigned char *inbuff;
  unsigned char *lcbuf,*lmbuf,*lybuf,*cbuf,*mbuf,*ybuf,*kbuf;

  int ibs;  // input buffer size
  int cbs;  // color channel buffer size
  unsigned int cnt;  // count of data in inbuff
  unsigned char cmd;  // command number obtained by next()

  int xres,yres;
  int xsize,ysize;

  int max;

public:
  bjcFile() { 
    infilename= 0;
    infile= 0;
    saver= 0;
    inbuff= lcbuf= lmbuf= lybuf= cbuf= mbuf= ybuf= kbuf= 0; 
    ibs=100000; cbs=200000;
    max= 0;
    reserve_buffers();
    xsize=ysize=0;
  }

  const char *fbin(char b) {
    bf[0]= (b & 0x80) ? '1' : '0';
    bf[1]= (b & 0x40) ? '1' : '0';
    bf[2]= (b & 0x20) ? '1' : '0';
    bf[3]= (b & 0x10) ? '1' : '0';
    bf[4]= (b & 0x08) ? '1' : '0';
    bf[5]= (b & 0x04) ? '1' : '0';
    bf[6]= (b & 0x02) ? '1' : '0';
    bf[7]= (b & 0x01) ? '1' : '0';
    bf[8]=0;
    return bf;
  }

  void reserve_buffers() {
    inbuff= new unsigned char[ibs];
    lcbuf= new unsigned char[cbs];
    lmbuf= new unsigned char[cbs];
    lybuf= new unsigned char[cbs];
    cbuf= new unsigned char[cbs];
    mbuf= new unsigned char[cbs];
    ybuf= new unsigned char[cbs];
    kbuf= new unsigned char[cbs];
    empty_buffers();
  }

  void empty_buffers() {
    for (int i=0; i<cbs; i++) 
      lcbuf[i]=lmbuf[i]=lybuf[i]=cbuf[i]=mbuf[i]=ybuf[i]=kbuf[i]=0;
  }

  void from (char *f) { 
    infilename= (f) ? strdup(f) : 0;  
    infile= fopen(infilename,"r");
  }

  void to (cmykSave *s) { 
    saver = s;
  }

  void flush_buffers(int c); // empties buffer and advances c lines;

  int un_rle(unsigned char *inbuf, int n, unsigned char *outbuf, int max);

  void process_color();
  
  int next(); // returns command number and leaves data in inbuff

  int process();

};

int bjcFile::next()
{
  unsigned char c1=0,c2=0;

  while (!feof(infile) && c1!=0x1b && (c2!=0x28 && c2!=0x5b)) { 
    c1=c2; c2=fgetc(infile); 
  }
  if (feof(infile)) return 0;
 
  cmd=fgetc(infile);
  if (feof(infile)) return 0;

  c1= fgetc(infile);
  if (feof(infile)) return 0;
  c2= fgetc(infile);
  if (feof(infile)) return 0;
  
  cnt= c1+256*c2;

  if (int c=fread(inbuff,1,cnt,infile)!=cnt) {
    fprintf(stderr,"read error for 0x%02x - not enough data (%d/%d)!\n",cmd,
	    c,cnt);
    return 0;
  }

  return cmd;
}

int bjcFile::process()
{
  if (!infile) return 0;
  fseek(infile,0,SEEK_SET);
  int colorlines= 0;

  while (next()) {
    switch(cmd) {
    case 0x64: 
      yres=inbuff[0]*256+inbuff[1];
      xres=inbuff[2]*256+inbuff[3];
      fprintf(stderr,"res=%dx%ddpi\n",xres,yres);
      break;
    case 0x65:
      if (colorlines) 
	flush_buffers(inbuff[0]*256+inbuff[1]);
      break;
    case 0x41:
      process_color();
      colorlines++;
      if (fgetc(infile)!=0x0d) {
	fprintf(stderr,"COLOR LINE NOT TERMINATED BY 0x0d @ %lx!!\n",
		ftell(infile));
      }
      break;
    case 0x62:
      if (colorlines) 
	flush_buffers(inbuff[0]*256+inbuff[1]);
      break;
    default:
      ;
    }
  }

  if (!xsize) {
    xsize= max*8;
    fprintf(stderr,"size=%dx%d\n",xsize,ysize);
    if (saver) saver->open(xsize,ysize);
  }

  return 1;
}

void bjcFile::flush_buffers(int c)
{
  ysize+= c;
  // fprintf(stderr,"|");
  if (saver && xsize) {
    saver->saveline(cbuf,mbuf,ybuf,kbuf,lcbuf,lmbuf,lybuf);
    for (int i=1; i<c; i++)
      saver->emptyline();
  }
  empty_buffers();
}

void bjcFile::process_color()
{
  int m= 0;
  switch(inbuff[0]) {
  case 0x59: // yellow
    // fprintf(stderr,"y");
    m= un_rle(inbuff+1,cnt-1,ybuf,cbs);
    break;
  case 0x4d: // magenta
    // fprintf(stderr,"m");
    m= un_rle(inbuff+1,cnt-1,mbuf,cbs);
    break;
  case 0x43: // cyan
    // fprintf(stderr,"c");
    m= un_rle(inbuff+1,cnt-1,cbuf,cbs);
    break;
  case 0x6d: // lightmagenta
    // fprintf(stderr,"m");
    m= un_rle(inbuff+1,cnt-1,lmbuf,cbs);
    break;
  case 0x63: // lightcyan
    // fprintf(stderr,"c");
    m= un_rle(inbuff+1,cnt-1,lcbuf,cbs);
    break;
  case 0x79: // lightyellow
    // fprintf(stderr,"c");
    m= un_rle(inbuff+1,cnt-1,lybuf,cbs);
    break;
  case 0x4b: // black
    // fprintf(stderr,"k");
    m= un_rle(inbuff+1,cnt-1,kbuf,cbs);
    break;
  default:
    fprintf(stderr,"unkown color component 0x%02x\n",inbuff[0]);
    exit(-1);
  }

  if (m>max) max=m;
  // fprintf(stderr,"%02x %5d  ",inbuff[0],m);
}

int bjcFile::un_rle(unsigned char *inbuf, int n, unsigned char *outbuf,int max)
{
  char *ib= (char *)inbuf;
  char cnt;

  int o= 0;
  if (n<=0) return 0;

  int i= 0;
  while (i<n) {
    cnt= ib[i];
    if (cnt<0) { 
      // cnt identical bytes
      // fprintf(stderr,"rle 0x%02x = %4d = %4d\n",cnt&0xff,cnt,1-cnt);
      int num= 1-cnt;
      // fprintf (stderr,"+%6d ",num);
      for (int j=0; j<num; j++) outbuf[o+j]=inbuf[i+1];
      o+= num;
      i+= 2;
    } else { 
      // cnt individual bytes
      // fprintf(stderr,"raw 0x%02x = %4d = %4d\n",cnt&0xff,cnt,cnt + 1);
      int num= cnt+1;
      // fprintf (stderr,"*%6d ",num);
      for (int j=0; j<num; j++) outbuf[o+j]=inbuf[i+j+1];
      o+= num;
      i+= num+1;
    }
  }
  // fprintf(stderr,"\n   -> = %d\n\n",o);
  
  return o;
}


int main(int argc, char **argv) 
{
  bjcFile bjc;

  fprintf(stderr,"%d args\n",argc);
  if (argc>1) {
    cmykSave mysave;
    if (argc>2)
      mysave.to(argv[2]);
    else 
      mysave.to(argv[1]);
    bjc.from(argv[1]);
    bjc.to(&mysave);
    bjc.process();
    bjc.process();
    mysave.close();
  }
}
