/*
	config.c - FRACTINT code relative to video configuration
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __TURBOC__
#include <dir.h>
#endif

#include "fractint.h"
#include "fractype.h"
#include "targa_lc.h"
int file_type = -1;	/* 0=GIF, 1=Tim's pot (may become Targa) TW 7/20/89 */

int filetype;
extern char far *resume_info;	/* pointer to resume info if allocated */
extern int  resume_len; 	/* length of resume info */
extern char FormName[40];	/* formula name */

find_fractal_info(gif_file,info)
char *gif_file;
struct fractal_info *info;
{
   FILE *fp;
   unsigned char gifstart[18];
   int rows, cols, colors;
   char temp1[81];
   int scan_extend, block_type, block_len;
   char far *resume_load;

   strcpy(temp1,gif_file);
   if (strchr(temp1,'.') == NULL) {
      strcat(temp1,DEFAULTFRACTALTYPE);
      if ((fp = fopen(temp1,"rb")) != NULL) {
	 fclose(fp);
	 }
      else {
	 strcpy(temp1,gif_file);
	 strcat(temp1,ALTERNATEFRACTALTYPE);
	 }
      }

   if((fp = fopen(temp1,"rb"))==NULL)
   {
      *gif_file = 0; /* failed ... zap filename */
      return(-1);
   }
   fread(gifstart,18,1,fp);
   if (strncmp(gifstart,"GIF",3)==0)
      filetype = 0; /* GIF */
   else  if (strncmp(gifstart,"TIW16BIT",8)==0)
      filetype = 1; /* TIW file type - change to Targa */
   else if(fread(info,sizeof(struct fractal_info),1,fp)==1 &&
		 strncmp(info->info_id,"Fractal",8)==0)
      filetype = 2; /* Targa 16 */
   else
   {
      *gif_file = 0; /* failed ... zap filename */
      fclose(fp);
      return(-1);
   }
   switch(filetype)
   {
   case 0: /* GIF */
      rows = gifstart[7]*256+gifstart[6];
      cols = gifstart[9]*256+gifstart[8];
      colors = 2 << (gifstart[10] & 7);
      break;
   case 1: /* TIW */
      cols   = *((int *)(gifstart+ 8));
      rows   = *((int *)(gifstart+10));
      colors = *((int *)(gifstart+12));
      break;
   case 2: /* TARGA */
      rows = *(int *)&gifstart[O_VSIZE];
      cols = *(int *)&gifstart[O_HSIZE];
      colors = info->colors;
      if(rows != info->ydots || cols != info->xdots)
      {
	 *gif_file = 0; /* failed ... zap filename */
	 fclose(fp);
	 return(-1);
      }else
      {
	 fclose(fp);
	 return(0);
      }
      break;
   default:
      *gif_file = 0; /* failed ... zap filename */
      fclose(fp);
      return(-1);
      break;
   }

   if (resume_info != NULL) /* free the prior area if there is one */
   {
      farmemfree(resume_info);
      resume_info = NULL;
   }

   /* Format of .fra (*not* part of the GIF standard) extension blocks is:
	  1 byte    '!', extension block identifier
	  1 byte    extension block number, arbitary? we always plug 255
	  1 byte    length of id, 11 with fractint
	  n bytes   alpha id, "fractintnnn" with fractint, nnn is secondary id
	  1 word    length of block info in bytes
		    (note that before fractint vsn 14, this was just a byte)
	  x bytes   block info
	  1 byte    0, block terminator
	  1 byte    ';', GIF terminator
      To scan extension blocks, we first look in file at length of save_info
      (the main extension block) from end of file, looking for a literal known
      to be at start of our block info.  Then we scan forward a bit, in case
      the file is from an earlier fractint vsn with shorter save_info.
      If save_info is found and is from vsn>=14, it includes the total length
      of all extension blocks; we then scan them all first to last to load
      any optional ones which are present.
      Defined extension blocks:
	fractint001	header, always present
	fractint002	resume info for interrupted resumable image
	fractint003	additional formula type info
   */

   fseek(fp,(long)(-2-sizeof(FRACTAL_INFO)),SEEK_END);
   fread(info,1,sizeof(FRACTAL_INFO),fp);
   if (strcmp(INFO_ID,info->info_id))
   {  /* didn't work 1st try, maybe an older vsn, maybe junk at eof, scan: */
      int offset,i;
      char tmpbuf[110];
      offset = 80; /* don't even check last 80 bytes of file for id */
      while (offset < sizeof(FRACTAL_INFO)+512) /* allow 512 garbage at eof */
      {
	 offset += 100; /* go back 100 bytes at a time */
	 fseek(fp,(long)(0-offset),SEEK_END);
	 fread(tmpbuf,1,110,fp); /* read 10 extra for string compare */
	 for (i = 0; i < 100; ++i)
	    if (!strcmp(INFO_ID,&tmpbuf[i])) /* found header? */
	    {
	       fseek(fp,(long)(i-offset),SEEK_END);
	       fread(info,1,sizeof(FRACTAL_INFO),fp);
	       offset = 10000; /* force exit from outer loop */
	       break;
	    }
      }
   }

   if (!strcmp(INFO_ID,info->info_id)) /* we found and read info */
   {
      if (info->version >= 4) /* load any additional extension blocks */
      {
	 fseek(fp,1L-info->tot_extend_len,SEEK_CUR);
	 scan_extend = 1;
	 while (scan_extend)
	 {
	    if (fgetc(fp) != '!' /* if not what we expect just give up */
	      || fread(temp1,1,13,fp) != 13
	      || strncmp(&temp1[2],"fractint",8))
	       break;
	    temp1[13] = 0;
	    block_type = atoi(&temp1[10]); /* e.g. "fractint002" */
	    block_len = (unsigned char)fgetc(fp);
	    switch (block_type)
	    {
	       case 1: /* back to "fractint001", we're all done */
		  scan_extend = 0;
		  break;
	       case 2: /* resume info */
		  if ((resume_info = farmemalloc((long)block_len)) == NULL)
		  {
		     info->calc_status = 3; /* not resumable after all */
		     fseek(fp,(long)block_len,SEEK_CUR);
		  }
		  else
		  {
		     resume_load = resume_info;
		     resume_len = block_len;
		     while (--block_len >= 0) /* gross but it works */
			*(resume_load++) = fgetc(fp);
		  }
		  break;
	       case 3: /* formula info */
		  fread(FormName,40,1,fp);
		  /* perhaps in future add more here, check block_len for
		     backward compatibility */
		  break;
	       default:
		  fseek(fp,(long)block_len,SEEK_CUR); /* skip unrecognized block */
	    }
	    if (fgetc(fp) != 0) /* skip block trailer */
	       break;
	 }
      }
      fclose(fp);
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
   info->xdots = rows;
   info->ydots = cols;
   info->colors = colors;
   info->version = 0;

   /* zero means we won */
   fclose(fp);
   return(0);
}

readconfig()		/* search for, read, decode fractint.cfg file */
{
char tempstring[101];
FILE *cfgfile;
int count, i, j, ax, bx, cx, dx, dotmode, xdots, ydots, colors, commas[10];
#ifdef __TURBOC__
strcpy(tempstring,searchpath("fractint.cfg"));
#else
_searchenv("fractint.cfg","PATH",tempstring);
#endif
if (tempstring[0] == 0)
	return(-1);				/* can't find the file  */
if (strcmp(&tempstring[2],"\\\\fractint.cfg") == 0)     /* stupid answer! */
	strcpy(&tempstring[2],"\\fractint.cfg");
if ((cfgfile = fopen(tempstring,"r")) == NULL)
	return(-1);				/* ?? can't open file   */

count = 0;					/* build a new videomode file */
while (feof(cfgfile) == 0 && count < MAXVIDEOMODES) {	/* scan through strings */
	if (!fgets(tempstring, 100, cfgfile)) break;
	tempstring[strlen(tempstring)-1] = 0;
	if (tempstring[0] <= 32) continue;	/* comment line 	*/
	j = 9;
	for (i = 0; i <= j; i++) commas[i] = 0;
	for (i = strlen(tempstring); i >= 0 && j >= 0; i--)  /* check for commas */
		if (tempstring[i] == ',') {
			tempstring[i] = 0;
			commas[--j] = i+1;
			}
	sscanf(&tempstring[commas[0]],"%x",&ax);
	sscanf(&tempstring[commas[1]],"%x",&bx);
	sscanf(&tempstring[commas[2]],"%x",&cx);
	sscanf(&tempstring[commas[3]],"%x",&dx);
	dotmode     = atoi(&tempstring[commas[4]]);
	xdots	    = atoi(&tempstring[commas[5]]);
	ydots	    = atoi(&tempstring[commas[6]]);
	colors	    = atoi(&tempstring[commas[7]]);
	if (	i >= 0 || j != 0 ||
		dotmode < 0 || dotmode > 30 ||
		xdots < 160 || xdots > 2048 ||
		ydots < 160 || ydots > 2048 ||
		(colors != 2 && colors != 4 && colors != 16 && colors != 256)
		) {
		buzzer(2);
		printf("\n\n There is a bad entry in fractint.cfg\n\n");
		printf(" ==> %s \n", tempstring);
		exit(-1);
		}
	tempstring[commas[8]+25] = 0;
	if (commas[0] >= 25) tempstring[25] = 0;
	strcpy(videoentry.name,    tempstring);
	strcpy(videoentry.comment, &tempstring[commas[8]]);
	if (tempstring[commas[8]] == ' ')
		strcpy(videoentry.comment, &tempstring[commas[8]+1]);
	videoentry.videomodeax =   ax;
	videoentry.videomodebx =   bx;
	videoentry.videomodecx =   cx;
	videoentry.videomodedx =   dx;
	videoentry.dotmode     =   dotmode;
	videoentry.xdots       =   xdots;
	videoentry.ydots       =   ydots;
	videoentry.colors      =   colors;
	tovideotable(count);
	count++;
	}

if (count > 0) strcpy(videoentry.name, "END");
videoentry.colors = 0;
tovideotable(count);
fclose(cfgfile);
if (count <= 0) return(-1);
return(0);					/* successful return	*/
}


makeconfig()		/* routine to build a FRACTINT.CFG file */
{
char tempstring[101];
FILE *cfgfile;
int count;

#ifdef __TURBOC__
strcpy(tempstring,searchpath("fractint.cfg"));
#else
_searchenv("fractint.cfg","PATH",tempstring);
#endif

if (tempstring[0] != 0) {
	buzzer(2);
	printf("\n There is a FRACTINT.CFG file already located in the PATH.\n\n");
	printf(" I won't make another one until after you manually remove\n");
	printf(" the old one.  Safety first!\n\n");
	exit(-1);
	}

if ((cfgfile = fopen("fractint.cfg","w")) == NULL)
	exit(-1);				/* ?? can't open file   */
fprintf(cfgfile,"   Full FRACTINT.CFG File, built by a 'fractint batch=config' command\n\n");
fprintf(cfgfile," name of adapter/mode    | AX | BX | CX | DX |mode|  x |  y |clrs| comments\n");
fprintf(cfgfile," =============================================================================\n\n");
for (count = 0; count < maxvideomode; count++) {	/* write the entries */
	fromvideotable(count);
#ifdef __TURBOC__
	fprintf(cfgfile,"%-25.25s,%4x,%4x,%4x,%4x,%4d,%4d,%4d,%4d, %-25.25s\n",
#else
	fprintf(cfgfile,"%-25s,%4x,%4x,%4x,%4x,%4d,%4d,%4d,%4d, %-25s\n",
#endif
		videoentry.name,
		videoentry.videomodeax,
		videoentry.videomodebx,
		videoentry.videomodecx,
		videoentry.videomodedx,
		videoentry.dotmode,
		videoentry.xdots,
		videoentry.ydots,
		videoentry.colors,
		videoentry.comment);
	}
fclose(cfgfile);
exit(0);
}

/* A TRULY persistent routine doing it's DARNDEST to find a good mode */

getGIFmode(v1)
struct fractal_info *v1;
{
   int i;

   /* try EXACT match with a configured mode */
	for(i=0;i<maxvideomode;i++) {
	   fromvideotable(i);
	    if(v1->videomodeax == videoentry.videomodeax &&
	   v1->videomodebx == videoentry.videomodebx &&
	   v1->videomodecx == videoentry.videomodecx &&
	   v1->videomodedx == videoentry.videomodedx &&
	   v1->dotmode	   == videoentry.dotmode     &&
	   v1->xdots	   == videoentry.xdots	     &&
	   v1->ydots	   == videoentry.ydots	     &&
	   v1->colors	   == videoentry.colors      )
		break;
	}
    if(i<maxvideomode) /* gotit! */
       return(i);

   /* try to match xdots, ydots, and colors to a configured mode */
	for(i=0;i<maxvideomode;i++) {
	   fromvideotable(i);
	    if(v1->xdots       == videoentry.xdots	 &&
	   v1->ydots	   == videoentry.ydots	     &&
	   v1->colors	   == videoentry.colors      )
		break;
	}
    if(i<maxvideomode) /* gotit! */
       return(i);

    /* ABSOLUTELY the LAST gasp! ANY mode with right xdot and ydot??? */
	for(i=0;i<maxvideomode;i++) {
	   fromvideotable(i);
	    if(v1->xdots       == videoentry.xdots	 &&
	   v1->ydots	   == videoentry.ydots	     )
		break;
	}
    if(i<maxvideomode) /* gotit! */
       return(i);

    /* give up! */
       return(maxvideomode);
}
