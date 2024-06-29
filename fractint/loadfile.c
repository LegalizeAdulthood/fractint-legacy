/*
        loadfile.c - load an existing fractal image, control level
*/

#include <string.h>
#include <time.h>
#include <errno.h>
#ifdef XFRACT
#include <unistd.h>
#endif
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "targa_lc.h"

/* routines in this module      */

static int  find_fractal_info(char *,struct fractal_info *,struct ext_blk_2 *,
                        struct ext_blk_3 *,struct ext_blk_4 *,struct ext_blk_5 *);
static void load_ext_blk(char far *loadptr,int loadlen);
static void skip_ext_blk(int *,int *);
static void backwardscompat(struct fractal_info *info);
static int fix_bof(void);
static int fix_period_bof(void);

int filetype;
int loaded3d;
static FILE *fp;
int fileydots, filexdots, filecolors;
float fileaspectratio;
short skipxdots,skipydots;      /* for decoder, when reducing image */

#ifdef XFRACT
int decode_fractal_info();
#endif

int read_overlay()      /* read overlay/3D files, if reqr'd */
{
   struct fractal_info read_info;
   char oldfloatflag;
   char msg[110];
   struct ext_blk_2 blk_2_info;
   struct ext_blk_3 blk_3_info;
   struct ext_blk_4 blk_4_info;
   struct ext_blk_5 blk_5_info;

   showfile = 1;                /* for any abort exit, pretend done */
   initmode = -1;               /* no viewing mode set yet */
   oldfloatflag = usr_floatflag;
   loaded3d = 0;
   if(fastrestore)
      viewwindow=0;
   if(has_ext(readname) == NULL)
      strcat(readname,".gif");

   if(find_fractal_info(readname,&read_info,&blk_2_info,&blk_3_info,
                        &blk_4_info,&blk_5_info)) {
      /* didn't find a useable file */
      sprintf(msg,"Sorry, %s isn't a file I can decode.",readname);
      stopmsg(0,msg);
      return(-1);
      }

   maxit        = read_info.iterationsold;
   fractype     = read_info.fractal_type;
   if (fractype < 0 || fractype >= num_fractal_types) {
      sprintf(msg,"Warning: %s has a bad fractal type; using 0",readname);
      fractype = 0;
   }
   curfractalspecific = &fractalspecific[fractype];
   xxmin        = read_info.xmin;
   xxmax        = read_info.xmax;
   yymin        = read_info.ymin;
   yymax        = read_info.ymax;
   param[0]     = read_info.creal;
   param[1]     = read_info.cimag;
   save_release = 1100; /* unless we find out better later on */

   invert = 0;
   if(read_info.version > 0) {
      param[2]      = read_info.parm3;
      roundfloatd(&param[2]);
      param[3]      = read_info.parm4;
      roundfloatd(&param[3]);
      potparam[0]   = read_info.potential[0];
      potparam[1]   = read_info.potential[1];
      potparam[2]   = read_info.potential[2];
      if(*s_makepar == '\0')
         colors = read_info.colors;
      potflag       = (potparam[0] != 0.0);
      rflag         = read_info.rflag;
      rseed         = read_info.rseed;
      inside        = read_info.inside;
      LogFlag       = read_info.logmapold;
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
         previewfactor   = read_info.previewfactor;
         xtrans          = read_info.xtrans;
         ytrans          = read_info.ytrans;
         red_crop_left   = read_info.red_crop_left;
         red_crop_right  = read_info.red_crop_right;
         blue_crop_left  = read_info.blue_crop_left;
         blue_crop_right = read_info.blue_crop_right;
         red_bright      = read_info.red_bright;
         blue_bright     = read_info.blue_bright;
         xadjust         = read_info.xadjust;
         eyeseparation   = read_info.eyeseparation;
         glassestype     = read_info.glassestype;
         }
      }

   if(read_info.version > 2) {
      save_release = 1300;
      outside      = read_info.outside;
      }

   calc_status = 0;       /* defaults if version < 4 */
   xx3rd = xxmin;
   yy3rd = yymin;
   usr_distest = 0;
   calctime = 0;
   if(read_info.version > 3) {
      save_release = 1400;
      xx3rd       = read_info.x3rd;
      yy3rd       = read_info.y3rd;
      calc_status = read_info.calc_status;
      usr_stdcalcmode = read_info.stdcalcmode;
      three_pass = 0;
      if(usr_stdcalcmode == 127)
      {
         three_pass = 1;
         usr_stdcalcmode = '3';
      }
      usr_distest     = read_info.distestold;
      usr_floatflag   = (char)read_info.floatflag;
      bailout     = read_info.bailoutold;
      calctime    = read_info.calctime;
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
      pot16bit     = read_info.pot16bit;
      if (pot16bit)
         filexdots >>= 1;
      fileaspectratio = read_info.faspectratio;
      if (fileaspectratio < 0.01)       /* fix files produced in early v14.1 */
         fileaspectratio = screenaspect;
      save_system  = read_info.system;
      save_release = read_info.release; /* from fmt 5 on we know real number */
      if (read_info.version == 5        /* except a few early fmt 5 cases: */
          && (save_release <= 0 || save_release >= 4000)) {
         save_release = 1410;
         save_system = 0;
         }
      if (!display3d && read_info.flag3d > 0) {
         loaded3d       = 1;
         Ambient        = read_info.ambient;
         RANDOMIZE      = read_info.randomize;
         haze           = read_info.haze;
         transparent[0] = read_info.transparent[0];
         transparent[1] = read_info.transparent[1];
         }
      }

   rotate_lo = 1; rotate_hi = 255;
   distestwidth = 71;
   if(read_info.version > 5) {
      rotate_lo         = read_info.rotate_lo;
      rotate_hi         = read_info.rotate_hi;
      distestwidth      = read_info.distestwidth;
      }

   if(read_info.version > 6) {
      param[2]          = read_info.dparm3;
      param[3]          = read_info.dparm4;
      }

   if(read_info.version > 7) {
      fillcolor         = read_info.fillcolor;
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
   maxfn    =   (char)read_info.maxfn          ;
   major_method = (enum Major)read_info.inversejulia >> 8;
   minor_method = (enum Minor)read_info.inversejulia & 255;
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
      usr_floatflag = (char)((curfractalspecific->isinteger) ? 0 : 1);
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
      if(outside==REAL || outside==IMAG || outside==MULT || outside==SUM
        || outside==ATAN)
         if(forcesymmetry == 999)
            forcesymmetry = 1000;
      }
   if(save_release < 1725) { /* pre-version 17.25 */
      set_if_old_bif(); /* translate bifurcation types */
      functionpreloaded = 1;
   }

   if(read_info.version > 9)
   { /* post-version 18.22 */
      bailout     = read_info.bailout; /* use long bailout */
      bailoutest = (enum bailouts)read_info.bailoutest;
   }
   else
      bailoutest = Mod;
   setbailoutformula(bailoutest);

   if(read_info.version > 9) {
     /* post-version 18.23 */
      maxit = read_info.iterations; /* use long maxit */
     /* post-version 18.27 */
      old_demm_colors = read_info.old_demm_colors;
   }

   if (read_info.version > 10) { /* post-version 19.20 */
      LogFlag = read_info.logmap;
      usr_distest= read_info.distest;
   }

   if (read_info.version > 11) { /* post-version 19.20, inversion fix */
      inversion[0] = read_info.dinvert[0];
      inversion[1] = read_info.dinvert[1];
      inversion[2] = read_info.dinvert[2];
      Log_Fly_Calc = read_info.logcalc;
      stoppass     = read_info.stoppass;
   }

   backwards_v18();
   backwards_v19();

   if (display3d)                   /* PB - a klooge till the meaning of */
      usr_floatflag = oldfloatflag; /*  floatflag in line3d is clarified */

   if (overlay3d) {
      initmode = adapter;          /* use previous adapter mode for overlays */
      if (filexdots > xdots || fileydots > ydots) {
         static FCODE msg[]={"Can't overlay with a larger image"};
         stopmsg(0,msg);
         initmode = -1;
         return(-1);
         }
      }
   else {
      int olddisplay3d,i;
      char oldfloatflag;
      olddisplay3d = display3d;
      oldfloatflag = floatflag;
      display3d = loaded3d;      /* for <tab> display during next */
      floatflag = usr_floatflag; /* ditto */
      if(*s_makepar != 0)
         i = get_video_mode(&read_info,&blk_3_info);
      else
         i = 0;
      display3d = olddisplay3d;
      floatflag = oldfloatflag;
      if (i) {
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
            initmode = -1;
            return(-1);
            }
      }

   if (resume_info != NULL) { /* free the prior area if there is one */
      farmemfree(resume_info);
      resume_info = NULL;
      }

   if(blk_2_info.got_data == 1)
          {
          resume_info = blk_2_info.resume_data;
          resume_len = blk_2_info.length;
          }

   if(blk_3_info.got_data == 1)
          {
          char *nameptr;
          switch (read_info.fractal_type) {
             case LSYSTEM:
                nameptr = LName;
                break;
             case IFS:
             case IFS3D:
                nameptr = IFSName;
                break;
             default:
                nameptr = FormName;
                uses_p1 = blk_3_info.uses_p1;
                uses_p2 = blk_3_info.uses_p2;
                uses_p3 = blk_3_info.uses_p3;
                break;
             }
          blk_3_info.form_name[ITEMNAMELEN] = 0;
          strcpy(nameptr,blk_3_info.form_name);
          /* perhaps in future add more here, check block_len for
             backward compatibility */
          }

   if (rangeslen) { /* free prior ranges */
     farmemfree((char far *)ranges);
     rangeslen = 0;
   }

   if(blk_4_info.got_data == 1)
          {
          ranges = (int far *)blk_4_info.range_data;
          rangeslen = blk_4_info.length;
#ifdef XFRACTINT
          fix_ranges(ranges,rangeslen,1);
#endif
          }

   if(blk_5_info.got_data == 1)
          {
          bf_math = 1;
          init_bf_length(read_info.bflength);
          far_memcpy((char far *)bfxmin,blk_5_info.apm_data,blk_5_info.length);
          farmemfree(blk_5_info.apm_data);
          }
   else
      bf_math = 0;
   showfile = 0;                   /* trigger the file load */
   return(0);
}

static int find_fractal_info(char *gif_file,struct fractal_info *info,
       struct ext_blk_2 *blk_2_info,struct ext_blk_3 *blk_3_info,
       struct ext_blk_4 *blk_4_info,struct ext_blk_5 *blk_5_info)
{
   BYTE gifstart[18];
   char temp1[81];
   int scan_extend, block_type, block_len, data_len;
   int fractinf_len;
   int hdr_offset;
   struct formula_info fload_info;
   int i, j, k;

   blk_2_info->got_data = 0; /* initialize to no data */
   blk_3_info->got_data = 0; /* initialize to no data */
   blk_4_info->got_data = 0; /* initialize to no data */
   blk_5_info->got_data = 0; /* initialize to no data */

   if((fp = fopen(gif_file,"rb"))==NULL)
      return(-1);
   fread(gifstart,13,1,fp);
   if (strncmp((char *)gifstart,"GIF",3) != 0) { /* not GIF, maybe old .tga? */
      fclose(fp);
      return(-1);
   }

   filetype = 0; /* GIF */
   GET16(gifstart[6],filexdots);
   GET16(gifstart[8],fileydots);
   filecolors = 2 << (gifstart[10] & 7);
   fileaspectratio = 0; /* unknown */
   if (gifstart[12]) { /* calc reasonably close value from gif header */
      fileaspectratio = (float)((64.0 / ((double)(gifstart[12]) + 15.0))
                      * (double)fileydots / (double)filexdots);
      if ( fileaspectratio > screenaspect-0.03
        && fileaspectratio < screenaspect+0.03)
         fileaspectratio = screenaspect;
      }
   else
      if (fileydots * 4 == filexdots * 3) /* assume the common square pixels */
         fileaspectratio = screenaspect;

   if(*s_makepar == 0 && (gifstart[10] & 0x80)!=0)
   {
      for (i = 0; i < filecolors; i++)
      {
         for (j = 0; j < 3; j++) {
            if ((k = getc(fp)) < 0)
               break;
            dacbox[i][j] = (BYTE)(k >> 2);
         }
         if(k < 0)
            break;
      }
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
        fractint001     header, always present
        fractint002     resume info for interrupted resumable image
        fractint003     additional formula type info
        fractint004     ranges info
        fractint005     extended precision parameters
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
                  if ((blk_2_info->resume_data = farmemalloc((long)data_len)) == NULL)
                     info->calc_status = 3; /* not resumable after all */
                  else {
                     fseek(fp,(long)(0-block_len),SEEK_CUR);
                     load_ext_blk(blk_2_info->resume_data,data_len);
                     blk_2_info->length = data_len;
                     blk_2_info->got_data = 1; /* got data */
                     }
                  break;
               case 3: /* formula info */
                  skip_ext_blk(&block_len,&data_len); /* once to get lengths */
                /* check data_len for backward compatibility */
                  fseek(fp,(long)(0-block_len),SEEK_CUR);
                  load_ext_blk((char far *)&fload_info,data_len);
                  strcpy(blk_3_info->form_name,fload_info.form_name);
                  blk_3_info->length = data_len;
                  blk_3_info->got_data = 1; /* got data */
                  if (data_len < sizeof(fload_info)) { /* must be old GIF */
                     blk_3_info->uses_p1 = 1;
                     blk_3_info->uses_p2 = 1;
                     blk_3_info->uses_p3 = 1;
                  }
                  else {
                     blk_3_info->uses_p1 = fload_info.uses_p1;
                     blk_3_info->uses_p2 = fload_info.uses_p2;
                     blk_3_info->uses_p3 = fload_info.uses_p3;
                  }
                  break;
               case 4: /* ranges info */
                  skip_ext_blk(&block_len,&data_len); /* once to get lengths */
                  if ((blk_4_info->range_data = (int far *)farmemalloc((long)data_len)) != NULL) {
                     fseek(fp,(long)(0-block_len),SEEK_CUR);
                     load_ext_blk((char far *)blk_4_info->range_data,data_len);
                     blk_4_info->length = data_len/2;
                     blk_4_info->got_data = 1; /* got data */
                     }
                  break;
               case 5: /* extended precision parameters  */
                  skip_ext_blk(&block_len,&data_len); /* once to get lengths */
                  if ((blk_5_info->apm_data = farmemalloc((long)data_len)) != NULL) {
                     fseek(fp,(long)(0-block_len),SEEK_CUR);
                     load_ext_blk(blk_5_info->apm_data,data_len);
                     blk_5_info->length = data_len;
                     blk_5_info->got_data = 1; /* got data */
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
   info->xdots = (short)filexdots;
   info->ydots = (short)fileydots;
   info->colors = (short)filecolors;
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
            *(loadptr++) = (char)fgetc(fp);
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
      case LAMBDACOS    :
         fractype = LAMBDATRIGFP;
         trigndx[0] = COS;
         break;
      case LAMBDAEXP    :
         fractype = LAMBDATRIGFP;
         trigndx[0] = EXP;
         break;
      case MANDELSINE   :
         fractype = MANDELTRIGFP;
         trigndx[0] = SIN;
         break;
      case MANDELCOS    :
         fractype = MANDELTRIGFP;
         trigndx[0] = COS;
         break;
      case MANDELEXP    :
         fractype = MANDELTRIGFP;
         trigndx[0] = EXP;
         break;
      case MANDELSINH   :
         fractype = MANDELTRIGFP;
         trigndx[0] = SINH;
         break;
      case LAMBDASINH   :
         fractype = LAMBDATRIGFP;
         trigndx[0] = SINH;
         break;
      case MANDELCOSH   :
         fractype = MANDELTRIGFP;
         trigndx[0] = COSH;
         break;
      case LAMBDACOSH   :
         fractype = LAMBDATRIGFP;
         trigndx[0] = COSH;
         break;
      case LMANDELSINE  :
         fractype = MANDELTRIG;
         trigndx[0] = SIN;
         break;
      case LLAMBDASINE  :
         fractype = LAMBDATRIG;
         trigndx[0] = SIN;
         break;
      case LMANDELCOS   :
         fractype = MANDELTRIG;
         trigndx[0] = COS;
         break;
      case LLAMBDACOS   :
         fractype = LAMBDATRIG;
         trigndx[0] = COS;
         break;
      case LMANDELSINH  :
         fractype = MANDELTRIG;
         trigndx[0] = SINH;
         break;
      case LLAMBDASINH  :
         fractype = LAMBDATRIG;
         trigndx[0] = SINH;
         break;
      case LMANDELCOSH  :
         fractype = MANDELTRIG;
         trigndx[0] = COSH;
         break;
      case LLAMBDACOSH  :
         fractype = LAMBDATRIG;
         trigndx[0] = COSH;
         break;
      case LMANDELEXP   :
         fractype = MANDELTRIG;
         trigndx[0] = EXP;
         break;
      case LLAMBDAEXP   :
         fractype = LAMBDATRIG;
         trigndx[0] = EXP;
         break;
      case DEMM         :
         fractype = MANDELFP;
         usr_distest = (info->ydots - 1) * 2;
         break;
      case DEMJ         :
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
        set_trig_array(0,s_sin);
        break;
   }
}

void backwards_v18(void)
{
  if(!functionpreloaded)
    set_if_old_bif(); /* old bifs need function set, JCO 7/5/92 */
  if(fractype==MANDELTRIG && usr_floatflag==1
         && save_release < 1800 && bailout == 0)
    bailout = 2500;
  if(fractype==LAMBDATRIG && usr_floatflag==1
         && save_release < 1800 && bailout == 0)
    bailout = 2500;
}

void backwards_v19(void)
{
  if(fractype==MARKSJULIA && save_release < 1825)
    if(param[2] == 0)
       param[2] = 2;
    else
       param[2] += 1;
  if(fractype==MARKSJULIAFP && save_release < 1825)
    if(param[2] == 0)
       param[2] = 2;
    else
       param[2] += 1;
  if((fractype==FORMULA || fractype==FFORMULA) && save_release < 1824)
    inversion[0] = inversion[1] = inversion[2] = invert = 0;
  if(fix_bof())
    no_mag_calc = 1; /* fractal has old bof60/61 problem with magnitude */
  else
    no_mag_calc = 0;
  if(fix_period_bof())
    use_old_period = 1; /* fractal uses old periodicity method */
  else
    use_old_period = 0;
  if(save_release < 1827 && distest)
    use_old_distest = 1; /* use old distest code */
  else
    use_old_distest = 0; /* use new distest code */
}

int check_back(void) {
/*
   put the features that need to save the value in save_release for backwards
   compatibility in this routine
*/
int ret = 0;
   if (fractype == LYAPUNOV ||
       fractype == FROTH || fractype == FROTHFP ||
       fix_bof() || fix_period_bof() || use_old_distest || decomp[0] == 2 ||
       (fractype == FORMULA && save_release <= 1920) ||
       (fractype == FFORMULA && save_release <= 1920) ||
       (LogFlag != 0 && save_release <= 1920) ||
       (fractype == TRIGSQR && save_release < 1900) ||
       (inside == STARTRAIL && save_release < 1825) ||
       (maxit > 32767 && save_release <= 1950) ||
       (distest && save_release <=1950) )
     ret = 1;
   return(ret);
}

static int fix_bof(void)
{
int ret = 0;
 if (inside <= BOF60 && inside >= BOF61 && save_release < 1826)
    if ((curfractalspecific->calctype == StandardFractal &&
        (curfractalspecific->flags & BAILTEST) == 0) ||
        (fractype==FORMULA || fractype==FFORMULA))
        ret = 1;
return (ret);
}

static int fix_period_bof(void)
{
int ret = 0;
 if (inside <= BOF60 && inside >= BOF61 && save_release < 1826)
    ret = 1;
return (ret);
}

/* browse code RB*/

#define MAX_WINDOWS_OPEN 450
#define BROWSE_DATA 32768l
/* 450 * sizeof(window) must be < (65535 - BROWSE_DATA) = 32767 */
/* 450 * 67 = 30150 */

  struct window {
     double xmin;       /* for fgetwindow on screen browser */
     double xmax;
     double ymin;
     double ymax;
     double x3rd;
     double y3rd;
     char name[13];/* for filename */
     int boxcount;
     int far *savebox; /* for coordinates and colors of box */
     };

#ifndef XFRACT
/* For expanded memory: */
static BYTE far *expmemorybrowse;
static int oldexppage;
static unsigned int emmhandle = 0;

/* For extended memory: */
static BYTE far *charbuf = NULL;   /* currently used only with XMM */
static unsigned int xmmhandle = 0;
#define XMMWRITELEN 8192 /* max amount kB transferred to/from extended mem at once */
#endif

/* prototypes */
static void drawindow(int, struct window far *);
static char is_visible_window( struct fractal_info * );
static void transform( struct dblcoords * );
static char paramsOK( struct fractal_info * );
static char typeOK( struct fractal_info *, struct ext_blk_3 * );
static char functionOK( struct fractal_info *, int );
static void unflip( int );
static void check_history( char *, char * );

char browsename[13]; /* name for browse file */
int coords_flipped;

#ifndef XFRACT
static void _fastcall near exp_seek(int page)   /* expanded mem seek */
{
   if (page != oldexppage || page == 0) { /* time to get a new page? */
      oldexppage = page;
      emmgetpage(page,emmhandle);
      }
   }
#endif

/* here because must be visible inside several routines */
static struct affine *cvt; 

/* fgetwindow reads all .GIF files and draws window outlines on the screen */
int fgetwindow(void)
  {
    struct affine stack_cvt;
    struct fractal_info read_info;
    struct ext_blk_2 blk_2_info;
    struct ext_blk_3 blk_3_info;
    struct ext_blk_4 blk_4_info;
    struct ext_blk_5 blk_5_info;
    time_t thistime,lastime;
    char mesg[40],newname[60],oldname[60];
    int c,i,index,done,wincount,toggle,color_of_box;
    struct window far *winlist; /* that _should_ do! */
    char drive[FILE_MAX_DRIVE];
    char dir[FILE_MAX_DIR];
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT];
    char tmpmask[FILE_MAX_PATH];
    int no_memory_left = 0;
    int vid_too_big = 0;
    int far *save_some_mem;
#ifndef XFRACT
    int success = 0;
    long longtmp, memorysize;
    unsigned int avalmempages;
    int mempages = 0;
    int subblock, currpage, currblock;
    struct XMM_Move MoveStruct;

   memorysize = MAX_WINDOWS_OPEN * 8192L; /* 450 * 8192 */
   if (sxdots + sydots > 3276)
      vid_too_big = 1;
      /* 3276 = 8196/2.5 based on memory page size */

   if ((debugflag == 620 || debugflag != 622 || debugflag != 624)
        && !vid_too_big){
      /* 622=xmm test, 620=exp test, 624=far mem test */
      /* Try Expanded Memory first */
      oldexppage = 0;
      mempages = (int)((memorysize + 16383) >> 14);
      avalmempages = emmgetfree();
      if(mempages > (int)avalmempages) mempages = avalmempages - 1;
      if ((expmemorybrowse = emmquery()) != NULL
        && (emmhandle = emmallocate(mempages)) != 0) {
         goto common_okend;
         }
   }
   if ((debugflag == 622 || (debugflag != 620 && debugflag != 624))
        && !vid_too_big) {
      /* Try Extended Memory */
      /* Must have 450 * 8192 kB present, or this fails */
      if ((charbuf = farmemalloc((long)XMMWRITELEN)) != NULL
        && xmmquery() != 0
        && (xmmhandle = xmmallocate((unsigned int)(longtmp = (memorysize+1023) >> 10))) != 0) {
         longtmp >>= 3;
         mempages = (int)longtmp;
         for (i = 0; i < XMMWRITELEN; i++)
            charbuf[i] = 0;
         MoveStruct.SourceHandle = 0;    /* Source is in conventional memory */
         MoveStruct.SourceOffset = (unsigned long)charbuf;
         MoveStruct.DestHandle   = xmmhandle;
         MoveStruct.Length       = XMMWRITELEN;
         MoveStruct.DestOffset   = 0;
         while (--longtmp >= 0) {
            if ((success = xmmmoveextended(&MoveStruct)) == 0) break;
            MoveStruct.DestOffset += XMMWRITELEN;
            }
         if (success) {
            goto common_okend;
            }
         xmmdeallocate(xmmhandle);       /* Clear the memory */
         xmmhandle = 0;                  /* Signal same */
         }
      }

common_okend:
#endif
     /* set up complex-plane-to-screen transformation */
     cvt = &stack_cvt; /* use stack */
     setup_convert_to_screen(cvt);

     /* steal an array */
     winlist = (struct window far *)MK_FP(extraseg,BROWSE_DATA);

   if (sxdots + sydots > 4096)
      vid_too_big = 2;
      /* 4096 = 10240/2.5 based on size of boxx+boxy+boxvalues */
     /* don't think this logic is needed */
     if(debugflag == 626)
     {
     coords_flipped = 0;
     if (sxmin > sxmax && symin > symax) /* flip x and y */
        coords_flipped = 1;
     else if (sxmin > sxmax) /* flip x */
        coords_flipped = 2;
     else if (symin > symax) /* flip y */
        coords_flipped = 3;
     unflip(coords_flipped); /* put coords in correct order */
     }
     find_special_colors();
     color_of_box = color_medium;
     rescan:  /* entry for changed browse parms */
     time(&lastime);
     toggle = 0;
     wincount = 0;
     no_sub_images = FALSE;
     splitpath(readname,drive,dir,NULL,NULL);
     splitpath(browsemask,NULL,NULL,fname,ext);
     makepath(tmpmask,drive,dir,fname,ext);
     if ((save_some_mem = farmemalloc(10240))==NULL) /* steal 10K! */
        no_memory_left = 1; /* to display message, else OOL */
     done=(vid_too_big==2) || fr_findfirst(tmpmask);
                                   /* draw all visible windows */
     while (!done)
     {
       if(keypressed())
       {
          getakey();
          break;
       }
       splitpath(DTA.filename,NULL,NULL,fname,ext);
       makepath(tmpmask,drive,dir,fname,ext);
       if( !find_fractal_info(tmpmask,&read_info,&blk_2_info,
                              &blk_3_info,&blk_4_info,&blk_5_info) &&
           (typeOK(&read_info,&blk_3_info) || !brwschecktype) &&
           (paramsOK(&read_info) || !brwscheckparms) &&
           stricmp(browsename,DTA.filename) &&
           blk_5_info.got_data != 1 &&
           is_visible_window(&read_info)
         )
         {
           winlist[wincount].xmin = read_info.xmin ;
           winlist[wincount].xmax = read_info.xmax ;
           winlist[wincount].ymin = read_info.ymin ;
           winlist[wincount].ymax = read_info.ymax ;
           winlist[wincount].x3rd = read_info.x3rd ;
           winlist[wincount].y3rd = read_info.y3rd ;
           far_strcpy(winlist[wincount].name,DTA.filename);
           drawindow(color_of_box,&winlist[wincount]);
           boxcount <<= 1; /*boxcount*2;*/ /* double for byte count */
           winlist[wincount].boxcount = boxcount;
#ifndef XFRACT
           if (emmhandle != 0){  /* Expanded memory browse? */
              currpage = wincount >> 1;
              currblock = wincount % 2;
              if (currpage >= mempages) {
                 boxcount >>= 1; /* can't allocate any more */
                 clearbox(); /* clean up the mess */
                 no_memory_left = 1;
              } else {
                 exp_seek(currpage);
                 if (currblock == 0) /* clear the memory as we go, for speed */
                     emmclearpage(currpage, emmhandle);
                 subblock = currblock << 13; /* * 8192, 2 subblocks per page */
                 far_memcpy(expmemorybrowse+subblock,boxx,boxcount);
                 far_memcpy(expmemorybrowse+subblock+boxcount,boxy,boxcount);
                 far_memcpy(expmemorybrowse+subblock+(boxcount<<1),boxvalues,boxcount>>1);
                 wincount++;
              }
              exp_seek(0); /* flush the last page out of the page frame */
           }
           else if (xmmhandle != 0){     /* Extended memory browse? */
              if (wincount >= mempages) {
                 boxcount >>= 1; /* can't allocate any more */
                 clearbox(); /* clean up the mess */
                 no_memory_left = 1;
              } else {
                 far_memcpy(charbuf,boxx,boxcount);
                 far_memcpy(charbuf+boxcount,boxy,boxcount);
                 far_memcpy(charbuf+(boxcount<<1),boxvalues,boxcount>>1);
                 MoveStruct.Length = XMMWRITELEN;
                 MoveStruct.SourceHandle = 0; /* Source is conventional memory */
                 MoveStruct.SourceOffset = (unsigned long)charbuf;
                 MoveStruct.DestHandle = xmmhandle;
                 MoveStruct.DestOffset = (unsigned long)(wincount) * XMMWRITELEN;
                 xmmmoveextended(&MoveStruct);
                 wincount++;
              }
           }
           else
#endif
                if ((vid_too_big==2) || (winlist[wincount].savebox = farmemalloc(5*boxcount))==NULL){
              boxcount >>= 1; /* can't allocate any more */
              clearbox(); /* clean up the mess */
              no_memory_left = 1;
           }
           else {
              far_memcpy(winlist[wincount].savebox,boxx,boxcount);
              far_memcpy(winlist[wincount].savebox+boxcount,boxy,boxcount);
              far_memcpy(winlist[wincount].savebox+(boxcount<<1),boxvalues,boxcount>>1);
              wincount++;
           }
         }

        if(blk_2_info.got_data == 1) /* Clean up any far memory allocated */
           farmemfree(blk_2_info.resume_data);
        if(blk_4_info.got_data == 1) /* Clean up any far memory allocated */
           farmemfree(blk_4_info.range_data);
        if(blk_5_info.got_data == 1) /* Clean up any far memory allocated */
           farmemfree(blk_5_info.apm_data);

        done=(fr_findnext() || wincount >= MAX_WINDOWS_OPEN || no_memory_left);
      }
      farmemfree(save_some_mem); /* release memory for messages, yuk! */
      if (no_memory_left)
      {
         static FCODE msg[] = {"Sorry...out of memory,     displayed."};
       msg[25] = (char)(wincount % 10 + (int)'0');
       msg[24] = (char)((wincount % 100) / 10 + (int)'0');
       msg[23] = (char)((wincount % 1000) / 100 + (int)'0');
       texttempmsg(msg);/* doesn't work if NO far memory available, go figure */
      }
      if (wincount >= MAX_WINDOWS_OPEN)
      { /* hard code message at MAX_WINDOWS_OPEN = 450 */
         static FCODE msg[] = {"Sorry...no more space, 450 displayed."};
       texttempmsg(msg);
      }
      if (vid_too_big==1)
      {
         static FCODE msg[] = {"Xdots + Ydots > 3276, using far mem."};
       texttempmsg(msg);
      }
      if (vid_too_big==2)
      {
         static FCODE msg[] = {"Xdots + Ydots > 4096."};
       texttempmsg(msg);
      }
 c=0;
 if (wincount)
 {
      buzzer(0); /*let user know we've finished */
      index=0;done = 0;
      showtempmsg(winlist[index].name);
      while ( !done)  /* on exit done = 1 for quick exit,
                                 done = 2 for erase boxes and  exit
                                 done = 3 for rescan
                                 done = 4 for set boxes and exit to save image */
      {
        while (!keypressed())
        {
          time(&thistime);
          if (difftime(thistime,lastime) > .2 ) {
          lastime=thistime;
          toggle = 1- toggle;
          }
          if (toggle)
            drawindow(color_bright,&winlist[index]);   /* flash current window */
          else
            drawindow(color_dark,&winlist[index]);
        }

      c=getakey();
      switch (c) {
         case RIGHT_ARROW:
         case LEFT_ARROW:
         case DOWN_ARROW:
         case UP_ARROW:
           cleartempmsg();
           drawindow(color_of_box,&winlist[index]);/* dim last window */
           if (c==RIGHT_ARROW || c== UP_ARROW) {
              index++;                     /* shift attention to next window */
              if (index >= wincount) index=0;
           }
           else {
             index -- ;
             if ( index <0 )  index = wincount -1 ;
           }
           showtempmsg(winlist[index].name);
           break;

        case CTL_INSERT:
          color_of_box += key_count(CTL_INSERT);
          for (i=0 ; i < wincount ; i++)
              drawindow(color_of_box,&winlist[i]);
          break;

        case CTL_DEL:
          color_of_box -= key_count(CTL_DEL);
          for (i=0 ; i < wincount ; i++)
              drawindow(color_of_box,&winlist[i]);
          break;

        case ENTER:
        case ENTER_2:   /* this file please */
          far_strcpy(browsename,winlist[index].name);
          done = 1;
          break;

        case ESC:
        case 'l':
        case 'L':
          autobrowse = FALSE;
          done = 2;
          break;

        case 'D': /* delete file */
          cleartempmsg();
          strcpy(mesg,"");
          strcat(mesg,"Delete ");
          far_strcat(mesg,winlist[index].name);
          strcat(mesg,"? (Y/N)");
          showtempmsg(mesg);
          while (!keypressed()) ;
          cleartempmsg();
          c = getakey();
          if ( c == 'Y' && doublecaution ) {
           static FCODE msg[] = {"ARE YOU SURE???? (Y/N)"};
           texttempmsg(msg);
            if ( getakey() != 'Y') c = 'N';
          }
          if ( c == 'Y' ) {
          splitpath(readname,drive,dir,NULL,NULL);
          splitpath(winlist[index].name,NULL,NULL,fname,ext);
          makepath(tmpmask,drive,dir,fname,ext);
          if ( !unlink(tmpmask)) {
          /* do a rescan */
          done = 3;
            far_strcpy(oldname,winlist[index].name);
            tmpmask[0] = '\0';
            check_history(oldname,tmpmask);
            break;
            }
          else if( errno == EACCES ) {
              static FCODE msg[] = {"Sorry...it's a read only file, can't del"};
              texttempmsg(msg);
              showtempmsg(winlist[index].name);
              break;
              }
          }
          {
          static FCODE msg[] = {"file not deleted (phew!)"};
          texttempmsg(msg);
          }
          showtempmsg(winlist[index].name);
          break;

        case 'R':
         cleartempmsg();
         stackscreen();
         newname[0] = 0;
         strcpy(mesg,"");
         {
         static FCODE msg[] = {"Enter the new filename for "};
         far_strcat((char far *)mesg,msg);
         }
         splitpath(readname,drive,dir,NULL,NULL);
         splitpath(winlist[index].name,NULL,NULL,fname,ext);
         makepath(tmpmask,drive,dir,fname,ext);
         strcpy(newname,tmpmask);
         strcat(mesg,tmpmask);
         i = field_prompt(0,mesg,NULL,newname,60,NULL);
         unstackscreen();
         if( i != -1)
          if (!rename(tmpmask,newname))
            if (errno == EACCES)
            {
               static FCODE msg[] = {"sorry....can't rename"};
                texttempmsg(msg);
            }
          else {
           splitpath(newname,NULL,NULL,fname,ext);
           makepath(tmpmask,NULL,NULL,fname,ext);
           far_strcpy(oldname,winlist[index].name);
           check_history(oldname,tmpmask);
           far_strcpy(winlist[index].name,tmpmask);
           }
         showtempmsg(winlist[index].name);
         break;

        case 2: /* ctrl B */
          cleartempmsg();
          stackscreen();
          done = abs(get_browse_params());
          unstackscreen();
          showtempmsg(winlist[index].name);
          break;

        case 's': /* save image with boxes */
          autobrowse = FALSE;
          drawindow(color_of_box,&winlist[index]); /* current window white */
          done = 4;
          break;

        case '\\': /*back out to last image */
          done = 2;
          break;

        default:
          break;
     } /*switch */
    } /*while*/

    /* now clean up memory (and the screen if necessary) */
    cleartempmsg();
    for (index=wincount-1;index>=0;index--){ /* don't need index, reuse it */
       if (done >= 1 && done < 4 && (winlist[index].boxcount > 0)) {
          boxcount = winlist[index].boxcount;
#ifndef XFRACT
           if (emmhandle != 0){  /* Expanded memory browse? */
              exp_seek(index >> 1);
              subblock = (index % 2) * 8192; /* 2 subblocks per page */
              far_memcpy(boxx,expmemorybrowse+subblock,boxcount);
              far_memcpy(boxy,expmemorybrowse+subblock+boxcount,boxcount);
              far_memcpy(boxvalues,expmemorybrowse+subblock+(boxcount<<1),boxcount>>1);
           }
           else if (xmmhandle != 0){     /* Extended memory browse? */
              MoveStruct.Length       = XMMWRITELEN;
              MoveStruct.SourceHandle = xmmhandle; /* Source is in extended memory */
              MoveStruct.SourceOffset = (unsigned long)index * XMMWRITELEN;
              MoveStruct.DestHandle   = 0;
              MoveStruct.DestOffset   = (unsigned long)charbuf;
              xmmmoveextended(&MoveStruct);
              far_memcpy(boxx,charbuf,boxcount);
              far_memcpy(boxy,charbuf+boxcount,boxcount);
              far_memcpy(boxvalues,charbuf+(boxcount<<1),boxcount>>1);
           }
           else
#endif
                {
              far_memcpy(boxx,winlist[index].savebox,boxcount);
              far_memcpy(boxy,winlist[index].savebox+boxcount,boxcount);
              far_memcpy(boxvalues,winlist[index].savebox+(boxcount<<1),boxcount>>1);
              farmemfree(winlist[index].savebox);
           }
          boxcount >>= 1;
          clearbox();
       }
    }
    if (done == 3) {
       no_memory_left = 0; /* reset the flag */
       goto rescan; /* hey everybody I just used the g word! */
    }
 }/*if*/
 else {
   static FCODE msg[] = {"sorry.. I can't find anything"};
   buzzer(1); /*no suitable files in directory! */
   texttempmsg(msg);
   no_sub_images = TRUE;
 }
#ifndef XFRACT
 if (charbuf != NULL)
    farmemfree((void far *)charbuf);
 if (emmhandle != 0)     /* Expanded memory browse? */
    emmdeallocate(emmhandle);
 if (xmmhandle != 0)     /* Extended memory browse? */
    xmmdeallocate(xmmhandle);
 emmhandle = xmmhandle = 0;
 charbuf   = NULL;
#endif
 if(debugflag == 626)
    unflip(coords_flipped);
 return(c);
}

static void drawindow(int colour,struct window far *info)
{
   int cross_size;
   unsigned int temp;
   struct dblcoords tl,bl,tr,br;
   struct coords itl,ibl,itr,ibr;
   tl.x=info->xmin;
   tl.y=info->ymax;
   tr.x=(info->xmax)-(info->x3rd-info->xmin);
   tr.y=(info->ymax)+(info->ymin-info->y3rd);
   bl.x=info->x3rd;
   bl.y=info->y3rd;
   br.x=info->xmax;
   br.y=info->ymin;
   /* tranform maps real plane co-ords onto the current screen view
     see below */
   transform(&tl);itl.x=(int)tl.x;itl.y=(int)tl.y;
   transform(&tr);itr.x=(int)tr.x;itr.y=(int)tr.y;
   transform(&bl);ibl.x=(int)bl.x;ibl.y=(int)bl.y;
   transform(&br);ibr.x=(int)br.x;ibr.y=(int)br.y;
   boxcolor=colour;
   boxcount = 0;
   temp = (unsigned int)sqrt(sqr(br.x - tl.x) + sqr(tl.y - br.y));
    if (temp >= (unsigned int)minbox) {
    /* big enough on screen to show up as a box so draw it */
    /* corner pixels */
     addbox(itl);
     addbox(itr);
     addbox(ibl);
     addbox(ibr);
     drawlines(itl,itr,ibl.x-itl.x,ibl.y-itl.y); /* top & bottom lines */
     drawlines(itl,ibl,itr.x-itl.x,itr.y-itl.y); /* left & right lines */
     dispbox();
    }
    else { /* draw crosshairs */
    cross_size = ydots / 45;
    if (cross_size < 2) cross_size = 2;
    itr.x = itl.x - cross_size;
    itr.y = itl.y;
    ibl.y = itl.y - cross_size;
    ibl.x = itl.x;
    drawlines(itl,itr,ibl.x-itr.x,0); /* top & bottom lines */
    drawlines(itl,ibl,0,itr.y-ibl.y); /* left & right lines */
    dispbox();
   }
}

/* maps points onto view screen*/
static void transform(struct dblcoords *point)
{
  double tmp_pt_x;
  tmp_pt_x = cvt->a * point->x + cvt->b * point->y + cvt->e;
  point->y = cvt->c * point->x + cvt->d * point->y + cvt->f;
  point->x = tmp_pt_x;
}

static char is_visible_window( struct fractal_info *info )
{
 struct dblcoords tl,tr,bl,br;
 int cornercount;
 double toobig;
 toobig = sqrt(sqr((double)sxdots)+sqr((double)sydots)) * 1.5;
  /* arbitrary value... stops browser zooming out too far */
 cornercount=0;
 tl.x=info->xmin;
 tl.y=info->ymax;
 tr.x=(info->xmax)-(info->x3rd-info->xmin);
 tr.y=(info->ymax)+(info->ymin-info->y3rd);
 bl.x=info->x3rd;
 bl.y=info->y3rd;
 br.x=info->xmax;
 br.y=info->ymin;

 transform(&tl);
 transform(&bl);  /* transform the corners to screen space */
 transform(&tr);
 transform(&br);
   if (sqrt(sqr(tr.x-bl.x)+sqr(tr.y-bl.y)) < toosmall ) return(FALSE);
 /* reject anything too small onscreen */
   if (sqrt(sqr(tr.x-bl.x)+sqr(tr.y-bl.y)) > toobig   ) return(FALSE);
 /* or too big... */
 /* now see how many corners are on the screen, accept if one or more */
 if ( tl.x >=(0-sxoffs) && tl.x <= (sxdots-sxoffs) && tl.y >=(0-syoffs) && tl.y<= (sydots-syoffs) ) cornercount ++;
 if ( bl.x >=(0-sxoffs) && bl.x <= (sxdots-sxoffs) && bl.y >=(0-syoffs) && bl.y<= (sydots-syoffs) ) cornercount ++;
 if ( tr.x >=(0-sxoffs) && tr.x <= (sxdots-sxoffs) && tr.y >=(0-syoffs) && tr.y<= (sydots-syoffs) ) cornercount ++;
 if ( br.x >=(0-sxoffs) && br.x <= (sxdots-sxoffs) && br.y >=(0-syoffs) && br.y<= (sydots-syoffs) ) cornercount ++;

 if (cornercount >=1 ) return( TRUE );
    else return( FALSE );
 }

static char paramsOK( struct fractal_info *info )
{
double tmpparm3, tmpparm4;
#define MINDIF 0.001

   if( info->version > 6) {
     tmpparm3 = info->dparm3;
     tmpparm4 = info->dparm4;
   }
   else{
     tmpparm3 = info->parm3;
     roundfloatd(&tmpparm3);
     tmpparm4 = info->parm4;
     roundfloatd(&tmpparm4);
   }
   if( fabs(info->creal - param[0]) < MINDIF &&
       fabs(info->cimag - param[1]) < MINDIF &&
       fabs(tmpparm3 - param[2]) < MINDIF &&
       fabs(tmpparm4 - param[3]) < MINDIF &&
       info->invert[0] - inversion[0] < MINDIF)
      return(1); /* parameters are in range */
   else
      return(0);
}

static char functionOK( struct fractal_info *info, int numfn)
{
 int i, mzmatch;
   mzmatch = 0;
   for(i=0; i<numfn; i++){
     if( info->trigndx[i] != trigndx[i] )
        mzmatch++;
   }
   if(mzmatch > 0)
     return(0);
   else
     return(1); /* they all match */
}

static char typeOK( struct fractal_info *info, struct ext_blk_3 *blk_3_info )
{
 int numfn;
   if( (fractype == FORMULA || fractype == FFORMULA) &&
     (info->fractal_type == FORMULA || info->fractal_type == FFORMULA) )
   {
       if( !stricmp(blk_3_info->form_name,FormName) )
       {
         numfn = maxfn;
         if (numfn>0)
           return(functionOK(info, numfn));
         else
           return(1); /* match up formula names with no functions */
       }
       else
         return(0); /* two formulas but names don't match */
   }
   else if(info->fractal_type == fractype ||
           info->fractal_type == curfractalspecific->tofloat)
   {
     numfn = (curfractalspecific->flags >> 6) & 7;
     if (numfn>0)
       return(functionOK(info, numfn));
     else
       return(1); /* match types with no functions */
   }
   else
       return(0); /* no match */
}

static void unflip ( int axis )
{
 double txmax, txmin, tymax, tymin, tx3rd, ty3rd;

   txmax = xxmax;
   tymax = yymax;
   txmin = xxmin;
   tymin = yymin;
   tx3rd = xx3rd;
   ty3rd = yy3rd;

   switch(axis)
   {
   case 0:            /* no flip needed, exit */
      break;
   case 1:            /* reverse X and Y axis */
      xxmin = sxmin = txmax;
      yymax = symax = tymin;
      xxmax = sxmax = txmin;
      yymin = symin = tymax;
      xx3rd = sx3rd = txmax + txmin - tx3rd;
      yy3rd = sy3rd = tymax + tymin - ty3rd;
      break;
   case 2:            /* reverse X-axis */
      xxmin = sxmin = txmax + txmin - tx3rd;
      yymax = symax = tymax + tymin - ty3rd;
      xxmax = sxmax = tx3rd;
      yymin = symin = ty3rd;
      xx3rd = sx3rd = txmax;
      yy3rd = sy3rd = tymin;
      break;
   case 3:            /* reverse Y-axis */
      xxmin = sxmin = tx3rd;
      yymax = symax = ty3rd;
      xxmax = sxmax = txmax + txmin - tx3rd;
      yymin = symin = tymax + tymin - ty3rd;
      xx3rd = sx3rd = txmin;
      yy3rd = sy3rd = tymax;
      break;
   }
}

static void check_history ( char *oldname, char *newname )
{
int i;

/* file_name_stack[] is maintained in framain2.c.  It is the history */
/*  file for the browser and holds a maximum of 16 images.  The history */
/*  file needs to be adjusted if the rename or delete functions of the */
/*  browser are used. */
/* name_stack_ptr is also maintained in framain2.c.  It is the index into */
/*  file_name_stack[]. */

   for (i=0;i<name_stack_ptr;i++) {
      if (stricmp(file_name_stack[i],oldname) == 0) /* we have a match */
         strcpy(file_name_stack[i],newname);    /* insert the new name */
   }
}

