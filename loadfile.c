/*
	loadfile.c - load an existing fractal image, control level
	This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef XFRACT
#include <unistd.h>
#endif
#include "fractint.h"
#include "fractype.h"
#include "targa_lc.h"
#include "prototyp.h"

/* routines in this module	*/

static int  find_fractal_info(char *,struct fractal_info *);
static void load_ext_blk(char far *loadptr,int loadlen);
static void skip_ext_blk(int *,int *);
static void backwardscompat();

int filetype;
int loaded3d;

extern char   maxfn;                    /* num trig functions if formula */
extern int    showfile; 		/* has file been displayed yet? */
extern char   readname[];		/* name of fractal input file */
extern char   far *resume_info; 	/* pointer to resume info if alloc'd */
extern int    resume_len;		/* length of resume info */
extern int    adapter;
extern int    calc_status;
extern int    initmode; 		/* initial video mode	    */
extern long   calctime;
extern int    initbatch;		/* 1 if batch run (no kbd)  */
extern int    maxit;
extern int    initincr; 		/* maxiter incrmnt	    */
extern char   usr_stdcalcmode;		/* pass mode		    */
extern int    fractype; 		/* fractal type 	    */
extern double xxmin,xxmax;		/* corner values	    */
extern double yymin,yymax;		/* corner values	    */
extern double xx3rd,yy3rd;		/* corner values	    */
extern double param[]; 		    /* parameters		    */
extern int    fillcolor;		/* fill color: -1 = normal  */
extern int    inside;			/* inside color: 1=blue     */
extern int    outside;			/* outside color, if set    */
extern int    finattract;		/* finite attractor option  */
extern int    forcesymmetry;
extern int    LogFlag;			/* non-zero if logarithmic palettes */
extern int    rflag, rseed;
extern int    usr_periodicitycheck;
extern char   useinitorbit;
extern _CMPLX initorbit;
extern int    potflag;			/* continuous potential flag */
extern int    pot16bit;
extern double potparam[3];		/* three potential parameters*/
extern double inversion[];
extern int    decomp[];
extern int    usr_distest;		/* non-zero if distance estimator   */
extern int    distestwidth;
extern int    init3d[20];		/* '3d=nn/nn/nn/...' values */
extern char   usr_floatflag;		/* floating-point fractals? */
extern char   floatflag;
extern int    usr_biomorph;
extern char   FormName[];
extern char   LName[];
extern char   IFSName[];
extern int    bailout;			/* user input bailout value */
extern int    previewfactor;
extern int    xtrans;
extern int    ytrans;
extern int    red_crop_left;
extern int    red_crop_right;
extern int    blue_crop_left;
extern int    blue_crop_right;
extern int    red_bright;
extern int    blue_bright;
extern int    xadjust;
extern int    eyeseparation;
extern int    glassestype;
extern int    display3d;		/* 3D display flag: 0 = OFF */
extern int    overlay3d;		/* 3D overlay flag: 0 = OFF */
extern int    viewwindow;		/* 0 for full screen, 1 for window */
extern float  viewreduction;		/* window auto-sizing */
extern float  finalaspectratio; 	/* for view shape and rotation */
extern int    xdots, ydots;		/* # of dots on the logical screen */
extern int    viewxdots,viewydots;	/* explicit view sizing */
extern int    save_system,save_release;
extern int    Ambient;
extern int    RANDOMIZE;
extern int    haze;
extern int    transparent[2];
extern int    rotate_lo,rotate_hi;
extern int far *ranges;
extern int    rangeslen;
extern int    invert;
extern int    num_fractal_types;
extern float  screenaspect;
extern     double mxmaxfp;
extern     double mxminfp;
extern     double mymaxfp;
extern     double myminfp;
extern     int   zdots;
extern     float originfp;
extern     float depthfp;
extern     float heightfp;
extern     float widthfp;
extern     float distfp;
extern     float eyesfp;
extern     int   neworbittype;
extern     short juli3Dmode;
extern     int   major_method;  /* inverse julia method */
extern     int   minor_method;

static FILE *fp;

int fileydots, filexdots, filecolors;
float fileaspectratio;

int skipxdots,skipydots;	/* for decoder, when reducing image */


void loadfile_overlay() { }	/* for restore_active_ovly */

#ifdef XFRACT
extern int decode_fractal_info();
#endif

int read_overlay()	/* read overlay/3D files, if reqr'd */
{
   struct fractal_info read_info;
   char oldfloatflag;
   char msg[110];

   ENTER_OVLY(OVLY_LOADFILE);

   showfile = 1;		/* for any abort exit, pretend done */
   initmode = -1;		/* no viewing mode set yet */
   oldfloatflag = usr_floatflag;
   loaded3d = 0;

   if(strchr(readname,'.') == NULL)
      strcat(readname,".gif");

   if(find_fractal_info(readname,&read_info)) { /* didn't find a useable file */
      sprintf(msg,"Sorry, %s isn't a file I can decode.",readname);
      stopmsg(0,msg);
      EXIT_OVLY;
      return(-1);
      }

   maxit	= read_info.iterations;
   fractype	= read_info.fractal_type;
   if (fractype < 0 || fractype >= num_fractal_types) {
      sprintf(msg,"Warning: %s has a bad fractal type; using 0",readname);
      fractype = 0;
   }
   curfractalspecific = &fractalspecific[fractype];
   xxmin	= read_info.xmin;
   xxmax	= read_info.xmax;
   yymin	= read_info.ymin;
   yymax	= read_info.ymax;
   param[0]	= read_info.creal;
   param[1]	= read_info.cimag;
   save_release = 1100; /* unless we find out better later on */

   invert = 0;
   if(read_info.version > 0) {
      param[2]	    = read_info.parm3;
      roundfloatd(&param[2]);
      param[3]	    = read_info.parm4;
      roundfloatd(&param[3]);
      potparam[0]   = read_info.potential[0];
      potparam[1]   = read_info.potential[1];
      potparam[2]   = read_info.potential[2];
      potflag	    = (potparam[0] != 0.0);
      rflag	    = read_info.rflag;
      rseed	    = read_info.rseed;
      inside	    = read_info.inside;
      LogFlag	    = read_info.logmap;
      inversion[0]  = read_info.invert[0];
      inversion[1]  = read_info.invert[1];
      inversion[2]  = read_info.invert[2];
      if (inversion[0] != 0.0)
	 invert = 3;
      decomp[0]     = read_info.decomp[0];
      decomp[1]     = read_info.decomp[1];
      usr_biomorph  = read_info.biomorph;
      forcesymmetry = read_info.symmetry;
      }

   if(read_info.version > 1) {
      save_release  = 1200;
      if (!display3d
	&& (read_info.version <= 4 || read_info.flag3d > 0
	    || (curfractalspecific->flags&PARMS3D) )) {
	 int i;
	 for (i = 0; i < 16; i++)
	    init3d[i] = read_info.init3d[i];
	 previewfactor	 = read_info.previewfactor;
	 xtrans 	 = read_info.xtrans;
	 ytrans 	 = read_info.ytrans;
	 red_crop_left	 = read_info.red_crop_left;
	 red_crop_right  = read_info.red_crop_right;
	 blue_crop_left  = read_info.blue_crop_left;
	 blue_crop_right = read_info.blue_crop_right;
	 red_bright	 = read_info.red_bright;
	 blue_bright	 = read_info.blue_bright;
	 xadjust	 = read_info.xadjust;
	 eyeseparation	 = read_info.eyeseparation;
	 glassestype	 = read_info.glassestype;
	 }
      }

   if(read_info.version > 2) {
      save_release = 1300;
      outside	   = read_info.outside;
      }

   calc_status = 0;	  /* defaults if version < 4 */
   xx3rd = xxmin;
   yy3rd = yymin;
   usr_distest = 0;
   calctime = 0;
   if(read_info.version > 3) {
      save_release = 1400;
      xx3rd	  = read_info.x3rd;
      yy3rd	  = read_info.y3rd;
      calc_status = read_info.calc_status;
      usr_stdcalcmode = read_info.stdcalcmode;
      usr_distest     = read_info.distest;
      usr_floatflag   = read_info.floatflag;
      bailout	  = read_info.bailout;
      calctime	  = read_info.calctime;
      trigndx[0]  = read_info.trigndx[0];
      trigndx[1]  = read_info.trigndx[1];
      trigndx[2]  = read_info.trigndx[2];
      trigndx[3]  = read_info.trigndx[3];
      finattract  = read_info.finattract;
      initorbit.x = read_info.initorbit[0];
      initorbit.y = read_info.initorbit[1];
      useinitorbit = read_info.useinitorbit;
      usr_periodicitycheck = read_info.periodicity;
      }

   pot16bit = 0;
   save_system = 0;
   if(read_info.version > 4) {
      pot16bit	   = read_info.pot16bit;
      if (pot16bit)
	 filexdots >>= 1;
      fileaspectratio = read_info.faspectratio;
      if (fileaspectratio < 0.01)	/* fix files produced in early v14.1 */
	 fileaspectratio = screenaspect;
      save_system  = read_info.system;
      save_release = read_info.release; /* from fmt 5 on we know real number */
      if (read_info.version == 5	/* except a few early fmt 5 cases: */
	  && (save_release <= 0 || save_release >= 2000)) {
	 save_release = 1410;
	 save_system = 0;
	 }
      if (!display3d && read_info.flag3d > 0) {
	 loaded3d	= 1;
	 Ambient	= read_info.ambient;
	 RANDOMIZE	= read_info.randomize;
	 haze		= read_info.haze;
	 transparent[0] = read_info.transparent[0];
	 transparent[1] = read_info.transparent[1];
	 }
      }

   rotate_lo = 1; rotate_hi = 255;
   distestwidth = 71;
   if(read_info.version > 5) {
      rotate_lo 	= read_info.rotate_lo;
      rotate_hi 	= read_info.rotate_hi;
      distestwidth	= read_info.distestwidth;
      }

   if(read_info.version > 6) {
      param[2]		= read_info.dparm3;
      param[3]		= read_info.dparm4;
      }

   if(read_info.version > 7) {
      fillcolor		= read_info.fillcolor;
      }

   if(read_info.version > 8) {
   mxmaxfp   =  read_info.mxmaxfp        ;
   mxminfp   =  read_info.mxminfp        ;
   mymaxfp   =  read_info.mymaxfp        ;
   myminfp   =  read_info.myminfp        ;
   zdots     =  read_info.zdots          ;		
   originfp  =  read_info.originfp       ;
   depthfp   =  read_info.depthfp        ;	
   heightfp  =  read_info.heightfp       ;
   widthfp   =  read_info.widthfp        ;	
   distfp    =  read_info.distfp         ;	
   eyesfp    =  read_info.eyesfp         ;	
   neworbittype = read_info.orbittype    ;
   juli3Dmode   = read_info.juli3Dmode   ;
   maxfn    =   read_info.maxfn          ;
   major_method = read_info.inversejulia >> 8;
   minor_method = read_info.inversejulia & 255;
   param[4] = read_info.dparm5;
   param[5] = read_info.dparm6;
   param[6] = read_info.dparm7;
   param[7] = read_info.dparm8;
   param[8] = read_info.dparm9;
   param[9] = read_info.dparm10;
      }

   if(read_info.version < 4) { /* pre-version 14.0? */
      backwardscompat(&read_info); /* translate obsolete types */
      if(LogFlag)
	 LogFlag = 2;
      usr_floatflag = (curfractalspecific->isinteger) ? 0 : 1;
      }

   if (read_info.version < 5) { /* pre-version 15.0? */
      if (LogFlag == 2) /* logmap=old changed again in format 5! */
	 LogFlag = -1;
      if (decomp[0] > 0 && decomp[1] > 0)
	 bailout = decomp[1];
      }
   if(potflag) /* in version 15.x and 16.x logmap didn't work with pot */
      if(read_info.version == 6 || read_info.version == 7)
         LogFlag = 0;
   set_trig_pointers(-1);

   if(read_info.version < 9) { /* pre-version 18.0? */
      /* forcesymmetry==1000 means we want to force symmetry but don't
         know which symmetry yet, will find out in setsymmetry() */
      if(outside==REAL || outside==IMAG || outside==MULT || outside==SUM)
         if(forcesymmetry == 999)
            forcesymmetry = 1000;
      }
   if(save_release < 1725) { /* pre-version 17.25 */
      set_if_old_bif(); /* translate bifurcation types */
   }

   if (display3d)		    /* PB - a klooge till the meaning of */
      usr_floatflag = oldfloatflag; /*	floatflag in line3d is clarified */

   if (overlay3d) {
      initmode = adapter;	   /* use previous adapter mode for overlays */
      if (filexdots > xdots || fileydots > ydots) {
	 static char far msg[]={"Can't overlay with a larger image"};
	 stopmsg(0,msg);
	 EXIT_OVLY;
	 initmode = -1;
	 return(-1);
	 }
      }
   else {
      int olddisplay3d,oldfloatflag,i;
      olddisplay3d = display3d;
      oldfloatflag = floatflag;
      display3d = loaded3d;      /* for <tab> display during next */
      floatflag = usr_floatflag; /* ditto */
      i = get_video_mode(&read_info);
      display3d = olddisplay3d;
      floatflag = oldfloatflag;
      if (i) {
	 EXIT_OVLY;
	 initmode = -1;
	 return(-1);
	 }
      }

   if (display3d) {
      calc_status = 0;
      fractype = PLASMA;
      curfractalspecific = &fractalspecific[PLASMA];
      param[0] = 0;
      if (!initbatch)
	 if (get_3d_params() < 0) {
	    EXIT_OVLY;
	    initmode = -1;
	    return(-1);
	    }
      }

   showfile = 0;		   /* trigger the file load */

   EXIT_OVLY;
   return(0);
}


static int find_fractal_info(gif_file,info)
char *gif_file;
struct fractal_info *info;
{
   BYTE gifstart[18];
   char temp1[81];
   int scan_extend, block_type, block_len, data_len;
   int fractinf_len;
   int hdr_offset;

   if((fp = fopen(gif_file,"rb"))==NULL)
      return(-1);

   fread(gifstart,18,1,fp);
#ifndef XFRACT
   if (strncmp(gifstart,"GIF",3)!=0) { /* not GIF, maybe old .tga? */
#else
   if (strncmp((char *)gifstart,"GIF",3)!=0) { /* not GIF, maybe old .tga? */
#endif
      if(fread(info,FRACTAL_INFO_SIZE,1,fp)==1 &&
		 strncmp(info->info_id,"Fractal",8)==0) {
	 filetype = 1; /* Targa 16 */
	 GET16(gifstart[O_VSIZE],fileydots);
	 GET16(gifstart[O_HSIZE],filexdots);
	 filecolors = info->colors;
	 fileaspectratio = screenaspect;
	 if(fileydots == info->ydots && filexdots == info->xdots) {
	    fclose(fp);
	    return(0);
	    }
	 }
      fclose(fp);
      return(-1);
      }

   filetype = 0; /* GIF */
   GET16(gifstart[6],filexdots);
   GET16(gifstart[8],fileydots);
   filecolors = 2 << (gifstart[10] & 7);
   fileaspectratio = 0; /* unknown */
   if (gifstart[12]) { /* calc reasonably close value from gif header */
      fileaspectratio = (64.0 / ((double)(gifstart[12]) + 15.0))
		      * (double)fileydots / (double)filexdots;
      if ( fileaspectratio > screenaspect-0.03
	&& fileaspectratio < screenaspect+0.03)
	 fileaspectratio = screenaspect;
      }
   else
      if (fileydots * 4 == filexdots * 3) /* assume the common square pixels */
	 fileaspectratio = screenaspect;

   if (resume_info != NULL) { /* free the prior area if there is one */
      farmemfree(resume_info);
      resume_info = NULL;
      }
   if (rangeslen) { /* free prior ranges */
      farmemfree((char far *)ranges);
      rangeslen = 0;
      }

   /* Format of .gif extension blocks is:
	  1 byte    '!', extension block identifier
	  1 byte    extension block number, 255
	  1 byte    length of id, 11
	 11 bytes   alpha id, "fractintnnn" with fractint, nnn is secondary id
       n * {
	  1 byte    length of block info in bytes
	  x bytes   block info
	   }
	  1 byte    0, extension terminator
      To scan extension blocks, we first look in file at length of fractal_info
      (the main extension block) from end of file, looking for a literal known
      to be at start of our block info.  Then we scan forward a bit, in case
      the file is from an earlier fractint vsn with shorter fractal_info.
      If fractal_info is found and is from vsn>=14, it includes the total length
      of all extension blocks; we then scan them all first to last to load
      any optional ones which are present.
      Defined extension blocks:
	fractint001	header, always present
	fractint002	resume info for interrupted resumable image
	fractint003	additional formula type info
	fractint004	ranges info
   */

   memset(info,0,FRACTAL_INFO_SIZE);
   fractinf_len = FRACTAL_INFO_SIZE + (FRACTAL_INFO_SIZE+254)/255;
   fseek(fp,(long)(-1-fractinf_len),SEEK_END);
   fread(info,1,FRACTAL_INFO_SIZE,fp);
   if (strcmp(INFO_ID,info->info_id) == 0) {
#ifdef XFRACT
       decode_fractal_info(info,1);
#endif
      hdr_offset = -1-fractinf_len;
   } else {
      /* didn't work 1st try, maybe an older vsn, maybe junk at eof, scan: */
      int offset,i;
      char tmpbuf[110];
      hdr_offset = 0;
      offset = 80; /* don't even check last 80 bytes of file for id */
      while (offset < fractinf_len+513) { /* allow 512 garbage at eof */
	 offset += 100; /* go back 100 bytes at a time */
	 fseek(fp,(long)(0-offset),SEEK_END);
	 fread(tmpbuf,1,110,fp); /* read 10 extra for string compare */
	 for (i = 0; i < 100; ++i)
	    if (!strcmp(INFO_ID,&tmpbuf[i])) { /* found header? */
	       strcpy(info->info_id,INFO_ID);
	       fseek(fp,(long)(hdr_offset=i-offset),SEEK_END);
	       fread(info,1,FRACTAL_INFO_SIZE,fp);
#ifdef XFRACT
	       decode_fractal_info(info,1);
#endif
	       offset = 10000; /* force exit from outer loop */
	       break;
	       }
	 }
      }

   if (hdr_offset) { /* we found INFO_ID */

      if (info->version >= 4) {
	 /* first reload main extension block, reasons:
	      might be over 255 chars, and thus earlier load might be bad
	      find exact endpoint, so scan back to start of ext blks works
	    */
	 fseek(fp,(long)(hdr_offset-15),SEEK_END);
	 scan_extend = 1;
	 while (scan_extend) {
	    if (fgetc(fp) != '!' /* if not what we expect just give up */
	      || fread(temp1,1,13,fp) != 13
	      || strncmp(&temp1[2],"fractint",8))
	       break;
	    temp1[13] = 0;
	    block_type = atoi(&temp1[10]); /* e.g. "fractint002" */
	    switch (block_type) {
	       case 1: /* "fractint001", the main extension block */
		  if (scan_extend == 2) { /* we've been here before, done now */
		     scan_extend = 0;
		     break;
		     }
		  load_ext_blk((char far *)info,FRACTAL_INFO_SIZE);
#ifdef XFRACT
		  decode_fractal_info(info,1);
#endif
		  scan_extend = 2;
		  /* now we know total extension len, back up to first block */
		  fseek(fp,0L-info->tot_extend_len,SEEK_CUR);
		  break;
	       case 2: /* resume info */
		  skip_ext_blk(&block_len,&data_len); /* once to get lengths */
		  if ((resume_info = farmemalloc((long)data_len)) == NULL)
		     info->calc_status = 3; /* not resumable after all */
		  else {
		     fseek(fp,(long)(0-block_len),SEEK_CUR);
		     load_ext_blk(resume_info,data_len);
		     resume_len = data_len;
		     }
		  break;
	       case 3: /* formula info */
		  {
		  char *nameptr;
		  char tmpname[40];
		  load_ext_blk(tmpname,40);
		  switch (info->fractal_type) {
		     case LSYSTEM:
			nameptr = LName;
			break;
		     case IFS:
		     case IFS3D:
			nameptr = IFSName;
			break;
		     default:
			nameptr = FormName;
			break;
		     }
		  tmpname[ITEMNAMELEN] = 0;
		  strcpy(nameptr,tmpname);
		  /* perhaps in future add more here, check block_len for
		     backward compatibility */
		  }
		  break;
	       case 4: /* ranges info */
		  skip_ext_blk(&block_len,&data_len); /* once to get lengths */
		  if ((ranges = (int far *)farmemalloc((long)data_len))) {
		     fseek(fp,(long)(0-block_len),SEEK_CUR);
		     load_ext_blk((char far *)ranges,data_len);
		     rangeslen = data_len/2;
#ifdef XFRACTINT
		     fix_ranges(ranges,rangeslen,1);
#endif
		     }
		  break;
	       default:
		  skip_ext_blk(&block_len,&data_len);
	       }
	    }
	 }

      fclose(fp);
      fileaspectratio = screenaspect; /* if not >= v15, this is correct */
      return(0);
      }

   strcpy(info->info_id, "GIFFILE");
   info->iterations = 150;
   info->fractal_type = PLASMA;
   info->xmin = -1;
   info->xmax = 1;
   info->ymin = -1;
   info->ymax = 1;
   info->x3rd = -1;
   info->y3rd = -1;
   info->creal = 0;
   info->cimag = 0;
   info->videomodeax=255;
   info->videomodebx=255;
   info->videomodecx=255;
   info->videomodedx=255;
   info->dotmode = 0;
   info->xdots = filexdots;
   info->ydots = fileydots;
   info->colors = filecolors;
   info->version = 0; /* this forces lots more init at calling end too */

   /* zero means we won */
   fclose(fp);
   return(0);
}

static void load_ext_blk(char far *loadptr,int loadlen)
{
   int len;
   while ((len = fgetc(fp)) > 0) {
      while (--len >= 0) {
	 if (--loadlen >= 0)
	    *(loadptr++) = fgetc(fp);
	 else
	    fgetc(fp); /* discard excess characters */
	 }
      }
}

static void skip_ext_blk(int *block_len, int *data_len)
{
   int len;
   *data_len = 0;
   *block_len = 1;
   while ((len = fgetc(fp)) > 0) {
      fseek(fp,(long)len,SEEK_CUR);
      *data_len += len;
      *block_len += len + 1;
      }
}


/* switch obsolete fractal types to new generalizations */
static void backwardscompat(struct fractal_info *info)
{
   switch(fractype) {
      case LAMBDASINE:
	 fractype = LAMBDATRIGFP;
	 trigndx[0] = SIN;
	 break;
      case LAMBDACOS	:
	 fractype = LAMBDATRIGFP;
	 trigndx[0] = COS;
	 break;
      case LAMBDAEXP	:
	 fractype = LAMBDATRIGFP;
	 trigndx[0] = EXP;
	 break;
      case MANDELSINE	:
	 fractype = MANDELTRIGFP;
	 trigndx[0] = SIN;
	 break;
      case MANDELCOS	:
	 fractype = MANDELTRIGFP;
	 trigndx[0] = COS;
	 break;
      case MANDELEXP	:
	 fractype = MANDELTRIGFP;
	 trigndx[0] = EXP;
	 break;
      case MANDELSINH	:
	 fractype = MANDELTRIGFP;
	 trigndx[0] = SINH;
	 break;
      case LAMBDASINH	:
	 fractype = LAMBDATRIGFP;
	 trigndx[0] = SINH;
	 break;
      case MANDELCOSH	:
	 fractype = MANDELTRIGFP;
	 trigndx[0] = COSH;
	 break;
      case LAMBDACOSH	:
	 fractype = LAMBDATRIGFP;
	 trigndx[0] = COSH;
	 break;
      case LMANDELSINE	:
	 fractype = MANDELTRIG;
	 trigndx[0] = SIN;
	 break;
      case LLAMBDASINE	:
	 fractype = LAMBDATRIG;
	 trigndx[0] = SIN;
	 break;
      case LMANDELCOS	:
	 fractype = MANDELTRIG;
	 trigndx[0] = COS;
	 break;
      case LLAMBDACOS	:
	 fractype = LAMBDATRIG;
	 trigndx[0] = COS;
	 break;
      case LMANDELSINH	:
	 fractype = MANDELTRIG;
	 trigndx[0] = SINH;
	 break;
      case LLAMBDASINH	:
	 fractype = LAMBDATRIG;
	 trigndx[0] = SINH;
	 break;
      case LMANDELCOSH	:
	 fractype = MANDELTRIG;
	 trigndx[0] = COSH;
	 break;
      case LLAMBDACOSH	:
	 fractype = LAMBDATRIG;
	 trigndx[0] = COSH;
	 break;
      case LMANDELEXP	:
	 fractype = MANDELTRIG;
	 trigndx[0] = EXP;
	 break;
      case LLAMBDAEXP	:
	 fractype = LAMBDATRIG;
	 trigndx[0] = EXP;
	 break;
      case DEMM 	:
	 fractype = MANDELFP;
	 usr_distest = (info->ydots - 1) * 2;
	 break;
      case DEMJ 	:
	 fractype = JULIAFP;
	 usr_distest = (info->ydots - 1) * 2;
	 break;
      case MANDELLAMBDA :
	 useinitorbit = 2;
	 break;
      }
   curfractalspecific = &fractalspecific[fractype];
}

/* switch old bifurcation fractal types to new generalizations */
void set_if_old_bif(void)
{
/* set functions if not set already, may need to check 'functionpreloaded'
   before calling this routine.  JCO 7/5/92 */

   ENTER_OVLY(OVLY_LOADFILE);
   switch(fractype) {
      case BIFURCATION:
      case LBIFURCATION:
      case BIFSTEWART:
      case LBIFSTEWART:
      case BIFLAMBDA:
      case LBIFLAMBDA:
        set_trig_array(0,"ident");
        break;

      case BIFEQSINPI:
      case LBIFEQSINPI:
      case BIFADSINPI:
      case LBIFADSINPI:
        set_trig_array(0,"sin");
        break;
   }
   EXIT_OVLY;
}
