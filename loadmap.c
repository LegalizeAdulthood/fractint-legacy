/** loadmap.c **/


#include	<stdio.h>
#include	<stdlib.h>
#include	<dos.h>
#include	<string.h>
#include	"fractint.h"

typedef unsigned char uchar;

typedef struct palett {
   uchar red;
   uchar green;
   uchar blue;
} Palettetype;

extern Palettetype	dacbox[ 256 ];
extern unsigned far    *tga16;
extern long	far    *tga32;
extern char	far    *mapdacbox;
extern int		colorstate; /* comments in cmdfiles */
extern char		colorfile[];

/***************************************************************************/

void SetTgaColors() {
unsigned	r, g, b, index;
    if (tga16 != NULL)
	for( index = 0; index < 256; index++ ) {
		r = dacbox[index].red	<< 2;
		g = dacbox[index].green << 2;
		b = dacbox[index].blue	<< 2;
		tga16[index] = ((r&248)<<7) | ((g&248)<<2) | (b>>3);
		tga32[index] = ((long)r<<16) | (g<<8) | b;
	}
}

int ValidateLuts( char * fn )
{
FILE * f;
unsigned	r, g, b, index;
unsigned char	line[160];
unsigned char	temp[81];
	strcpy (temp,fn);
	if (strchr(temp,'.') == NULL) /* Did name have an extension? */
		strcat(temp,".map");  /* No? Then add .map */
	findpath( temp, line);	      /* search the dos path */
	f = fopen( line, "r" );
	if (f == NULL) {
		sprintf(line,"Could not load color map %s",fn);
		stopmsg(0,line);
		return 1;
		}
	for( index = 0; index < 256; index++ ) {
		if (fgets(line,100,f) == NULL)
			break;
		sscanf( line, "%d %d %d", &r, &g, &b );
		/** load global dac values **/
		dacbox[index].red   = r >> 2;	/* maps default to 8 bits */
		dacbox[index].green = g >> 2;	/* DAC wants 6 bits */
		dacbox[index].blue  = b >> 2;
	}
	fclose( f );
	while (index < 256)  { /* zap unset entries */
		dacbox[index].red = dacbox[index].blue = dacbox[index].green = 40;
		++index;
	}
	SetTgaColors();
	colorstate = 2;
	strcpy(colorfile,fn);
	return 0;
}


/***************************************************************************/

int SetColorPaletteName( char * fn )
{
	char msg[200];
	int  i;
	if( ValidateLuts( fn ) != 0)
		return 1;
	if( mapdacbox == NULL && (mapdacbox = farmemalloc(768L)) == NULL) {
		static char far msg[]={"Insufficient memory for color map."};
		stopmsg(0,msg);
		return 1;
		}
	far_memcpy((char far *)mapdacbox,(char far *)dacbox,768);
	/* PB, 900829, removed atexit(RestoreMap) stuff, goodbye covers it */
	return 0;
}


