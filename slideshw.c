/***********************************************************************/
/* These routines are called by getakey to allow keystrokes to control */
/* Fractint to be read from a file.				       */
/***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "fractint.h"

int slideshw(void);
int startslideshow(void);
void stopslideshow(void);
void recordshw(int key);

static void sleep(int);
static int  showtempmsg_txt(int,int,int,int,char *);
static void message(int secs, char far *buf);
static void slideshowerr(char far *msg);
static int  get_scancode(char *mn);
static char far *get_mnemonic(int code);

struct scancodes
{
   int code;
   char far *mnemonic;
};
static struct scancodes far scancodes[] =
{
   {  ENTER,	  "ENTER"       },
   {  INSERT,	  "INSERT"      },
   {  DELETE,	  "DELETE"      },
   {  ESC,	  "ESC"         },
   {  TAB,	  "TAB"         },
   {  PAGE_UP,	  "PAGEUP"      },
   {  PAGE_DOWN,  "PAGEDOWN"    },
   {  HOME,	  "HOME"        },
   {  END,	  "END"         },
   {  LEFT_ARROW, "LEFT"        },
   {  RIGHT_ARROW,"RIGHT"       },
   {  UP_ARROW,   "UP"          },
   {  DOWN_ARROW, "DOWN"        },
   {  F1,	  "F1"          },
   {  -1,	  NULL		}
};
#define stop sizeof(scancodes)/sizeof(struct scancodes)-1

static int get_scancode(char *mn)
{
   int i;
   i = 0;
   for(i=0;i< stop;i++)
      if(far_strcmp((char far *)mn,scancodes[i].mnemonic)==0)
	 break;
   return(scancodes[i].code);
}

static char far *get_mnemonic(int code)
{
   int i;
   i = 0;
   for(i=0;i< stop;i++)
      if(code == scancodes[i].code)
	 break;
   return(scancodes[i].mnemonic);
}
#undef stop

char busy = 0;
static FILE *fp = NULL;
extern int slides;
extern int text_type;
extern int calc_status;
extern char autoname[];
static long starttick;
static long ticks;
static int slowcount;
static unsigned int quotes;
static char calcwait = 0;
static int repeats = 0;
static int last = 0;
static char far smsg[] = "MESSAGE";
static char far sgoto[] = "GOTO";
static char far scalcwait[] = "CALCWAIT";
static char far swait[] = "WAIT";

/* places a temporary message on the screen in text mode */
static int showtempmsg_txt(int row, int col, int attr,int secs,char *txt)
{
   int savescrn[80];
   int i;
   if(text_type > 1)
      return(1);
   for(i=0;i<80;i++)
   {
      movecursor(row,i);
      savescrn[i] = get_a_char();
   }
   putstring(row,col,attr,txt);
   movecursor(25,80);
   sleep(secs);
   for(i=0;i<80;i++)
   {
      movecursor(row,i);
      put_a_char(savescrn[i]);
   }
   return(0);
}

static void message(int secs, char far *buf)
{
   int i;
   char nearbuf[41];
   i = -1;
   while(buf[++i] && i< 40)
      nearbuf[i] = buf[i];
   nearbuf[i] = 0;
   if(text_type < 2)
      showtempmsg_txt(0,0,7,secs,nearbuf);
   else if (showtempmsg(nearbuf) == 0)
      {
	 sleep(secs);
	 cleartempmsg();
      }
}

/* this routine reads the file autoname and returns keystrokes */
int slideshw()
{
   int out,err,i;
   char buffer[81];
   if(calcwait)
   {
      if(calc_status == 1 || busy) /* restart timer - process not done */
	 return(0); /* wait for calc to finish before reading more keystrokes */
      calcwait = 0;
   }
   if(fp==NULL)   /* open files first time through */
      if(startslideshow()==0)
	 {
	 stopslideshow();
	 return (0);
	 }

   if(ticks) /* if waiting, see if waited long enough */
   {
      if(clock() - starttick < ticks) /* haven't waited long enough */
	 return(0);
      ticks = 0;
   }
   if (++slowcount <= 18)
   {
      starttick = clock();
      ticks = CLK_TCK/5; /* a slight delay so keystrokes are visible */
      if (slowcount > 10)
	 ticks /= 2;
   }
   if(repeats>0)
   {
      repeats--;
      return(last);
   }
start:
   if(quotes) /* reading a quoted string */
   {
      if((out=fgetc(fp)) != '\"' && out != EOF)
	 return(last=out);
      quotes = 0;
   }
   /* skip white space: */
   while ((out=fgetc(fp)) == ' ' || out == '\t' || out == '\n') { }
   switch(out)
   {
      case EOF:
	 stopslideshow();
	 return(0);
      case '\"':        /* begin quoted string */
	 quotes = 1;
	 goto start;
      case ';':         /* comment from here to end of line, skip it */
	 while((out=fgetc(fp)) != '\n' && out != EOF) { }
	 goto start;
      case '*':
	 if (fscanf(fp,"%d",&repeats) != 1
	   || repeats <= 1 || repeats >= 256 || feof(fp))
	 {
	    static char far msg[] = "error in * argument";
	    slideshowerr(msg);
	    last = repeats = 0;
	 }
	 repeats -= 2;
	 return(out = last);
   }

   i = 0;
   while(1) /* get a token */
   {
      if(i < 80)
	 buffer[i++] = out;
      if((out=fgetc(fp)) == ' ' || out == '\t' || out == '\n' || out == EOF)
	 break;
   }
   buffer[i] = 0;
   if(buffer[i-1] == ':')
      goto start;
   out = -12345;
   if(isdigit(buffer[0]))	/* an arbitrary scan code number - use it */
	 out=atoi(buffer);
   else if(far_strcmp((char far *)buffer,smsg)==0)
      {
	 int secs;
	 out = 0;
	 if (fscanf(fp,"%d",&secs) != 1)
	 {
	    static char far msg[] = "MESSAGE needs argument";
	    slideshowerr(msg);
	 }
	 else
	 {
	    int len;
	    char buf[41];
	    buf[40] = 0;
	    fgets(buf,40,fp);
	    len = strlen(buf);
	    buf[len-1]=0; /* zap newline */
	    message(secs,(char far *)buf);
	 }
	 out = 0;
      }
   else if(far_strcmp((char far *)buffer,sgoto)==0)
      {
	 if (fscanf(fp,"%s",buffer) != 1)
	 {
	    static char far msg[] = "GOTO needs target";
	    slideshowerr(msg);
	    out = 0;
	 }
	 else
	 {
	    char buffer1[80];
	    rewind(fp);
	    strcat(buffer,":");
	    do
	    {
	       err = fscanf(fp,"%s",buffer1);
	    } while( err == 1 && strcmp(buffer1,buffer) != 0);
	    if(feof(fp))
	    {
	       static char far msg[] = "GOTO target not found";
	       slideshowerr(msg);
	       return(0);
	    }
	    goto start;
	 }
      }
   else if((i = get_scancode(buffer)) > 0)
	 out = i;
   else if(far_strcmp(swait,(char far *)buffer)==0)
      {
	 float fticks;
	 err = fscanf(fp,"%f",&fticks); /* how many ticks to wait */
	 fticks *= CLK_TCK;		/* convert from seconds to ticks */
	 if(err==1)
	 {
	    ticks = fticks;
	    starttick = clock();	/* start timing */
	 }
	 else
	 {
	    static char far msg[] = "WAIT needs argument";
	    slideshowerr(msg);
	 }
	 slowcount = out = 0;
      }
   else if(far_strcmp(scalcwait,(char far *)buffer)==0) /* wait for calc to finish */
      {
	 calcwait = 1;
	 slowcount = out = 0;
      }
   else if((i=check_vidmode_keyname(buffer)))
      out = i;
   if(out == -12345)
   {
      char msg[80];
      sprintf(msg,"Can't understand %s",buffer);
      slideshowerr(msg);
      out = 0;
   }
   return(last=out);
}

startslideshow()
{
   if((fp=fopen(autoname,"r"))==NULL)
      slides = 0;
   ticks = 0;
   quotes = 0;
   calcwait = 0;
   slowcount = 0;
   return(slides);
}

void stopslideshow()
{
   if(fp)
      fclose(fp);
   fp = NULL;
   slides = 0;
}

void recordshw(int key)
{
   char far *mn;
   float dt;
   int i;
   dt = ticks;	   /* save time of last call */
   ticks=clock();  /* current time */
   if(fp==NULL)
      if((fp=fopen(autoname,"w"))==NULL)
	 return;
   dt = ticks-dt;
   dt /= CLK_TCK;  /* dt now in seconds */
   if(dt > .5) /* don't bother with less than half a second */
   {
      if(quotes) /* close quotes first */
      {
	 quotes=0;
	 fprintf(fp,"\"\n",dt);
      }
      fprintf(fp,"WAIT %4.1f\n",dt);
   }
   if(key >= 32 && key < 128)
   {
      if(!quotes)
      {
	 quotes=1;
	 fputc('\"',fp);
      }
      fputc(key,fp);
   }
   else
   {
      if(quotes) /* not an ASCII character - turn off quotes */
      {
	 fprintf(fp,"\"\n");
	 quotes=0;
      }
      if((mn=get_mnemonic(key)) != NULL)
	  fprintf(fp,"%Fs",mn);
      else if (check_vidmode_key(0,key) >= 0)
	 {
	    char buf[10];
	    vidmode_keyname(key,buf);
	    fprintf(fp,buf);
	 }
      else /* not ASCII and not FN key */
	 fprintf(fp,"%4d",key);
      fputc('\n',fp);
   }
}

/* suspend process # of seconds */
static void sleep(int secs)
{
   long stop;
   stop = clock() + (long)secs*CLK_TCK;
   while(clock() < stop && kbhit() == 0) { } /* bailout if key hit */
}

static void slideshowerr(char far *msg)
{
   char msgbuf[300];
   static char far errhdg[] = "Slideshow error:\n";
   stopslideshow();
   far_strcpy(msgbuf,errhdg);
   far_strcat(msgbuf,msg);
   stopmsg(0,msgbuf);
}

