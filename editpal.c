
/*
 * editpal.c
 *
 * Edits VGA 256-color palettes.
 *
 * This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
 *
 *
 * Key to initials:
 *
 *    EAN - Ethan Nagel [70022,2552]
 *
 *
 * Revision History:
 *
 *   10-22-90 EAN     Initial release.
 *
 *   10-23-90 EAN     "Discovered" get_line/put_line functions, integrated
 *			them in (instead of only getcolor/putcolor). Much
 *			faster!
 *		      Redesigned color editors (now at top of palette) and
 *			re-assigned some keys.
 *		      Added (A)uto option.
 *		      Fixed memory allocation problem.	Now uses shared
 *			FRACTINT data area (strlocn).  Uses 6 bytes DS.
 *
 *   10-27-90 EAN     Added save to memory option - will save screen image to
 *			memory, if enough mem avail.  (disk otherwise).
 *		      Added s(T)ripe mode - works like (S)hade except only
 *			changes every n'th entry.
 *		      Added temporary palette save/restore.  (Can work like
 *			an undo feature.)  Thanks to Pieter Branderhorst for
 *			idea.
 *
 *   10-28-90 EAN     The (H)ide function now makes the palette invisible,
 *			while allowing any other operations (except '\\' -
 *			move/resize) to continue.
 *
 *   10-29-90 PB (in EAN's absence, <grin>)
 *		      Change 'c' to 'd' and 's' to '=' for below.
 *		      Add 'l' to load palette from .map, 's' to store .map.
 *		      Add 'c' to invoke color cycling.
 *		      Change cursor to use whatever colors it can from
 *		      the palette (not fixed 0 and 255).
 *		      Restore colors 0 and 255 to real values whenever
 *		      palette is not on display.
 *		      Rotate 255 colors instead of 254.
 *		      Reduce cursor blink rate.
 *
 *   11-15-90 EAN     Minor "bug" fixes.  Continuous rotation now at a fixed 
 *                      rate - once every timer tick (18.2 sec);  Blanks out
 *                      color samples when rotating; Editors no longer rotate 
 *                      with the colors in color rotation mode;  Eliminated 
 *                      (Z)oom mode; other minor fixes.
 * 
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dos.h>     /* for FP_SEG & FP_OFF */

#ifdef __TURBOC__
#   include <mem.h>   /* to get mem...() declarations */
#endif

#include "fractint.h" /* for overlay stuff */


/*
 * misc. #defines
 */

#define FONT_DEPTH	    8	  /* font size */

#define CSIZE_MIN	    8	  /* csize cannot be smaller than this */

#define CURSOR_SIZE	    5	  /* length of one side of the x-hair cursor */

#define CURSOR_BLINK_RATE   3	  /* timer ticks between cursor blinks */

#define FAR_RESERVE	8192L	  /* amount of far mem we will leave avail. */

#define MAX_WIDTH	 1024	  /* palette editor cannot be wider than this */

#define FILENAME "FRACTINT.$$1"   /* file where screen portion is */
				  /* stored */

#define TITLE	"FRACTINT"

#define TITLE_LEN (8)


#define newx(size)     mem_alloc(size)
#define new(class)     (class *)(mem_alloc(sizeof(class)))
#define delete(block)



/*
 * Stuff from fractint
 */

extern unsigned char dacbox[256][3];	 /* DAC spindac() will use	     */
extern int	     sxdots;		 /* width of physical screen	     */
extern int	     sydots;		 /* depth of physical screen	     */
extern int	     sxoffs;		 /* start of logical screen	     */
extern int	     syoffs;		 /* start of logical screen	     */
extern int	     daccount;		 /* ? # enteries spindac() updates ? */
extern int	     lookatmouse;	 /* mouse mode for getakey(), etc    */
extern int	     strlocn[]; 	 /* 10K buffer to store classes in   */
extern int	     colors;		 /* # colors avail.		     */
extern int	     color_dark;	 /* darkest color in palette	     */
extern int	     color_medium;	 /* nearest to medbright gray color  */

void		     spindac(int dir, int inc);
void		     putcolor(int x, int y, int color);
int		     getcolor(int x, int y);
void		     put_line(int row, int start, int stop, unsigned char *pixels);
void		     get_line(int row, int start, int stop, unsigned char *pixels);
int		     getakey(void);
int		     keypressed(void);
long		     readticker(void);
unsigned char far *  findfont(int fontparm);
void		     buzzer(int type); /* 0=normal, 1=interrupt, 2=error */
void		     save_palette(void);
void		     load_palette(void);
void		     find_special_colors(void);
void                 rotate(int dir);


#define UPARROW 	 1072	  /* the keys we will be using */
#define DOWNARROW	 1080
#define LEFTARROW	 1075
#define RIGHTARROW	 1077
#define PGDN		 1081
#define PGUP		 1073
#define ENTER		   13
#define KP_ENTER	 1013
#define ESCAPE		   27
#define CTRL_UPARROW	 1141
#define CTRL_DOWNARROW	 1145
#define CTRL_LEFTARROW	 1115
#define CTRL_RIGHTARROW  1116
#define INSERT		 1082
#define DELETE		 1083
#define F2		 1060
#define F3		 1061
#define F4		 1062
#define F5		 1063
#define F6		 1064
#define F7		 1065
#define F8		 1066
#define F9		 1067
#define SF2		 1085
#define SF3		 1086
#define SF4		 1087
#define SF5		 1088
#define SF6		 1089
#define SF7		 1090
#define SF8		 1091
#define SF9		 1092



/*
 * basic data types
 */


typedef struct
   {
   unsigned char red,
		 green,
		 blue;
   } PALENTRY;


typedef unsigned char BOOLEAN;



/*
 * static data
 */


static unsigned char far *font8x8;
static unsigned char	 *line_buff;   /* must be alloced!!! */
static unsigned char	  fg_color,
			  bg_color;


/*
 * Interface to FRACTINT's graphics stuff
 */


static void setpal(int pal, int r, int g, int b)
   {
   dacbox[pal][0] = r;
   dacbox[pal][1] = g;
   dacbox[pal][2] = b;
   spindac(0,1);
   }


static void getpal(int pal, PALENTRY *rgb)
   {
   *rgb = ( (PALENTRY *)dacbox )[pal];
   }


static void setpalrange(int first, int how_many, PALENTRY *pal)
   {
   memmove(dacbox+first, pal, how_many*3);
   spindac(0,1);
   }


static void getpalrange(int first, int how_many, PALENTRY *pal)
   {
   memmove(pal, dacbox+first, how_many*3);
   }


static void clip_put_line(int row, int start, int stop, unsigned char *pixels)
   {
   if ( row < 0 || row >= sydots || start > sxdots || stop < 0 )
      return ;

   if ( start < 0 )
      {
      pixels += -start;
      start = 0;
      }

   if ( stop >= sxdots )
      stop = sxdots - 1;

   if ( start > stop )
      return ;

   put_line(row, start, stop, pixels);
   }


static void clip_get_line(int row, int start, int stop, unsigned char *pixels)
   {
   if ( row < 0 || row >= sydots || start > sxdots || stop < 0 )
      return ;

   if ( start < 0 )
      {
      pixels += -start;
      start = 0;
      }

   if ( stop >= sxdots )
      stop = sxdots - 1;

   if ( start > stop )
      return ;

   get_line(row, start, stop, pixels);
   }


static void clip_putcolor(int x, int y, int color)
   {
   if ( x < 0 || y < 0 || x >= sxdots || y >= sydots )
      return ;

   putcolor(x, y, color);
   }


static int clip_getcolor(int x, int y)
   {
   if ( x < 0 || y < 0 || x >= sxdots || y >= sydots )
      return (0);

   return ( getcolor(x, y) );
   }


static void hline(int x, int y, int width, int color)
   {
   memset(line_buff, color, width);
   clip_put_line(y, x, x+width-1, line_buff);
   }


static void vline(int x, int y, int depth, int color)
   {
   while (depth-- > 0)
      clip_putcolor(x, y++, color);
   }


static void getrow(int x, int y, int width, unsigned char *buff)
   {
   clip_get_line(y, x, x+width-1, buff);
   }


static void putrow(int x, int y, int width, unsigned char *buff)
   {
   clip_put_line(y, x, x+width-1, buff);
   }


static void vgetrow(int x, int y, int depth, unsigned char *buff)
   {
   while (depth-- > 0)
      *buff++ = clip_getcolor(x, y++);
   }


static void vputrow(int x, int y, int depth, unsigned char *buff)
   {
   while (depth-- > 0)
      clip_putcolor(x, y++, *buff++);
   }


static void fillrect(int x, int y, int width, int depth, int color)
   {
   while (depth-- > 0)
      hline(x, y++, width, color);
   }


static void rect(int x, int y, int width, int depth, int color)
   {
   hline(x, y, width, color);
   hline(x, y+depth-1, width, color);

   vline(x, y, depth, color);
   vline(x+width-1, y, depth, color);
   }


static void displayc(int x, int y, int fg, int bg, int ch)
   {
   int		      xc, yc;
   unsigned char      t;
   unsigned char far *ptr;

   ptr = ((unsigned char far *)font8x8) + ch*FONT_DEPTH;

   for (yc=0; yc<FONT_DEPTH; yc++, y++, ++ptr)
      {
      for (xc=0, t=*ptr; xc<8; xc++, t<<=1)
	 line_buff[xc] = (t&0x80) ? (unsigned)fg : (unsigned)bg;
      putrow(x, y, 8, line_buff);
      }
   }


static void displayf(int x, int y, int fg, int bg, char *format, ...)
   {
   char buff[81];
   int	ctr;

   va_list arg_list;

   va_start(arg_list, format);
   vsprintf(buff, format, arg_list);
   va_end(arg_list);

   for(ctr=0; buff[ctr]!='\0'; ctr++, x+=8)
      displayc(x, y, fg, bg, buff[ctr]);
   }


/*
 * create smooth shades between two colors
 *
 */


static void mkpalrange(PALENTRY *p1, PALENTRY *p2, PALENTRY pal[], int num, int skip)
   {
   int	  curr;
   double rm = (double)((int) p2->red	- (int) p1->red  ) / num,
	  gm = (double)((int) p2->green - (int) p1->green) / num,
	  bm = (double)((int) p2->blue	- (int) p1->blue ) / num;

   for (curr=0; curr<num; curr+=skip)
      {
      pal[curr].red   = (p1->red   == p2->red  ) ? p1->red   : (int) p1->red   + (int) ( rm * curr );
      pal[curr].green = (p1->green == p2->green) ? p1->green : (int) p1->green + (int) ( gm * curr );
      pal[curr].blue  = (p1->blue  == p2->blue ) ? p1->blue  : (int) p1->blue  + (int) ( bm * curr );
      }
   }



/*
 * draw and horizontal/vertical dotted lines
 */


static void hdline(int x, int y, int width)
   {
   int ctr;
   unsigned char *ptr;

   for (ctr=0, ptr=line_buff; ctr<width; ctr++, ptr++)
      *ptr = (ctr&2) ? bg_color : fg_color;

   putrow(x, y, width, line_buff);
   }


static void vdline(int x, int y, int depth)
   {
   int ctr;

   for (ctr=0; ctr<depth; ctr++, y++)
      clip_putcolor(x, y, (ctr&2) ? bg_color : fg_color);
   }


static void drect(int x, int y, int width, int depth)
   {
   hdline(x, y, width);
   hdline(x, y+depth-1, width);

   vdline(x, y, depth);
   vdline(x+width-1, y, depth);
   }


/*
 * A very simple memory "allocator".
 *
 * Each call to mem_alloc() returns size bytes from the array mem_block.
 *
 * Be sure to call mem_init() before using mem_alloc()!
 *
 * Uses 4 bytes of DS.
 *
 */

static char	*mem_block;
static unsigned  mem_avail;


static void mem_init(void *block, unsigned size)
   {
   mem_block = (char *)block;
   mem_avail = size;
   }


static void *mem_alloc(unsigned size)
   {
   void *block;

   if (size & 1)
      ++size;	/* allocate even sizes */

   if (mem_avail < size)   /* don't let this happen! */
      {
      char buf[81];
      sprintf(buf, "Error %s(%d): Out of memory!!!\n", __FILE__, __LINE__);
      stopmsg(0, buf);
      exit(1);
      }

   block = mem_block;
   mem_avail -= size;
   mem_block += size;

   return(block);
   }



/*
 * Class:     Cursor
 *
 * Purpose:   Draw the blinking cross-hair cursor.
 *
 * Note:      Only one Cursor can exist (referenced through the_cursor).
 *	      IMPORTANT: Call Cursor_Construct before you use any other
 *	      Cursor_ function!  Call Cursor_Destroy before exiting to
 *	      deallocate memory.
 *
 * Size:      11 + CURSOR_SIZE*4  (if CURSOR_SIZE=5, 31 bytes)
 *
 * Note:      Uses 2 bytes DS
 */

struct _Cursor
   {

   int	   x, y;
   int	   hidden;	 /* >0 if mouse hidden */
   long    last_blink;
   BOOLEAN blink;
   char    t[CURSOR_SIZE],	  /* save line segments here */
	   b[CURSOR_SIZE],
	   l[CURSOR_SIZE],
	   r[CURSOR_SIZE];
   } ;

#define Cursor struct _Cursor

/* private: */

   static  void    Cursor__Draw      (void);
   static  void    Cursor__Save      (void);
   static  void    Cursor__Restore   (void);

/* public: */

   static  BOOLEAN Cursor_Construct (void);
   static  void    Cursor_Destroy   (void);

   static  void    Cursor_SetPos    (int x, int y);
   static  void    Cursor_Move	    (int xoff, int yoff);
   static  int	   Cursor_GetX	    (void);
   static  int	   Cursor_GetY	    (void);
   static  int	   Cursor_GetColor  (void);   /* returns color under cursor */
   static  void    Cursor_Hide	    (void);
   static  void    Cursor_Show	    (void);
   static  BOOLEAN Cursor_IsHidden  (void);

static Cursor *the_cursor = NULL;


static BOOLEAN Cursor_Construct(void)
   {
   if (the_cursor != NULL)
      return(FALSE);

   the_cursor = new(Cursor);

   the_cursor->x	  = sxdots/2;
   the_cursor->y	  = sydots/2;
   the_cursor->hidden	  = 1;
   the_cursor->blink	  = FALSE;
   the_cursor->last_blink = 0;

   return (TRUE);
   }


static void Cursor_Destroy(void)
   {
   if (the_cursor != NULL)
      delete(the_cursor);

   the_cursor = NULL;
   }



static void Cursor__Draw(void)
   {
   int color;

   find_special_colors();
   color = (the_cursor->blink) ? color_medium : color_dark;

   vline(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, color);
   vline(the_cursor->x, the_cursor->y+2,	     CURSOR_SIZE, color);

   hline(the_cursor->x-CURSOR_SIZE-1, the_cursor->y, CURSOR_SIZE, color);
   hline(the_cursor->x+2,	      the_cursor->y, CURSOR_SIZE, color);
   }


static void Cursor__Save(void)
   {
   vgetrow(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, the_cursor->t);
   vgetrow(the_cursor->x, the_cursor->y+2,	       CURSOR_SIZE, the_cursor->b);

   getrow(the_cursor->x-CURSOR_SIZE-1, the_cursor->y,  CURSOR_SIZE, the_cursor->l);
   getrow(the_cursor->x+2,	       the_cursor->y,  CURSOR_SIZE, the_cursor->r);
   }


static void Cursor__Restore(void)
   {
   vputrow(the_cursor->x, the_cursor->y-CURSOR_SIZE-1, CURSOR_SIZE, the_cursor->t);
   vputrow(the_cursor->x, the_cursor->y+2,	       CURSOR_SIZE, the_cursor->b);

   putrow(the_cursor->x-CURSOR_SIZE-1, the_cursor->y,  CURSOR_SIZE, the_cursor->l);
   putrow(the_cursor->x+2,	       the_cursor->y,  CURSOR_SIZE, the_cursor->r);
   }


static void Cursor_SetPos(int x, int y)
   {
   if (!the_cursor->hidden)
      Cursor__Restore();

   the_cursor->x = x;
   the_cursor->y = y;

   if (!the_cursor->hidden)
      {
      Cursor__Save();
      Cursor__Draw();
      }
   }


static void Cursor_Move(int xoff, int yoff)
   {
   if ( !the_cursor->hidden )
      Cursor__Restore();

   the_cursor->x += xoff;
   the_cursor->y += yoff;

   if (the_cursor->x < 0)	the_cursor->x = 0;
   if (the_cursor->y < 0)	the_cursor->y = 0;
   if (the_cursor->x >= sxdots) the_cursor->x = sxdots-1;
   if (the_cursor->y >= sydots) the_cursor->y = sydots-1;

   if ( !the_cursor->hidden )
      {
      Cursor__Save();
      Cursor__Draw();
      }
   }


static int Cursor_GetX(void)   { return(the_cursor->x); }

static int Cursor_GetY(void)   { return(the_cursor->y); }

static int Cursor_GetColor(void) { return(getcolor(the_cursor->x, the_cursor->y)); }


static void Cursor_Hide(void)
   {
   if ( the_cursor->hidden++ == 0 )
      Cursor__Restore();
   }


static void Cursor_Show(void)
   {
   if ( --the_cursor->hidden == 0)
      {
      Cursor__Save();
      Cursor__Draw();
      }
   }


static int Cursor_WaitKey(void)   /* blink cursor while waiting for a key */
   {
   long tick;

   while ( !keypressed() )
      {
      tick = readticker();

      if ( (tick - the_cursor->last_blink) > CURSOR_BLINK_RATE )
	 {
	 the_cursor->blink = (the_cursor->blink) ? FALSE : TRUE;
	 the_cursor->last_blink = tick;
	 if ( !the_cursor->hidden )
	    Cursor__Draw();
	 }
      else if ( tick < the_cursor->last_blink )
	 the_cursor->last_blink = tick;
      }

   return( keypressed() );
   }



/*
 * Class:     MoveBox
 *
 * Purpose:   Handles the rectangular move/resize box.
 *
 * Near:   19 + sxdots*2 + sydots*2  (if 640x480, 2259 bytes, 1024x768, 3603)
 */

struct _MoveBox
   {
   int	    x, y;
   int	    base_width,
	    base_depth;
   int	    csize;
   BOOLEAN  moved;
   BOOLEAN  should_hide;
   char    *t, *b,
	   *l, *r;
   } ;

#define MoveBox struct _MoveBox

/* private: */

   static void	   MoveBox__Draw     (MoveBox *this);
   static void	   MoveBox__Erase    (MoveBox *this);
   static void	   MoveBox__Move     (MoveBox *this, int key);

/* public: */

   static MoveBox *MoveBox_Construct  (int x, int y, int csize, int base_width,
				      int base_depth);
   static void	   MoveBox_Destroy    (MoveBox *this);
   static BOOLEAN  MoveBox_Process    (MoveBox *this); /* returns FALSE if ESCAPED */
   static BOOLEAN  MoveBox_Moved      (MoveBox *this);
   static BOOLEAN  MoveBox_ShouldHide (MoveBox *this);
   static int	   MoveBox_X	      (MoveBox *this);
   static int	   MoveBox_Y	      (MoveBox *this);
   static int	   MoveBox_CSize      (MoveBox *this);

   static void	   MoveBox_SetPos     (MoveBox *this, int x, int y);
   static void	   MoveBox_SetCSize   (MoveBox *this, int csize);



static MoveBox *MoveBox_Construct(int x, int y, int csize, int base_width, int base_depth)
   {
   MoveBox *this = new(MoveBox);

   this->x	     = x;
   this->y	     = y;
   this->csize	     = csize;
   this->base_width  = base_width;
   this->base_depth  = base_depth;
   this->moved	     = FALSE;
   this->should_hide = FALSE;
   this->t	     = newx(sxdots);
   this->b	     = newx(sxdots);
   this->l	     = newx(sydots);
   this->r	     = newx(sydots);

   return(this);
   }


static void MoveBox_Destroy(MoveBox *this)
   {
   delete(this->t);
   delete(this->b);
   delete(this->l);
   delete(this->r);
   delete(this);
   }


static BOOLEAN MoveBox_Moved(MoveBox *this) { return(this->moved); }

static BOOLEAN MoveBox_ShouldHide(MoveBox *this) { return(this->should_hide); }

static int MoveBox_X(MoveBox *this)	 { return(this->x); }

static int MoveBox_Y(MoveBox *this)	 { return(this->y); }

static int MoveBox_CSize(MoveBox *this)  { return(this->csize); }


static void MoveBox_SetPos(MoveBox *this, int x, int y)
   {
   this->x = x;
   this->y = y;
   }


static void MoveBox_SetCSize(MoveBox *this, int csize)
   {
   this->csize = csize;
   }


static void MoveBox__Draw(MoveBox *this)  /* private */
   {
   int width = this->base_width + this->csize*16+1,
       depth = this->base_depth + this->csize*16+1;
   int x     = this->x,
       y     = this->y;


   getrow (x, y,	 width, this->t);
   getrow (x, y+depth-1, width, this->b);

   vgetrow(x,	      y, depth, this->l);
   vgetrow(x+width-1, y, depth, this->r);

   hdline(x, y, 	width);
   hdline(x, y+depth-1, width);

   vdline(x,	     y, depth);
   vdline(x+width-1, y, depth);
   }


static void MoveBox__Erase(MoveBox *this)   /* private */
   {
   int width = this->base_width + this->csize*16+1,
       depth = this->base_depth + this->csize*16+1;

   vputrow(this->x,	    this->y, depth, this->l);
   vputrow(this->x+width-1, this->y, depth, this->r);

   putrow(this->x, this->y,	    width, this->t);
   putrow(this->x, this->y+depth-1, width, this->b);
   }


#define BOX_INC     1
#define CSIZE_INC   2

static void MoveBox__Move(MoveBox *this, int key)
   {
   BOOLEAN done  = FALSE;
   BOOLEAN first = TRUE;
   int	   xoff  = 0,
	   yoff  = 0;

   while ( !done )
      {
      switch(key)
	 {
	 case CTRL_RIGHTARROW:	 xoff += BOX_INC*4;   break;
	 case RIGHTARROW:	 xoff += BOX_INC;     break;
	 case CTRL_LEFTARROW:	 xoff -= BOX_INC*4;   break;
	 case LEFTARROW:	 xoff -= BOX_INC;     break;
	 case CTRL_DOWNARROW:	 yoff += BOX_INC*4;   break;
	 case DOWNARROW:	 yoff += BOX_INC;     break;
	 case CTRL_UPARROW:	 yoff -= BOX_INC*4;   break;
	 case UPARROW:		 yoff -= BOX_INC;     break;

	 default:
	    done = TRUE;
	 }

      if (!done)
	 {
	 if (!first)
	    getakey();	     /* delete key from buffer */
	 else
	    first = FALSE;
	 key = keypressed();   /* peek at the next one... */
	 }
      }

   xoff += this->x;
   yoff += this->y;   /* (xoff,yoff) = new position */

   if (xoff < 0) xoff = 0;
   if (yoff < 0) yoff = 0;

   if (xoff+this->base_width+this->csize*16+1 > sxdots)
       xoff = sxdots - (this->base_width+this->csize*16+1);

   if (yoff+this->base_depth+this->csize*16+1 > sydots)
      yoff = sydots - (this->base_depth+this->csize*16+1);

   if ( xoff!=this->x || yoff!=this->y )
      {
      MoveBox__Erase(this);
      this->y = yoff;
      this->x = xoff;
      MoveBox__Draw(this);
      }
   }


static BOOLEAN MoveBox_Process(MoveBox *this)
   {
   int	   key;
   int	   orig_x     = this->x,
	   orig_y     = this->y,
	   orig_csize = this->csize;

   MoveBox__Draw(this);

   while (1)
      {
      Cursor_WaitKey();
      key = getakey();

      if (key==ENTER || key==KP_ENTER || key==ESCAPE || key=='H' || key=='h')
	 {
	 if (this->x != orig_x || this->y != orig_y || this->csize != orig_csize)
	    this->moved = TRUE;
	 else
	   this->moved = FALSE;
	 break;
	 }

      switch(key)
	 {
	 case UPARROW:
	 case DOWNARROW:
	 case LEFTARROW:
	 case RIGHTARROW:
	 case CTRL_UPARROW:
	 case CTRL_DOWNARROW:
	 case CTRL_LEFTARROW:
	 case CTRL_RIGHTARROW:
	    MoveBox__Move(this, key);
	    break;

	 case PGUP:   /* shrink */
	    if (this->csize > CSIZE_MIN)
	       {
	       int t = this->csize - CSIZE_INC;
	       int change;

	       if (t < CSIZE_MIN)
		  t = CSIZE_MIN;

	       MoveBox__Erase(this);

	       change = this->csize - t;
	       this->csize = t;
	       this->x += (change*16) / 2;
	       this->y += (change*16) / 2;
	       MoveBox__Draw(this);
	       }
	    break;

	 case PGDN:   /* grow */
	    {
	    int max_width = min(sxdots, MAX_WIDTH);

	    if (this->base_depth+(this->csize+CSIZE_INC)*16+1 < sydots	&&
		this->base_width+(this->csize+CSIZE_INC)*16+1 < max_width )
	       {
	       MoveBox__Erase(this);
	       this->x -= (CSIZE_INC*16) / 2;
	       this->y -= (CSIZE_INC*16) / 2;
	       this->csize += CSIZE_INC;
	       if (this->y+this->base_depth+this->csize*16+1 > sydots)
		  this->y = sydots - (this->base_depth+this->csize*16+1);
	       if (this->x+this->base_width+this->csize*16+1 > max_width)
		  this->x = max_width - (this->base_width+this->csize*16+1);
	       if (this->y < 0)
		  this->y = 0;
	       if (this->x < 0)
		  this->x = 0;
	       MoveBox__Draw(this);
	       }
	    }
	    break;
	 }
      }

   MoveBox__Erase(this);

   this->should_hide = (key == 'H' || key == 'h') ? TRUE : FALSE;

   return( (key==ESCAPE) ? FALSE : TRUE );
   }



/*
 * Class:     CEditor
 *
 * Purpose:   Edits a single color component (R, G or B)
 *
 * Note:      Calls the "other_key" function to process keys it doesn't use.
 *	      The "change" function is called whenever the value is changed
 *	      by the CEditor.
 *
 * Size:      18 bytes.
 *
 */

struct _CEditor
   {
   int	     x, y;
   char      letter;
   int	     val;
   BOOLEAN   done;
   BOOLEAN   hidden;
   void    (*other_key)(int key, struct _CEditor *ce, void *info);
   void    (*change)(struct _CEditor *ce, void *info);
   void     *info;

   } ;

#define CEditor struct _CEditor

/* public: */

   static CEditor *CEditor_Construct( int x, int y, char letter,
				      void (*other_key)(int,CEditor*,void*),
				      void (*change)(CEditor*,void*), void *info);
   static void CEditor_Destroy	 (CEditor *this);
   static void CEditor_Draw	 (CEditor *this);
   static void CEditor_SetPos	 (CEditor *this, int x, int y);
   static void CEditor_SetVal	 (CEditor *this, int val);
   static int  CEditor_GetVal	 (CEditor *this);
   static void CEditor_SetDone	 (CEditor *this, BOOLEAN done);
   static void CEditor_SetHidden (CEditor *this, BOOLEAN hidden);
   static int  CEditor_Edit	 (CEditor *this);

#define CEditor_WIDTH (8*3+4)
#define CEditor_DEPTH (8+4)



static CEditor *CEditor_Construct( int x, int y, char letter,
				   void (*other_key)(int,CEditor*,void*),
				   void (*change)(CEditor*,void*), void *info)
   {
   CEditor *this = new(CEditor);

   this->x	   = x;
   this->y	   = y;
   this->letter    = letter;
   this->val	   = 0;
   this->other_key = other_key;
   this->hidden    = FALSE;
   this->change    = change;
   this->info	   = info;

   return(this);
   }


static void CEditor_Destroy(CEditor *this)
   {
   delete(this);
   }


static void CEditor_Draw(CEditor *this)
   {
   if (this->hidden)
      return;

   Cursor_Hide();
   displayf(this->x+2, this->y+2, fg_color, bg_color, "%c%02d", this->letter, this->val);
   Cursor_Show();
   }


static void CEditor_SetPos(CEditor *this, int x, int y)
   {
   this->x = x;
   this->y = y;
   }


static void CEditor_SetVal(CEditor *this, int val)
   {
   this->val = val;
   }


static int CEditor_GetVal(CEditor *this)
   {
   return(this->val);
   }


static void CEditor_SetDone(CEditor *this, BOOLEAN done)
   {
   this->done = done;
   }


static void CEditor_SetHidden(CEditor *this, BOOLEAN hidden)
   {
   this->hidden = hidden;
   }


static int CEditor_Edit(CEditor *this)
   {
   int key;

   this->done = FALSE;

   if (!this->hidden)
      {
      Cursor_Hide();
      rect(this->x, this->y, CEditor_WIDTH, CEditor_DEPTH, fg_color);
      Cursor_Show();
      }

   while ( !this->done )
      {
      Cursor_WaitKey();
      key = getakey();

      switch( key )
	 {
	 case PGUP:
	    if (this->val < 63)
	       {
	       this->val += 5;
	       if (this->val > 63)
		  this->val = 63;
	       CEditor_Draw(this);
	       this->change(this, this->info);
	       }
	    break;

	 case '+':
	    if (this->val < 63)
	       {
	       ++this->val;
	       CEditor_Draw(this);
	       this->change(this, this->info);
	       }
	    break;

	 case PGDN:
	    if (this->val > 0)
	       {
	       this->val -= 5;
	       if (this->val < 0)
		  this->val = 0;
	       CEditor_Draw(this);
	       this->change(this, this->info);
	       }
	    break;

	 case '-':
	    if (this->val > 0)
	       {
	       --this->val;
	       CEditor_Draw(this);
	       this->change(this, this->info);
	       }
	    break;

	 case '0':
	 case '1':
	 case '2':
	 case '3':
	 case '4':
	 case '5':
	 case '6':
         case '7':
         case '8':
         case '9':
	    this->val = (key - '0') * 10;
            if (this->val > 63)
               this->val = 63;
	    CEditor_Draw(this);
	    this->change(this, this->info);
	    break;

	 default:
	    this->other_key(key, this, this->info);
	    break;
	 } /* switch */
      } /* while */

   if (!this->hidden)
      {
      Cursor_Hide();
      rect(this->x, this->y, CEditor_WIDTH, CEditor_DEPTH, bg_color);
      Cursor_Show();
      }

   return(key);
   }



/*
 * Class:     RGBEditor
 *
 * Purpose:   Edits a complete color using three CEditors for R, G and B
 *
 * Size:
 *    Near:   25 bytes.
 */

struct _RGBEditor
   {
   int	     x, y;	      /* position */
   int	     curr;	      /* 0=r, 1=g, 2=b */
   int	     pal;	      /* palette number */
   BOOLEAN   done;
   BOOLEAN   hidden;
   CEditor  *color[3];	      /* color editors 0=r, 1=g, 2=b */
   void    (*other_key)(int key, struct _RGBEditor *e, void *info);
   void    (*change)(struct _RGBEditor *e, void *info);
   void     *info;
   } ;

#define RGBEditor struct _RGBEditor

/* private: */

   static void	    RGBEditor__other_key (int key, CEditor *ceditor, void *info);
   static void	    RGBEditor__change	 (CEditor *ceditor, void *info);

/* public: */

   static RGBEditor *RGBEditor_Construct(int x, int y,
		     void (*other_key)(int,RGBEditor*,void*),
		     void (*change)(RGBEditor*,void*), void *info);
   static void	   RGBEditor_Destroy  (RGBEditor *this);
   static void	   RGBEditor_SetPos   (RGBEditor *this, int x, int y);
   static void	   RGBEditor_SetDone  (RGBEditor *this, BOOLEAN done);
   static void	   RGBEditor_SetHidden (RGBEditor *this, BOOLEAN hidden);
   static void     RGBEditor_BlankSampleBox(RGBEditor *this);
   static void	   RGBEditor_Update   (RGBEditor *this);
   static void	   RGBEditor_Draw     (RGBEditor *this);
   static int	   RGBEditor_Edit     (RGBEditor *this);
   static void	   RGBEditor_SetPal   (RGBEditor *this, int pal);
   static void	   RGBEditor_GetPal   (RGBEditor *this);
   static void	   RGBEditor_SetRGB   (RGBEditor *this, int pal, PALENTRY *rgb);
   static PALENTRY RGBEditor_GetRGB   (RGBEditor *this);

#define RGBEditor_WIDTH 62
#define RGBEditor_DEPTH (1+1+CEditor_DEPTH*3-2+2)

#define RGBEditor_BWIDTH ( RGBEditor_WIDTH - (2+CEditor_WIDTH+1 + 2) )
#define RGBEditor_BDEPTH ( RGBEditor_DEPTH - 4 )



static RGBEditor *RGBEditor_Construct(int x, int y, void (*other_key)(int,RGBEditor*,void*),
				      void (*change)(RGBEditor*,void*), void *info)
   {
   RGBEditor  *this	= new(RGBEditor);
   static char letter[] = "RGB";
   int	       ctr;

   for (ctr=0; ctr<3; ctr++)
      this->color[ctr] = CEditor_Construct(0, 0, letter[ctr], RGBEditor__other_key,
					   RGBEditor__change, this);

   RGBEditor_SetPos(this, x, y);
   this->curr	   = 0;
   this->pal	   = 1;
   this->hidden    = FALSE;
   this->other_key = other_key;
   this->change    = change;
   this->info	   = info;

   return(this);
   }


static void RGBEditor_Destroy(RGBEditor *this)
   {
   CEditor_Destroy(this->color[0]);
   CEditor_Destroy(this->color[1]);
   CEditor_Destroy(this->color[2]);
   delete(this);
   }


static void RGBEditor_SetDone(RGBEditor *this, BOOLEAN done)
   {
   this->done = done;
   }


static void RGBEditor_SetHidden(RGBEditor *this, BOOLEAN hidden)
   {
   this->hidden = hidden;
   CEditor_SetHidden(this->color[0], hidden);
   CEditor_SetHidden(this->color[1], hidden);
   CEditor_SetHidden(this->color[2], hidden);
   }


static void RGBEditor__other_key(int key, CEditor *ceditor, void *info) /* private */
   {
   RGBEditor *this = (RGBEditor *)info;

   switch( key )
      {
      case 'R':
      case 'r':
	 if (this->curr != 0)
	    {
	    this->curr = 0;
	    CEditor_SetDone(ceditor, TRUE);
	    }
	 break;

      case 'G':
      case 'g':
	 if (this->curr != 1)
	    {
	    this->curr = 1;
	    CEditor_SetDone(ceditor, TRUE);
	    }
	 break;

      case 'B':
      case 'b':
	 if (this->curr != 2)
	    {
	    this->curr = 2;
	    CEditor_SetDone(ceditor, TRUE);
	    }
	 break;

      case DELETE:   /* move to next CEditor */
	 if ( ++this->curr > 2)
	    this->curr = 0;
	 CEditor_SetDone(ceditor, TRUE);
	 break;

      case INSERT:   /* move to prev CEditor */
	 if ( --this->curr < 0)
	    this->curr = 2;
	 CEditor_SetDone(ceditor, TRUE);
	 break;

      default:
	 this->other_key(key, this, this->info);
	 if (this->done)
	    CEditor_SetDone(ceditor, TRUE);
	 break;
      }
   }



#ifdef __TURBOC__
#   pragma argsused   /* kills "arg not used" warning */
#endif

static void RGBEditor__change(CEditor *ceditor, void *info) /* private */
   {
   RGBEditor *this = (RGBEditor *)info;

   setpal(this->pal, CEditor_GetVal(this->color[0]),
	  CEditor_GetVal(this->color[1]), CEditor_GetVal(this->color[2]));

   this->change(this, this->info);
   }


static void RGBEditor_SetPos(RGBEditor *this, int x, int y)
   {
   this->x = x;
   this->y = y;

   CEditor_SetPos(this->color[0], x+2, y+2);
   CEditor_SetPos(this->color[1], x+2, y+2+CEditor_DEPTH-1);
   CEditor_SetPos(this->color[2], x+2, y+2+CEditor_DEPTH-1+CEditor_DEPTH-1);
   }


static void RGBEditor_BlankSampleBox(RGBEditor *this)
   {
   if (this->hidden)
      return ;

   Cursor_Hide();
   fillrect(this->x+2+CEditor_WIDTH+1+1, this->y+2+1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, bg_color);
   Cursor_Show();
   }


static void RGBEditor_Update(RGBEditor *this)
   {
   if (this->hidden)
      return ;

   Cursor_Hide();
   fillrect(this->x+2+CEditor_WIDTH+1+1, this->y+2+1, RGBEditor_BWIDTH-2, RGBEditor_BDEPTH-2, this->pal);
   CEditor_Draw(this->color[0]);
   CEditor_Draw(this->color[1]);
   CEditor_Draw(this->color[2]);
   Cursor_Show();
   }


static void RGBEditor_Draw(RGBEditor *this)
   {
   if (this->hidden)
      return ;

   Cursor_Hide();
   drect(this->x, this->y, RGBEditor_WIDTH, RGBEditor_DEPTH);
   fillrect(this->x+1, this->y+1, RGBEditor_WIDTH-2, RGBEditor_DEPTH-2, bg_color);
   rect(this->x+1+CEditor_WIDTH+2, this->y+2, RGBEditor_BWIDTH, RGBEditor_BDEPTH, fg_color);
   RGBEditor_Update(this);
   Cursor_Show();
   }


static int RGBEditor_Edit(RGBEditor *this)
   {
   int key;

   this->done = FALSE;

   if (!this->hidden)
      {
      Cursor_Hide();
      rect(this->x, this->y, RGBEditor_WIDTH, RGBEditor_DEPTH, fg_color);
      Cursor_Show();
      }

   while ( !this->done )
      key = CEditor_Edit( this->color[this->curr] );

   if (!this->hidden)
      {
      Cursor_Hide();
      drect(this->x, this->y, RGBEditor_WIDTH, RGBEditor_DEPTH);
      Cursor_Show();
      }

   return (key);
   }


static void RGBEditor_SetPal(RGBEditor *this, int pal)
   {
   PALENTRY rgb;

   this->pal = pal;

   getpal(pal, &rgb);

   CEditor_SetVal(this->color[0], rgb.red);
   CEditor_SetVal(this->color[1], rgb.green);
   CEditor_SetVal(this->color[2], rgb.blue);
   }


static void RGBEditor_SetRGB(RGBEditor *this, int pal, PALENTRY *rgb)
   {
   this->pal = pal;
   CEditor_SetVal(this->color[0], rgb->red);
   CEditor_SetVal(this->color[1], rgb->green);
   CEditor_SetVal(this->color[2], rgb->blue);
   }


static PALENTRY RGBEditor_GetRGB(RGBEditor *this)
   {
   PALENTRY pal;

   pal.red   = CEditor_GetVal(this->color[0]);
   pal.green = CEditor_GetVal(this->color[1]);
   pal.blue  = CEditor_GetVal(this->color[2]);

   return(pal);
   }



/*
 * Class:     PalTable
 *
 * Purpose:   This is where it all comes together.  Creates the two RGBEditors
 *	      and the palette. Moves the cursor, hides/restores the screen,
 *	      handles (S)hading, (C)opying, e(X)clude mode, the "Y" exclusion
 *	      mode, (Z)oom option, (H)ide palette, rotation, etc.
 *
 */

enum stored_at_values
   {
   NOWHERE,
   DISK,
   MEMORY
   } ;

struct	_PalTable
   {
   int		 x, y;
   int		 csize;
   int		 active;   /* which RGBEditor is active (0,1) */
   int		 curr[2];
   RGBEditor	*rgb[2];
   MoveBox	*movebox;
   BOOLEAN	 done;
   BOOLEAN	 exclude;
   BOOLEAN	 auto_select;
   PALENTRY	 pal[256];
   int           hidden;

   int		 stored_at;
   FILE 	*file;
   char far	*memory;

   PALENTRY far *save_pal[8];
   } ;

#define PalTable struct _PalTable

/* private: */

   static void	  PalTable__HlPal	(PalTable *this, int pnum, int color);
   static void	  PalTable__Draw	(PalTable *this);
   static BOOLEAN PalTable__SetCurr	(PalTable *this, int which, int curr);
   static void	  PalTable__OffsetCurr	(PalTable *this, int which, int off);
   static BOOLEAN PalTable__MemoryAlloc (PalTable *this, long size);
   static void	  PalTable__SaveRect	(PalTable *this);
   static void	  PalTable__RestoreRect (PalTable *this);
   static void	  PalTable__SetPos	(PalTable *this, int x, int y);
   static void	  PalTable__SetCSize	(PalTable *this, int csize);
   static void	  PalTable__DoCurs	(PalTable *this, int key);
   static void    PalTable__Rotate	(PalTable *this, int dir);
   static void    PalTable__UpdateDAC   (PalTable *this);
   static void	  PalTable__other_key	(int key, RGBEditor *rgb, void *info);
   static void	  PalTable__change	(RGBEditor *rgb, void *info);

/* public: */

   static PalTable *PalTable_Construct (void);
   static void	    PalTable_Destroy   (PalTable *this);
   static void	    PalTable_Process   (PalTable *this);
   static void	    PalTable_SetHidden (PalTable *this, BOOLEAN hidden);
   static void	    PalTable_Hide      (PalTable *this, RGBEditor *rgb, BOOLEAN hidden);


#define PalTable_PALX (1)
#define PalTable_PALY (2+RGBEditor_DEPTH+2)


static void PalTable__HlPal(PalTable *this, int pnum, int color)
   {
   int x    = this->x + PalTable_PALX + (pnum%16)*this->csize,
       y    = this->y + PalTable_PALY + (pnum/16)*this->csize,
       size = this->csize;

   if (this->hidden)
      return ;

   Cursor_Hide();

   if (color < 0)
      drect(x, y, size+1, size+1);
   else
      rect(x, y, size+1, size+1, color);

   Cursor_Show();
   }


static void PalTable__Draw(PalTable *this)
   {
   int pal;
   int xoff, yoff;
   int width;

   if (this->hidden)
      return ;

   Cursor_Hide();

   width = 1+(this->csize*16)+1+1;

   rect(this->x, this->y, width, 2+RGBEditor_DEPTH+2+(this->csize*16)+1+1, fg_color);

   fillrect(this->x+1, this->y+1, width-2, 2+RGBEditor_DEPTH+2+(this->csize*16)+1+1-2, bg_color);

   hline(this->x, this->y+PalTable_PALY-1, width, fg_color);

   if ( width - (RGBEditor_WIDTH*2+4) >= TITLE_LEN*8 )
      {
      int center = (width - TITLE_LEN*8) / 2;

      displayf(this->x+center, this->y+2+RGBEditor_DEPTH/2-4, fg_color, bg_color, TITLE);
      }

   RGBEditor_Draw(this->rgb[0]);
   RGBEditor_Draw(this->rgb[1]);

   for (pal=0; pal<256; pal++)
      {
      xoff = PalTable_PALX + (pal%16) * this->csize;
      yoff = PalTable_PALY + (pal/16) * this->csize;
      fillrect(this->x + xoff + 1, this->y + yoff + 1, this->csize-1, this->csize-1, (pal%colors==fg_color)?bg_color:pal);
      }

   if (this->active == 0)
      {
      PalTable__HlPal(this, this->curr[1], -1);
      PalTable__HlPal(this, this->curr[0], fg_color);
      }
   else
      {
      PalTable__HlPal(this, this->curr[0], -1);
      PalTable__HlPal(this, this->curr[1], fg_color);
      }

   Cursor_Show();
   }



static BOOLEAN PalTable__SetCurr(PalTable *this, int which, int curr)
   {
   BOOLEAN redraw = (which < 0) ? TRUE : FALSE;

   if ( redraw )
      {
      which = this->active;
      curr = this->curr[which];
      }
   else
      if ( curr==fg_color || curr==bg_color || curr == this->curr[which] )
	 return (FALSE);

   Cursor_Hide();

   PalTable__HlPal(this, this->curr[0], bg_color);
   PalTable__HlPal(this, this->curr[1], bg_color);

   this->curr[which] = curr;

   if (this->curr[0] != this->curr[1])
      PalTable__HlPal(this, this->curr[this->active==0?1:0], -1);
   PalTable__HlPal(this, this->curr[this->active], fg_color);

   RGBEditor_SetRGB(this->rgb[which], this->curr[which], &(this->pal[this->curr[which]]));

   if (redraw)
      {
      int other = (which==0) ? 1 : 0;
      RGBEditor_SetRGB(this->rgb[other], this->curr[other], &(this->pal[this->curr[other]]));
      RGBEditor_Update(this->rgb[0]);
      RGBEditor_Update(this->rgb[1]);
      }
   else
      RGBEditor_Update(this->rgb[which]);

   if (this->exclude)
      PalTable__UpdateDAC(this);

   Cursor_Show();

   return(TRUE);
   }


static void PalTable__OffsetCurr(PalTable *this, int which, int off)
   {
   PalTable__SetCurr(this, which, this->curr[which]+off);
   }


static BOOLEAN PalTable__MemoryAlloc(PalTable *this, long size)
   {
   char far *temp;

   temp = farmemalloc(FAR_RESERVE);   /* minimum free space */

   if (temp == NULL)
      {
      this->stored_at = NOWHERE;
      return (FALSE);	/* can't do it */
      }

   this->memory = farmemalloc( size );

   farmemfree(temp);

   if ( this->memory == NULL )
      {
      this->stored_at = NOWHERE;
      return (FALSE);
      }
   else
      {
      this->stored_at = MEMORY;
      return (TRUE);
      }
   }


static void far *normalize(void far *ptr)
   {
   unsigned seg = FP_SEG(ptr),
            off = FP_OFF(ptr);

   seg += off>>4;
   off &= 0xF;

   return ( (void far *)(((unsigned long)seg<<16) + off) );
   }


static void PalTable__SaveRect(PalTable *this)
   {
   char buff[MAX_WIDTH];
   int	width = PalTable_PALX + this->csize*16 + 1 + 1,
	depth = PalTable_PALY + this->csize*16 + 1 + 1;
   int	yoff;


   /* first, do any de-allocationg */

   switch( this->stored_at )
      {
      case NOWHERE:
	 break;

      case DISK:
	 break;

      case MEMORY:
	 if (this->memory != NULL)
	    farmemfree(this->memory);
	 this->memory = NULL;
	 break;
      } ;

   /* allocate space and store the rect */

   if ( PalTable__MemoryAlloc(this, (long)width*depth) )
      {
      char far  *ptr = this->memory;
      char far	*bufptr = buff; /* MSC needs this indirection to get it right */

      Cursor_Hide();
      for (yoff=0; yoff<depth; yoff++)
	 {
	 getrow(this->x, this->y+yoff, width, buff);
	 hline (this->x, this->y+yoff, width, bg_color);
	 movedata(FP_SEG(bufptr), FP_OFF(bufptr), FP_SEG(ptr), FP_OFF(ptr), width);
	 ptr = (char far *)normalize(ptr+width);
	 }
      Cursor_Show();
      }

   else /* to disk */
      {
      this->stored_at = DISK;

      if ( this->file == NULL )
	 {
	 this->file = fopen(FILENAME, "w+b");
	 if (this->file == NULL)
	    {
	    this->stored_at = NOWHERE;
	    buzzer(3);
	    return ;
	    }
	 }

      rewind(this->file);
      Cursor_Hide();
      for (yoff=0; yoff<depth; yoff++)
	 {
	 getrow(this->x, this->y+yoff, width, buff);
	 hline (this->x, this->y+yoff, width, bg_color);
	 if ( fwrite(buff, width, 1, this->file) != 1 )
            {
            buzzer(3);
	    break;
            }
	 }
      Cursor_Show();
      }

   }


static void PalTable__RestoreRect(PalTable *this)
   {
   char buff[MAX_WIDTH];
   int	width = PalTable_PALX + this->csize*16 + 1 + 1,
	depth = PalTable_PALY + this->csize*16 + 1 + 1;
   int	yoff;

   if (this->hidden)
      return;

   switch ( this->stored_at )
      {
      case DISK:
	 rewind(this->file);
	 Cursor_Hide();
	 for (yoff=0; yoff<depth; yoff++)
	    {
	    if ( fread(buff, width, 1, this->file) != 1 )
               {
               buzzer(3);
	       break;
               }
	    putrow(this->x, this->y+yoff, width, buff);
	    }
	 Cursor_Show();
	 break;

      case MEMORY:
	 {
	 char far  *ptr = this->memory;
	 char far  *bufptr = buff; /* MSC needs this indirection to get it right */

	 Cursor_Hide();
	 for (yoff=0; yoff<depth; yoff++)
	    {
	    movedata(FP_SEG(ptr), FP_OFF(ptr), FP_SEG(bufptr), FP_OFF(bufptr), width);
	    putrow(this->x, this->y+yoff, width, buff);
	    ptr = (char far *)normalize(ptr+width);
	    }
	 Cursor_Show();
	 break;
	 }

      case NOWHERE:
	 break;
      } /* switch */
   }


static void PalTable__SetPos(PalTable *this, int x, int y)
   {
   int width = PalTable_PALX + this->csize*16 + 1 + 1;

   this->x = x;
   this->y = y;

   RGBEditor_SetPos(this->rgb[0], x+2, y+2);
   RGBEditor_SetPos(this->rgb[1], x+width-2-RGBEditor_WIDTH, y+2);
   }


static void PalTable__SetCSize(PalTable *this, int csize)
   {
   this->csize = csize;
   PalTable__SetPos(this, this->x, this->y);
   }


#define CURS_INC 1

static void PalTable__DoCurs(PalTable *this, int key)
   {
   BOOLEAN done  = FALSE;
   BOOLEAN first = TRUE;
   int	   xoff  = 0,
	   yoff  = 0;

   while ( !done )
      {
      switch(key)
	 {
	 case CTRL_RIGHTARROW:	 xoff += CURS_INC*4;   break;
	 case RIGHTARROW:	 xoff += CURS_INC;     break;
	 case CTRL_LEFTARROW:	 xoff -= CURS_INC*4;   break;
	 case LEFTARROW:	 xoff -= CURS_INC;     break;
	 case CTRL_DOWNARROW:	 yoff += CURS_INC*4;   break;
	 case DOWNARROW:	 yoff += CURS_INC;     break;
	 case CTRL_UPARROW:	 yoff -= CURS_INC*4;   break;
	 case UPARROW:		 yoff -= CURS_INC;     break;

	 default:
	    done = TRUE;
	 }

      if (!done)
	 {
	 if (!first)
	    getakey();	     /* delete key from buffer */
	 else
	    first = FALSE;
	 key = keypressed();   /* peek at the next one... */
	 }
      }

   Cursor_Move(xoff, yoff);

   if (this->auto_select)
      PalTable__SetCurr(this, this->active, Cursor_GetColor());
   }


#ifdef __TURBOC__
#   pragma argsused
#endif

static void PalTable__change(RGBEditor *rgb, void *info)
   {
   PalTable *this = (PalTable *)info;
   this->pal[this->curr[this->active]] = RGBEditor_GetRGB(rgb);

   if (this->curr[0] == this->curr[1])
      {
      int      other = this->active==0 ? 1 : 0;
      PALENTRY color;

      color = RGBEditor_GetRGB(this->rgb[this->active]);
      RGBEditor_SetRGB(this->rgb[other], this->curr[other], &color);

      Cursor_Hide();
      RGBEditor_Update(this->rgb[other]);
      Cursor_Show();
      }

   }


static void PalTable__UpdateDAC(PalTable *this)
   {
   if ( this->exclude )
      {
      memset(dacbox, 0, 256*3);
      if (this->exclude == 1)
         {
         int a = this->curr[this->active];
         memmove(dacbox[a], &this->pal[a], 3);
         }
      else 
         {
         int a = this->curr[0],
   	     b = this->curr[1];

         if (a>b)
	    {
	    int t=a;
	    a=b;
	    b=t;
	    }

         memmove(dacbox[a], &this->pal[a], 3*(1+(b-a)));
         }
      }
   else
      memmove(dacbox[0], this->pal, 3*colors);

   if ( !this->hidden )
      {
      memset(dacbox[0],         0, 3);   /* dacbox[0] = (0,0,0) */
      memset(dacbox[colors-1], 48, 3);   /* dacbox[colors-1] = (48,48,48) */
      }

   spindac(0,1);
   }


static void PalTable__Rotate(PalTable *this, int dir)
   {
   PALENTRY hold;
   int      first, last, size;

   first = 1;
   last  = colors - 1;
   size  = 1 + (last-first);

   if (dir == 1)
      {
      memmove(&hold, &this->pal[last],  3);
      memmove(&this->pal[first+1], &this->pal[first], 3*(size-1));
      memmove(&this->pal[first], &hold, 3);
      }
   else
      {
      memmove(&hold, &this->pal[first], 3);
      memmove(&this->pal[first], &this->pal[first+1], 3*(size-1));
      memmove(&this->pal[last], &hold,  3);
      }

   Cursor_Hide();

   /* update the DAC.  */

   PalTable__UpdateDAC(this);

   /* update the editors. */

   RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
   RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
   RGBEditor_Update(this->rgb[0]);
   RGBEditor_Update(this->rgb[1]);

   Cursor_Show();
   }



static void PalTable__other_key(int key, RGBEditor *rgb, void *info)
   {
   PalTable *this = (PalTable *)info;
   BOOLEAN   again;

   do
      {
      again = FALSE;

      switch(key)
         {
         case '\\':    /* move/resize */
	    {
	    if (this->hidden)
	       break;	     /* cannot move a hidden pal */
	    Cursor_Hide();
	    PalTable__RestoreRect(this);
	    MoveBox_SetPos(this->movebox, this->x, this->y);
	    MoveBox_SetCSize(this->movebox, this->csize);
	    if ( MoveBox_Process(this->movebox) )
	       {
	       if ( MoveBox_ShouldHide(this->movebox) )
	          PalTable_SetHidden(this, TRUE);
	       else if ( MoveBox_Moved(this->movebox) )
	          {
	          PalTable__SetPos(this, MoveBox_X(this->movebox), MoveBox_Y(this->movebox));
	          PalTable__SetCSize(this, MoveBox_CSize(this->movebox));
	          PalTable__SaveRect(this);
	          }
	       }
	    PalTable__Draw(this);
	    Cursor_Show();

	    RGBEditor_SetDone(this->rgb[this->active], TRUE);

	    if (this->auto_select)
	       PalTable__SetCurr(this, this->active, Cursor_GetColor());
	    break;
	    }

         case 'Y':    /* exclude range */
         case 'y':
	    if ( this->exclude==2 )    
	       this->exclude = 0;
	    else
	       this->exclude = 2;
            PalTable__UpdateDAC(this);
	    break;

         case 'X':
         case 'x':     /* exclude current entry */
	    if ( this->exclude==1 )
	       this->exclude = 0;
	    else
	       this->exclude = 1;
            PalTable__UpdateDAC(this);
	    break;

         case RIGHTARROW:
         case LEFTARROW:
         case UPARROW:
         case DOWNARROW:
         case CTRL_RIGHTARROW:
         case CTRL_LEFTARROW:
         case CTRL_UPARROW:
         case CTRL_DOWNARROW:
	    PalTable__DoCurs(this, key);
	    break;

         case ESCAPE:
	    this->done = TRUE;
	    RGBEditor_SetDone(rgb, TRUE);
	    break;

         case ' ':     /* select the other palette register */
	    this->active = (this->active==0) ? 1 : 0;
	    if (this->auto_select)
	       PalTable__SetCurr(this, this->active, Cursor_GetColor());
	     else
	       PalTable__SetCurr(this, -1, 0);

	    if (this->exclude)
               PalTable__UpdateDAC(this);

	    RGBEditor_SetDone(rgb, TRUE);
	    break;

         case ENTER:    /* set register to color under cursor.  useful when not */
         case KP_ENTER: /* in auto_select mode */
	    PalTable__SetCurr(this, this->active, Cursor_GetColor());

	    if (this->exclude)
	       PalTable__UpdateDAC(this);

	    RGBEditor_SetDone(rgb, TRUE);
	    break;

         case 'D':    /* copy (Duplicate?) color in inactive to color in active */
         case 'd':
	    {
	    int	  a = this->active,
		     b = (a==0) ? 1 : 0;
	    PALENTRY t;

	    t = RGBEditor_GetRGB(this->rgb[b]);
	    Cursor_Hide();

	    RGBEditor_SetRGB(this->rgb[a], this->curr[a], &t);
	    RGBEditor_Update(this->rgb[a]);
	    setpal(this->curr[a], t.red, t.green, t.blue);
	    PalTable__change(this->rgb[a], this);
	    this->pal[this->curr[a]] = t;

	    Cursor_Show();
	    break;
	    }

         case '=':    /* create a shade range between the two entries */
	    {
	    int a = this->curr[0],
	        b = this->curr[1];

	    if (a > b)
	       {
	       int t = a;
	       a = b;
	       b = t;
	       }

	    if (a != b)
	       {
	       mkpalrange(&this->pal[a], &this->pal[b], &this->pal[a], b-a, 1);
               PalTable__UpdateDAC(this);
	       }

	    break;
	    }

         case 'T':
         case 't':   /* s(T)ripe mode */
	    {
	    int key;

	    Cursor_Hide();
	    key = getakey();
	    Cursor_Show();

	    if (key >= '1' && key <= '9')
	       {
	       int a = this->curr[0],
		   b = this->curr[1];

	       if (a > b)
	          {
	          int t = a;
	          a = b;
	          b = t;
	          }

	       if (a != b)
	          {
	          mkpalrange(&this->pal[a], &this->pal[b], &this->pal[a], b-a, key-'0');
                  PalTable__UpdateDAC(this);
	          }
	       }

	    break;
	    }

         case 'A':   /* toggle auto-select mode */
         case 'a':
	    this->auto_select = (this->auto_select) ? FALSE : TRUE;
	    if (this->auto_select)
	       {
	       PalTable__SetCurr(this, this->active, Cursor_GetColor());
	       if (this->exclude)
	          PalTable__UpdateDAC(this);
	       }
	    break;

         case 'H':
         case 'h': /* toggle hide/display of palette editor */
	    Cursor_Hide();
	    PalTable_Hide(this, rgb, (this->hidden) ? FALSE : TRUE);
	    Cursor_Show();
	    break;

         case '.':   /* rotate once */
         case ',':
	    {
	    int dir = (key=='.') ? +1 : -1;

	    PalTable__Rotate(this, dir);
            break;
	    }

         case '>':   /* continuous rotation (until a key is pressed) */
         case '<':
	    {
	    int dir = (key=='>') ? +1 : -1;
            long tick;

	    Cursor_Hide();

            if ( !this->hidden )
               {
               RGBEditor_BlankSampleBox(this->rgb[0]);
               RGBEditor_BlankSampleBox(this->rgb[1]);
               RGBEditor_SetHidden(this->rgb[0], TRUE);
               RGBEditor_SetHidden(this->rgb[1], TRUE);
               }

	    while ( !keypressed() )
               {
               tick = readticker();
	       PalTable__Rotate(this, dir);
               while (readticker() == tick) ;   /* wait until a tick passes */
               }

	    key = getakey();	       
            again = (key=='<' || key=='>') ? TRUE : FALSE;  

            if ( !this->hidden )
               {
               RGBEditor_SetHidden(this->rgb[0], FALSE);
               RGBEditor_SetHidden(this->rgb[1], FALSE);
               RGBEditor_Update(this->rgb[0]);
               RGBEditor_Update(this->rgb[1]);
               }

	    Cursor_Show();
	    break;
	    }

         case 'I':     /* invert the fg & bg colors */
         case 'i':
	    {
	    unsigned char temp = fg_color;
	    fg_color = bg_color;
	    bg_color = temp;

	    Cursor_Hide();
	    PalTable__Draw(this);
	    Cursor_Show();
	    RGBEditor_SetDone(this->rgb[this->active], TRUE);
	    break;
	    }

         case F2:	  /* restore a palette */
         case F3:
         case F4:
         case F5:
         case F6:
         case F7:
         case F8:
         case F9:
	    {
	    int which = key - F2;

	    if ( this->save_pal[which] != NULL )
	       {
	       struct SREGS seg;

	       Cursor_Hide();

	       segread(&seg);
	       movedata(FP_SEG(this->save_pal[which]), FP_OFF(this->save_pal[which]),
		        seg.ds, (unsigned)(this->pal), 256*3);

               PalTable__UpdateDAC(this);

	       PalTable__SetCurr(this, -1, 0);
	       Cursor_Show();
	       RGBEditor_SetDone(this->rgb[this->active], TRUE);
	       }
	    else
	       buzzer(3);	 /* error buzz */
	    break;
	    }

         case SF2:   /* save a palette */
         case SF3:
         case SF4:
         case SF5:
         case SF6:
         case SF7:
         case SF8:
         case SF9:
	    {
	    int which = key - SF2;

	    if ( this->save_pal[which] != NULL )
	       {
	       struct SREGS seg;

	       segread(&seg);
	       movedata(seg.ds, (unsigned)(this->pal), FP_SEG(this->save_pal[which]),
		       FP_OFF(this->save_pal[which]), 256*3);
	       }
	    else
	       buzzer(3); /* oops! short on memory! */
	    break;
	    }

         case 'L':     /* load a .map palette */
         case 'l':
	    {
	    load_palette();
	    getpalrange(0, colors, this->pal);
            PalTable__UpdateDAC(this);
	    RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
	    RGBEditor_Update(this->rgb[0]);
	    RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
	    RGBEditor_Update(this->rgb[1]);
	    break;
	    }

         case 'S':     /* save a .map palette */
         case 's':
	    {
            setpalrange(0, colors, this->pal);
	    save_palette();
            PalTable__UpdateDAC(this);
	    break;
	    }

         case 'C':     /* color cycling sub-mode */
         case 'c':
            {
	    BOOLEAN oldhidden = this->hidden;

	    Cursor_Hide();
	    if ( !oldhidden )
	       PalTable_Hide(this, rgb, TRUE);
            setpalrange(0, colors, this->pal);
	    rotate(0);
	    getpalrange(0, colors, this->pal);
            PalTable__UpdateDAC(this);
	    if ( !oldhidden )
               {
	       RGBEditor_SetRGB(this->rgb[0], this->curr[0], &(this->pal[this->curr[0]]));
	       RGBEditor_SetRGB(this->rgb[1], this->curr[1], &(this->pal[this->curr[1]]));
	       PalTable_Hide(this, rgb, FALSE);
               }
	    Cursor_Show();
	    break;
            }
         } /* switch */
      }
   while (again);
   }


static void PalTable__MkDefaultPalettes(PalTable *this)  /* creates default Fkey palettes */
   {
   PALENTRY	black,
		white;
   PALENTRY	temp[256];
   int		ctr;
   struct SREGS seg;

   black.red = black.green = black.blue = 0;
   white.red = white.green = white.blue = 63;
   mkpalrange(&black, &white, temp, 256, 1); /* boring! */

   segread(&seg);

   for (ctr=0; ctr<8; ctr++)   /* copy temp into all fkey saves */
      movedata(seg.ss, (unsigned)(temp), FP_SEG(this->save_pal[ctr]),
	       FP_OFF(this->save_pal[ctr]), 256*3);
   }



static PalTable *PalTable_Construct(void)
   {
   PalTable	*this = new(PalTable);
   int		 csize;
   int		 ctr;
   PALENTRY far *mem_block;
   void far	*temp;

   temp = farmemalloc(FAR_RESERVE);

   if ( temp != NULL )
      {
      mem_block = (PALENTRY far *)farmemalloc(256L*3 * 8);

      if ( mem_block == NULL )
	 {
	 for (ctr=0; ctr<8; ctr++)
	    this->save_pal[ctr] = NULL;
	 }
      else
	 {
	 for (ctr=0; ctr<8; ctr++)
	    this->save_pal[ctr] = mem_block + (256*ctr);

	 PalTable__MkDefaultPalettes(this);
	 }
      farmemfree(temp);
      }

   this->rgb[0] = RGBEditor_Construct(0, 0, PalTable__other_key,
		  PalTable__change, this);
   this->rgb[1] = RGBEditor_Construct(0, 0, PalTable__other_key,
		  PalTable__change, this);

   this->movebox = MoveBox_Construct(0,0,0, PalTable_PALX+1, PalTable_PALY+1);

   this->active      = 0;
   this->curr[0]     = 1;
   this->curr[1]     = 1;
   this->auto_select = TRUE;
   this->exclude     = FALSE;
   this->hidden      = FALSE;
   this->stored_at   = NOWHERE;
   this->file	     = NULL;
   this->memory      = NULL;


   RGBEditor_SetPal(this->rgb[0], this->curr[0]);
   RGBEditor_SetPal(this->rgb[1], this->curr[1]);

   PalTable__SetPos(this, 0, 0);

   csize = ( (sydots-(PalTable_PALY+1+1)) / 2 ) / 16;
   if (csize<CSIZE_MIN)
      csize = CSIZE_MIN;
   PalTable__SetCSize(this, csize);

   return(this);
   }


static void PalTable_SetHidden(PalTable *this, BOOLEAN hidden)
   {
   this->hidden = hidden;
   RGBEditor_SetHidden(this->rgb[0], hidden);
   RGBEditor_SetHidden(this->rgb[1], hidden);
   PalTable__UpdateDAC(this);
   }


static void PalTable_Hide(PalTable *this, RGBEditor *rgb, BOOLEAN hidden)
   {
   if (hidden)
      {
      PalTable__RestoreRect(this);
      PalTable_SetHidden(this, TRUE);
      }
   else
      {
      PalTable_SetHidden(this, FALSE);
      if (this->stored_at == NOWHERE)  /* do we need to save screen? */
	 PalTable__SaveRect(this);
      PalTable__Draw(this);
      if (this->auto_select)
	 PalTable__SetCurr(this, this->active, Cursor_GetColor());
      RGBEditor_SetDone(rgb, TRUE);
      }
   }


static void PalTable_Destroy(PalTable *this)
   {

   if (this->file != NULL)
      {
      fclose(this->file);
      remove(FILENAME);
      }

   if (this->memory != NULL)
      farmemfree(this->memory);

   if (this->save_pal[0] != NULL)
      farmemfree((unsigned char far *)this->save_pal[0]);

   RGBEditor_Destroy(this->rgb[0]);
   RGBEditor_Destroy(this->rgb[1]);
   MoveBox_Destroy(this->movebox);
   delete(this);
   }


static void PalTable_Process(PalTable *this)
   {
   getpalrange(0, colors, this->pal);
   PalTable__UpdateDAC(this);

   if (!this->hidden)
      {
      MoveBox_SetPos(this->movebox, this->x, this->y);
      MoveBox_SetCSize(this->movebox, this->csize);
      if ( !MoveBox_Process(this->movebox) )
         {
         setpalrange(0, colors, this->pal);
	 return ;
         }

      PalTable__SetPos(this, MoveBox_X(this->movebox), MoveBox_Y(this->movebox));
      PalTable__SetCSize(this, MoveBox_CSize(this->movebox));

      if ( MoveBox_ShouldHide(this->movebox) )
	 PalTable_SetHidden(this, TRUE);
      else
	 {
	 PalTable__SaveRect(this);
	 PalTable__Draw(this);
	 }
      }

   PalTable__SetCurr(this, this->active,	  Cursor_GetColor());
   PalTable__SetCurr(this, (this->active==1)?0:1, Cursor_GetColor());
   Cursor_Show();

   this->done = FALSE;

   while ( !this->done )
      RGBEditor_Edit(this->rgb[this->active]);

   Cursor_Hide();
   PalTable__RestoreRect(this);
   setpalrange(0, colors, this->pal);
   }


/*
 * interface to FRACTINT
 */



void EditPalette(void)	     /* called by fractint */
   {
   int	     olddaccount    = daccount;
   int	     oldlookatmouse = lookatmouse;
   int	     oldsxoffs	    = sxoffs;
   int	     oldsyoffs	    = syoffs;
   PalTable *pt;

   ENTER_OVLY(OVLY_ROTATE);

   mem_init(strlocn, 10*1024);

   if ( (font8x8 = findfont(0)) == NULL )
      return ;

   line_buff = newx(max(sxdots,sydots));

   daccount = 256;
   lookatmouse = 3;
   sxoffs = syoffs = 0;

   bg_color = 0;
   fg_color = 255%colors;

   Cursor_Construct();
   pt = PalTable_Construct();
   PalTable_Process(pt);
   PalTable_Destroy(pt);
   Cursor_Destroy();

   daccount = olddaccount;
   lookatmouse = oldlookatmouse;
   sxoffs = oldsxoffs;
   syoffs = oldsyoffs;
   delete(line_buff);
   EXIT_OVLY;
   }

