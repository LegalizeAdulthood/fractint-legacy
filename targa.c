/** targa.c 


**/


#ifdef __TURBOC__
#	pragma	warn -par
#endif


#include	<stdio.h>
#include	<stdlib.h>
#include	<dos.h>
#include	"targa.h"

#ifndef true
#	define	true	1
#	define	false	0
#endif

/*************  ****************/

extern unsigned int _dataseg;

extern char far runningontarga[];

extern void	helptitle();
extern void	helpmessage(unsigned char far *);
extern int	LoadColorMap( void );

/*************  ****************/

typedef unsigned char uchar;

/*************  ****************/

void	WriteTGA( int x, int y, int index );
int		ReadTGA ( int x, int y );
void	LineTGA ( uchar far * lineData, int y, int cnt );
void	EndTGA  ( void );
void	StartTGA( int xDim, int yDim );

/*************  ****************/

int				xorTARGA;
int				targaType;
unsigned	tga16[ 256 ];
long			tga32[ 256 ];

/*************  ****************/

static int	xdots;
static int	ydots;
static int	initialized;

/*************  ****************/

void			(*DoPixel) ( int x, int y, int index );
void			(*PutPixel)( int x, int y, int index );
void			(*PutLine) ( uchar far * lineData, int y, int cnt );
unsigned	(*GetPixel)( int x, int y );

/**************************************************************************/

void
TErase( color )
int color;
{
int cnt, max, seg;

	max = targa->MaxBanks;
	seg = targa->memloc;
	for( cnt = 0; cnt < max; cnt += 2 ) {
		outp( DESTREG, cnt );
		outp( SRCREG,  cnt + 1 );
		erasesegment(seg,color);		/** general.asm ?? **/
	}
}


/**************************************************************************/

unsigned
Row16Calculate( unsigned line, unsigned x1 )
{
unsigned page, offset, pixel;

	page = (line >> 5) & 15;
	outp( DESTREG, page );
	offset = ((line - (page << 5)) << 10) | (x1 << 1);	/* calc the pixel offset */
	return( offset );
}


/**************************************************************************/

void
PutPix16( int x, int y, int index )
{
unsigned far * ip;

	/**************/
#ifdef __TURBOC__
	ip = MK_FP( MEMSEG, Row16Calculate( y, x ) );
#else
	FP_SEG(ip) = MEMSEG;
	FP_OFF(ip) = Row16Calculate( y, x );
#endif
	if( ! xorTARGA )
		*ip = tga16[index];
	else
		*ip = *ip ^ 0x7fff;
}


/**************************************************************************/

unsigned
GetPix16( int x, int y )
{
unsigned	pixel, index;
unsigned far * ip;
static unsigned last;
	/**************/
#ifdef __TURBOC__
	ip = MK_FP( MEMSEG, Row16Calculate( y, x ) );
#else
	FP_SEG(ip) = MEMSEG;
	FP_OFF(ip) = Row16Calculate( y, x );
#endif
	pixel = *ip & 0x7FFF;
	if( pixel == tga16[last] ) return( last );
	for( index = 0; index < 256; index++ )
		if( pixel == tga16[index] ) {
			last = index;
			return( index );
		}
	return( 0 );
}


/**************************************************************************/

unsigned
Row32Calculate( unsigned line, unsigned x1 )
{
int page, offset, pixel;

	page = (line >> 4) & 31;
	outp( DESTREG, page );
	offset = ((line - (page << 4)) << 11) | (x1 << 2);	/* calc the pixel offset */
	return( offset );
}


/**************************************************************************/

void
PutPix32( int x, int y, int index )
{
long far * lp;
	/**************/
#ifdef __TURBOC__
	lp = MK_FP( MEMSEG, Row32Calculate( y, x ) );
#else
	FP_SEG(lp) = MEMSEG;
	FP_OFF(lp) = Row32Calculate( y, x );
#endif

/****
printf( "%lx\n", tga32[index] );
****/

	if( ! xorTARGA )
		*lp = tga32[index];
	else
		*lp = *lp ^ 0x00FFFFFF;
}


/**************************************************************************/

unsigned
GetPix32( int x, int y )
{
int		index;
long	pixel;
long	far * lp;
static int	last;

	/**************/
#ifdef __TURBOC__
	lp = MK_FP( MEMSEG, Row32Calculate( y, x ) );
#else
	FP_SEG(lp) = MEMSEG;
	FP_OFF(lp) = Row32Calculate( y, x );
#endif
	pixel = *lp & 0x00FFFFFF;
	if( pixel == tga32[last] ) return( last );
	for( index = 0; index < 256; index++ )
		if( pixel == tga32[index] ) {
			last = index;
			return( index );
		}
	return( 0 );
}


/**************************************************************************/

void 
DoFirstPixel( int x, int y, int index )
{
	PutPixel = DoPixel;
	TSetMode( targa->mode | 1 );
	TErase( 0 );
	(*PutPixel)( x, ydots-y, index&0xFF );
	TSetMode( targa->mode & 0xFFFE );
}


/***************************************************************************/

void
WriteTGA( int x, int y, int index )
{
	TSetMode( targa->mode | 1 );
	(*PutPixel)( x, ydots-y, index&0xFF );	/* fix origin to match EGA/VGA */
	TSetMode( targa->mode & 0xFFFE );
}


/***************************************************************************/

int
ReadTGA( int x, int y )
{
int val;
	TSetMode( targa->mode | 1 );
	val = (*GetPixel)( x, ydots-y );
	TSetMode( targa->mode & 0xFFFE );
	return( val );
}


/***************************************************************************/

void
LineTGA( uchar far * lineData, int y, int cnt )
{
/**** not yet
	TSetMode( targa->mode | 1 );
	TSetMode( targa->mode & 0xFFFE );
 	return;
****/
}


/***************************************************************************/


void
EndTGA( void )
{
	if( initialized ) {
		GraphEnd();
		initialized = 0;
	}
 	return;
}


/***************************************************************************/

void
StartTGA( int xDim, int yDim )
{
int	itemp;
int	v1, v2, v3, index;
union REGS regs;

static int onlyonce;

	/****************/
	xdots = xDim;
	ydots = yDim;

	/****************/
	if( initialized ) return;
	if( ! onlyonce ) {
		onlyonce = 1;
		atexit( EndTGA );
	}
	initialized = 1;

	/****************/
	/** first, set up so default palette will be loaded if we're on a VGA **/
	regs.x.ax = 0x1200;
	regs.x.bx = 0x0031;
	/** set mono text mode **/
	regs.x.ax = 7;	/* 'cause TARGA can live at 0xA000, we DO NOT want to */
	int86( 0x10, &regs, &regs ); /* have an EGA/VGA in graphics mode!! */
	int86( 0x10, &regs, &regs );
	helptitle();
	helpmessage(runningontarga);
	
	/****************/
	/*** look for and activate card ***/
	itemp = GraphInit( 16 );
	if( itemp == 0 ) {		/* all is ok */
		VCenterDisplay( ydots + 1 );
	 	targaType = targa->boardType;
	}
	else {	/* no targa found */
    printf( "\n\nCould not find Targa Card\n ...aborting !\n" );
		exit( 1 );
  }
	if( targaType == 16 ) {
		GetPixel = GetPix16;
		DoPixel = PutPix16;
	}
	else {
		GetPixel = GetPix32;
		DoPixel = PutPix32;
	}
	PutPixel = DoFirstPixel;	/* on first pixel --> erase */
	if( yDim == 482 ) SetOverscan( 1 );
	TSetMode( targa->mode & 0xFFFE );

	/****************/
	if( ! LoadColorMap() ) {
		printf( "\n\nColor Map Not Found\n ... aborting!\n" );
		exit( 1 );
	}

	/****************/
 	return;
}







