#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "prototyp.h"
#include "helpdefs.h"

void setwait(long *wait)
{
    char msg[80];
    int kbdchar;
    for(;;)
    {
       sprintf(msg,"Delay %4ld",*wait);
       while(strlen(msg) < 15)
          strcat(msg," ");
       msg[15] = 0;   
       showtempmsg((char far *)msg);
       kbdchar = getakey();
       switch(kbdchar)
       {
         case    RIGHT_ARROW_2:
         case    UP_ARROW_2:
            (*wait) += 100;
            break;
         case    RIGHT_ARROW:
         case    UP_ARROW:
            (*wait) += 10;
            break;
         case    DOWN_ARROW_2:
         case    LEFT_ARROW_2:
            (*wait) -= 100;
            break;
         case    LEFT_ARROW:
         case    DOWN_ARROW:
            (*wait) -= 10;
            break;
         default:
            cleartempmsg();
            return;   
       }
       if(*wait < 0)
          *wait = 0;
    }
}


int ant(void)
{
   int oldhelpmode;
   long maxpts;
   long count;
   int kbdchar;
   long step;
   long wait;
   char rule[256];
   int numcolors;
   static int x,y;
   int color;
   int direction=0;  /* 0=left 1=up 2=right 3=down */
   oldhelpmode = helpmode;
   helpmode = ANTCOMMANDS;
   sprintf(rule,"%.17g",param[0]);
   wait = step = abs(orbit_delay);
   maxpts = (long)param[1];
   maxpts = labs(maxpts);
   if(step == 1)
      wait = 0;
   else
      step = 0;   

   count = 0;   
   numcolors = strlen(rule);
   if(numcolors == 0)
   {
      stopmsg(0,"Empty rule string");
      goto exit_ant;
   }   
   for(color=0;color<numcolors;color++)
   {
       if(rule[color] != '1')
          rule[color] = '0';
       rule[color] = (char)(rule[color] - (int)'0'); 
   }
   x = xdots/2;
   y = ydots/2;

   for(;;)
   {
      color = getcolor(x,y);
      putcolor(x,y,15);
      kbdchar=keypressed();
      if(kbdchar || step)
      {
         int done = 0;
         if(kbdchar == 0)
            kbdchar = getakey();
         switch(kbdchar)
         {   
            case SPACE:
               step = 1 - step;
               break;
            case ESC:
               done = 1;
               break;   
            case RIGHT_ARROW:
            case UP_ARROW:
            case DOWN_ARROW:
            case LEFT_ARROW:
            case RIGHT_ARROW_2:
            case UP_ARROW_2:
            case DOWN_ARROW_2:
            case LEFT_ARROW_2:
               setwait(&wait);
               break;
            default:
               done = 1;
               break;   
         }               
         if(done)
            goto exit_ant;
         if(keypressed())
            getakey();   
      }
      if(wait > 0 && step == 0)
         sleepms(wait);
      if(rule[color%numcolors])
         direction++;
      else 
         direction--;
      if(direction < 0)
         direction = 3;            
      else if(direction > 3)
         direction = 0;
      putcolor(x,y,++color);
      switch(direction)
      {
         case 0:
            x--;
            break;
         case 1:
            y--;
            break;
         case 2:
            x++;
            break;
         case 3:
            y++;
            break;
      }      
      if(x < 0 || x >= xdots || y < 0 || y >= ydots)
         break;
      if(++count > maxpts)
         break;
   }
   exit_ant:
   helpmode = oldhelpmode;
   return(0);
}

