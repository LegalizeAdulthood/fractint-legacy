/** loadmap.c **/


#include	<stdio.h>
#include	<stdlib.h>
#include	<dos.h>
#include	<string.h>

typedef unsigned char uchar;

typedef struct palett {
   uchar red;
   uchar green;
   uchar blue;
} Palettetype;

extern char		loadPalette;
extern Palettetype	dacbox[ 256 ];
extern unsigned far    *tga16;
extern long	far    *tga32;

char far *mapdacbox = NULL;

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
unsigned char	line[101];
unsigned char	temp[81];
	strcpy (temp,fn);
	if (strchr(temp,'.') == NULL) /* Did name have an extension? */
		strcat(temp,".map");  /* No? Then add .map */
	findpath( temp, line);	      /* search the dos path */
	f = fopen( line, "r" );
	if (f == NULL)
		return 1;
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
	SetTgaColors();
	return 0;
}


/***************************************************************************/

int SetColorPaletteName( char * fn )
{
	extern unsigned char far *farmemalloc(long);
	char msg[200];
	char far *saveptr;
	char *dacptr;
	int  i;
	if( ValidateLuts( fn ) != 0) {
		sprintf(msg,"Could not load color map %s",fn);
		stopmsg(0,msg);
		return 1;
		}
	if( mapdacbox == NULL && (mapdacbox = farmemalloc(768L)) == NULL) {
		stopmsg(0,"Insufficient memory for color map.");
		return 1;
		}
	saveptr = mapdacbox;
	dacptr = (char *)&dacbox[0];
	for (i = 0; i < 768; ++i)
		*(saveptr++) = *(dacptr++);
	loadPalette = 1;
	/* PB, 900829, removed atexit(RestoreMap) stuff, goodbye covers it */
	return 0;
}


/***************************************************************************/

