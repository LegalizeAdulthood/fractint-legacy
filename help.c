
/*
 * help.c
 *
 * This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
 *
 *
 * Revision history:
 *
 *   2-26-90  EAN     Initial version.
 *
 *
 */


#ifndef TEST /* kills all those assert macros in production version */
#define NDEBUG
#endif

#define INCLUDE_COMMON	/* include common code in helpcom.h */


#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys\types.h>
#include <sys\stat.h>
#include "fractint.h"
#include "helpcom.h"
#include "helpdefs.h"


#define MAX_HIST	   16	     /* number of pages we'll remember */

#define ALT_F1		 1104
#define BACK_TAB	 1015
#define BACKSPACE	    8

#define ACTION_CALL	    0	     /* values returned by help_topic() */
#define ACTION_PREV	    1
#define ACTION_PREV2	    2	     /* special - go back two topics */
#define ACTION_INDEX	    3
#define ACTION_QUIT	    4

#define F_HIST		    (1<<0)   /* flags for help_topic() */
#define F_INDEX 	    (1<<1)

#define MAX_PAGE_SIZE	    (80*25)  /* no page of text may be larger */

#define TEXT_START_ROW	    2	     /* start print the help text here */


typedef struct
   {
   unsigned char r, c;
   int		 width;
   unsigned	 offset;
   int		 topic_num;
   unsigned	 topic_off;
   } LINK;


typedef struct
   {
   int	    topic_num;
   unsigned topic_off;
   } LABEL;


typedef struct
   {
   unsigned	 offset;
   unsigned	 len;
   int		 margin;
   } PAGE;


typedef struct
   {
   int	    topic_num;
   unsigned topic_off;
   int	    link;
   } HIST;


struct help_sig_info
   {
   unsigned long sig;
   int		 version;
   unsigned long base;	   /* only if added to fractint.exe */
   } ;


void print_document(char *outfname, int (*msg_func)(int,int), int save_extraseg );
static int print_doc_msg_func(int pnum, int num_pages);



void help_overlay(void) { }



/* stuff from fractint */

extern int  lookatmouse;
extern long timer_start;
extern int  helpmode;
extern int  text_type;	 /* 0=real color text, 1=640x200x2, 2=??mono?? */
extern int  textcbase;
extern int  textrbase;
extern int  extraseg;
extern int  release;

void putstring	     (int row, int col, int attr, unsigned char far *msg);
int  putstringcenter (int row, int col, int width, int attr, char far *msg);
void setattr	     (int row, int col, int attr, int width);
void movecursor      (int row, int col);
void setclear	     (void);
void helptitle	     (void);
int  getakey	     (void);
int  keypressed      (void);
void stackscreen     (void);
void unstackscreen   (void);
void findpath	     (char *filename, char *path);
int  farread	     (int handle, void far *buf, unsigned len);
int  farwrite	     (int handle, void far *buf, unsigned len);


static int	      help_file = -1; /* help file handle */
static long	      base_off;       /* offset to help info in help file */
static int	      max_links;      /* max # of links in any page */
static int	      max_pages;      /* max # of pages in any topic */
static int	      num_label;      /* number of labels */
static int	      num_topic;      /* number of topics */
static int	      curr_hist = 0;  /* current pos in history */

/* these items alloc'ed in init_help... */

static long	 far *topic_offset;	   /* 4*num_topic */
static LABEL	 far *label;		   /* 4*num_label */
static HIST	 far *hist;		   /* 6*MAX_HIST (96 bytes) */

/* these items alloc'ed only while help is active... */

static char	  far *buffer;		 /* MAX_PAGE_SIZE (2048 bytes) */
static LINK	  far *link_table;	 /* 10*max_links */
static PAGE	  far *page_table;	 /* 4*max_pages  */


static void help_seek(long pos)
   {
   lseek(help_file, base_off+pos, SEEK_SET);
   }


static void displayc(int row, int col, int color, int ch)
   {
   static char *s = "?";

   if (text_type == 1)	 /* if 640x200x2 mode */
      {
      /*
       * This is REALLY ugly, but it works.  Non-current links (ones that
       * would be bold if 640x200 supported it) are in upper-case and the
       * current item is inversed.
       *
       */

      if (color & INVERSE)
	 color = INVERSE;
      else if (color & BRIGHT)
	 {
	 color = 0;   /* normal */
	 if (ch>='a' && ch<='z')
	    ch += 'A' - 'a';
	 }
      else
	 color = 0;   /* normal */
      }

   s[0] = ch;
   putstring(row, col, color, s);
   }


static void display_text(int row, int col, int color, char far *text, unsigned len)
   {
   while (len-- > 0)
      {
      if (*text == CMD_LITERAL)
	 {
	 ++text;
	 --len;
	 }
      displayc(row, col++, color, *text++);
      }
   }


static void display_parse_text(char far *text, unsigned len, int start_margin, int *num_link, LINK far *link)
   {
   char far  *curr;
   int	      row, col;
   int	      tok;
   int	      size,
	      width;

   textcbase = SCREEN_INDENT;
   textrbase = TEXT_START_ROW;

   curr = text;
   row = 0;
   col = 0;

   size = width = 0;

   if (start_margin >= 0)
      tok = TOK_PARA;
   else
      tok = -1;

   while ( 1 )
      {
      switch ( tok )
	 {
	 case TOK_PARA:
	    {
	    int indent,
		margin;

	    if (size > 0)
	       {
	       ++curr;
	       indent = *curr++;
	       margin = *curr++;
	       len  -= 3;
	       }
	    else
	       {
	       indent = start_margin;
	       margin = start_margin;
	       }

	    col = indent;

	    while (1)
	       {
	       tok = find_token_length(ONLINE, curr, len, &size, &width);

	       if (tok == TOK_DONE || tok == TOK_NL || tok == TOK_FF )
		  break;

	       if (tok == TOK_PARA)
		  {
		  col = 0;   /* fake a new-line */
		  row++;
		  break;
		  }

	       if (tok == TOK_XONLINE || tok == TOK_XDOC)
		  {
		  curr += size;
		  len  -= size;
		  continue;
		  }

	       /* now tok is TOK_SPACE or TOK_LINK or TOK_WORD */

	       if (col+width > SCREEN_WIDTH)
		  {	     /* go to next line... */
		  col = margin;
		  ++row;

		  if ( tok == TOK_SPACE )
		     width = 0;   /* skip spaces at start of a line */
		  }

	       if (tok == TOK_LINK)
		  {
		  display_text(row, col, C_HELP_LINK, curr+7, width);
		  if (num_link != NULL)
		     {
		     link[*num_link].r	       = row;
		     link[*num_link].c	       = col;
		     link[*num_link].topic_num = *( (int far *) (curr+1) );
		     link[*num_link].topic_off = *( (int far *) (curr+3) );
		     link[*num_link].offset    = (unsigned) ((curr+7) - text);
		     link[*num_link].width     = width;
		     ++(*num_link);
		     }
		  }
	       else if (tok == TOK_WORD )
		  display_text(row, col, C_HELP_BODY, curr, width);

	       col += width;
	       curr += size;
	       len -= size;
	       }

	    width = size = 0;
	    break;
	    }

	 case TOK_CENTER:
	    col = find_line_width(ONLINE, curr, len);
	    col = (SCREEN_WIDTH-col)/2;
	    if (col < 0)
	       col = 0;
	    break;

	 case TOK_NL:
	    col = 0;
	    ++row;
	    break;

	 case TOK_LINK:
	    display_text(row, col, C_HELP_LINK, curr+7, width);
	    if (num_link != NULL)
	       {
	       link[*num_link].r	 = row;
	       link[*num_link].c	 = col;
	       link[*num_link].topic_num = *( (int far *) (curr+1) );
	       link[*num_link].topic_off = *( (int far *) (curr+3) );
	       link[*num_link].offset	 = (unsigned) ((curr+7) - text);
	       link[*num_link].width	 = width;
	       ++(*num_link);
	       }
	    break;

	 case TOK_XONLINE:  /* skip */
	 case TOK_FF:	    /* ignore */
	 case TOK_XDOC:     /* ignore */
	 case TOK_DONE:
	 case TOK_SPACE:
	    break;

	 case TOK_WORD:
	    display_text(row, col, C_HELP_BODY, curr, width);
	    break;
	 } /* switch */

      curr += size;
      len  -= size;
      col  += width;

      if (len == 0)
	 break;

      tok = find_token_length(ONLINE, curr, len, &size, &width);
      } /* while */

   textcbase = 0;
   textrbase = 0;
   }


static void color_link(LINK far *link, int color)
   {
   textcbase = SCREEN_INDENT;
   textrbase = TEXT_START_ROW;

   if (text_type == 1)	 /* if 640x200x2 mode */
      display_text(link->r, link->c, color, buffer+link->offset, link->width);
   else
      setattr(link->r, link->c, color, link->width);

   textcbase = 0;
   textrbase = 0;
   }



/* #define PUT_KEY(name, descrip) putstring(-1,-1,C_HELP_INSTR_KEYS,name), putstring(-1,-1,C_HELP_INSTR," "descrip"  ") */
#define PUT_KEY(name, descrip) putstring(-1,-1,C_HELP_INSTR,name); putstring(-1,-1,C_HELP_INSTR,":"descrip"  ")


static void helpinstr(void)
   {
   int ctr;

   for (ctr=0; ctr<80; ctr++)
     putstring(24, ctr, C_HELP_INSTR, " ");

   movecursor(24, 1);
   PUT_KEY("F1",               "Index");
   PUT_KEY("\x18\x19\x1B\x1A", "Select");
   PUT_KEY("Enter",            "Go to");
   PUT_KEY("Backspace",        "Last topic");
   PUT_KEY("Escape",           "Exit help");
   }


static void printinstr(void)
   {
   int ctr;

   for (ctr=0; ctr<80; ctr++)
     putstring(24, ctr, C_HELP_INSTR, " ");

   movecursor(24, 1);
   PUT_KEY("Escape", "Abort");
   }


#undef PUT_KEY


static void display_page(char far *title, char far *text, unsigned text_len, int page, int num_pages, int start_margin, int *num_link, LINK far *link)
   {
   char temp[9];

   helptitle();
   helpinstr();
   setattr(2, 0, C_HELP_BODY, 80*22);
   putstringcenter(1, 0, 80, C_HELP_HDG, title);
   sprintf(temp, "%2d of %d", page+1, num_pages);
   putstring(1, 79-(6 + ((num_pages>=10)?2:1)), C_HELP_INSTR, temp);

   if (text != NULL)
      display_parse_text(text, text_len, start_margin, num_link, link);

   movecursor(25, 80);	 /* hide cursor */
   }



/*
 * int overlap(int a, int a2, int b, int b2);
 *
 * If a, a2, b, and b2 are points on a line, this function returns the
 * distance of intersection between a-->a2 and b-->b2.	If there is no
 * intersection between the lines this function will return a negative number
 * representing the distance between the two lines.
 *
 * There are six possible cases of intersection between the lines:
 *
 *			a		      a2
 *			|		      |
 *     b	 b2	|		      |       b 	b2
 *     |---(1)---|	|		      |       |---(2)---|
 *			|		      |
 *		b	|     b2      b       |      b2
 *		|------(3)----|       |------(4)-----|
 *			|		      |
 *		 b	|		      |   b2
 *		 |------+--------(5)----------+---|
 *			|		      |
 *			|     b       b2      |
 *			|     |--(6)--|       |
 *			|		      |
 *			|		      |
 *
 */


static int overlap(int a, int a2, int b, int b2)
   {
   if ( b < a )
      {
      if ( b2 >= a2 )
	 return ( a2 - a );	       /* case (5) */

      return ( b2 - a );	       /* case (1), case (3) */
      }

   if ( b2 <= a2 )
      return ( b2 - b );	       /* case (6) */

   return ( a2 - b );		       /* case (2), case (4) */
   }


static int dist(int a, int b)
   {
   int t = a - b;

   return (abs(t));
   }


#ifdef __TURBOC__
#   pragma warn -def /* turn off "Possible use before definition" warning */
#endif




static int find_link_updown(LINK far *link, int num_link, int curr_link, int up)
   {
   int	     ctr,
	     curr_c2,
	     best_overlap,
	     temp_overlap;
   LINK far *curr,
	far *temp,
	far *best;
   int	     temp_dist;

   curr    = &link[curr_link];
   best    = NULL;
   curr_c2 = curr->c + curr->width - 1;

   for (ctr=0, temp=link; ctr<num_link; ctr++, temp++)
      {
      if ( ctr != curr_link &&
	   ( (up && temp->r < curr->r) || (!up && temp->r > curr->r) ) )
	 {
	 temp_overlap = overlap(curr->c, curr_c2, temp->c, temp->c+temp->width-1);
	 /* if >= 3 lines between, prioritize on vertical distance: */
	 if ((temp_dist = dist(temp->r, curr->r)) >= 4)
	    temp_overlap -= temp_dist * 100;

	 if (best != NULL)
	    {
	    if ( best_overlap >= 0 && temp_overlap >= 0 )
	       {     /* if they're both under curr set to closest in y dir */
	       if ( dist(best->r, curr->r) > temp_dist )
		  best = NULL;
	       }
	    else
	       {
	       if ( best_overlap < temp_overlap )
		  best = NULL;
	       }
	    }

	 if (best == NULL)
	    {
	    best = temp;
	    best_overlap = temp_overlap;
	    }
	 }
      }

   return ( (best==NULL) ? -1 : (int)(best-link) );
   }


static int find_link_leftright(LINK far *link, int num_link, int curr_link, int left)
   {
   int	     ctr,
	     curr_c2,
	     best_c2,
	     temp_c2,
	     best_dist,
	     temp_dist;
   LINK far *curr,
	far *temp,
	far *best;

   curr    = &link[curr_link];
   best    = NULL;
   curr_c2 = curr->c + curr->width - 1;

   for (ctr=0, temp=link; ctr<num_link; ctr++, temp++)
      {
      temp_c2 = temp->c + temp->width - 1;

      if ( ctr != curr_link &&
	   ( (left && temp_c2 < curr->c) || (!left && temp->c > curr_c2) ) )
	 {
	 temp_dist = dist(curr->r, temp->r);

	 if (best != NULL)
	    {
	    if ( best_dist == 0 && temp_dist == 0 )  /* if both on curr's line... */
	       {
	       if ( (  left && dist(curr->c, best_c2) > dist(curr->c, temp_c2) ) ||
		    ( !left && dist(curr_c2, best->c) > dist(curr_c2, temp->c) ) )
		  best = NULL;
	       }
	    else
	       {
	       if ( best_dist >= temp_dist )   /* if temp is closer... */
		  best = NULL;
	       }
	    } /* if (best...) */

	 if (best == NULL)
	    {
	    best      = temp;
	    best_dist = temp_dist;
	    best_c2   = temp_c2;
	    }
	 }
      } /* for */

   return ( (best==NULL) ? -1 : (int)(best-link) );
   }


#ifdef __TURBOC__
#   pragma warn .def   /* back to default */
#   pragma warn -par   /* now turn off "Parameter not used" warning */
#endif


static int find_link_key(LINK far *link, int num_link, int curr_link, int key)
   {
   switch (key)
      {
      case TAB:      return ( (curr_link>=num_link-1) ? -1 : curr_link+1 );
      case BACK_TAB: return ( (curr_link<=0)	      ? -1 : curr_link-1 );
      default:	     assert(0);  return (-1);
      }
   }


#ifdef __TURBOC__
#   pragma warn .par /* back to default */
#endif


static int do_move_link(LINK far *link, int num_link, int *curr, int (*f)(LINK far *,int,int,int), int val)
   {
   int t;

   if (num_link > 1)
      {
      if ( f == NULL )
	 t = val;
      else
	 t = (*f)(link, num_link, *curr, val);

      if ( t >= 0 && t != *curr )
	 {
	 color_link(&link[*curr], C_HELP_LINK);
	 *curr = t;
	 color_link(&link[*curr], C_HELP_CURLINK);
	 return (1);
	 }
      }

   return (0);
   }


static int help_topic(HIST *curr, HIST *next, int flags)
   {
   int	     len;
   int	     key;
   int	     num_pages;
   int	     num_link;
   int	     page;
   int	     curr_link;
   char      title[81];
   long      where;
   int	     draw_page;
   int	     action;

   where     = topic_offset[curr->topic_num]+2; /* +2 to skip flags */
   curr_link = curr->link;

   help_seek(where);

   read(help_file, (char *)&num_pages, sizeof(int));
   assert(num_pages>0 && num_pages<=max_pages);

   farread(help_file, (char far *)page_table, 6*num_pages);

   len = 0;
   read(help_file, (char *)&len, 1);
   assert(len>=0 && len<81);
   read(help_file, (char *)title, len);
   title[len] = '\0';

   where += 2 + num_pages*6 + 1 + len + 2;

   for(page=0; page<num_pages; page++)
      if (curr->topic_off >= page_table[page].offset &&
	  curr->topic_off <  page_table[page].offset+page_table[page].len )
	 break;

   assert(page < num_pages);

   action = -1;
   draw_page = 2;

   do
      {
      if (draw_page)
	 {
	 help_seek(where+page_table[page].offset);
	 farread(help_file, buffer, page_table[page].len);

	 num_link = 0;
	 display_page(title, buffer, page_table[page].len, page, num_pages,
		      page_table[page].margin, &num_link, link_table);

	 if (draw_page==2)
	    {
	    assert(num_link<=0 || (curr_link>=0 && curr_link<num_link));
	    }
	 else if (draw_page==3)
	    curr_link = num_link - 1;
	 else
	    curr_link = 0;

	 if (num_link > 0)
	    color_link(&link_table[curr_link], C_HELP_CURLINK);

	 draw_page = 0;
	 }

      key = getakey();

      switch(key)
	 {
	 case PAGE_DOWN:
	    if (page<num_pages-1)
	       {
	       page++;
	       draw_page = 1;
	       }
	    break;

	 case PAGE_UP:
	    if (page>0)
	       {
	       page--;
	       draw_page = 1;
	       }
	    break;

	 case HOME:
	    if ( page != 0 )
	       {
	       page = 0;
	       draw_page = 1;
	       }
	    else
	       do_move_link(link_table, num_link, &curr_link, NULL, 0);
	    break;

	 case END:
	    if ( page != num_pages-1 )
	       {
	       page = num_pages-1;
	       draw_page = 3;
	       }
	    else
	       do_move_link(link_table, num_link, &curr_link, NULL, num_link-1);
	    break;

	 case TAB:
	    if ( !do_move_link(link_table, num_link, &curr_link, find_link_key, key) &&
		 page<num_pages-1 )
	       {
	       ++page;
	       draw_page = 1;
	       }
	    break;

	 case BACK_TAB:
	    if ( !do_move_link(link_table, num_link, &curr_link, find_link_key, key) &&
		 page>0 )
	       {
	       --page;
	       draw_page = 3;
	       }
	    break;

	 case DOWN_ARROW:
	    if ( !do_move_link(link_table, num_link, &curr_link, find_link_updown, 0) &&
		 page<num_pages-1 )
	       {
	       ++page;
	       draw_page = 1;
	       }
	    break;

	 case UP_ARROW:
	    if ( !do_move_link(link_table, num_link, &curr_link, find_link_updown, 1) &&
		 page>0 )
	       {
	       --page;
	       draw_page = 3;
	       }
	    break;

	 case LEFT_ARROW:
	    do_move_link(link_table, num_link, &curr_link, find_link_leftright, 1);
	    break;

	 case RIGHT_ARROW:
	    do_move_link(link_table, num_link, &curr_link, find_link_leftright, 0);
	    break;

	 case ESC:	   /* exit help */
	    action = ACTION_QUIT;
	    break;

	 case BACKSPACE:   /* prev topic */
	 case ALT_F1:
	    if (flags & F_HIST)
	       action = ACTION_PREV;
	    break;

	 case F1:    /* help index */
	    if (!(flags & F_INDEX))
	       action = ACTION_INDEX;
	    break;

	 case ENTER:
	 case ENTER_2:
	    if (num_link > 0)
	       {
	       next->topic_num = link_table[curr_link].topic_num;
	       next->topic_off = link_table[curr_link].topic_off;
	       action = ACTION_CALL;
	       }
	    break;
	 } /* switch */
      }
   while ( action == -1 );

   curr->topic_off = page_table[page].offset;
   curr->link	   = curr_link;

   return (action);
   }


int help(int action)
   {
   static char far unknowntopic_msg[] = "Unknown Help Topic";
   HIST      curr;
   int	     oldlookatmouse;
   int	     oldhelpmode;
   int	     flags;
   HIST      next;

   ENTER_OVLY(OVLY_HELP);

   if (helpmode == -1)	 /* is help disabled? */
      {
      EXIT_OVLY;
      return (0);
      }

   if (help_file == -1)
      {
      buzzer(2);
      EXIT_OVLY;
      return (0);
      }

   buffer = farmemalloc((long)MAX_PAGE_SIZE + sizeof(LINK)*max_links +
			sizeof(PAGE)*max_pages);

   if (buffer == NULL)
      {
      buzzer(2);
      EXIT_OVLY;
      return (0);
      }

   link_table = (LINK far *)(&buffer[MAX_PAGE_SIZE]);
   page_table = (PAGE far *)(&link_table[max_links]);

   oldlookatmouse = lookatmouse;
   lookatmouse = 0;
   timer_start -= clock();
   stackscreen();

   if (helpmode >= 0)
      {
      next.topic_num = label[helpmode].topic_num;
      next.topic_off = label[helpmode].topic_off;
      }
   else
      {
      next.topic_num = helpmode;
      next.topic_off = 0;
      }

   oldhelpmode = helpmode;

   if (curr_hist <= 0)
      action = ACTION_CALL;  /* make sure it isn't ACTION_PREV! */

   do
      {
      switch(action)
	 {
	 case ACTION_PREV2:
	    if (curr_hist > 0)
	       curr = hist[--curr_hist];

	    /* fall-through */

	 case ACTION_PREV:
	    if (curr_hist > 0)
	       curr = hist[--curr_hist];
	    break;

	 case ACTION_QUIT:
	    break;

	 case ACTION_INDEX:
	    next.topic_num = label[HELP_INDEX].topic_num;
	    next.topic_off = label[HELP_INDEX].topic_off;

	    /* fall-through */

	 case ACTION_CALL:
	    curr = next;
	    curr.link = 0;
	    break;
	 } /* switch */

      flags = 0;
      if (curr.topic_num == label[HELP_INDEX].topic_num)
	 flags |= F_INDEX;
      if (curr_hist > 0)
	 flags |= F_HIST;

      if ( curr.topic_num >= 0 )
	 action = help_topic(&curr, &next, flags);
      else
	 {
	 if ( curr.topic_num == -100 )
	    {
	    print_document("FRACTINT.DOC", print_doc_msg_func, 1);
	    action = ACTION_PREV2;
	    }

	 else if ( curr.topic_num == -101 )
	    action = ACTION_PREV2;

	 else
	    {
	    display_page(unknowntopic_msg, NULL, 0, 0, 1, 0, NULL, NULL);
	    action = -1;
	    while (action == -1)
	       {
	       switch (getakey())
		  {
		  case ESC:	 action = ACTION_QUIT;	break;
		  case ALT_F1:	 action = ACTION_PREV;	break;
		  case F1:	 action = ACTION_INDEX; break;
		  } /* switch */
	       } /* while */
	    }
	 } /* else */

      if ( action != ACTION_PREV && action != ACTION_PREV2 )
	 {
	 if (curr_hist >= MAX_HIST)
	    {
	    int ctr;

	    for (ctr=0; ctr<MAX_HIST-1; ctr++)
	       hist[ctr] = hist[ctr+1];

	    curr_hist = MAX_HIST-1;
	    }
	 hist[curr_hist++] = curr;
	 }
      }
   while (action != ACTION_QUIT);

   farmemfree((unsigned char far *)buffer);

   unstackscreen();
   lookatmouse = oldlookatmouse;
   helpmode = oldhelpmode;
   timer_start += clock();

   EXIT_OVLY;
   return(0);
   }



static int dos_version(void)
   {
   union REGS r;

   r.x.ax = 0x3000;
   intdos(&r, &r);

   return (r.h.al*100 + r.h.ah);
   }


static int exe_path(char *filename, char *path)
   {
   char *ptr;

   if (dos_version() >= 300)  /* DOS version 3.00+ ? */
      {
#ifdef __TURBOC__
      strcpy(path, _argv[0]);
#else  /* assume MSC */
      extern char **__argv;
      strcpy(path, __argv[0]);	 /* note: __argv may be undocumented in MSC */
#endif

      ptr = strrchr(path, '\\');
      if (ptr == NULL)
	 ptr = path;
      else
	 ++ptr;
      strcpy(ptr, filename);
      return (1);
      }

   return (0);
   }


static int find_file(char *filename, char *path)
   {
   int handle;

   if ( exe_path(filename, path) )
      if ( (handle=open(path, O_RDONLY)) != -1)
	 {
	 close(handle);
	 return (1);
	 }

   findpath(filename,path);
   return ( (path[0]) ? 1 : 0);
   }


static int _read_help_topic(int topic, int off, int len, void far *buf)
   {
   static int  curr_topic = -1;
   static long curr_base;
   static int  curr_len;
   int	       read_len;

   if ( topic != curr_topic )
      {
      int t;

      curr_topic = topic;

      curr_base = topic_offset[topic];

      curr_base += 2;			       /* skip flags */

      help_seek(curr_base);
      read(help_file, (char *)&t, 2);	       /* read num_pages */
      curr_base += 2 + t*6;		       /* skip page info */

      if (t>0)
	 help_seek(curr_base);
      t = 0;
      read(help_file, (char *)&t, 1);	       /* read title_len */
      curr_base += 1 + t;		       /* skip title */

      if (t>0)
	 help_seek(curr_base);
      read(help_file, (char *)&curr_len, 2);   /* read topic len */
      curr_base += 2;
      }

   read_len = (off+len > curr_len) ? curr_len - off : len;

   if (read_len > 0)
      {
      help_seek(curr_base + off);
      farread(help_file, (char far *)buf, read_len);
      }

   return ( curr_len - (off+len) );
   }


int read_help_topic(int label_num, int off, int len, void far *buf)
   /*
    * reads text from a help topic.  Returns number of bytes from (off+len)
    * to end of topic.	On "EOF" returns a negative number representing
    * number of bytes not read.
    */
   {
   int ret;

   ENTER_OVLY(OVLY_HELP);

   ret = _read_help_topic(label[label_num].topic_num,
			  label[label_num].topic_off + off, len, buf);

   EXIT_OVLY;

   return ( ret );
   }


#define PRINT_BUFFER_SIZE  (32767)	 /* max. size of help topic in doc. */
#define TEMP_FILE_NAME	   "HELP.$$$"    /* temp file for storing extraseg  */
					 /*    while printing document	    */
#define MAX_NUM_TOPIC_SEC  (10) 	 /* max. number of topics under any */
					 /*    single section (CONTENT)     */


typedef struct PRINT_DOC_INFO
   {
   int	     cnum;	    /* current CONTENT num */
   int	     tnum;	    /* current topic num */

   long      content_pos;   /* current CONTENT item offset in file */
   int	     num_page;	    /* total number of pages in document */

   int	     num_contents,  /* total number of CONTENT entries */
	     num_topic;     /* number of topics in current CONTENT */

   int	     topic_num[MAX_NUM_TOPIC_SEC]; /* topic_num[] for current CONTENT entry */

   char far *buffer;	    /* text buffer */

   char      id[81];	    /* buffer to store id in */
   char      title[81];     /* buffer to store title in */

   int	   (*msg_func)(int pnum, int num_page);

   FILE     *file;	    /* file to sent output to */
   int	     margin;	    /* indent text by this much */
   int	     start_of_line; /* are we at the beginning of a line? */
   int	     spaces;	    /* number of spaces in a row */
   } PRINT_DOC_INFO;


void print_document(char *outfname, int (*msg_func)(int,int), int save_extraseg );


static void printerc(PRINT_DOC_INFO *info, int c, int n)
   {
   while ( n-- > 0 )
      {
      if (c==' ')
	 ++info->spaces;

      else if (c=='\n' || c=='\f')
	 {
	 info->start_of_line = 1;
	 info->spaces = 0;   /* strip spaces before a new-line */
	 putc(c, info->file);
	 }

      else
	 {
	 if (info->start_of_line)
	    {
	    info->spaces += info->margin;
	    info->start_of_line = 0;
	    }

	 while (info->spaces > 0)
	    {
	    fputc(' ', info->file);
	    --info->spaces;
	    }

	 fputc(c, info->file);
	 }
      }
   }


static void printers(PRINT_DOC_INFO *info, char far *s, int n)
   {
   if (n > 0)
      {
      while ( n-- > 0 )
	 printerc(info, *s++, 1);
      }
   else
      {
      while ( *s != '\0' )
	 printerc(info, *s++, 1);
      }
   }


static int print_doc_get_info(int cmd, PD_INFO *pd, PRINT_DOC_INFO *info)
   {
   int t;

   switch (cmd)
      {
      case PD_GET_CONTENT:
	 if ( ++info->cnum >= info->num_contents )
	    return (0);

	 help_seek( info->content_pos );

	 read(help_file, (char *)&t, 2);	/* read flags */
	 info->content_pos += 2;
	 pd->new_page = (t & 1) ? 1 : 0;

	 t = 0;
	 read(help_file, (char *)&t, 1);	/* read id len */
	 assert(t<80);
	 read(help_file, (char *)info->id, t);	/* read the id */
	 info->content_pos += 1 + t;
	 info->id[t] = '\0';

	 t = 0;
	 read(help_file, (char *)&t, 1);	/* read title len */
	 assert(t<80);
	 read(help_file, (char *)info->title, t); /* read the title */
	 info->content_pos += 1 + t;
	 info->title[t] = '\0';

	 t = 0;
	 read(help_file, (char *)&t, 1);	/* read num_topic */
	 assert(t<MAX_NUM_TOPIC_SEC);
	 read(help_file, (char *)info->topic_num, t*2);  /* read topic_num[] */
	 info->num_topic = t;
	 info->content_pos += 1 + t*2;

	 info->tnum = -1;

	 pd->id = info->id;
	 pd->title = info->title;
	 return (1);

      case PD_GET_TOPIC:
	 if ( ++info->tnum >= info->num_topic )
	    return (0);

	 t = _read_help_topic(info->topic_num[info->tnum], 0, PRINT_BUFFER_SIZE, info->buffer);

	 assert(t <= 0);

	 pd->curr = info->buffer;
	 pd->len  = PRINT_BUFFER_SIZE + t;   /* same as ...SIZE - abs(t) */
	 return (1);

      case PD_GET_LINK_PAGE:
	 pd->i = *(int far *)(pd->s+4);
	 return ( (pd->i == -1) ? 0 : 1 );

      case PD_RELEASE_TOPIC:
	 return (1);

      default:
	 return (0);
      }
   }


static int print_doc_output(int cmd, PD_INFO *pd, PRINT_DOC_INFO *info)
   {
   switch (cmd)
      {
      case PD_HEADING:
	 {
	 char line[81];
	 char buff[40];
	 int  width = PAGE_WIDTH + PAGE_INDENT;
	 int  keep_going;

	 if ( info->msg_func != NULL )
	    keep_going = (*info->msg_func)(pd->pnum, info->num_page);
	 else
	    keep_going = 1;

	 info->margin = 0;

	 memset(line, ' ', 81);
	 sprintf(buff, "Fractint Version %d.%01d%c",release/100, (release%100)/10,
				( (release%10) ? '0'+(release%10) : ' ') );
	 memmove(line + ( (width-strlen(buff)) / 2)-4, buff, strlen(buff));

	 sprintf(buff, "Page %d", pd->pnum);
	 memmove(line + (width - strlen(buff)), buff, strlen(buff));

	 printerc(info, '\n', 1);
	 printers(info, line, width);
	 printerc(info, '\n', 2);

	 info->margin = PAGE_INDENT;

	 return ( keep_going );
	 }

      case PD_FOOTING:
	 info->margin = 0;
	 printerc(info, '\f', 1);
	 info->margin = PAGE_INDENT;
	 return (1);

      case PD_PRINT:
	 printers(info, pd->s, pd->i);
	 return (1);

      case PD_PRINTN:
	 printerc(info, *pd->s, pd->i);
	 return (1);

      case PD_PRINT_SEC:
	 info->margin = TITLE_INDENT;
	 if (pd->id[0] != '\0')
	    {
	    printers(info, pd->id, 0);
	    printerc(info, ' ', 1);
	    }
	 printers(info, pd->title, 0);
	 printerc(info, '\n', 1);
	 info->margin = PAGE_INDENT;
	 return (1);

      case PD_START_SECTION:
      case PD_START_TOPIC:
      case PD_SET_SECTION_PAGE:
      case PD_SET_TOPIC_PAGE:
      case PD_PERIODIC:
	 return (1);

      default:
	 return (0);
      }
   }


static int print_doc_msg_func(int pnum, int num_pages)
   {
   char temp[10];
   int	key;

   if ( pnum == -1 )	/* successful completion */
      {
      buzzer(0);
      putstringcenter(7, 0, 80, C_HELP_LINK, "Done -- Press any key");
      getakey();
      return (0);
      }

   if ( pnum == -2 )   /* aborted */
      {
      buzzer(1);
      putstringcenter(7, 0, 80, C_HELP_LINK, "Aborted -- Press any key");
      getakey();
      return (0);
      }

   if (pnum == 0)   /* initialization */
      {
      helptitle();
      printinstr();
      setattr(2, 0, C_HELP_BODY, 80*22);
      putstringcenter(1, 0, 80, C_HELP_HDG, "Generating FRACTINT.DOC");

      putstring(7, 30, C_HELP_BODY, "Completed:");

      movecursor(25,80);   /* hide cursor */
      }


   sprintf(temp, "%d%%", (int)( (100.0 / num_pages) * pnum ) );
   putstring(7, 41, C_HELP_LINK, temp);

   while ( keypressed() )
      {
      key = getakey();
      if ( key == ESC )
	 return (0);	/* user abort */
      }

   return (1);	 /* AOK -- continue */
   }

int makedoc_msg_func(int pnum, int num_pages)
   {
   if (pnum >= 0)
      {
      printf("\rcompleted %d%%", (int)( (100.0 / num_pages) * pnum ) );
      return (1);
      }
   if ( pnum == -2 )
      printf("\n*** aborted");
   printf("\n");
   return (0);
   }



void print_document(char *outfname, int (*msg_func)(int,int), int save_extraseg )
   {
   static char far err_no_temp[]  = "Unable to create temporary file.\n";
   static char far err_no_out[]   = "Unable to create output file.\n";
   static char far err_badwrite[] = "Error writing temporary file.\n";
   static char far err_badread[]  = "Error reading temporary file.\nSystem may be corrupt!\nSave your image and re-start FRACTINT!\n";

   PRINT_DOC_INFO info;
   int		  success;
   int		  temp_file = -1;
   char      far *msg = NULL;

   ENTER_OVLY(OVLY_HELP);

   info.buffer = MK_FP(extraseg, 0);

   help_seek(8L);
   read(help_file, (char *)&info.num_contents, 2);
   read(help_file, (char *)&info.num_page, 2);

   info.cnum = info.tnum = -1;
   info.content_pos = 12 + num_topic*4 + num_label*4;
   info.msg_func = msg_func;

   if ( msg_func != NULL )
      msg_func(0, info.num_page);   /* initialize */

   if ( save_extraseg )
      {
      if ( (temp_file=open(TEMP_FILE_NAME, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1 )
	 {
	 msg = err_no_temp;
	 goto ErrorAbort;
	 }

      if ( farwrite(temp_file, info.buffer, PRINT_BUFFER_SIZE) != PRINT_BUFFER_SIZE )
	 {
	 msg = err_badwrite;
	 goto ErrorAbort;
	 }
      }

   if ( (info.file = fopen(outfname, "wt")) == NULL )
      {
      msg = err_no_out;
      goto ErrorAbort;
      }

   info.margin = PAGE_INDENT;
   info.start_of_line = 1;
   info.spaces = 0;

   success = process_document((PD_FUNC)print_doc_get_info,
			      (PD_FUNC)print_doc_output,   &info);
   fclose(info.file);

   if ( save_extraseg )
      {
      if ( lseek(temp_file, 0L, SEEK_SET) != 0L )
	 {
	 msg = err_badread;
	 goto ErrorAbort;
	 }

      if ( farread(temp_file, info.buffer, PRINT_BUFFER_SIZE) != PRINT_BUFFER_SIZE )
	 {
	 msg = err_badread;
	 goto ErrorAbort;
	 }
      }

ErrorAbort:
   if (temp_file != -1)
      {
      close(temp_file);
      remove(TEMP_FILE_NAME);
      temp_file = -1;
      }

   if ( msg != NULL )
      {
      helptitle();
      stopmsg(1, msg);
      }

   else if ( msg_func != NULL )
      msg_func((success) ? -1 : -2, info.num_page );

   EXIT_OVLY;
   }


int init_help(void)
   {
   struct help_sig_info hs;
   char 		path[81];

   ENTER_OVLY(OVLY_HELP);

   help_file = -1;

#ifdef TEST	/* leave this code out of the release version */
   /*
    * We don't worry about using far arrays for errors here since this code
    * won't be in the release version.
    */

   if ( find_file("FRACTINT.HLP", path) )
      {
      if ( (help_file = open(path, O_RDONLY|O_BINARY)) != -1 )
	 {
	 read(help_file, (char *)&hs, 6);

	 if ( hs.sig != HELP_SIG )
	    {
		static char far msg[] = {"Invalid help signature in FRACTINT.HLP!\n"};
	close(help_file);
	stopmsg(1, msg);
	    }

	 else if ( hs.version != HELP_VERSION )
	    {
	    static char far msg[] = {"Wrong help version in FRACTINT.HLP!\n"};
	    close(help_file);
	    stopmsg(1, msg);
	    }

	 else
	    base_off = 6;
	 }
      else
	 {
	     static char far msg[] =
		 {"Help system was unable to open FRACTINT.HLP!\n"};
	 stopmsg(1, msg);
	 }
      }

#endif

   if ( help_file == -1 )
      {
      static char far err_no_open[]    = "Help system was unable to open FRACTINT.EXE!\n";
      static char far err_no_exe[]     = "Help system couldn't find FRACTINT.EXE!\n";
      static char far err_not_in_exe[] = "Help not found in FRACTINT.EXE!\n";
      static char far err_wrong_ver[]  = "Wrong help version in FRACTINT.EXE!\n";

      if ( find_file("FRACTINT.EXE", path) )
	 {
	 if ( (help_file = open(path, O_RDONLY|O_BINARY)) != -1 )
	    {
	    long help_offset;
	    for (help_offset = -10L; help_offset >= -128L; help_offset--)
	       {
	       lseek(help_file, help_offset, SEEK_END);
	       read(help_file, (char *)&hs, 10);
	       if (hs.sig == HELP_SIG)  break;
	       }

	    if ( hs.sig != HELP_SIG )
	       {
	       close(help_file);
	       help_file = -1;
	       stopmsg(1, err_not_in_exe);
	       }

	    else if ( hs.version != HELP_VERSION )
	       {
	       close(help_file);
	       help_file = -1;
	       stopmsg(1, err_wrong_ver);
	       }

	    else
	       base_off = hs.base;

	    }
	 else
	    stopmsg(1, err_no_open);
	 }
      else
	 stopmsg(1, err_no_exe);
      }

   help_seek(0L);

   read(help_file, (char *)&max_pages, 2);
   read(help_file, (char *)&max_links, 2);
   read(help_file, (char *)&num_topic, 2);
   read(help_file, (char *)&num_label, 2);
   help_seek(12L);  /* skip num_contents and num_doc_pages */

   assert(max_pages > 0);
   assert(max_links >= 0);
   assert(num_topic > 0);
   assert(num_label > 0);

   /* allocate one big chunk for all three arrays */

   topic_offset = (long far *)farmemalloc(4L*num_topic + 4L*num_label + sizeof(HIST)*MAX_HIST);

   if (topic_offset == NULL)
      {
      static char far err_no_mem[] = "Not enough memory for help system!\n";
      close(help_file);
      help_file = -1;
      stopmsg(1, err_no_mem);

      EXIT_OVLY;      /* JIC stopmsg continues for some reason? */
      return (-2);
      }

   /* split off the other arrays */

   label = (LABEL far *)(&topic_offset[num_topic]);
   hist  = (HIST far *)(&label[num_label]);

   /* read in the tables... */

   farread(help_file, topic_offset, num_topic*4);
   farread(help_file, label, num_label*4);

   /* finished! */

   EXIT_OVLY;
   return (0);	/* success */
   }


void end_help(void)
   {
   ENTER_OVLY(OVLY_HELP);
   if (help_file != -1)
      {
      close(help_file);
      farmemfree((unsigned char far *)topic_offset);
      help_file = -1;
      }
   EXIT_OVLY;
   }


