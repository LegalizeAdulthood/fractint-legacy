/*
   "Disk-Video" (and RAM-Video and Expanded-Memory Video) routines

   Reworked with fast caching July '90 by Pieter Branderhorst.
   (I'm proud of this cache handler, had to get my name on it!)
   Caution when modifying any code in here:  bugs are possible which
   slow the cache substantially but don't cause incorrect results.
   Do timing tests for a variety of situations after any change.

*/

#include <stdio.h>
#include <dos.h>
#include "fractint.h"

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

static int timetodisplay;
static FILE *fp = NULL;
static int disktarga;

#define BLOCKLEN 64	/* must be a power of 2, must match next */
#define BLOCKSHIFT 6	/* must match above */
#define CACHEMIN 4	/* minimum cache size in Kbytes */
#define CACHEMAX 24	/* maximum cache size in Kbytes */
#define HASHSIZE 256	/* power of 2, near CACHEMAX/(BLOCKLEN+8) */

static struct cache {	/* structure of each cache entry */
   long offset; 		   /* pixel offset in image */
   unsigned char pixel[BLOCKLEN];  /* one pixel per byte (this *is* faster) */
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

static char far *charbuf = NULL;   /* currently used only with XMM */

/* For expanded memory: */
static unsigned char far *expmemoryvideo;
static int oldexppage,expoffset;
static unsigned int emmhandle = 0;

/* For extended memory: */
static unsigned int xmmhandle = 0;
static long extoffset;
static unsigned char far *extbufptr, far *endreadbuf, far *endwritebuf;
struct XMM_Move
  {
    unsigned long   Length;
    unsigned int    SourceHandle;
    unsigned long   SourceOffset;
    unsigned int    DestHandle;
    unsigned long   DestOffset;
  };
#define XMMREADLEN 64	/* amount transferred from extended mem at once */
#define XMMWRITELEN 256 /* max amount transferred to extended mem at once,
			   must be a factor of 1024 and >= XMMREADLEN */

int startdisk(),pot_startdisk();
void enddisk();
int readdisk(unsigned int,unsigned int);
void writedisk(unsigned int,unsigned int,unsigned int);
int targa_startdisk(FILE *, int);
void targa_readdisk (unsigned int, unsigned int,
		     unsigned char *, unsigned char *, unsigned char *);
void targa_writedisk(unsigned int, unsigned int,
		     unsigned char, unsigned char, unsigned char);

static int common_startdisk();
static void findload_cache(long);
static struct cache far *find_cache(long);
static void write_cache_lru();
static void (*put_char)(unsigned char),disk_putc(unsigned char),
	    exp_putc(unsigned char),  ext_putc(unsigned char);
static unsigned char (*get_char)(),disk_getc(),exp_getc(),ext_getc();
static void (*doseek)(),disk_seek(),exp_seek(),ext_seek();
void dvid_status(int,char *);

int made_dsktemp = 0;

int startdisk()
{
   if (!diskisactive)
      return(0);
   headerlength = disktarga = 0;
   return (common_startdisk(sxdots,sydots));
   }

int pot_startdisk()
{
   int i;
   if (dotmode == 11) /* ditch the original disk file */
      enddisk();
   else
      showtempmsg("clearing 16bit pot work area");
   headerlength = disktarga = 0;
   i = common_startdisk(sxdots,sydots<<1);
   cleartempmsg();
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
   i = common_startdisk(sxdots*3,sydots);
   high_offset = 100000000L; /* targa not necessarily init'd to zeros */
   return (i);
}

static int common_startdisk(int newrowsize, int newcolsize)
{
   int i,j,success;
   long memorysize;
   unsigned int far *fwd_link;
   struct cache far *ptr1;
   long longtmp;
   unsigned int cache_size;
   int exppages;
   struct XMM_Move MoveStruct;
   unsigned char far *tempfar;

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

   /* allocate cache: try for the max, but don't take more than 1/2
      of what's available, demand a certain minimum or nogo at all */
   longtmp = (long)CACHEMAX << 11;
   while ((tempfar = farmemalloc(longtmp)) == NULL && longtmp > (CACHEMIN << 11))
      longtmp -= 2048;
   if (tempfar != NULL)
      farmemfree(tempfar);
   longtmp >>= 1; /* take 1/2 of what we got above */
   cache_size = (unsigned short)longtmp >> 10;
   cache_start = (struct cache far *)farmemalloc(longtmp);
   cache_end = (cache_lru = cache_start) + (cache_size<<10)/sizeof(*cache_start);
   hash_ptr  = (unsigned int far *)farmemalloc((long)(HASHSIZE<<1));
   if (cache_start == NULL || hash_ptr == NULL) {
      stopmsg(0,"*** insufficient free memory for cache buffers ***");
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
   rowsize = newrowsize;
   colsize = newcolsize;

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
	&& (xmmhandle = xmmallocate(longtmp = (memorysize+1023) >> 10)) != 0) {
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
      if ((fp = fopen("FRACTINT.DSK","w+b")) != NULL) {
	 made_dsktemp = 1;
	 while (--memorysize >= 0) /* "clear the screen" (write to the disk) */
	    putc(0,fp);
	 if (ferror(fp)) {
	    stopmsg(0,"*** insufficient free disk space ***");
	    fclose(fp);
	    fp = NULL;
	    rowsize = 0;
	    return(-1);
	    }
	 fclose(fp); /* so clusters aren't lost if we crash while running */
	 fp = fopen("FRACTINT.DSK","r+b"); /* reopen */
	 }
      if (fp == NULL) {
	 stopmsg(0,"*** Couldn't create FRACTINT.DSK ***");
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
   if (col >= rowsize || row >= colsize) /* if init bad, rowsize is 0 */
      return(0);
   if (--timetodisplay < 0) {  /* time to display status? */
      if (dotmode == 11) {
	 sprintf(buf," reading line %4d",
		(row >= sydots) ? row-sydots : row); /* adjust when potfile */
	 dvid_status(0,buf);
	 }
      timetodisplay = 1000;
      }
   if (row != cur_row) /* try to avoid ghastly code generated for multiply */
      cur_row_base = (long)(cur_row = row) * rowsize;
   offset = cur_row_base + col;
   col_subscr = (short)offset & (BLOCKLEN-1); /* offset within cache entry */
   if (cur_offset + col_subscr != offset)     /* same entry as last ref? */
      findload_cache(offset - col_subscr);
   return (cur_cache->pixel[col_subscr]);
   }

void targa_readdisk(unsigned int col, unsigned int row,
		    unsigned char *red, unsigned char *green, unsigned char *blue)
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
   if (col >= rowsize || row >= colsize)
      return;
   if (--timetodisplay < 0) {  /* time to display status? */
      if (dotmode == 11) {
	 sprintf(buf," writing line %4d",
		(row >= sydots) ? row-sydots : row); /* adjust when potfile */
	 dvid_status(0,buf);
	 }
      timetodisplay = 1000;
      }
   if (row != cur_row) /* try to avoid ghastly code generated for multiply */
      cur_row_base = (long)(cur_row = row) * rowsize;
   offset = cur_row_base + col;
   col_subscr = (short)offset & (BLOCKLEN-1);
   if (cur_offset + col_subscr != offset)
      findload_cache(offset - col_subscr);
   if (cur_cache->pixel[col_subscr] != (color & 0xff)) {
      cur_cache->pixel[col_subscr] = color;
      cur_cache->dirty = 1;
      }
   }

void targa_writedisk(unsigned int col, unsigned int row,
		    unsigned char red, unsigned char green, unsigned char blue)
{
   writedisk(col*=3,row,blue);
   writedisk(++col, row,green);
   writedisk(col+1, row,red);
}

static void findload_cache(long offset) /* used by read/write */
{
   unsigned int tbloffset;
   int i,j;
   unsigned int far *fwd_link;
   unsigned char far *pixelptr;
   unsigned char tmpchar;
   cur_offset = offset; /* note this for next reference */
   /* check if required entry is in cache - lookup by hash */
   tbloffset = hash_ptr[ ((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1) ];
   while (tbloffset != 0xffff) { /* follow the hash chain */
      (char far *)cur_cache = (char far *)cache_start + tbloffset;
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
   }

static struct cache far *find_cache(long offset) /* lookup for write_cache_lru */
{
   unsigned int tbloffset;
   struct cache far *ptr1;
   tbloffset = hash_ptr[ ((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1) ];
   while (tbloffset != 0xffff) {
      (char far *)ptr1 = (char far *)cache_start + tbloffset;
      if (ptr1->offset == offset)
	 return (ptr1);
      tbloffset = ptr1->hashlink;
      }
   return (NULL);
}

static void write_cache_lru()
{
   int i,j;
   unsigned char far *pixelptr;
   long offset;
   unsigned char tmpchar;
   struct cache far *ptr1, far *ptr2;
   /* scan back to also write any preceding dirty blocks */
   ptr1 = cache_lru;
   while ((ptr2 = find_cache(ptr1->offset - BLOCKLEN)) != NULL && ptr2->dirty)
      ptr1 = ptr2;
   (*doseek)(ptr1->offset >> pixelshift);
   seek_offset = -1; /* force a seek before next read */
   /* write all consecutive dirty blocks (often whole cache in 1pass modes) */
   while (ptr1 != NULL && ptr1->dirty) {
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
	       (*put_char)( ((((((*pixelptr << 1 | *(pixelptr+1)) << 1
			  | *(pixelptr+2)) << 1 | *(pixelptr+3)) << 1
			  | *(pixelptr+4)) << 1 | *(pixelptr+5)) << 1
			  | *(pixelptr+6)) << 1 | *(pixelptr+7));
	       pixelptr += 8;
	       }
	    break;
	 }
      ptr1->dirty = 0;
      ptr1 = find_cache(ptr1->offset + BLOCKLEN);
      }
}

/* Seek, get_char, put_char routines follow for expanded, extended, disk.
   Note that the calling logic always separates get_char and put_char
   sequences with a seek between them.	A get_char is never followed by
   a put_char nor v.v. without a seek between them.
   */

static void disk_seek(long offset)     /* real disk seek */
{
   fseek(fp,offset+headerlength,SEEK_SET);
   }

static unsigned char disk_getc()       /* real disk get_char */
{
   return(getc(fp));
   }

void disk_putc(unsigned char c) /* real disk put_char */
{
   putc(c,fp);
   }

static void exp_seek(long offset)      /* expanded mem seek */
{
   int page;
   expoffset = (short)offset & 0x3fff;
   page = offset >> 14;
   if (page != oldexppage) { /* time to get a new page? */
      oldexppage = page;
      emmgetpage(page,emmhandle);
      }
   }

static unsigned char exp_getc()        /* expanded mem get_char */
{
   if (expoffset > 0x3fff) /* wrapped into a new page? */
      exp_seek((long)(oldexppage+1) << 14);
   return(expmemoryvideo[expoffset++]);
   }

static void exp_putc(unsigned char c)  /* expanded mem put_char */
{
   if (expoffset > 0x3fff) /* wrapped into a new page? */
      exp_seek((long)(oldexppage+1) << 14);
   expmemoryvideo[expoffset++] = c;
   }

static void ext_writebuf()	       /* subrtn for extended mem seek/put_char */
{
   struct XMM_Move MoveStruct;
   MoveStruct.Length = extbufptr - charbuf;
   MoveStruct.SourceHandle = 0; /* Source is conventional memory */
   MoveStruct.SourceOffset = (unsigned long)charbuf;
   MoveStruct.DestHandle = xmmhandle;
   MoveStruct.DestOffset = extoffset;
   xmmmoveextended(&MoveStruct);
   }

static void ext_seek(long offset)      /* extended mem seek */
{
   if (extbufptr > endreadbuf) /* only true if there was a put_char sequence */
      ext_writebuf();
   extoffset = offset;
   extbufptr = endreadbuf = charbuf;
   }

static unsigned char ext_getc()        /* extended mem get_char */
{
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
   }

static void ext_putc(unsigned char c)  /* extended mem get_char */
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
   memset(buf,' ',40);
   strcpy(buf,msg);
   buf[strlen(msg)] = ' ';
   buf[40] = 0;
   putstring(BOXROW+8+line,BOXCOL+12,C_DVID_HI,buf);
   movecursor(25,80);
}

