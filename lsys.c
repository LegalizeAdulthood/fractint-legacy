#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __TURBOC__
#include <alloc.h>
#elif !defined(__386BSD__)
#include <malloc.h>
#endif
#include "fractint.h"
#include "prototyp.h"

#define size	ssize
/* Needed for use of asm -- helps decide which pointer to function
 * to put into the struct lsys_cmds.
 */
extern int cpu;
extern int fpu;
extern int debugflag;
extern int xdots,ydots;
extern int colors;
extern char LFileName[];
extern char LName[];
extern double param[];
extern int overflow;
extern float  screenaspect;

struct lsys_cmd {
  char ch;
  void (*f)(long n);
  long n;
};

static double	  _fastcall getnumber(char far **);
static struct lsys_cmd far * _fastcall findsize(struct lsys_cmd far *,struct lsys_cmd far **,int);
static int	  _fastcall findscale(struct lsys_cmd far *, struct lsys_cmd far **, int);
static struct lsys_cmd far * _fastcall drawLSys(struct lsys_cmd far *, struct lsys_cmd far **, int);
static int	  _fastcall readLSystemFile(char *);
static void	  _fastcall free_rules_mem(void);
static int	  _fastcall save_rule(char *,char far **);
static struct lsys_cmd far *SizeTransform(char far *s);
static struct lsys_cmd far *DrawTransform(char far *s);
static void free_lcmds();

static long aspect; /* aspect ratio of each pixel, ysize/xsize */

/* Some notes to Adrian from PB, made when I integrated with v15:
     printfs changed to work with new user interface
     bug at end of readLSystemFile, the line which said rulind=0 fixed
       to say *rulind=0
     the calloc was not worthwhile, it was just for a 54 byte area, cheaper
       to keep it as a static;	but there was a static 201 char buffer I
       changed to not be static
     use of strdup was a nono, caused problems running out of space cause
       the memory allocated each time was never freed; I've changed to
       use far memory and to free when done
   */

long sins[50];
long coss[50];
/* dmaxangle is maxangle - 1. */
char maxangle,dmaxangle,curcolor;
long size;
unsigned long realangle;
long xpos,ypos;
char counter,angle,reverse,stackoflow;
long lsys_Xmin, lsys_Xmax, lsys_Ymin, lsys_Ymax;

/* Macro to take an FP number and turn it into a
 * 16/16-bit fixed-point number.
 */
#define FIXEDMUL	524288L
#define FIXEDPT(x)	((long) (FIXEDMUL * (x)))

/* The number by which to multiply sines, cosines and other
 * values with magnitudes less than or equal to 1.
 * sins and coss are a 3/29 bit fixed-point scheme (so the
 * range is +/- 2, with good accuracy.	The range is to
 * avoid overflowing when the aspect ratio is taken into
 * account.
 */
#define FIXEDLT1	536870912.0

/* Multiply by this number to convert an unsigned 32-bit long
 * to an angle from 0-2PI.
 */
#define ANGLE2DOUBLE	(2*PI / 4294967296.)

static int ispow2(long n)
{
  return (n == (n & -n));
}


#ifdef XFRACT
static void lsys_doplus(long n)
{
  if (reverse) {
    if (++angle == maxangle)
      angle = 0;
  }
  else {
    if (angle)
      angle--;
    else
      angle = dmaxangle;
  }
}
#endif


#ifdef XFRACT
/* This is the same as lsys_doplus, except maxangle is a power of 2. */
static void lsys_doplus_pow2(long n)
{
  if (reverse) {
    angle++;
    angle &= dmaxangle;
  }
  else {
    angle--;
    angle &= dmaxangle;
  }
}
#endif


#ifdef XFRACT
static void lsys_dominus(long n)
{
  if (reverse) {
    if (angle)
      angle--;
    else
      angle = dmaxangle;
  }
  else {
    if (++angle == maxangle)
      angle = 0;
  }
}
#endif


#ifdef XFRACT
static void lsys_dominus_pow2(long n)
{
  if (reverse) {
    angle--;
    angle &= dmaxangle;
  }
  else {
    angle++;
    angle &= dmaxangle;
  }
}
#endif

static void lsys_doslash(long n)
{
  if (reverse)
    realangle -= n;
  else
    realangle += n;
}
#ifdef XFRACT
#define lsys_doslash_386 lsys_doslash
#endif

static void lsys_dobslash(long n)
{
  if (reverse)
    realangle += n;
  else
    realangle -= n;
}

#ifdef XFRACT
#define lsys_dobslash_386 lsys_dobslash
#endif

static void lsys_doat(long n)
{
  size = multiply(size, n, 19);
}

#ifdef XFRACT
#define lsys_doat_386 lsys_doat
#endif

static void lsys_dopipe(long n)
{
  angle += maxangle / 2;
  angle %= maxangle;
}


#ifdef XFRACT
static void lsys_dopipe_pow2(long n)
{
  angle += maxangle >> 1;
  angle &= dmaxangle;
}
#endif


#ifdef XFRACT
static void lsys_dobang(long n)
{
  reverse = ! reverse;
}
#endif

static void lsys_dosizedm(long n)
{
  double angle = (double) realangle * ANGLE2DOUBLE;
  double s, c;
  long fixedsin, fixedcos;
  FPUsincos(&angle, &s, &c);
  fixedsin = (long) (s * FIXEDLT1);
  fixedcos = (long) (c * FIXEDLT1);

  xpos += multiply(multiply(size, aspect, 19), fixedcos, 29);
  ypos += multiply(size, fixedsin, 29);

/* xpos+=size*aspect*cos(realangle*PI/180);  */
/* ypos+=size*sin(realangle*PI/180);         */
  if (xpos>lsys_Xmax) lsys_Xmax=xpos;
  if (ypos>lsys_Ymax) lsys_Ymax=ypos;
  if (xpos<lsys_Xmin) lsys_Xmin=xpos;
  if (ypos<lsys_Ymin) lsys_Ymin=ypos;
}

static void lsys_dosizegf(long n)
{
  xpos += multiply(size, (long) coss[angle], 29);
  ypos += multiply(size, (long) sins[angle], 29);
/* xpos+=size*coss[angle];                   */
/* ypos+=size*sins[angle];                   */
  if (xpos>lsys_Xmax) lsys_Xmax=xpos;
  if (ypos>lsys_Ymax) lsys_Ymax=ypos;
  if (xpos<lsys_Xmin) lsys_Xmin=xpos;
  if (ypos<lsys_Ymin) lsys_Ymin=ypos;
}

#ifdef XFRACT
#define lsys_dosizegf_386 lsys_dosizegf
#endif

static void lsys_dodrawd(long n)
{
  double angle = (double) realangle * ANGLE2DOUBLE;
  double s, c;
  long fixedsin, fixedcos;
  int lastx, lasty;
  FPUsincos(&angle, &s, &c);
  fixedsin = (long) (s * FIXEDLT1);
  fixedcos = (long) (c * FIXEDLT1);

  lastx=(int) (xpos >> 19);
  lasty=(int) (ypos >> 19);
  xpos += multiply(multiply(size, aspect, 19), fixedcos, 29);
  ypos += multiply(size, fixedsin, 29);
/* xpos+=size*aspect*cos(realangle*PI/180);   */
/* ypos+=size*sin(realangle*PI/180);          */
  draw_line(lastx,lasty,(int)(xpos >> 19),(int)(ypos>>19),curcolor);
}

static void lsys_dodrawm(long n)
{
  double angle = (double) realangle * ANGLE2DOUBLE;
  double s, c;
  long fixedsin, fixedcos;
  FPUsincos(&angle, &s, &c);
  fixedsin = (long) (s * FIXEDLT1);
  fixedcos = (long) (c * FIXEDLT1);

/* xpos+=size*aspect*cos(realangle*PI/180);   */
/* ypos+=size*sin(realangle*PI/180);          */
  xpos += multiply(multiply(size, aspect, 19), fixedcos, 29);
  ypos += multiply(size, fixedsin, 29);
}

static void lsys_dodrawg(long n)
{
  xpos += multiply(size, (long) coss[angle], 29);
  ypos += multiply(size, (long) sins[angle], 29);
/* xpos+=size*coss[angle];                    */
/* ypos+=size*sins[angle];                    */
}

#ifdef XFRACT
#define  lsys_dodrawg_386 lsys_dodrawg
#endif

static void lsys_dodrawf(long n)
{
  int lastx = (int) (xpos >> 19);
  int lasty = (int) (ypos >> 19);
  xpos += multiply(size, (long) coss[angle], 29);
  ypos += multiply(size, (long) sins[angle], 29);
/* xpos+=size*coss[angle];                    */
/* ypos+=size*sins[angle];                    */
  draw_line(lastx,lasty,(int)(xpos>>19),(int)(ypos>>19),curcolor);
}

static void lsys_dodrawc(long n)
{
  curcolor = ((int) n) % colors;
}

static void lsys_dodrawgt(long n)
{
  curcolor -= n;
  if ((curcolor &= colors-1) == 0)
    curcolor = colors-1;
}

static void lsys_dodrawlt(long n)
{
  curcolor += n;
  if ((curcolor &= colors-1) == 0)
    curcolor = 1;
}

static double _fastcall getnumber(char far **str)
{
   char numstr[30];
   float ret;
   int i,root,inverse;

   root=0;
   inverse=0;
   strcpy(numstr,"");
   (*str)++;
   switch (**str)
   {
   case 'q':
      root=1;
      (*str)++;
      break;
   case 'i':
      inverse=1;
      (*str)++;
      break;
   }
   switch (**str)
   {
   case 'q':
      root=1;
      (*str)++;
      break;
   case 'i':
      inverse=1;
      (*str)++;
      break;
   }
   i=0;
   while (**str<='9' && **str>='0' || **str=='.')
   {
      numstr[i++]= **str;
      (*str)++;
   }
   (*str)--;
   numstr[i]=0;
   ret=atof(numstr);
   if (root)
     ret=sqrt(ret);
   if (inverse)
     ret = 1/ret;
   return ret;
}

static struct lsys_cmd far * _fastcall findsize(struct lsys_cmd far *command, struct lsys_cmd far **rules, int depth)
{
   struct lsys_cmd far **rulind;
   int tran;

if (overflow)     /* integer math routines overflowed */
    return NULL;

#ifndef __TURBOC__
   if (stackavail() < 400) { /* leave some margin for calling subrtns */
      stackoflow = 1;
      return NULL;
      }
#endif

   while (command->ch && command->ch !=']') {
      if (! (counter++)) {
	 /* let user know we're not dead */
	 if (thinking(1,"L-System thinking (higher orders take longer)")) {
	    counter--;
	    return NULL;
	 }
      }
      tran=0;
      if (depth) {
	 for(rulind=rules;*rulind;rulind++)
	    if ((*rulind)->ch==command->ch) {
	       tran=1;
	       if (findsize((*rulind)+1,rules,depth-1) == NULL)
		  return(NULL);
	    }
      }
      if (!depth || !tran) {
	if (command->f)
	  (*command->f)(command->n);
	else if (command->ch == '[') {
	  char saveang,saverev;
	  long savesize,savex,savey;
	  unsigned long saverang;

	  saveang=angle;
	  saverev=reverse;
	  savesize=size;
	  saverang=realangle;
	  savex=xpos;
	  savey=ypos;
	  if ((command=findsize(command+1,rules,depth)) == NULL)
	     return(NULL);
	  angle=saveang;
	  reverse=saverev;
	  size=savesize;
	  realangle=saverang;
	  xpos=savex;
	  ypos=savey;
	}
      }
      command++;
   }
   return command;
}

static int _fastcall findscale(struct lsys_cmd far *command, struct lsys_cmd far **rules, int depth)
{
   float horiz,vert;
   double xmin, xmax, ymin, ymax;
   double locsize;
   double locaspect;
   int i;
   struct lsys_cmd far *fsret;
   locaspect=screenaspect*xdots/ydots;
   aspect = FIXEDPT(locaspect);
   for(i=0;i<maxangle;i++) {
      sins[i]=(long) ((sin(2*i*PI/maxangle)) * FIXEDLT1);
      coss[i]=(long) ((locaspect * cos(2*i*PI/maxangle)) * FIXEDLT1);
   }
   xpos=ypos=lsys_Xmin=lsys_Xmax=lsys_Ymax=lsys_Ymin=angle=reverse=realangle=counter=0;
   size=FIXEDPT(1);
   fsret = findsize(command,rules,depth);
   thinking(0, NULL); /* erase thinking message if any */
   xmin = (double) lsys_Xmin / FIXEDMUL;
   xmax = (double) lsys_Xmax / FIXEDMUL;
   ymin = (double) lsys_Ymin / FIXEDMUL;
   ymax = (double) lsys_Ymax / FIXEDMUL;
   locsize = (double) size / FIXEDMUL;
   if (fsret == NULL)
      return 0;
   if (xmax == xmin)
      horiz = 1E37;
   else
      horiz = (xdots-10)/(xmax-xmin);
   if (ymax == ymin)
      vert = 1E37;
   else
      vert = (ydots-6) /(ymax-ymin);
   locsize = (vert<horiz) ? vert : horiz;

   if (horiz == 1E37)
      xpos = FIXEDPT(xdots/2);
   else
      xpos = FIXEDPT(-xmin*(locsize)+5+((xdots-10)-(locsize)*(xmax-xmin))/2);
   if (vert == 1E37)
      ypos = FIXEDPT(ydots/2);
   else
      ypos = FIXEDPT(-ymin*(locsize)+3+((ydots-6)-(locsize)*(ymax-ymin))/2);
   size = FIXEDPT(locsize);
   return 1;
}

static struct lsys_cmd far * _fastcall drawLSys(struct lsys_cmd far *command,struct lsys_cmd far **rules,int depth)
{
   struct lsys_cmd far **rulind;
   int tran;

if (overflow)     /* integer math routines overflowed */
    return NULL;

#ifndef __TURBOC__
   if (stackavail() < 400) { /* leave some margin for calling subrtns */
      stackoflow = 1;
      return NULL;
      }
#endif

   while (command->ch && command->ch !=']') {
      if (!(counter++)) {
	 if (check_key()) {
	    counter--;
	    return NULL;
	 }
      }
      tran=0;
      if (depth) {
	 for(rulind=rules;*rulind;rulind++)
	    if ((*rulind)->ch == command->ch) {
	       tran=1;
	       if (drawLSys((*rulind)+1,rules,depth-1) == NULL)
		  return NULL;
	    }
      }
      if (!depth||!tran) {
	if (command->f)
	  (*command->f)(command->n);
	else if (command->ch == '[') {
	  char saveang,saverev,savecolor;
	  long savesize,savex,savey;
	  unsigned long saverang;

	  saveang=angle;
	  saverev=reverse;
	  savesize=size;
	  saverang=realangle;
	  savex=xpos;
	  savey=ypos;
	  savecolor=curcolor;
	  if ((command=drawLSys(command+1,rules,depth)) == NULL)
	     return(NULL);
	  angle=saveang;
	  reverse=saverev;
	  size=savesize;
	  realangle=saverang;
	  xpos=savex;
	  ypos=savey;
	  curcolor=savecolor;
	}
      }
      command++;
   }
   return command;
}

#define MAXRULES 27 /* this limits rules to 25 */
static char far *ruleptrs[MAXRULES];
static struct lsys_cmd far *rules2[MAXRULES];

static int _fastcall readLSystemFile(char *str)
{
   int c;
   char far **rulind;
   int err=0;
   int linenum,check=0;
   char inline[161],fixed[161],*word;
   FILE *infile;
   char msgbuf[481]; /* enough for 6 full lines */

   if (find_file_item(LFileName,str,&infile) < 0)
      return -1;
   while ((c = fgetc(infile)) != '{')
      if (c == EOF) return -1;

   maxangle=0;
   for(linenum=0;linenum<MAXRULES;++linenum) ruleptrs[linenum]=NULL;
   rulind= &ruleptrs[1];
   msgbuf[0]=linenum=0;

   while(fgets(inline,160,infile))  /* Max line length 160 chars */
   {
      linenum++;
      if ((word = strchr(inline,';'))) /* strip comment */
	 *word = 0;
      strlwr(inline);

      if (strspn(inline," \t\n") < strlen(inline)) /* not a blank line */
      {
	 word=strtok(inline," =\t\n");
	 if (!strcmp(word,"axiom"))
	 {
	    save_rule(strtok(NULL," \t\n"),&ruleptrs[0]);
	    check=1;
	 }
	 else if (!strcmp(word,"angle"))
	 {
	    maxangle=atoi(strtok(NULL," \t\n"));
	    dmaxangle = maxangle - 1;
	    check=1;
	 }
	 else if (!strcmp(word,"}"))
	    break;
	 else if (strlen(word)==1)
	 {
	    char *tok;
	    tok = strtok(NULL, " \t\n");
	    strcpy(fixed, word);
	    if (tok != NULL) {     /* Some strcat's die if they cat with NULL */
		strcat(fixed, tok);
	    }
	    save_rule(fixed, rulind++);
	    check=1;
	 }
	 else
	    if (err<6)
	    {
	       sprintf(&msgbuf[strlen(msgbuf)],
		       "Syntax error line %d: %s\n",linenum,word);
	       ++err;
	    }
	 if (check)
	 {
	    check=0;
	    if(word=strtok(NULL," \t\n"))
	       if (err<6)
	       {
		  sprintf(&msgbuf[strlen(msgbuf)],
			 "Extra text after command line %d: %s\n",linenum,word);
		  ++err;
	       }
	 }
      }
   }
   fclose(infile);
   if (!ruleptrs[0] && err<6)
   {
      strcat(msgbuf,"Error:  no axiom\n");
      ++err;
   }
   if ((maxangle<3||maxangle>50) && err<6)
   {
      strcat(msgbuf,"Error:  illegal or missing angle\n");
      ++err;
   }
   if (err)
   {
      msgbuf[strlen(msgbuf)-1]=0; /* strip trailing \n */
      stopmsg(0,msgbuf);
      return -1;
   }
   *rulind=NULL;
   return 0;
}

static char loaded=0;

int Lsystem()
{
   int order;
   char far **rulesc;
   struct lsys_cmd far **sc;

   if ( (!loaded) && LLoad())
     return -1;

   overflow = 0;		/* reset integer math overflow flag */

   order=param[0];
   if (order<=0)
     order=0;
   stackoflow = 0;

   sc = rules2;
   for (rulesc = ruleptrs; *rulesc; rulesc++)
     *sc++ = SizeTransform(*rulesc);
   *sc = NULL;

   if (findscale(rules2[0], &rules2[1], order)) {
      realangle = angle = reverse = 0;

      free_lcmds();
      sc = rules2;
      for (rulesc = ruleptrs; *rulesc; rulesc++)
	*sc++ = DrawTransform(*rulesc);
      *sc = NULL;

      /* !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR */
      if ((curcolor=15) > colors) curcolor=colors-1;
      drawLSys(rules2[0], &rules2[1], order);
   }
   if (stackoflow)
   {
      static char far msg[]={"insufficient memory, try a lower order"};
      stopmsg(0,msg);
   }
   if (overflow) {
      static char far msg[]={"Integer math routines failed, try a lower order"};
      stopmsg(0,msg);
      }
   free_rules_mem();
   free_lcmds();
   loaded=0;
   return 0;
}

int LLoad()
{
   char i;
   if (readLSystemFile(LName)) { /* error occurred */
      free_rules_mem();
      loaded=0;
      return -1;
   }
   for(i=0;i<maxangle;i++) {
      sins[i]=(long) ((sin(2*i*PI/maxangle)) * FIXEDLT1);
      coss[i]=(long) (((double) aspect / (double) FIXEDMUL * cos(2*i*PI/maxangle)) * FIXEDLT1);
   }
   loaded=1;
   return 0;
}

static void _fastcall free_rules_mem()
{
   int i;
   for(i=0;i<MAXRULES;++i)
      if(ruleptrs[i]) farmemfree(ruleptrs[i]);
}

static int _fastcall save_rule(char *rule,char far **saveptr)
{
   int i;
   char far *tmpfar;
   i=strlen(rule)+1;
   if((tmpfar=farmemalloc((long)i))==NULL) {
       stackoflow = 1;
       return -1;
       }
   *saveptr=tmpfar;
   while(--i>=0) *(tmpfar++)= *(rule++);
   return 0;
}

static struct lsys_cmd far *SizeTransform(char far *s)
{
  struct lsys_cmd far *ret;
  struct lsys_cmd far *doub;
  int maxval = 10;
  int n = 0;
  void (*f)(long);
  long num;

  void (*plus)(long) = (ispow2(maxangle)) ? lsys_doplus_pow2 : lsys_doplus;
  void (*minus)(long) = (ispow2(maxangle)) ? lsys_dominus_pow2 : lsys_dominus;
  void (*pipe)(long) = (ispow2(maxangle)) ? lsys_dopipe_pow2 : lsys_dopipe;

  void (*slash)(long) =  (cpu == 386) ? lsys_doslash_386 : lsys_doslash;
  void (*bslash)(long) = (cpu == 386) ? lsys_dobslash_386 : lsys_dobslash;
  void (*at)(long) =     (cpu == 386) ? lsys_doat_386 : lsys_doat;
  void (*dogf)(long) =   (cpu == 386) ? lsys_dosizegf_386 : lsys_dosizegf;

  ret = (struct lsys_cmd far *) farmemalloc((long) maxval * sizeof(struct lsys_cmd));
  if (ret == NULL) {
       stackoflow = 1;
       return NULL;
       }
  while (*s) {
    f = NULL;
    num = 0;
    ret[n].ch = *s;
    switch (*s) {
      case '+': f = plus;            break;
      case '-': f = minus;           break;
      case '/': f = slash;           num = (long) (getnumber(&s) * 11930465L);    break;
      case '\\': f = bslash;         num = (long) (getnumber(&s) * 11930465L);    break;
      case '@': f = at;              num = FIXEDPT(getnumber(&s));    break;
      case '|': f = pipe;            break;
      case '!': f = lsys_dobang;     break;
      case 'd':
      case 'm': f = lsys_dosizedm;   break;
      case 'g':
      case 'f': f = dogf;       break;
      case '[': num = 1;        break;
      case ']': num = 2;        break;
      default:
	num = 3;
	break;
    }
    ret[n].f = f;
    ret[n].n = num;
    if (++n == maxval) {
      doub = (struct lsys_cmd far *) farmemalloc((long) maxval*2*sizeof(struct lsys_cmd));
      if (doub == NULL) {
         farmemfree(ret);
         stackoflow = 1;
         return NULL;
         }
      far_memcpy(doub, ret, maxval*sizeof(struct lsys_cmd));
      farmemfree(ret);
      ret = doub;
      maxval <<= 1;
    }
    s++;
  }
  ret[n].ch = 0;
  ret[n].f = NULL;
  ret[n].n = 0;
  n++;

  doub = (struct lsys_cmd far *) farmemalloc((long) n*sizeof(struct lsys_cmd));
  if (doub == NULL) {
       farmemfree(ret);
       stackoflow = 1;
       return NULL;
       }
  far_memcpy(doub, ret, n*sizeof(struct lsys_cmd));
  farmemfree(ret);
  return doub;
}

static struct lsys_cmd far *DrawTransform(char far *s)
{
  struct lsys_cmd far *ret;
  struct lsys_cmd far *doub;
  int maxval = 10;
  int n = 0;
  void (*f)(long);
  long num;

  void (*plus)(long) = (ispow2(maxangle)) ? lsys_doplus_pow2 : lsys_doplus;
  void (*minus)(long) = (ispow2(maxangle)) ? lsys_dominus_pow2 : lsys_dominus;
  void (*pipe)(long) = (ispow2(maxangle)) ? lsys_dopipe_pow2 : lsys_dopipe;

  void (*slash)(long) =  (cpu == 386) ? lsys_doslash_386 : lsys_doslash;
  void (*bslash)(long) = (cpu == 386) ? lsys_dobslash_386 : lsys_dobslash;
  void (*at)(long) =     (cpu == 386) ? lsys_doat_386 : lsys_doat;
  void (*drawg)(long) =  (cpu == 386) ? lsys_dodrawg_386 : lsys_dodrawg;

  ret = (struct lsys_cmd far *) farmemalloc((long) maxval * sizeof(struct lsys_cmd));
  if (ret == NULL) {
       stackoflow = 1;
       return NULL;
       }
  while (*s) {
    f = NULL;
    num = 0;
    ret[n].ch = *s;
    switch (*s) {
      case '+': f = plus;            break;
      case '-': f = minus;           break;
      case '/': f = slash;           num = (long) (getnumber(&s) * 11930465L);    break;
      case '\\': f = bslash;         num = (long) (getnumber(&s) * 11930465L);    break;
      case '@': f = at;              num = FIXEDPT(getnumber(&s));    break;
      case '|': f = pipe;            break;
      case '!': f = lsys_dobang;     break;
      case 'd': f = lsys_dodrawd;    break;
      case 'm': f = lsys_dodrawm;    break;
      case 'g': f = drawg;           break;
      case 'f': f = lsys_dodrawf;    break;
      case 'c': f = lsys_dodrawc;    num = getnumber(&s);    break;
      case '<': f = lsys_dodrawlt;   num = getnumber(&s);    break;
      case '>': f = lsys_dodrawgt;   num = getnumber(&s);    break;
      case '[': num = 1;        break;
      case ']': num = 2;        break;
      default:
	num = 3;
	break;
    }
    ret[n].f = f;
    ret[n].n = num;
    if (++n == maxval) {
      doub = (struct lsys_cmd far *) farmemalloc((long) maxval*2*sizeof(struct lsys_cmd));
      if (doub == NULL) {
           farmemfree(ret);
           stackoflow = 1;
           return NULL;
           }
      far_memcpy(doub, ret, maxval*sizeof(struct lsys_cmd));
      farmemfree(ret);
      ret = doub;
      maxval <<= 1;
    }
    s++;
  }
  ret[n].ch = 0;
  ret[n].f = NULL;
  ret[n].n = 0;
  n++;

  doub = (struct lsys_cmd far *) farmemalloc((long) n*sizeof(struct lsys_cmd));
  if (doub == NULL) {
       farmemfree(ret);
       stackoflow = 1;
       return NULL;
       }
  far_memcpy(doub, ret, n*sizeof(struct lsys_cmd));
  farmemfree(ret);
  return doub;
}

static void free_lcmds()
{
  struct lsys_cmd far **sc = rules2;

  while (*sc)
    farmemfree(*sc++);
}

