/**************************************
**
** PORT.H : Miscellaneous definitions for portability.	Please add
** to this file for any new machines/compilers you may have.
**
** XFRACT file "SHARED.H" merged into PORT.H on 3/14/92 by --CWM--
*/

#ifndef PORT_H		/* If this is defined, this file has been	*/
#define PORT_H 		/* included already in this module.		*/

#ifdef XFRACT		/* XFRACT forces unix configuration! --CWM-- */
#ifdef MSDOS
#undef MSDOS
#endif
#ifdef __MSDOS__
#undef __MSDOS__
#endif
#ifndef unix
#define unix
#endif
#endif

#ifdef MSDOS		/* Microsoft C 5.1 for OS/2 and MSDOS */
	typedef unsigned char  U8;
	typedef signed char    S8;
	typedef unsigned short U16;
	typedef signed short   S16;
	typedef unsigned long  U32;
	typedef signed long    S32;
	typedef unsigned char  BYTE;
	typedef unsigned char  CHAR;
	typedef void          *VOIDPTR;
	typedef void far      *VOIDFARPTR;
	typedef void const    *VOIDCONSTPTR;

	#define CONST          const
	#define PRINTER        "/dev/prn"
	#define LOBYTEFIRST    1
	#define SLASHC         '\\'
	#define SLASH          "\\"
	#define SLASHSLASH     "\\\\"
	#define SLASHDOT       "\\."
	#define DOTSLASH       ".\\"
	#define DOTDOTSLASH    "..\\"
	#define READMODE	"rb"    /* Correct DOS text-mode        */
	#define WRITEMODE	"wb"    /* file open "feature".         */

	#define write1(ptr,len,n,stream) fwrite(ptr,len,n,stream)
	#define write2(ptr,len,n,stream) fwrite(ptr,len,n,stream)
	#define rand15() rand()

#else			/* may be Turbo-C */
#ifdef __MSDOS__		/* TURBO-C */
	typedef unsigned char  U8;
	typedef signed char    S8;
	typedef unsigned short U16;
	typedef signed short   S16;
	typedef unsigned long  U32;
	typedef signed long    S32;
	typedef unsigned char  BYTE;
	typedef unsigned char  CHAR;
	typedef void          *VOIDPTR;
	typedef void far      *VOIDFARPTR;
	typedef void const    *VOIDCONSTPTR;

	#define CONST          const
	#define PRINTER        "/dev/prn"
	#define LOBYTEFIRST    1
	#define SLASHC         '\\'
	#define SLASH          "\\"
	#define SLASHSLASH     "\\\\"
	#define SLASHDOT       "\\."
	#define DOTSLASH       ".\\"
	#define DOTDOTSLASH    "..\\"
	#define READMODE	"rb"    /* Correct DOS text-mode        */
	#define WRITEMODE	"wb"    /* file open "feature".         */

	#define write1(ptr,len,n,stream) fwrite(ptr,len,n,stream)
	#define write2(ptr,len,n,stream) fwrite(ptr,len,n,stream)
	#define rand15() rand()

#else			/* Have to nest because #elif is not portable */
#ifdef AMIGA		/* Lattice C 3.02 for Amiga */
	typedef UBYTE          U8;
	typedef BYTE           S8;
	typedef UWORD          U16;
	typedef WORD           S16;
	typedef unsigned int   U32;
	typedef int            S32;
	typedef UBYTE          BYTE;
	typedef UBYTE          CHAR;
	typedef void          *VOIDPTR;
	typedef void          *VOIDFARPTR;
	typedef void          *VOIDCONSTPTR;

	#define CONST
	#define PRINTER        "PRT:"
	#define LOBYTEFIRST    0
	#define SLASHC         '/'
	#define SLASH          "/"
	#define SLASHSLASH     "//"
	#define SLASHDOT       "/."
	#define DOTSLASH       "./"
	#define DOTDOTSLASH    "../"
	#define READMODE	"rb"
	#define WRITEMODE	"wb"

	#define write1(ptr,len,n,stream) (fputc(*(ptr),stream),1)
	#define write2(ptr,len,n,stream) (fputc((*(ptr))&255,stream),fputc((*(ptr))>>8,stream),1)
	#define rand15() (rand()&0x7FFF)

#else
#ifdef unix			/* Unix machine */
	typedef unsigned char  U8;
	typedef char           S8;
	typedef unsigned short U16;
	typedef short          S16;
	typedef unsigned long  U32;
	typedef long           S32;
	typedef unsigned char  BYTE;
	typedef char           CHAR;
#ifdef BADVOID
	typedef char          *VOIDPTR;
	typedef char          *VOIDFARPTR;
	typedef char          *VOIDCONSTPTR;
#else
	typedef void          *VOIDPTR;
	typedef void          *VOIDFARPTR;
	typedef void          *VOIDCONSTPTR;
#endif

#	define CONST
#	define PRINTER        "/dev/lp"
#	define SLASHC         '/'
#	define SLASH          "/"
#	define SLASHSLASH     "//"
#	define SLASHDOT       "/."
#	define DOTSLASH       "./"
#	define DOTDOTSLASH    "../"
#	define READMODE	"r"
#	define WRITEMODE	"w"

#	define write1(ptr,len,n,stream) (fputc(*(ptr),stream),1)
#	define write2(ptr,len,n,stream) (fputc((*(ptr))&255,stream),fputc((*(ptr))>>8,stream),1)
#	define rand15() (rand()&0x7FFF)

#	include "unix.h"


#endif
#endif
#endif
#endif

#ifdef LOBYTEFIRST
#define GET16(c,i)		(i) = *((U16*)(&(c)))
#else
#define GET16(c,i)              (i) = (*(unsigned char *)&(c))+\
				((*((unsigned char*)&(c)+1))<<8)
#endif

#endif	/* PORT_H */
