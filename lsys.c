#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include "fractint.h"

extern int xdots,ydots;
extern int colors;
extern char LFileName[];
extern char LName[];
extern double param[];

int Lsystem();
extern int thinking(int,char *);

static float	  _fastcall getnumber(char far **);
static char far * _fastcall findsize(char far *,char far **,int);
static int	  _fastcall findscale(char far *, char far **, int);
static char far * _fastcall drawLSys(char far *, char far **, int);
static int	  _fastcall readLSystemFile(char *);
static void	  _fastcall free_rules_mem(void);
static int	  _fastcall save_rule(char *,char far **);

static float aspect; /* aspect ratio of each pixel, ysize/xsize */

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


static float far sins[50];
static float far coss[50];
static char maxangle,curcolor;
static float xpos,ypos,size,realangle;
static char counter,angle,reverse,stackoflow;
static float Xmin,Xmax,Ymin,Ymax;


static float _fastcall getnumber(char far **str)
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
      numstr[i++]=**str;
      (*str)++;
   }
   (*str)--;
   numstr[i]=0;
   ret=atof(numstr);
   if (root) ret=sqrt(ret);
   if (inverse) ret=1/ret;
   return ret;
}


static char far * _fastcall findsize(char far *command,char far **rules,int depth)
{
   char far **rulind,tran;

#ifndef __TURBOC__
   if (stackavail() < 400) { /* leave some margin for calling subrtns */
      stackoflow = 1;
      return NULL;
      }
#endif

   while ((*command)&&(*command!=']'))
   {
      if (!(counter++))
      {
	 /* let user know we're not dead */
	 if (thinking(1,"L-System thinking (higher orders take longer)"))
	 {
	    counter--;
	    return NULL;
	 }
      }
      tran=0;
      if (depth)
      {
	 for(rulind=rules;*rulind;rulind++)
	    if (**rulind==*command)
	    {
	       tran=1;
	       if (findsize((*rulind)+1,rules,depth-1) == NULL)
		  return(NULL);
	    }
      }
      if (!depth || !tran)
	 switch (*command)
	 {
	 case '+':
	    if (reverse)
	    {
	       if (++angle==maxangle)
		  angle=0;
	    }
	    else
	    {
	       if (angle)
		  angle--;
	       else
		  angle=maxangle-1;
	    }
	    break;
	 case '-':
	    if (reverse)
	    {
	       if (angle)
		  angle--;
	       else angle=maxangle-1;
	    }
	    else
	    {
	       if (++angle==maxangle)
		  angle=0;
	    }
	    break;
	 case '/':
	    if (reverse)
	    {
		 realangle-=getnumber(&command);
		 while (realangle<0) realangle+=360;
	    }
	    else
	    {
		 realangle+=getnumber(&command);
		 while (realangle>=360) realangle-=360;
	    }
	    break;
	 case '\\':
	    if (reverse)
	    {
		 realangle+=getnumber(&command);
		 while (realangle>=360) realangle-=360;
	    }
	    else
	    {
	    realangle-=getnumber(&command);
	    while (realangle<0) realangle+=360;
	    }
	    break;
	 case '@':
	    size*=getnumber(&command);
	    break;
	 case '|':
	    angle+=maxangle/2;
	    angle%=maxangle;
	    break;
	 case '!':
	    reverse=!reverse;
	    break;
	 case 'd':
	 case 'm':
	    xpos+=size*aspect*cos(realangle*PI/180);
	    ypos+=size*sin(realangle*PI/180);
	    if (xpos>Xmax) Xmax=xpos;
	    if (ypos>Ymax) Ymax=ypos;
	    if (xpos<Xmin) Xmin=xpos;
	    if (ypos<Ymin) Ymin=ypos;
	    break;
	 case 'g':
	 case 'f':
	    xpos+=size*coss[angle];
	    ypos+=size*sins[angle];
	    if (xpos>Xmax) Xmax=xpos;
	    if (ypos>Ymax) Ymax=ypos;
	    if (xpos<Xmin) Xmin=xpos;
	    if (ypos<Ymin) Ymin=ypos;
	    break;
	 case '[':
	    {
	       char saveang,saverev;
	       float savesize,saverang,savex,savey;

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
	    break;
	 }
      command++;
   }
   return command;
}


static int _fastcall findscale(char far *command, char far **rules, int depth)
{
   float horiz,vert;
   int i;
   char far *fsret;
   aspect=SCREENASPECT*xdots/ydots;
   for(i=0;i<maxangle;i++)
   {
      sins[i]=sin(2*i*PI/maxangle);
      coss[i]=aspect*cos(2*i*PI/maxangle);
   }
   xpos=ypos=Xmin=Xmax=Ymax=Ymin=angle=reverse=realangle=counter=0;
   size=1;
   fsret = findsize(command,rules,depth);
   thinking(0,NULL); /* erase thinking message if any */
   if (fsret == NULL)
      return(0);
   if (Xmax==Xmin)
      horiz=1E37;
   else
      horiz=(xdots-10)/(Xmax-Xmin);
   if (Ymax==Ymin)
      vert=1E37;
   else
      vert=(ydots-6)/(Ymax-Ymin);
   size=vert<horiz?vert:horiz;
   if (horiz==1E37)
      xpos=xdots/2;
   else
      xpos=-Xmin*(size)+5+((xdots-10)-(size)*(Xmax-Xmin))/2;
   if (vert==1E37)
      ypos=ydots/2;
   else
      ypos=-Ymin*(size)+3+((ydots-6)-(size)*(Ymax-Ymin))/2;
   return(1);
}

static char far * _fastcall drawLSys(char far *command,char far **rules,int depth)
{
   char far **rulind,tran;
   int lastx,lasty;

#ifndef __TURBOC__
   if (stackavail() < 400) { /* leave some margin for calling subrtns */
      stackoflow = 1;
      return NULL;
      }
#endif

   while (*command&&*command!=']')
   {
      if (!(counter++))
      {
	 if (check_key())
	 {
	    counter--;
	    return NULL;
	 }
      }
      tran=0;
      if (depth)
      {
	 for(rulind=rules;*rulind;rulind++)
	    if (**rulind==*command)
	    {
	       tran=1;
	       if (drawLSys((*rulind)+1,rules,depth-1) == NULL)
		  return(NULL);
	    }
      }
      if (!depth||!tran)
	 switch (*command)
	 {
	 case '+':
	    if (reverse)
	    {
	       if (++angle==maxangle) angle=0;
	    }
	    else
	    {
	       if (angle) angle--;
	       else angle=maxangle-1;
	    }
	    break;
	 case '-':
	    if (reverse)
	    {
	       if (angle) angle--;
	       else angle=maxangle-1;
	    }
	    else
	    {
	       if (++angle==maxangle) angle=0;
	    }
	    break;
	 case '/':
	    if (reverse)
	    {
		 realangle-=getnumber(&command);
		 while (realangle<0) realangle+=360;
	    }
	    else
	    {
		 realangle+=getnumber(&command);
		 while (realangle>=360) realangle-=360;
	    }
	    break;
	 case '\\':
	    if (reverse)
	    {
		 realangle+=getnumber(&command);
		 while (realangle>=360) realangle-=360;
	    }
	    else
	    {
	    realangle-=getnumber(&command);
	    while (realangle<0) realangle+=360;
	    }
	    break;
	 case '@':
	    size *= getnumber(&command);
	    break;
	 case '|':
	    angle+=maxangle/2;
	    angle%=maxangle;
	    break;
	 case '!':
	    reverse=!reverse;
	    break;
	 case 'd':
	    lastx=xpos;
	    lasty=ypos;
	    xpos+=size*aspect*cos(realangle*PI/180);
	    ypos+=size*sin(realangle*PI/180);
	    draw_line(lastx,lasty,(int)xpos,(int)ypos,curcolor);
	    break;
	 case 'm':
	    xpos+=size*aspect*cos(realangle*PI/180);
	    ypos+=size*sin(realangle*PI/180);
	    break;
	 case 'g':
	    xpos+=size*coss[angle];
	    ypos+=size*sins[angle];
	    break;
	 case 'f':
	    lastx=xpos;
	    lasty=ypos;
	    xpos+=size*coss[angle];
	    ypos+=size*sins[angle];
	    draw_line(lastx,lasty,(int)xpos,(int)ypos,curcolor);
	    break;
	 case '[':
	    {
	       char saveang,saverev,savecolor;
	       float savesize,saverang,savex,savey;

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
	    break;
	 case 'c':
	    curcolor=((int)getnumber(&command))%colors;
	    break;
	 case '>':
	    curcolor-=getnumber(&command);
	    if ((curcolor &= colors-1) == 0) curcolor = colors-1;
	    break;
	 case '<':
	    curcolor+=getnumber(&command);
	    if ((curcolor &= colors-1) == 0) curcolor = 1;
	    break;
	 }
      command++;
   }
   return command;
}


#define MAXRULES 27 /* this limits rules to 25 */
static char far *ruleptrs[MAXRULES];

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
   rulind=&ruleptrs[1];
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
	    check=1;
	 }
	 else if (!strcmp(word,"}"))
	    break;
	 else if (strlen(word)==1)
	 {
	    strcat(strcpy(fixed,word),strtok(NULL," \t\n"));
	    save_rule(fixed,rulind++);
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
   if ((!loaded)&&LLoad()) return (-1);
   order=param[0];
   if (order<=0) order=0;
   stackoflow = 0;
   if (findscale(ruleptrs[0],&ruleptrs[1],order)) {
      realangle=angle=reverse=0;
/* !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR */
      if ((curcolor=15) > colors) curcolor=colors-1;
      drawLSys(ruleptrs[0],&ruleptrs[1],order);
      }
   if (stackoflow)
   {
      static char far msg[]={"insufficient memory, try a lower order"};
      stopmsg(0,msg);
   }   
   free_rules_mem();
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

   for(i=0;i<maxangle;i++)
   {
      sins[i]=sin(2*i*PI/maxangle);
      coss[i]=aspect*cos(2*i*PI/maxangle);
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
   if((tmpfar=farmemalloc((long)i))==NULL) return -1;
   *saveptr=tmpfar;
   while(--i>=0) *(tmpfar++)=*(rule++);
   return 0;
}

