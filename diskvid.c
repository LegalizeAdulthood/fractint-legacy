/*
   "Disk-Video" (and RAM-Video and Expanded-Memory Video) routines

   Reworked with fast caching July '90 by Pieter Branderhorst.
   (I'm proud of this cache handler, had to get my name on it!)
   Caution when modifying any code in here:  bugs are possible which
   slow the cache substantially but don't cause incorrect results.
   Do timing tests for a variety of situations after any change.

*/

#include <stdio.h>
#ifndef XFRACT
#include <dos.h>
#endif
#include <string.h>
#include "fractint.h"
#include "prototyp.h"

extern int sxdots, sydots, colors;
extern int dotmode;	  /* video access method, 11 if really disk video */
extern int diskisactive;  /* set by fractint to disable re-init */
extern int debugflag;
extern int diskflag;

#define BOXROW	 6
#define BOXCOL	 11
#define BOXWIDTH 57
#define BOXDEPTH 12

int disk16bit=0;	   /* storing 16 bit values for continuous potential */

extern int Shadowing, AntiAliasing;

static int timetodisplay;
static FILE *fp = NULL;
static int disktarga;

#define BLOCKLEN 64	/* must be a power of 2, must match next */
#define BLOCKSHIFT 6	/* must match above */
#define CACHEMIN 4	/* minimum cache size in Kbytes */
#define CACHEMAX 64	/* maximum cache size in Kbytes */
#define FREEMEM  20	/* try to leave this much far memory unallocated */
#define HASHSIZE 1024	/* power of 2, near CACHEMAX/(BLOCKLEN+8) */

static struct cache {	/* structure of each cache entry */
   long offset; 		   /* pixel offset in image */
   BYTE pixel[BLOCKLEN];  /* one pixel per byte (this *is* faster) */
   unsigned int hashlink;	   /* ptr to next cache entry with same hash */
   unsigned int dirty : 1;	   /* changed since read? */
   unsigned int lru : 1;	   /* recently used? */
   } far *cache_end, far *cache_lru, far *cur_cache;

static struct cache far *cache_start = NULL;
static long high_offset;	   /* highwater mark of writes */
static long seek_offset;	   /* what we'll get next if we don't seek */
static long cur_offset; 	   /* offset of last block referenced */
static int cur_row;
static long cur_row_base;
static unsigned int far *hash_ptr = NULL;
static int pixelshift;
static int headerlength;
static unsigned int rowsize = 0;   /* doubles as a disk video not ok flag */
static unsigned int colsize;	   /* sydots, *2 when pot16bit */

static BYTE far *charbuf = NULL;   /* currently used only with XMM */

/* For expanded memory: */
static BYTE far *expmemoryvideo;
static int oldexppage,expoffset;
static unsigned int emmhandle = 0;

/* For extended memory: */
static unsigned int xmmhandle = 0;
static long extoffset;
static BYTE far *extbufptr, far *endreadbuf, far *endwritebuf;
#define XMMREADLEN 64	/* amount transferred from extended mem at once */
#define XMMWRITELEN 256 /* max amount transferred to extended mem at once,
			   must be a factor of 1024 and >= XMMREADLEN */

int  _fastcall common_startdisk(long newrowsize, long newcolsize, int colors);
static void _fastcall near findload_cache(long);
static struct cache far * _fastcall near find_cache(long);
static void near write_cache_lru(void);
static void (_fastcall near *put_char)(BYTE);
static void _fastcall near disk_putc(BYTE);
static void _fastcall near exp_putc(BYTE);
static void _fastcall near ext_putc(BYTE);
static BYTE (near *get_char)();
static BYTE near disk_getc();
static BYTE near exp_getc();
static BYTE near ext_getc();
static void (_fastcall near *doseek)(long);
static void _fastcall near disk_seek(long);
static void _fastcall near exp_seek(long);
static void _fastcall near ext_seek(long);

int made_dsktemp = 0;
char diskfilename[] = {"FRACTINT.$$$"};
int startdisk()
{
   if (!diskisactive)
      return(0);
   headerlength = disktarga = 0;
   return (common_startdisk(sxdots,sydots,colors));
   }

int pot_startdisk()
{
   int i;
   if (dotmode == 11) /* ditch the original disk file */
      enddisk();
   else
      showtempmsg("clearing 16bit pot work area");
   headerlength = disktarga = 0;
   i = common_startdisk(sxdots,sydots<<1,colors);
   cleartempmsg();
   if (i == 0)
      disk16bit = 1;
   return (i);
   }

int targa_startdisk(FILE *targafp,int overhead)
{
   int i;
   if (dotmode == 11) { /* ditch the original disk file, make just the targa */
      enddisk();      /* close the 'screen' */
      setnullvideo(); /* set readdot and writedot routines to do nothing */
      }
   headerlength = overhead;
   fp = targafp;
   disktarga = 1;
   i = common_startdisk(sxdots*3,sydots,colors);
   high_offset = 100000000L; /* targa not necessarily init'd to zeros */
   return (i);
}

extern char boxy[];

int _fastcall common_startdisk(long newrowsize, long newcolsize, int colors)
{
   int i,success;
   long memorysize;
   unsigned int far *fwd_link;
   struct cache far *ptr1;
   long longtmp;
   unsigned int cache_size;
   int exppages;
   struct XMM_Move MoveStruct;
   BYTE far *tempfar;

   if (diskflag)
      enddisk();
   if (dotmode == 11) { /* otherwise, real screen also in use, don't hit it */
      char buf[20];
      helptitle();
      setattr(1,0,C_DVID_BKGRD,24*80);	/* init rest to background */
      for (i = 0; i < BOXDEPTH; ++i)
	 setattr(BOXROW+i,BOXCOL,C_DVID_LO,BOXWIDTH);  /* init box */
      putstring(BOXROW+2,BOXCOL+4,C_DVID_HI,"'Disk-Video' mode");
      putstring(BOXROW+4,BOXCOL+4,C_DVID_LO,"Screen resolution: ");
      sprintf(buf,"%d x %d",sxdots,sydots);
      putstring(-1,-1,C_DVID_LO,buf);
      if (disktarga)
	 putstring(-1,-1,C_DVID_LO,"  24 bit Targa");
      else {
	 putstring(-1,-1,C_DVID_LO,"  Colors: ");
	 sprintf(buf,"%d",colors);
	 putstring(-1,-1,C_DVID_LO,buf);
	 }
      putstring(BOXROW+8,BOXCOL+4,C_DVID_LO,"Status:");
      dvid_status(0,"clearing the 'screen'");
      }
   cur_offset = seek_offset = high_offset = -1;
   cur_row    = -1;
   if (disktarga)
      pixelshift = 0;
   else {
      pixelshift = 3;
      i = 2;
      while (i < colors) {
	 i *= i;
	 --pixelshift;
	 }
      }
   timetodisplay = 1000;  /* time-to-display-status counter */

   /* allocate cache: try for the max; leave FREEMEMk free if we can get
      that much or more; if we can't get that much leave 1/2 of whatever
      there is free; demand a certain minimum or nogo at all */
   for (cache_size = CACHEMAX; cache_size >= CACHEMIN; --cache_size) {
      longtmp = (cache_size < FREEMEM) ? (long)cache_size << 11
				       : (long)(cache_size+FREEMEM) << 10;
      if ((tempfar = farmemalloc(longtmp))) {
	 farmemfree(tempfar);
	 break;
	 }
      }
   longtmp = (long)cache_size << 10;
   cache_start = (struct cache far *)farmemalloc(longtmp);
   if (cache_size == 64)
      --longtmp; /* safety for next line */
   cache_end = (cache_lru = cache_start) + longtmp / sizeof(*cache_start);
   hash_ptr  = (unsigned int far *)farmemalloc((long)(HASHSIZE<<1));
   if (cache_start == NULL || hash_ptr == NULL) {
      static char far msg[]={"*** insufficient free memory for cache buffers ***"};
      stopmsg(0,msg);
      return(-1);
      }
   if (dotmode == 11) {
      char buf[50];
      sprintf(buf,"Cache size: %dK\n\n",cache_size);
      putstring(BOXROW+6,BOXCOL+4,C_DVID_LO,buf);
      }

   /* preset cache to all invalid entries so we don't need free list logic */
   for (i = 0; i < HASHSIZE; ++i)
      hash_ptr[i] = 0xffff; /* 0xffff marks the end of a hash chain */
   longtmp = 100000000L;
   for (ptr1 = cache_start; ptr1 < cache_end; ++ptr1) {
      ptr1->dirty = ptr1->lru = 0;
      fwd_link = hash_ptr
	 + (((unsigned short)(longtmp+=BLOCKLEN) >> BLOCKSHIFT) & (HASHSIZE-1));
      ptr1->offset = longtmp;
      ptr1->hashlink = *fwd_link;
      *fwd_link = (char far *)ptr1 - (char far *)cache_start;
      }

   memorysize = (long)(newcolsize) * newrowsize;
   if ((i = (short)memorysize & (BLOCKLEN-1)) != 0)
      memorysize += BLOCKLEN - i;
   memorysize >>= pixelshift;
   diskflag = 1;
   rowsize = (unsigned int) newrowsize;
   colsize = (unsigned int) newcolsize;

   if (debugflag != 420 && debugflag != 422 /* 422=xmm test, 420=disk test */
     && disktarga == 0) {
      /* Try Expanded Memory */
      exppages = (memorysize + 16383) >> 14;
      if ((expmemoryvideo = emmquery()) != NULL
	&& (emmhandle = emmallocate(exppages)) != 0) {
	 if (dotmode == 11)
	    putstring(BOXROW+2,BOXCOL+23,C_DVID_LO,"Using your Expanded Memory");
	 for (oldexppage = 0; oldexppage < exppages; oldexppage++)
	    emmclearpage(oldexppage, emmhandle); /* clear the "video" */
	 put_char = exp_putc;
	 get_char = exp_getc;
	 doseek  = exp_seek;
	 goto common_okend;
	 }
      }

   if (debugflag != 420 && disktarga == 0) {
      /* Try Extended Memory */
      if ((charbuf = farmemalloc((long)XMMWRITELEN)) != NULL
	&& xmmquery() !=0
	&& (xmmhandle = xmmallocate((unsigned int)(longtmp = (memorysize+1023) >> 10))) != 0) {
	 if (dotmode == 11)
	    putstring(BOXROW+2,BOXCOL+23,C_DVID_LO,"Using your Extended Memory");
	 for (i = 0; i < XMMWRITELEN; i++)
	    charbuf[i] = 0;
	 MoveStruct.SourceHandle = 0;	 /* Source is in conventional memory */
	 MoveStruct.SourceOffset = (unsigned long)charbuf;
	 MoveStruct.DestHandle	 = xmmhandle;
	 MoveStruct.Length	 = XMMWRITELEN;
	 MoveStruct.DestOffset	 = 0;
	 longtmp *= (1024/XMMWRITELEN);
	 while (--longtmp >= 0) {
	    if ((success = xmmmoveextended(&MoveStruct)) == 0) break;
	    MoveStruct.DestOffset += XMMWRITELEN;
	    }
	 if (success) {
	    put_char = ext_putc;
	    get_char = ext_getc;
	    doseek  = ext_seek;
	    extbufptr = endreadbuf = charbuf;
	    endwritebuf = charbuf + XMMWRITELEN;
	    goto common_okend;
	    }
	 xmmdeallocate(xmmhandle);	 /* Clear the memory */
	 xmmhandle = 0; 		 /* Signal same */
	 }
      }

   if (dotmode == 11)
      putstring(BOXROW+2,BOXCOL+23,C_DVID_LO,"Using your Disk Drive");
   if (disktarga == 0) {
      if ((fp = fopen(diskfilename,"w+b")) != NULL) {
	 made_dsktemp = 1;
#if 1					/* added by Michael Snyder */
	 memset(boxy, 0, 1024);
	 while (memorysize > 0)
	 {
	    static char far cancel[] =
		"Disk Video initialization interrupted:\n";

	    fwrite(boxy, (memorysize > 1024) ? 1024 : (int)memorysize, 1, fp);
	    memorysize -= 1024;
	    if (keypressed())   	/* user interrupt */
	       if (stopmsg(2, cancel))  /* esc to cancel, else continue */
	       {
		  enddisk();
		  return -2;            /* -1 == failed, -2 == cancel   */
	       }
	 }
#else
	 while (--memorysize >= 0) /* "clear the screen" (write to the disk) */
	    putc(0,fp);
#endif
	 if (ferror(fp)) {
	    static char far msg[]={"*** insufficient free disk space ***"};
	    stopmsg(0,msg);
	    fclose(fp);
	    fp = NULL;
	    rowsize = 0;
	    return(-1);
	    }
	 fclose(fp); /* so clusters aren't lost if we crash while running */
	 fp = fopen(diskfilename,"r+b"); /* reopen */
	 }
      if (fp == NULL) {
         char msg[80];
	 static char far s1[]={"*** Couldn't create "};
	 sprintf(msg,"%Fs%s",s1,diskfilename);
	 stopmsg(0,msg);
	 rowsize = 0;
	 return(-1);
	 }
      }
   put_char = disk_putc;
   get_char = disk_getc;
   doseek  = disk_seek;

common_okend:
   if (dotmode == 11)
      dvid_status(0,"");
   return(0);
}

void enddisk()
{
   if (fp != NULL) {
      if (disktarga) /* flush the cache */
	 for (cache_lru = cache_start; cache_lru < cache_end; ++cache_lru)
	    if (cache_lru->dirty)
	       write_cache_lru();
      fclose(fp);
      }
   if (charbuf != NULL)
      farmemfree((void far *)charbuf);
   if (hash_ptr != NULL)
      farmemfree((void far *)hash_ptr);
   if (cache_start != NULL)
      farmemfree((void far *)cache_start);
   if (emmhandle != 0)	 /* Expanded memory video? */
      emmdeallocate(emmhandle);
   if (xmmhandle != 0)	 /* Extended memory video? */
      xmmdeallocate(xmmhandle);
   diskflag = rowsize = emmhandle = xmmhandle = disk16bit = 0;
   hash_ptr    = NULL;
   cache_start = NULL;
   charbuf     = NULL;
   fp	       = NULL;
}

int readdisk(unsigned int col, unsigned int row)
{
   int col_subscr;
   long offset;
   char buf[41];
   if (--timetodisplay < 0) {  /* time to display status? */
      if (dotmode == 11) {
	 sprintf(buf," reading line %4d",
		(row >= sydots) ? row-sydots : row); /* adjust when potfile */
	 dvid_status(0,buf);
	 }
      timetodisplay = 1000;
      }
   if (row != cur_row)	{ /* try to avoid ghastly code generated for multiply */
      if (row >= colsize) /* while we're at it avoid this test if not needed  */
	 return(0);
      cur_row_base = (long)(cur_row = row) * rowsize;
      }
   if (col >= rowsize)
      return(0);
   offset = cur_row_base + col;
   col_subscr = (short)offset & (BLOCKLEN-1); /* offset within cache entry */
   if (cur_offset != (offset & (0L-BLOCKLEN))) /* same entry as last ref? */
      findload_cache(offset & (0L-BLOCKLEN));
   return (cur_cache->pixel[col_subscr]);
}

int FromMemDisk(long offset, int size, void far *dest)
{
   int col_subscr =  (offset & (BLOCKLEN - 1));

   if (col_subscr + size > BLOCKLEN)		/* access violates  a */
      return 0;                                 /*   cache boundary   */

   if (cur_offset != (offset & (0L-BLOCKLEN))) /* same entry as last ref? */
      findload_cache (offset & (0L-BLOCKLEN));

   far_memcpy(dest, (void far *) &cur_cache->pixel[col_subscr], size);
   cur_cache->dirty = 0;
   return 1;
}


void targa_readdisk(unsigned int col, unsigned int row,
		    BYTE *red, BYTE *green, BYTE *blue)
{
   col *= 3;
   *blue  = readdisk(col,row);
   *green = readdisk(++col,row);
   *red   = readdisk(col+1,row);
}

void writedisk(unsigned int col, unsigned int row, unsigned int color)
{
   int col_subscr;
   long offset;
   char buf[41];
   if (--timetodisplay < 0) {  /* time to display status? */
      if (dotmode == 11) {
	 sprintf(buf," writing line %4d",
		(row >= sydots) ? row-sydots : row); /* adjust when potfile */
	 dvid_status(0,buf);
	 }
      timetodisplay = 1000;
      }
   if (row != cur_row)	{ /* try to avoid ghastly code generated for multiply */
      if (row >= colsize) /* while we're at it avoid this test if not needed  */
	 return;
      cur_row_base = (long)(cur_row = row) * rowsize;
      }
   if (col >= rowsize)
      return;
   offset = cur_row_base + col;
   col_subscr = (short)offset & (BLOCKLEN-1);
   if (cur_offset != (offset & (0L-BLOCKLEN))) /* same entry as last ref? */
      findload_cache(offset & (0L-BLOCKLEN));
   if (cur_cache->pixel[col_subscr] != (color & 0xff)) {
      cur_cache->pixel[col_subscr] = color;
      cur_cache->dirty = 1;
      }
   if (Shadowing) {
      unsigned Mask;
      Mask = (1 << AntiAliasing) - 1;
      if(!(col & Mask) && !(row & Mask))
	 ShadowPutColor(col, row, color);
      }
}

int ToMemDisk(long offset, int size, void far *src)
{
   int col_subscr =  (offset & (BLOCKLEN - 1));

   if (col_subscr + size > BLOCKLEN)		/* access violates  a */
      return 0;                                 /*   cache boundary   */

   if (cur_offset != (offset & (0L-BLOCKLEN))) /* same entry as last ref? */
      findload_cache (offset & (0L-BLOCKLEN));

   far_memcpy((void far *) &cur_cache->pixel[col_subscr], src, size);
   cur_cache->dirty = 1;
   return 1;
}

void targa_writedisk(unsigned int col, unsigned int row,
		    BYTE red, BYTE green, BYTE blue)
{
   writedisk(col*=3,row,blue);
   writedisk(++col, row,green);
   writedisk(col+1, row,red);
}

static void _fastcall near findload_cache(long offset) /* used by read/write */
{
#ifndef XFRACT
   unsigned int tbloffset;
   int i,j;
   unsigned int far *fwd_link;
   BYTE far *pixelptr;
   BYTE tmpchar;
   cur_offset = offset; /* note this for next reference */
   /* check if required entry is in cache - lookup by hash */
   tbloffset = hash_ptr[ ((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1) ];
   while (tbloffset != 0xffff) { /* follow the hash chain */
      cur_cache = (struct cache far *)((char far *)cache_start + tbloffset);
      if (cur_cache->offset == offset) { /* great, it is in the cache */
	 cur_cache->lru = 1;
	 return;
	 }
      tbloffset = cur_cache->hashlink;
      }
   /* must load the cache entry from backing store */
   while (1) { /* look around for something not recently used */
      if (++cache_lru >= cache_end)
	 cache_lru = cache_start;
      if (cache_lru->lru == 0)
	 break;
      cache_lru->lru = 0;
      }
   if (cache_lru->dirty) /* must write this block before reusing it */
      write_cache_lru();
   /* remove block at cache_lru from its hash chain */
   fwd_link = hash_ptr
	    + (((unsigned short)cache_lru->offset >> BLOCKSHIFT) & (HASHSIZE-1));
   tbloffset = (char far *)cache_lru - (char far *)cache_start;
   while (*fwd_link != tbloffset)
      fwd_link = &((struct cache far *)((char far *)cache_start+*fwd_link))->hashlink;
   *fwd_link = cache_lru->hashlink;
   /* load block */
   cache_lru->dirty  = 0;
   cache_lru->lru    = 1;
   cache_lru->offset = offset;
   pixelptr = &cache_lru->pixel[0];
   if (offset > high_offset) { /* never been this high before, just clear it */
      high_offset = offset;
      for (i = 0; i < BLOCKLEN; ++i)
	 *(pixelptr++) = 0;
      }
   else {
      if (offset != seek_offset)
	 (*doseek)(offset >> pixelshift);
      seek_offset = offset + BLOCKLEN;
      switch (pixelshift) {
	 case 0:
	    for (i = 0; i < BLOCKLEN; ++i)
	       *(pixelptr++) = (*get_char)();
	    break;
	 case 1:
	    for (i = 0; i < BLOCKLEN/2; ++i) {
	       tmpchar = (*get_char)();
	       *(pixelptr++) = tmpchar >> 4;
	       *(pixelptr++) = tmpchar & 15;
	       }
	    break;
	 case 2:
	    for (i = 0; i < BLOCKLEN/4; ++i) {
	       tmpchar = (*get_char)();
	       for (j = 6; j >= 0; j -= 2)
		  *(pixelptr++) = (tmpchar >> j) & 3;
	       }
	    break;
	 case 3:
	    for (i = 0; i < BLOCKLEN/8; ++i) {
	       tmpchar = (*get_char)();
	       for (j = 7; j >= 0; --j)
		  *(pixelptr++) = (tmpchar >> j) & 1;
	       }
	    break;
	 }
      }
   /* add new block to its hash chain */
   fwd_link = hash_ptr + (((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1));
   cache_lru->hashlink = *fwd_link;
   *fwd_link = (char far *)cache_lru - (char far *)cache_start;
   cur_cache = cache_lru;
#endif
   }

static struct cache far * _fastcall near find_cache(long offset)
/* lookup for write_cache_lru */
{
#ifndef XFRACT
   unsigned int tbloffset;
   struct cache far *ptr1;
   tbloffset = hash_ptr[ ((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1) ];
   while (tbloffset != 0xffff) {
      ptr1 = (struct cache far *)((char far *)cache_start + tbloffset);
      if (ptr1->offset == offset)
	 return (ptr1);
      tbloffset = ptr1->hashlink;
      }
   return (NULL);
#endif
}

static void near write_cache_lru()
{
   int i,j;
   BYTE far *pixelptr;
   long offset;
   BYTE tmpchar;
   struct cache far *ptr1, far *ptr2;
#define WRITEGAP 4 /* 1 for no gaps */
   /* scan back to also write any preceding dirty blocks, skipping small gaps */
   ptr1 = cache_lru;
   offset = ptr1->offset;
   i = 0;
   while (++i <= WRITEGAP) {
      if ((ptr2 = find_cache(offset -= BLOCKLEN)) != NULL && ptr2->dirty) {
	 ptr1 = ptr2;
	 i = 0;
	 }
      }
   /* write all consecutive dirty blocks (often whole cache in 1pass modes) */
   /* keep going past small gaps */
write_seek:
   (*doseek)(ptr1->offset >> pixelshift);
write_stuff:
   pixelptr = &ptr1->pixel[0];
   switch (pixelshift) {
      case 0:
	 for (i = 0; i < BLOCKLEN; ++i)
	    (*put_char)(*(pixelptr++));
	 break;
      case 1:
	 for (i = 0; i < BLOCKLEN/2; ++i) {
	    tmpchar = *(pixelptr++) << 4;
	    tmpchar += *(pixelptr++);
	    (*put_char)(tmpchar);
	    }
	 break;
      case 2:
	 for (i = 0; i < BLOCKLEN/4; ++i) {
	    for (j = 6; j >= 0; j -= 2)
	       tmpchar = (tmpchar << 2) + *(pixelptr++);
	    (*put_char)(tmpchar);
	    }
	 break;
      case 3:
	 for (i = 0; i < BLOCKLEN/8; ++i) {
	    (*put_char)((BYTE)
			((((((((((((((*pixelptr
                        << 1)
                        | *(pixelptr+1) )
                        << 1)
			| *(pixelptr+2) )
                        << 1)
                        | *(pixelptr+3) )
                        << 1)
			| *(pixelptr+4) )
                        << 1)
                        | *(pixelptr+5) )
                        << 1)
			| *(pixelptr+6) )
                        << 1)
                        | *(pixelptr+7)));
	    pixelptr += 8;
	    }
	 break;
      }
   ptr1->dirty = 0;
   offset = ptr1->offset + BLOCKLEN;
   if ((ptr1 = find_cache(offset)) && ptr1->dirty)
      goto write_stuff;
   i = 1;
   while (++i <= WRITEGAP) {
      if ((ptr1 = find_cache(offset += BLOCKLEN)) && ptr1->dirty)
	 goto write_seek;
      }
   seek_offset = -1; /* force a seek before next read */
}

/* Seek, get_char, put_char routines follow for expanded, extended, disk.
   Note that the calling logic always separates get_char and put_char
   sequences with a seek between them.	A get_char is never followed by
   a put_char nor v.v. without a seek between them.
   */

static void _fastcall near disk_seek(long offset)	/* real disk seek */
{
   fseek(fp,offset+headerlength,SEEK_SET);
   }

static BYTE near disk_getc()			/* real disk get_char */
{
   return(getc(fp));
   }

static void _fastcall near disk_putc(BYTE c)	/* real disk put_char */
{
   putc(c,fp);
   }

static void _fastcall near exp_seek(long offset)	/* expanded mem seek */
{
   int page;
   expoffset = (short)offset & 0x3fff;
   page = offset >> 14;
   if (page != oldexppage) { /* time to get a new page? */
      oldexppage = page;
      emmgetpage(page,emmhandle);
      }
   }

static BYTE near exp_getc()			/* expanded get_char */
{
   if (expoffset > 0x3fff) /* wrapped into a new page? */
      exp_seek((long)(oldexppage+1) << 14);
   return(expmemoryvideo[expoffset++]);
   }

static void _fastcall near exp_putc(BYTE c)	/* expanded put_char */
{
   if (expoffset > 0x3fff) /* wrapped into a new page? */
      exp_seek((long)(oldexppage+1) << 14);
   expmemoryvideo[expoffset++] = c;
   }

static void near ext_writebuf() /* subrtn for extended mem seek/put_char */
{
#ifndef XFRACT
   struct XMM_Move MoveStruct;
   MoveStruct.Length = extbufptr - charbuf;
   MoveStruct.SourceHandle = 0; /* Source is conventional memory */
   MoveStruct.SourceOffset = (unsigned long)charbuf;
   MoveStruct.DestHandle = xmmhandle;
   MoveStruct.DestOffset = extoffset;
   xmmmoveextended(&MoveStruct);
#endif
   }

static void _fastcall near ext_seek(long offset)	/* extended mem seek */
{
   if (extbufptr > endreadbuf) /* only true if there was a put_char sequence */
      ext_writebuf();
   extoffset = offset;
   extbufptr = endreadbuf = charbuf;
   }

static BYTE near ext_getc()			/* extended get_char */
{
#ifndef XFRACT
   struct XMM_Move MoveStruct;
   if (extbufptr >= endreadbuf) { /* drained the last read buffer we fetched? */
      MoveStruct.Length = XMMREADLEN;
      MoveStruct.SourceHandle = xmmhandle; /* Source is extended memory */
      MoveStruct.SourceOffset = extoffset;
      MoveStruct.DestHandle = 0;
      MoveStruct.DestOffset = (unsigned long)charbuf;
      xmmmoveextended(&MoveStruct);
      extoffset += XMMREADLEN;
      endreadbuf = XMMREADLEN + (extbufptr = charbuf);
      }
   return (*(extbufptr++));
#endif
   }

static void _fastcall near ext_putc(BYTE c)	/* extended get_char */
{
   if (extbufptr >= endwritebuf) { /* filled the local write buffer? */
      ext_writebuf();
      extoffset += XMMWRITELEN;
      extbufptr = charbuf;
      }
   *(extbufptr++) = c;
   }

void dvid_status(int line,char *msg)
{
   char buf[41];
   int attrib;
   memset(buf,' ',40);
   memcpy(buf,msg,strlen(msg));
   buf[40] = 0;
   attrib = C_DVID_HI;
   if (line >= 100) {
      line -= 100;
      attrib = C_STOP_ERR;
      }
   putstring(BOXROW+8+line,BOXCOL+12,attrib,buf);
   movecursor(25,80);
}
