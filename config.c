/*
	config.c - FRACTINT code relative to video configuration
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fractint.h"

/* looks for fractal_info within MAX_LOOK bytes of end of file */

#define MAX_LOOK 512    /* equals max bytes garbage at end of file */

find_fractal_info(gif_file,info)
char *gif_file;
struct fractal_info *info;
{
   long where;
   int ct;
   FILE *fp;
   ct = 0;
 
   if (strchr(gif_file,'.') == NULL)
      strcat(gif_file,DEFAULTFRACTALTYPE);


   if((fp = fopen(gif_file,"rb"))==NULL)
   {
      *gif_file = NULL; /* failed ... zap filename */
      return(-1);
   }
   fseek(fp,-1L*(long)sizeof(FRACTAL_INFO),SEEK_END);
   do  /* keep trying to find word INFO_ID */
   {
      /* should work on the first try, but you never know */
      fread(info,sizeof(FRACTAL_INFO),1,fp);

      /* creep up from the bottom */
      fseek(fp,-1L*(long)(sizeof(FRACTAL_INFO)+ct),SEEK_END);
   }
   while(ct++ < MAX_LOOK && strcmp(INFO_ID,info->info_id));
   if(ct >= MAX_LOOK)
   {
      *gif_file = NULL; /* failed ... zap filename */
	fclose(fp);
      return(1);
   }
   else
      /* zero means we won */
	fclose(fp);
      return(0);
}

readconfig()		/* search for, read, decode fractint.cfg file */
{
char tempstring[101];
FILE *cfgfile;
int count, i, j, ax, bx, cx, dx, dotmode, xdots, ydots, colors, commas[10];

_searchenv("fractint.cfg","PATH",tempstring);
if (tempstring[0] == 0)
	return(-1);				/* can't find the file	*/
if (strcmp(&tempstring[2],"\\\\fractint.cfg") == 0)	/* stupid answer! */
	strcpy(&tempstring[2],"\\fractint.cfg");
if ((cfgfile = fopen(tempstring,"r")) == NULL)
	return(-1);				/* ?? can't open file	*/

count = 0;					/* build a new videomode file */
while (feof(cfgfile) == 0 && count < 76) {		/* scan through strings */
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
		dotmode < 0 || dotmode > 8 ||
		xdots < 200 || xdots > 2048 ||
		ydots < 200 || ydots > 2048 ||
		(colors != 2 && colors != 4 && colors != 16 && colors != 256)
		) {
		printf("\007\n\n There is a bad entry in fractint.cfg\n\n");
		printf(" ==> %s \n", tempstring);
		exit(-1);
		}
	tempstring[commas[8]+25] = 0;
	if (commas[0] >= 25) tempstring[25] = 0;
	strcpy(videomode[count].name,    tempstring);
	strcpy(videomode[count].comment, &tempstring[commas[8]]);
	if (tempstring[commas[8]] == ' ')
		strcpy(videomode[count].comment, &tempstring[commas[8]+1]);
	videomode[count].videomodeax =   ax;
	videomode[count].videomodebx =   bx;
	videomode[count].videomodecx =   cx;
	videomode[count].videomodedx =   dx;
	videomode[count].dotmode     =   dotmode;
	videomode[count].xdots       =   xdots;
	videomode[count].ydots       =   ydots;
	videomode[count].colors      =   colors;
	count++;
	}

if (count > 0) strcpy(videomode[count].name, "END");
fclose(cfgfile);
if (count <= 0) return(-1);
return(0);					/* successful return	*/
}

makeconfig()		/* routine to build a FRACTINT.CFG file */
{
char tempstring[101];
FILE *cfgfile;
int count;

_searchenv("fractint.cfg","PATH",tempstring);
if (tempstring[0] != 0) {
	printf("\007\n");
	printf(" There is a FRACTINT.CFG file already located in the PATH.\n\n");
	printf(" I won't make another one until after you manually remove\n");
	printf(" the old one.  Safety first!\n\n");
	exit(-1);
	}

if ((cfgfile = fopen("fractint.cfg","w")) == NULL)
	exit(-1);				/* ?? can't open file	*/
fprintf(cfgfile,"   Full FRACTINT.CFG File, built by a 'fractint batch=config' command\n\n");
fprintf(cfgfile," name of adapter/mode    | AX | BX | CX | DX |mode|  x |  y |clrs| comments\n");
fprintf(cfgfile," =============================================================================\n\n");
for (count = 0; count < maxvideomode; count++)	/* write the entries */
	fprintf(cfgfile,"%-25s,%4x,%4x,%4x,%4x,%4d,%4d,%4d,%4d, %-25s\n",
		videomode[count].name,
		videomode[count].videomodeax,
		videomode[count].videomodebx,
		videomode[count].videomodecx,
		videomode[count].videomodedx,
		videomode[count].dotmode,
		videomode[count].xdots,
		videomode[count].ydots,
		videomode[count].colors,
		videomode[count].comment);

fclose(cfgfile);
exit(0);
}

/* A TRULY persistent routine doing it's DARNDEST to find a good mode */

getGIFmode(v1)
struct fractal_info *v1;
{
   int i;

   /* try EXACT match with a configured mode */
	for(i=0;i<maxvideomode;i++)
   	    if(v1->videomodeax == videomode[i].videomodeax &&
      	   v1->videomodebx == videomode[i].videomodebx &&
           v1->videomodecx == videomode[i].videomodecx &&
           v1->videomodedx == videomode[i].videomodedx &&
           v1->dotmode     == videomode[i].dotmode     &&
           v1->xdots       == videomode[i].xdots       &&
           v1->ydots       == videomode[i].ydots       &&
           v1->colors      == videomode[i].colors      ) 
          	break;
    if(i<maxvideomode) /* gotit! */
       return(i);

   /* try to match xdots, ydots, and colors to a configured mode */
	for(i=0;i<maxvideomode;i++)
   	    if(v1->xdots       == videomode[i].xdots       &&
           v1->ydots       == videomode[i].ydots       &&
           v1->colors      == videomode[i].colors      )
          	break;
    if(i<maxvideomode) /* gotit! */
       return(i);

    /* ABSOLUTELY the LAST gasp! ANY mode with right xdot and ydot??? */
	for(i=0;i<maxvideomode;i++)
   	    if(v1->xdots       == videomode[i].xdots       &&
           v1->ydots       == videomode[i].ydots       )
          	break;
    if(i<maxvideomode) /* gotit! */
       return(i);
    
    /* give up! */
       return(maxvideomode);   
}
