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
#include "targa_lc.h"
int file_type = -1;     /* 0=GIF, 1=Tim's pot (may become Targa) TW 7/20/89 */

/* looks for fractal_info within MAX_LOOK bytes of end of file */
#define MAX_LOOK 612            /* equals max bytes garbage at end of file */

int filetype;
find_fractal_info(gif_file,info)
char *gif_file;
struct fractal_info *info;
{
   long where;
   int ct;
   FILE *fp;
   unsigned char gifstart[18]; 
   int rows, cols, colors;
   char temp1[81];

   ct = 0;
 
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
      *gif_file = NULL; /* failed ... zap filename */
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
      *gif_file = NULL; /* failed ... zap filename */
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
         *gif_file = NULL; /* failed ... zap filename */
         fclose(fp);
         return(-1);
      }else
      {
         fclose(fp);
         return(0);
      }
      break;
   default:
      *gif_file = NULL; /* failed ... zap filename */
      fclose(fp);
      return(-1);
      break;
   }



   fseek(fp,-1L*(long)sizeof(FRACTAL_INFO),SEEK_END);
   do  /* keep trying to find word INFO_ID */
   {
       /* should work on the first try, but you never know */
       fread(info,1,sizeof(FRACTAL_INFO),fp);

       /* creep up from the bottom  - the '-101' is for old .FRA files */
       fseek(fp,-1L*(long)(sizeof(FRACTAL_INFO)-101+ct),SEEK_END);
   }
   while(ct++ < MAX_LOOK && strcmp(INFO_ID,info->info_id));

   if(ct < MAX_LOOK)
   {
      /* zero means we won */
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
	return(-1);				/* can't find the file	*/
if (strcmp(&tempstring[2],"\\\\fractint.cfg") == 0)	/* stupid answer! */
	strcpy(&tempstring[2],"\\fractint.cfg");
if ((cfgfile = fopen(tempstring,"r")) == NULL)
	return(-1);				/* ?? can't open file	*/

count = 0;					/* build a new videomode file */
while (feof(cfgfile) == 0 && count < MAXVIDEOMODES) {	/* scan through strings */
	if (!fgets(tempstring, 100, cfgfile)) break;
	tempstring[strlen(tempstring)-1] = 0;
	if (tempstring[0] <= 32) continue;	/* comment line		*/
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
	xdots       = atoi(&tempstring[commas[5]]);
	ydots       = atoi(&tempstring[commas[6]]);
	colors      = atoi(&tempstring[commas[7]]);
	if (    i >= 0 || j != 0 ||
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
	exit(-1);				/* ?? can't open file	*/
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
           v1->dotmode     == videoentry.dotmode     &&
           v1->xdots       == videoentry.xdots       &&
           v1->ydots       == videoentry.ydots       &&
           v1->colors      == videoentry.colors      ) 
          	break;
	}
    if(i<maxvideomode) /* gotit! */
       return(i);

   /* try to match xdots, ydots, and colors to a configured mode */
	for(i=0;i<maxvideomode;i++) {
	   fromvideotable(i);
   	    if(v1->xdots       == videoentry.xdots       &&
           v1->ydots       == videoentry.ydots       &&
           v1->colors      == videoentry.colors      )
          	break;
	}
    if(i<maxvideomode) /* gotit! */
       return(i);

    /* ABSOLUTELY the LAST gasp! ANY mode with right xdot and ydot??? */
	for(i=0;i<maxvideomode;i++) {
	   fromvideotable(i);
   	    if(v1->xdots       == videoentry.xdots       &&
           v1->ydots       == videoentry.ydots       )
          	break;
	}
    if(i<maxvideomode) /* gotit! */
       return(i);
    
    /* give up! */
       return(maxvideomode);   
}
