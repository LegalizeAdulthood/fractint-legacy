/**************************************
**
** PORT.H : Miscellaneous definitions for portability.  Please add
** to this file for any new machines/compilers you may have.
*/

#ifndef PORT_H		/* If this is defined, this file has been	*/
#define PORT_H 1	/* included already in this module.			*/

#ifdef MSDOS		/* Microsoft C 5.1 for OS/2 and MSDOS */
	typedef unsigned char U8;
	typedef signed char S8;
	typedef unsigned short U16;
	typedef signed short S16;
	typedef unsigned long U32;
	typedef signed long S32;
	#define CONST const
	#define PRINTER "/dev/prn"
	#define LOBYTEFIRST 1
#else			/* may be Turbo-C */
#ifdef __MSDOS__		/* TURBO-C */
	typedef unsigned char U8;
	typedef signed char S8;
	typedef unsigned short U16;
	typedef signed short S16;
	typedef unsigned long U32;
	typedef signed long S32;
	#define CONST const
	#define PRINTER "/dev/prn"
	#define LOBYTEFIRST 1
#else				/* Have to nest because #elif is not portable */
#ifdef AMIGA		/* Lattice C 3.02 for Amiga */
	typedef UBYTE U8;
	typedef BYTE S8;
	typedef UWORD U16;
	typedef WORD S16;
	typedef unsigned int U32;
	typedef int S32;
	#define CONST
	#define PRINTER "PRT:"
#else
#ifdef unix			/* AT&T 7300 (Unix PC) */
	typedef unsigned char U8;
	typedef char S8;
	typedef unsigned short U16;
	typedef short S16;
	typedef long U32;	/* May cause some problems */
	typedef long S32;
	#define CONST
	#define PRINTER "/dev/lp"
#endif
#endif
#endif
#endif

#ifdef MSDOS
	#define READMODE        "rb"	/* Correct DOS text-mode	*/
	#define WRITEMODE       "wb"	/* file open "feature".		*/
#else
#ifdef __MSDOS__
	#define READMODE        "rb"	/* Correct DOS text-mode	*/
	#define WRITEMODE       "wb"	/* file open "feature".		*/
#else
	#define READMODE        "r"
	#define WRITEMODE       "w"
#endif
#endif

#ifdef LOBYTEFIRST
	#define STORE16(c,i)	*((U16*)(&(c)))=(i)
	#define GET16(c,i)		(i)=*((U16*)(&(c)))
#else
	#define STORE16(c,i)	(c)=(i)&0xFF,*((char*)&(c)+1)=(i)>>8
	#define GET16(c,i)		(i)=(c)+(*((char*)&(c)+1))<<8
#endif

#endif	/* PORT_H */
