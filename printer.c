/* Printer.c
 *      Simple screen printing functions for FRACTINT
 *      By Matt Saucier CIS: [72371,3101]      7/2/89
 *      "True-to-the-spirit" of FRACTINT, this code makes few checks that you
 *      have specified a valid resolution for the printer (just in case yours
 *      has more dots/line than the Standard HP and IBM/EPSON,
 *      (eg, Wide Carridge, etc.))
 *
 *      This code supports printers attached to a LPTx (1-3) parallel port.
 *      It also now supports serial printers AFTER THEY ARE CONFIGURED AND
 *      WORKING WITH THE DOS MODE COMMAND, eg. MODE COM1:9600,n,8,1 (for HP)
 *      Printing calls are made directly to the BIOS for DOS can't handle fast
 *      transfer of data to the HP.  (Or maybe visa-versa, HP can't handle the
 *      slow transfer of data from DOS)
 *
 *      Supported Printers:     Tested Ok:
 *       HP LaserJet
 *          LJ+,LJII             MDS
 *       Toshiba PageLaser       MDS (Set FRACTINT to use HP)
 *       IBM Graphics            MDS
 *       EPSON
 *          Models?              Untested.
 *
 *      Future support to include OKI 20 (color) printer, and just about
 *      any printer you request.
 *
 *      Future modifications to include a more flexible, standard interface
 *      with the surrounding program, for easier portability to other
 *      programs.
 *
 * Compile with  MSC 5.1 or TurboC 2.0 (or QC?? Someone PLEASE Verify this)
 *
 * Tabs at 8.
 */
 
#include <bios.h>
#include <dos.h>
#include <stdarg.h>
#include "fractint.h"
 
/********      PROTOTYPES     ********/
 
int Printer_printf(int LPTn,char *fmt,...);
int Printer(int a,int b,int c);
 
/********  EXTRN GLOBAL VARS  ********/
 
extern int xdots,ydots,                /* size of screen                 */
           extraseg;                   /* used for buffering             */
 
/********       GLOBALS       ********/
 
int Printer_Resolution,      /* 75,100,150,300 for HP; 60,120,240 for IBM*/
    LPTNumber,                 /* ==1,2,3 LPTx; or 11,12,13,14 for COM1-4*/
    Printer_Type;                      /* ==1 HP,  ==2 IBM/EPSON         */
 
 
#ifdef __TURBOC__
    #define         _bios_printer(a,b,c)    biosprint((a),(c),(b))
    #define         _bios_serialcom(a,b,c)  bioscom((a),(c),(b))
#endif
 
void Print_Screen()
{
    register int y,j;                  /* indices                        */
    char buff[192];                    /* buffer for 192 sets of pixels  */
                                     /* This is very large so that we can*/
                                       /* get reasonable times printing  */
                                       /* from modes like 2048x2048 disk-*/
                                      /* video.  When this was 24, a 2048*/
                                      /* by 2048 pic took over 2 hours to*/
                                     /* print.  It takes about 15 min now*/
    int BuffSiz;                       /* how much of buff[] we'll use   */
    char far *es;                      /* pointer to extraseg for buffer */
    int i,x,k,                          /* more indices                   */
        imax,                          /* maximum i value (ydots/8)      */
        res,                           /* resolution we're gonna' use    */
        LPTn,                          /* printer number we're gonna use */
        COM,                         /* if LPTn>10 COM == com port to use*/
        high, low,                     /* misc                           */
                                     /************************************/
        ptrid;                         /* Printer Id code.               */
                                       /* Currently, the following are   */
                                       /* assigned:                      */
                                       /*            1.  HPLJ (all)      */
                                      /*                Toshiba PageLaser*/
                                       /*            2.  IBM Graphics    */
                                       /*            3.  Color Printer   */
                                     /************************************/
 
                                     /********   SETUP VARIABLES  ********/
    for (j=0;j<192;j++) {
        buff[j]=0;                     /* Clear buffer                   */
        }

    if (extraseg) {
        #ifdef __TURBOC__
            es=MK_FP(extraseg,0);
        #else
            FP_SEG(es)=extraseg;
            FP_OFF(es)=0;
        #endif
        }
    LPTn=LPTNumber-1;
    if (((LPTn<10)&&(LPTn>2))||
        (LPTn<0)||(LPTn>13)) LPTn=0;   /* default of LPT1 (==0)          */
    ptrid=Printer_Type;
    if ((ptrid<1)||(ptrid>3)) ptrid=2; /* default of IBM/EPSON           */
    res=Printer_Resolution;
    switch (ptrid) {
        case 1: if (res<75) res=75;
            if ( (res<= 75)&&(ydots> 600)) res=100;
            if ( (res<=100)&&(ydots> 800)) res=150;
            if (((res<=150)&&(ydots>1200))||(res>300)) res=300;
            break;
        case 2:
        case 3:
            if (res<60) res=60;
            if ((res<=60)&&(ydots>480)) res=120;
            if (((res<=120)&&(ydots>960))||(res>240)) res=240;
            break;
            }
 
         /*****  Set up buffer size for immediate user gratification *****/
         /*****    AKA, if we don't have to, don't buffer the data   *****/
    BuffSiz=8;
    if (xdots>1024) BuffSiz=192;
 
                                       /*****   Initialize printer  **** */
    printer(1,LPTn,0);
 
                                /******  INITIALIZE GRAPHICS MODES  ******/
    switch (ptrid) {
        case 1:
            Printer_printf(LPTn,"\x1B*t%iR\x1B*r0A",res);/* HP           */
            break;
        case 2:
        case 3:
            Printer_printf(LPTn,"\x1B\x33\x18");/* IBM                   */
            break;
            }
 
    if (keypressed()) return;       /* one last chance before we start...*/
 
                                       /*****  Get And Print Screen **** */
    switch (ptrid) {
        case 1: {                      /* HP LaserJet (et al)            */
                imax=(ydots/8)-1;
                for (x=0;((x<xdots)&&(!keypressed()));x+=BuffSiz) {
                    for (i=imax;((i>=0)&&(!keypressed()));i--) {
                        for (y=7;((y>=0)&&(!keypressed()));y--) {
                            for (j=0;((j<BuffSiz)&&(!keypressed()));j++) {
                                if ((x+j)<xdots) {
                                    buff[j]<<=1;
                                    buff[j]+=(getcolor(x+j,i*8+y)&1);
                                    }
                                }
                            }
                        for (j=0;j<BuffSiz;j++) {
                            *(es+j+BuffSiz*i)=buff[j];
                            buff[j]=0;
                            }
                        }
                    for (j=0;((j<BuffSiz)&&(!keypressed()));j++) {
                        if ((x+j)<xdots) {
                            Printer_printf(LPTn,"\x1B*b%iW"
                            ,imax+1);
                            for (i=imax;((i>=0)&&(!keypressed()));i--) {
                                printer(0,LPTn,*(es+j+BuffSiz*i));
                                }
                            }
                        }
                    }
                if (!keypressed()) Printer_printf(LPTn,"\x1B*rB\x0C");
                break;
                }
        case 2: {                      /* IBM Graphics/Epson             */
                for (x=0;((x<xdots)&&(!keypressed()));x+=8) {
                    switch (res) {
                        case 60:  Printer_printf(LPTn,"\x1BK"); break;
                        case 120: Printer_printf(LPTn,"\x1BL"); break;
                        case 240: Printer_printf(LPTn,"\x1BZ"); break;
                            }
                    high=ydots/256;
                    low=ydots-(high*256);
                    printer(0,LPTn,low);
                    printer(0,LPTn,high);
                    for (y=ydots-1;(y>=0);y--) {
                        buff[0]=0;
                        for (i=0;i<8;i++) {
                            buff[0]<<=1;
                            buff[0]+=(getcolor(x+i,y)&1);
                            }
                        printer(0,LPTn,buff[0]);
                        }
                    if (keypressed()) break;
                    printer(0,LPTn,13);
                    printer(0,LPTn,10);
                    }
                if (!keypressed()) printer(0,LPTn,12);
                break;
                }
        case 3: {                      /* IBM Graphics/Epson Color       */
                high=ydots/256;
                low=ydots%256;
                for (x=0;((x<xdots)&&(!keypressed()));x+=8)
                    {
                    for (k=0; k<8; k++)  /* colors */
                        {
                        Printer_printf(LPTn,"\x1Br%d",k); /* set printer color */
                        switch (res)
                            {
                            case 60:  Printer_printf(LPTn,"\x1BK"); break;
                            case 120: Printer_printf(LPTn,"\x1BL"); break;
                            case 240: Printer_printf(LPTn,"\x1BZ"); break;
                            }
                        printer(0,LPTn,low);
                        printer(0,LPTn,high);
                        for (y=ydots-1;y>=0;y--)
                            {
                            buff[0]=0;
                            for (i=0;i<8;i++)
                                {
                                buff[0]<<=1;
                                if ((getcolor(x+i,y)%8)==k)
                                    buff[0]++;
                                }
                            printer(0,LPTn,buff[0]);
                            }
                        printer(0,LPTn,13);
                        }
                    printer(0,LPTn,10);
                    }
                printer(0,LPTn,12);
                printer(0,LPTn,12);
                printer(1,LPTn,0);  /* reset */
                break;
                }
            }
}
 
/* This function prints a string to the the printer with BIOS calls.
 * It returns the printer's status.
 */
int Printer_printf(int LPTn,char *fmt,...)
{
char s[500];
int x=0;
va_list arg;
va_start(arg,fmt);
vsprintf(s,fmt,arg);
while ((s[x])&&(!keypressed())) {
    if (printer(0,LPTn,s[x++])!=0) {
        while (!keypressed()) {
            if (printer(0,LPTn,s[x-1])==0) break;
            }
        }
    }
return (printer(2,LPTn,0));
}
 
/* This function standardizes both _bios_printer and _bios_serialcom
 * in one function.  It takes it's arguments and re-aranges them and calls
 * the appropriate bios call.  It the return result !=0, there is a problem
 * with the printer.
 */
int printer(int a,int b,int c)
{
    if (b<9) return (((_bios_printer(a,b,c))+0x0010)&0x0010);
    return ((_bios_serialcom(((a!=0)?3:1),(b-10),c))&0x9E00);
}
