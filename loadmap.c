/** loadmap.c **/


#include	<stdio.h>
#include	<stdlib.h>
#include	<dos.h>

typedef unsigned char uchar;

typedef struct palett {
   uchar red;
   uchar green;
   uchar blue;
} Palettetype;

extern char					loadPalette;
extern int					targaType;
extern Palettetype	dacbox[ 256 ];
extern unsigned			tga16[ 256 ];
extern long					tga32[ 256 ];


static char * palName = "default.map";
static int		maploaded;
static Palettetype	low16[16];

/***************************************************************************/

int
CustomLut( void )
{
	return( loadPalette );
}


/***************************************************************************/

void
RestoreMap()
{
int index;
	for( index = 0; index < 16; index++ )
		dacbox[index] = low16[index];
	storedac();
}


/***************************************************************************/

void
ValidateLuts( FILE * f )
{
unsigned	r, g, b, index;
unsigned char   line[101];
	for( index = 0; index < 256; index++ ) {
		if( f != NULL && fgets(line,100,f) != NULL) {
			sscanf( line, "%d %d %d", &r, &g, &b );
			/** load global dac values **/
			dacbox[index].red   = r >> 2;	/* maps default to 8 bits */
			dacbox[index].green = g >> 2;	/* DAC wants 6 bits */
			dacbox[index].blue	= b >> 2;
		}
		else {
			f = NULL;
			r = dacbox[index].red   << 2;
			g = dacbox[index].green << 2;
			b = dacbox[index].blue  << 2;
		}

		/** load local tables **/
		tga16[index] = ((r&248)<<7) | ((g&248)<<2) | (b>>3);
		tga32[index] = ((long)r<<16) | (g<<8) | b;
/***
printf( "%08lx\n", tga32[index] );
***/
	}
}


/***************************************************************************/

int
LoadColorMap( void )
{
FILE *f;
unsigned	r, g, b, index;

	if( maploaded ) return( 1 );
	f = fopen( palName, "r" );
	if( f != NULL ) {
		/** save first 16 color map entries **/
		loaddac();
		for( index = 0; index < 16; index++ )
			low16[index] = dacbox[index];
		atexit( RestoreMap );
		ValidateLuts( f );
		fclose( f );
		maploaded = 1;
		return( 1 );
	}
	return( 0 );
}


/***************************************************************************/

void
SetColorPaletteName( char * fn )
{
	palName = fn;
	loadPalette = 0;
	if (LoadColorMap() == 0) {
		strcpy(palName,"default.map");
		printf("\007");
		}
	else
		loadPalette = 1;
}


/***************************************************************************/

