/* TPLUS.H, (C) 1991 The Yankee Programmer
   All Rights Reserved
   
   This code may be distributed only when bundled with the Fractint 
   source code.
   
   Mark C. Peterson
   The Yankee Programmer
   405-C Queen Street, Suite #181
   Southington, CT 06489
   (203) 276-9721
   
*/

#ifndef TPLUS_H
#define TPLUS_H

struct TPWrite {
   unsigned 
      COLOR0,  COLOR1,  COLOR2,  COLOR3,  VIDCON,  INDIRECT,   HUESAT,
      OVSTRT,  MASKL,   MASKH,   LBNK,    HBNK,    MODE1,      MODE2,    
      WBL,     WBH;
};

struct TPRead {
   unsigned
      VIDSTAT,          CTL,     MASKL,   LBNK,    READAD,     MODE1,       
      OVSTRT,  USCAN,   MASKH,   OSCAN,   HBNK,    ROWCNT,     MODE2,
      RBL,     RBH;
};

/* TPlus Register Data Offsets */
#define     XDOTS       0
#define     YDOTS       1
#define     ASP_RATIO   2
#define     DEPTH       4
#define     PAGE        5
#define     BOTBANK     7
#define     TOPBANK     8
#define     ZOOM        10
#define     DISPMODE    11
#define     DAC567DATA  15
#define     VTOP        19
#define     TOP         51
#define     BOT         53
#define     VPAN        55
#define     NOT_INT     56
#define     HIRES       61
#define     PERM        64
#define     BYCAP       68
#define     PE          69
#define     OVLE        70
#define     HPAN        71
#define     MEM_BASE    73
#define     MEM_MAP     74
#define     LIVEMIXSRC  91
#define     RGB         93
#define     SVIDEO      97
#define     BUFFPORTSRC 98
#define     CM1         101
#define     CM2         102
#define     LUTBYPASS   107
#define     CM3         113
#define     LIVEPORTSRC 115
#define     LIVE8       116
#define     VGASRC      123

struct _BOARD {
   int ThisBoard, ClearScreen;
   unsigned char far *Screen;
   unsigned VerPan, HorPan, Top, Bottom;
   unsigned xdots, ydots, Bank64k, RowBytes, RowsPerBank;
   unsigned Reg[128];
   void (*Plot)(unsigned x, unsigned y, unsigned long Color);
   unsigned long (*GetColor)(unsigned x, unsigned y);

   struct TPRead Read;
   struct TPWrite Write;
};

extern struct _BOARD far TPlus;

int
   CheckForTPlus(void),
   FillTPlusRegion(unsigned fx, unsigned fy, unsigned xdots, unsigned ydots,
                   unsigned long Color),
   SetBoard(int BoardNumber),
   SetColorDepth(int Depth),
   SetTPlusMode(int TMode, int NotIntFlag, int Depth, int Zoom),
   SetVGA_LUT(void),
   TPlusLUT(unsigned char far *LUTData, unsigned Index, unsigned Number,
            unsigned DosFlag);

void ClearTPlusScreen(void);
void SetTPlusModeLinear(void);
unsigned long ReadTPlusBankedPixel(unsigned x, unsigned y);
void WriteTPlusBankedPixel(unsigned x, unsigned y, unsigned long Color);
void TPlusZoom(int Zoom);


#endif