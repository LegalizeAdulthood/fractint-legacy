/* Log.c by Mark C. Peterson 8/14/89 */

#include "fmath.h"

extern void (*plot)(int x, int y, int Color);
extern int	xdots, ydots;		/* coordinates of dots on the screen  */
extern int	colors;				/* maximum colors available */
extern int  maxit;
extern char far *farmemalloc(long bytestoalloc);
extern void farmemfree(char far *farptr);

char far *LogTable = (char far *)0;
extern int LogFlag;
float fColors;

void ChkLogFlag(void) {
   float l, f, c = (float)256.0, m;
   unsigned n, x;

   if(!LogFlag) {
      if(LogTable) {
         farmemfree(LogTable);
         LogTable = (char far *)0;
      }
   }
   else {
      if(!LogTable) {
         if(LogTable = farmemalloc((long)maxit + 1)) {
            Fg2Float((long)maxit, 0, m);
            fLog14(m, m);
            fDiv(c, m, c);
            for(n = 1; n < maxit; n++) {
               Fg2Float((long)n, 0, f);
               fLog14(f, l);
               fMul16(l, c, l);
               LogTable[n] = Float2Fg(l, 0) + 1;
            }
            LogTable[0] = 0;
         }
         else
            buzzer(2);
      }
   }
}

