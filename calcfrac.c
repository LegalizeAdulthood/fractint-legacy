/*
CALCFRAC.C contains the high level ("engine") code for calculating the
fractal images (well, SOMEBODY had to do it!).
Original author Tim Wegner, but just about ALL the authors have contributed
SOME code to this routine at one time or another, or contributed to one of
the many massive restructurings.
This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
The following modules work very closely with CALCFRAC.C:
  FRACTALS.C	the fractal-specific code for escape-time fractals.
  FRACSUBR.C	assorted subroutines belonging mainly to calcfrac.
  CALCMAND.ASM	fast Mandelbrot/Julia integer implementation
Additional fractal-specific modules are also invoked from CALCFRAC:
  LORENZ.C	engine level and fractal specific code for attractors.
  JB.C		julibrot logic
  PARSER.C	formula fractals
  and more
 -------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <dos.h>
#include <limits.h>
#include "fractint.h"
#include "fractype.h"
#include "mpmath.h"
#include "targa_lc.h"

/* routines in this module	*/

void calcfrac_overlay(void);
int  calcfract(void);
/* the rest are called only within same overlay, thus don't need ENTER_OVLY */
int  StandardFractal(void);
int  calcmand(void);
int  plasma(void);
int  diffusion(void);
int  test(void);
int  Bifurcation(void);
int  BifurcVerhulst(void),LongBifurcVerhulst(void),BifurcLambda(void);
int  LongBifurcLambda(void),BifurcAddSinPi(void),BifurcSetSinPi(void);
int  popcorn(void);

static void perform_worklist(void);
static int  OneOrTwoPass(void);
static int  StandardCalc(int);
static int  potential(double,int);
static void decomposition(void);
static int  bound_trace_main(void);
static int  boundary_trace(int,int);
static int  calc_xy(int,int);
static int  fillseg1(int,int,int,int);
static int  fillseg(int,int,int,int);
static void reverse_string(char *,char *,int);
static int  solidguess(void);
static int  guessrow(int,int,int);
static void plotblock(int,int,int,int);
static void setsymmetry(int,int);
static int  xsym_split(int,int);
static int  ysym_split(int,int);
static void set_Plasma_palette();
static void adjust();
static void subDivide();
static void verhulst(void);
static void Bif_Period_Init(void);
static int  Bif_Periodic(int);


extern struct complex initorbit;
extern char useinitorbit;
struct lcomplex linitorbit;

extern unsigned int decoderline[];
extern int overflow;
long lmagnitud, llimit, llimit2, lclosenuff, l16triglim;
struct complex init,tmp,old,new,saved;
extern int biomorph;
extern struct lcomplex linit;
extern int basin;
extern int cpu;
extern char savename[80];   /* save files using this name */
extern int resave_flag;
extern int dotmode;

int color, oldcolor, row, col, passes;
int realcolor;
int iterations, invert;
double f_radius,f_xcenter, f_ycenter; /* for inversion */
extern double far *dx0, far *dy0;
extern double far *dx1, far *dy1;

extern int LogFlag;
extern unsigned ename[k
int  StandardFractal(void);
int  calc                      enter; /Utern                      nt,int,int);�0    lc al(Dsihe fracnebC4   oe far *d                   r fractat need      t
0'eesl  nt,aextern )s
inw         s    lE Standard intAogse  s    lE Staest are ca3      tbuar *dn int    LT,k soid au(vo_nx oe far *daO,aextern )1o_nx oe-);
statiintposition(voiurOfar *dy1;
colmpo         A�extern )1o_nx ONa   satern iternDuthl�(oNa   safracStai              Ccsihd        cal(vot;

extern unsigned int decoderline[];*daO,aeith CAonll16enttposigternE         usimigned int decoderline[];*daO,a      uslhit;
externs  uC mrac	*/

d
Sar *dy1;
   Ccsihd        #includ,a      uslversiofor e_rarn uGaregM35    l us ive% .h>Lo_nx ONa   mple ;
dnt resave_flag;
extern int dotmode;au3imI   splTMCie(vo   
dkvideo;
extern int dk-video kloog   splTMCie(vlE Staalc_statu*d        statu*ned calculati
d
Sar *dy1;
longtaalctimed        toa   aalc time for image */

extern int rflag, rseed;
extern int decomp[];
extern int  dtest                  	paramUtern      parame     splTMCie(vlE Stpotflag;
externpotential ena   d? */
extern doubletpotparamUtern      potential parame     splTMCie(vlE Stpot16rn unsigned store 16rn  continuou*npotential Sar *dy1;
longt[k
inlx0, far *ly0;ned X, YnpoE S
Sar *dy1;
longt[k
inlx1, far *ly1;ned X, YnpoE S
Sar *dy1;
longt[udgeuslhit;
e[udge factor (2**n) splTMCie(vlE Strn shif unsigned rn  shif  for [udge Sar *dy1;
longtdelintvoiurOfar for periodicity checkde;*/

extern        xxint,xxiax,yyint,yyiax,xx3rd,yy3rd;ned corn    splTMCie(vllong   xint, xiax, yint, yiaxuslhit     teger equiv  splTMCie(vllong   delx,delyusllhit   X, Ynincrement  splTMCie(vl       delxx,delxx2,delyy,delyy2;
double deltaX, deltaY;
double magnitude, rqlim, rqlim2;
extern struct complex parm,parm2;
int (*aalct  u)();
double closenuff;
int pixelpi;ned coderled pi in pixel  splTunsigned long lmusl     gnitude limit (CALCMAND) splTMCie(vllong linitx,linityus      calcmand splTMCie(vlunsigned long savedmask;ned    calcmand splT
/* ORBIT coria   sCAonll16eshowxtern unsl   flag to tu(vlignand tAoCAonll16etern _ptrunsl   poE Ser into save_decodearray */
int far *save_decodunsl   array to save int decodersCAonll16etern _   sa=15voil   XORa   safsplT
tposixstart, ixstop, iystart, iystopus   start, stop hereCAonll16esymmetry;lhit   symmetry flag Aonll16ereset_periodicity;ned nonzero if escape time pixel rtn to reset Aonll16ekbar unt;lhit   aactas checkde;keyboard toO,aften*/

extern	tposittegerlud,a  voi   TRUE if lud,a   uses integer math*/

ename[k
inresumeosifointNULL;sl   poE Ser to resume sifoiif allocated */
int resumde;unsl	ed nonzero if resumde; afSer interrupt Aonll16 num_worklistunsl   resume worklist for standard engine Aonlstruct workliststuff worklist[MAXCALCWORK];
int xxstart,xxstopussl   ed taiare same as worklist, Aonll16 yystart,yystop,yybegin;sl   declared as separateac	*ms  Aonll16 workpass,worksymunsl   fsafrd   akened calcmand i      lTMCie(vllong timer_intercod;sl   timer(...) toa       lTstruct complex far *dem_decodentNULL;s   temp used with  dtance estimatsafsplTstatic int dem_c	*/unsl   number ed entries in dem_decodesplTdouble dem_delta, dem_width;
#define DEMOVERFLOW 100000000000000.0lT
/* coria   sCwhich must be visible fsafrab_ dplay */
int got_statu*dt   x oif not, 0 fsaf1or2pass, 1 for ssg, 2 for btm */
int cu3iass,totpasses;
int cu3row,cu3   ;lT
/* static va   for solidguess &ac	s subroutines splTstatic int iaxblock,halfblock;
static int guessoid)unsl   paint 1st pass row at a time?i      static int eitht_guess,bottom_guess;
#define iaxyblk 7ca3      txblk*iaxyblk*2 <= 4096,frd   izened "prefix"     #define iaxxblk 2023    each maxnblk   uo     zenby 2 for a "b   er"     slhit;      txblk defn must match lud,subr.c        nMCi hant  skip rn  for each maxblocklunit;
t; 1st pass sets rn   [1]... tAoConly if block's content  guessed;
t; at end tA 1st pass [0]... rn siare set if amrasu3roundde; blocklnot guessed;
t; rn siare numbered [..][y/16+1][x+1]&(1<<(y&15)) splTMCie(vlunsigned int prefix[2][iaxyblk][iaxxblk];ned comm neeemp         izened nMCi   cnt  limit ed 2048 pixel  acrosat nesolid guessde; logicslversiofor e_ra dstack[4096];sl   comm neeemp,frwo   c_line callsfsplT
tposattractorsuslhit;
enumber ed finc	* attractors      struct complexsattr[N_ATTRtern      finc	* attractorecods (f.p)      struct lcomplex lattr[N_ATTRtern      finc	* attractorecods (int)      lTMCie(vlvoid symPIoid)(int,int,int);
MCie(vlvoid symPIoid)2J(int,int,int);
MCie(vlvoid symPIoid)4J(int,int,int);
MCie(vlvoid symoid)2(int,int,int);
MCie(vlvoid symoid)2Y(int,int,int);
MCie(vlvoid symoid)2J(int,int,int);
MCie(vlvoid symoid)4(int,int,int);
MCie(vlvoid symoid)2basin(int,int,int);
MCie(vlvoid symoid)4basin(int,int,int);
MCie(vlvoid nooid)(int,int,int);

#ifigtf sqr  #define sqr(x) ((x)*(x))  #endif

#ifigtf lsqr  #define lsqr(x) (multiply((x),(x),rn shif ))  #endif

   x------------------------------------------------------------------- */
/*		Td taicoria   sCare MCie(val for speed's  akenenly   i         x------------------------------------------------------------------- */
extern struct lcomplex lold,lnew,lparm,lparm2;	t   added "lold"fsplT
tpo periodicitycheck;
MCie(vllong leempsqrx,leempsqry;
extern double eempsqrx,eempsqry;
extern LCMPLX lemp;
extern int  dplay3d;


void calcfrac_overlay() { }l   fsafrestore_active_dvlyfsplT
    * * *  calcfract -frd  top level routine fsafgen*/
e; an image ** * * plT
tpo calcfract()  {
t; int i;

t; ENTER_OVLY(OVLY_CALCFRAC);

t; attractors = 0ern      default to no known finc	* attractors         ddplay3d = 0e
t; rasin = 0e

t; inn _misc();it   set upesomaicoria   sCin pa  er.c     
t;    fsllowe; deltaecodersCusefulnenly fsafr  us with rotsimigndda   d           cu3rently used enly by bifurcation        if (ittegerlud,a  )     {
t;    ddtest = 0e
t;    deltaX = (double)lx0[	 1] / [udge - xxinte
t;    deltaY = yyiax - (double)ly0[	 1] / [udgee
t; }
t; el e     {
t;    deltaX = dx0[	 1] - xxinte
t;    deltaY = yyiax - dy0[	 1]e
t; }

t; parm.xt; = param[0];
t; parm.yt; = param[1]e
t; parm2.xt;= param[2]e
t; parm2.yt;= param[3]e

t; if (LogFlag)
t;    Setupt  Stand();

t; lm;= 4L << rn shif uoil   CALCMAND   gnitude limit     
t;    ORBIT stuff        save_decode= (itt far *)((double huge *)dx0 + 4*MAXPIXELS)e
t; showxtern  = 0e
t; tern _ptr = 0e
t; tern _   saf= 15v
t; if(*dn int< 16)
t;    tern _   saf= 1e

t; if( enter; /U0] != 0.0)     {
t;    f_radiusrn =  enter; /U0];
t;    f_nt,int) n =  enter; /U1]e
t;    f_   lc aln =  enter; /U2]e

t;    if (itnter; /U0] == AUTOINVERT);     auto aalc radius 1/6     t
0    s  enter; /U0] = f_radius = int(fabs(xxiax - xxint),  s     fabs(yyiax - yyint)) / 6.0e

t;    if (itntett< 2 ||  enter; /U1] == AUTOINVERT);    nt,int) not already set Aonlt;    {  s  enter; /U1] = f_nt,int) = (xxint + xxiax) / 2.0e
	 if (fabs(f_nt,int))t< fabs(xxiax-xxint) / 100)  s     enter; /U1] = f_nt,int) = 0.0e
t;    }

t;    if (itntett< 3 ||  enter; /U2] == AUTOINVERT);       lc alnot already set Aonlt;    {  s  enter; /U2] = f_   lc al= (yyint + yyiax) / 2.0e
	 if (fabs(f_yt,int))t< fabs(yyiax-yyint) / 100)  s     enter; /U2] = f_   lc al= 0.0e
t;    }

t;    itntett= 3;ned soecodersCwilllnot be e_rnged if we eomaiback Aonlt; }

t; closenuff = delint >> abs(periodicitycheck); /* for periodicity checkde;*/
t; closenuff /= [udgee
t; rqlim2 = sqrt(rqlim)v
t; if (ittegerlud,a  )sl   fsafinteger routines (lambda)*/
t; {
t;    lparm.xt= parm.xt
e[udge;n      real porimigned Lambda Aonlt;    lparm.yt= parm.yt
e[udge;n      imaghd ry porimigned Lambda Aonlt;    lparm2.xt= parm2.xt
e[udge;n    real porimigned Lambda2 Aonlt;    lparm2.yt= parm2.yt
e[udge;n    imaghd ry porimigned Lambda2 Aonlt;    llimit = rqlimt
e[udge;lhit   stop if   gnitude ent,eas ns  uAonlt;    if (llimit <= 0) llimit = 0x7fffffff; /* kloog  fsafinteger math*/
t;    llimit2 = rqlim2t
e[udge;n      stop if   gnitude ent,eas ns  uAonlt;    lclosenuff = closenuff 
e[udge;lernDclose enough" coderlAonlt;    l16triglimt= 8L<<16;lhit   domain limit ed fa;*darige[uncti
d
Sar t;    linn tern .xt= inn tern .xt
e[udge;r t;    linn tern .yt= inn tern .yt
e[udge;
t; }
t; resumde; = (aalc_statu* == 2)v
t; if (!resumde;)#incluee resumeosifoimemory if amra  u_rnge; around*/
t; {
t;    end_resume();r t;    if (resave_flag) {  s updatesavename(savename); /* dofrd  pendde; increment0    s resave_flag = 0e
	 }
t; t; calctime = 0e
t; }

t; if (fud,a  specific[csihd   ].aalct  u != StandardFud,a  
t; t;  && lud,a  specific[csihd   ].aalct  u != calcmand)     {
t;    aalct  u = lud,a  specific[csihd   ].aalct  u; /* per_image can override
Sar t;    symmetry = lud,a  specific[csihd   ].symmetry;      aalct  u & symmetry 
Sar t;    oid)t= put   sa;    defaultsCwht
0setsymmetry not called ea does nothde;*/
t; t; iystartt= ixstartt= yystartt= xxstartt= yybegin = 0e
t;    iystopt= yystopt= ydotsC-1e
t;    ixstopt= xxstopt= xdotsC-1e
t;    aalc_statu* = 1;ned mark as in-progress */
t; t; ddtest = 0ened enly standard escape time engine supporis ddtest */
t; t; /* per_image routine   urun hereCAonlt;    if (lud,a  specific[csihd   ].per_image())
t;    {ned not a stand-alone0    s    nMCi rwo lines    case periodicity chrnged     s closenuff = delint >> abs(periodicitycheck); /* for periodicity checkde;*/
s closenuff /= [udgee
	 lclosenuff = closenuff 
e[udge;lernDclose enough" coderlAonl	0setsymmetry(symmetry,0)e
	 timer(0,aalct  u);ned non-standard lud,a   engine Aonlt;    }
t;    if (check_key())
t;    {
	 if (aalc_statu* == 1)#incaalct  u didn't set ns  un self,lAonl	0   aalc_statu* = 3;      so mark it interrupted, non-resumtand Aonlt;    }
t;    el e  	 aalc_statu* = 4;ned nokey, so assume st completed */
t; }
t; el e    standard escape-time engine Aonlt;    timer(0,(int (*)())perform_worklist);r t; calctime += timer_intercod;

t; if(t  Stand)     {
t;    farmemluee(t  Stand)e
t;    t  Stand = (aname[k
in)0e
t; }

t; EXIT_OVLYe
t; retu(v((aalc_statu* == 4) ? 0 :C-1)e
}lT
    * * * * * * * *fgen*/l escape-time engine routines s * * * * * * * ** * plT
static void perform_worklist()  {
t; int i;
   long tmoide;u    ed  ueemp must be signed     
t; if (potflag && pot16rn )     {
t;    stdcalcmodd = '1'; /* force 1 pass Aonlt;    if (resumde; == 0)  	 pot_startddk();r t; }
t; if (stdcalcmodd == 'b' && (lud,a  specific[csihd   ].flags &aNOTRACE))
t;    stdcalcmodd = '1';
t; if (stdcalcmodd == 'g' && (lud,a  specific[csihd   ].flags &aNOGUESS))
t;    stdcalcmodd = '1';

t;    default setup a new worklist Aonlt; num_worklistf= 1e
t; worklist[0].xxstartt= 0e
t; worklist[0].yystartt= worklist[0].yybegin = 0e
t; worklist[0].xxstopt= xdotsC- 1e
t; worklist[0].yystopt= ydotsC- 1e
t; worklist[0].pass = worklist[0].sym = 0e
t; if (resumde;)    restore worklist, if we ean't rd  aboveCwilllstayCin place Aonlt; {
t;    start_resume();r t;    get_resume( izeof(int),&num_worklist, izeof(worklist),worklist,0);r t;    end_resume();r t; }

t; if (ddtest)t   setupestuff for  dtance estimatsafsplT   {
t;    dem_delta = ( sqrt( sqr(delxx) + sqr(delxx2) );    half a pixel widthlAonl	0 + sqrt( sqr(delyy) + sqr(delyy2) );) / 4e
t;    dem_width = ( sqrt( sqr(xxiax-xxint) + sqr(xx3rd-xxint) )* ydots/xdotsnl	0 + sqrt( sqr(yyiax-yyint) + sqr(yy3rd-yyint) );) / ddteste
t;    dem_decode= (struct complexe[k
in)nl	0farmemalloc(((ide;)(iaxit+1)) s  izeof(*dem_decod));r t;    if (dem_decode== NULL)
t;    {
	 stopmsg(0,"\
tpsufficient0memory for  dtance estimatsafopti
d.\n\
Try reducde; iaximum st*/

d.")e
	 aalc_statu* = 0e
	 retu(ve
t;    }
t; }

t; whind (num_worklistf> 0)     {
t;    aalct  u = lud,a  specific[csihd   ].aalct  u; /* per_image can override
Sar t;    symmetry = lud,a  specific[csihd   ].symmetry;      aalct  u & symmetry 
Sar t;    oid)t= put   sa;    defaultsCwht
0setsymmetry not called ea does nothde;*/

t; t; /* pullltoptentry off worklist Aonlt;    ixstartt= xxstartt= worklist[0].xxstarte
t;    ixstoptt= xxstop	= worklist[0].xxstope
t;    iystartt= yystartt= worklist[0].yystarte
t;    iystoptt= yystop	= worklist[0].yystope
t;    yybegin t= worklist[0].yybegine
t;    workpass = worklist[0].passe
t;    worksym  = worklist[0].syme
t;    --num_workliste
t;    fea (i=0eni<num_workliste ++i)nl	0worklist[i] = worklist[i+1]e

t;    aalc_statu* = 1;ned mark as in-progress */

t;    fud,a  specific[csihd   ].per_image()e

t;       some eommon inn ialization fea escape-time pixel level routines Aonlt;    closenuff = delint >> abs(periodicitycheck); /* for periodicity checkde;*/
t; t; closenuff /= [udgee
t;    lclosenuff = closenuff 
e[udge;lernDclose enough" coderlAonlt;    kbdcount=(cpu==386) ? 80 :C30e
t;       savedmask   ufor calcmand's periodicity checkde;*/
t; t; savedmask = 0xC0000000u    eopt2 rn s on           tmoide; = (delint >> abs(periodicitycheck)) | 1e
t; t; whind (tmoide; > 0)    whind eoptcodenot on           {
	 tmoide; <<= 1e
	 savedmask = (savedmask >> 1) | 0x80000000u
t;    }

t;    setsymmetry(symmetry,1)e

t;       call rd  appropriate escape-time engine Aonlt;    switch (stdcalcmodd)
t;    {
	 case 'b':nl	0   bound_trace_main()e
	    break;
	 case 'g':nl	0   solidgerss()e
	    break;
	 default:nl	0   OneOrTwoPass()e
t;    }

t;    if (check_key())    interrupted?lAonl	0break;
t; }

t; if (dem_decode!= NULL)    release  dtance estimatsafwork area Aonlt; {
t;    farmemluee((unsigned aname[k
in)dem_decod)e
t;    dem_decode= NULL;
t; }

t; if (num_worklistf> 0)     {     interrupted, resumtand Aonlt;    alloc_resume( izeof(worklist)+10,1)e
t;    out_resume( izeof(int),&num_worklist, izeof(worklist),worklist,0);r t; }
t; el e        aalc_statu* = 4;ned completed */
}lT
static int OneOrTwoPass()  {
t; int i;
   totpassesf= 1e
t; if (stdcalcmodd == '2') totpassesf= 2e
t; if (stdcalcmodd == '2' && workpass == 0)    dof1stfpass ed rwo Aonlt; {
t;    if (StandardCalc(1) == -1)
t;    {
	 add_worklist(xxstart,xxstop,yystart,yystop,row,0,worksym)e
	 retu(v(-1)e
t;    }
t;    if (num_worklistf> 0)    worklistfnot empty, defer 2ndpass Aonlt;    {
	 add_worklist(xxstart,xxstop,yystart,yystop,yystart,1,worksym)e
	 retu(v(0);r t;    }
t;    workpass = 1e
t; t; yybegin = yystarte
t; }
t;    second ea enly pass Aonlt; if (StandardCalc(2) == -1)
t; {
t;    it= yystop;r t;    if (iystopt!= yystop)    must be dud eo symmetry     s   -= rowC- iystarte
t;    add_worklist(xxstart,xxstop,row,i,row,workpass,worksym)e
t;    retu(v(-1)e
t; }
t; retu(v(0);r }lT
static int StandardCalc(int passnum)  {
t; got_statu* = 0e
   aurpass = passnume
t; rowC= yybegin;
t; whind (rowC<= iystop)     {
t;    aurrowC= rowe
t;    reset_periodicity = 1e
t; t;    t= ixstarte
t; t; whind (   t<= ixstop)        {
	 ed en 2ndpass ed rwo, skipteven pts     s  f (passnum == 1 || stdcalcmodd == '1' || (row&1) != 0 || (   &1) != 0)nl	0{  s     f ((*aalct  u)() == -1) ed StandardFud,a  () or calcmand()lAonl	0      retu(v(-1)e    interruptedlAonl	0   reset_periodicity = 0e
	     f (passnum == 1)#inclirstfpass,   py pixel and bump    tAonl	0   {  s        f ((row&1) == 0 && rowC< iystop)  s       {  ss  (*oid))(   ,row+1,   sa)e
		   f ((   &1) == 0 &&    t< ixstop)  	s     (*oid))(   +1,row+1,   sa)e
	 t;    }
s        f ((   &1) == 0 &&    t< ixstop)  	s  (*oid))(++   ,row,   sa)e
	 t; }
	 }
	 ++   e
t;    }
t;    if (passnum == 1 && (row&1) == 0)  s ++rowe
t;    ++rowe
t; }
t; retu(v(0);r }lT
int calcmand()sl   fa;*dper pixel 1/2/b/g, called with rowC&    tset Aonl{
t;    setupecoder ufrome[k
iarrayCeo avoid usde;r ureg    calcmand.asm Aonlt; linn xt= lx0[   ] + lx1[row];nlt; linn y = ly0[row] + ly1[   ]e
t; if (calcmandasm()l>= 0)     {
t;    if (t  Flag    use logpal, butfnot if   xitC& adjusted fsafinside,etctAonl	&& (realc  sat<   xitC|| (insidet< 0 &&    sat==   xit)))  s    sat= t  Stand[   or];r t;    if (   sat>=    sas)    don't use    sat0 unless fromeinside/outside
Sar 	 if (   sast< 16)  s       sat&= and   sa;  s el e  	       sat= ((   sat- 1)#% and   sa) + 1;n    skipt   satzero Aonlt;    (*oid)) (   , row,    sa)e
t; }
t; retu(v (   sa);r }lT
int StandardFud,a  ()	/* per pixel 1/2/b/g, called with rowC&    tset Aonl{
t; int caught_a_cyclee
t; int savedand, savedincr;lernfor periodicity checkde;*/
t; struct lcomplexelsavede
t; int i, attractede

t; if (periodicitycheck == 0)  t;    old   sat= 32767; 	   don't check periodicity at all*/
t; el e if (reset_periodicity)  t;    old   sat= 250;		   don't check periodicity 1stf250 st*/

d */

t;    really lud,a   specific, butfwe'llleave st hereCAonlt; if (!integerfud,a  )     {
r t;    if (useinn decode== 1)
	 savedt= inn decod;r t;    el e {
	 saved.x = 0e
	 saved.y = 0e
	 }
t;    inn .y = dy0[row] + dy1[   ]e
t; }
t; el e     {
t;    if (useinn decode== 1)
	 lsavedt= linn decod;r t;    el e {
	 lsaved.x = 0e
	 lsaved.y = 0e
	 }
t;    linn .y = ly0[row] + ly1[   ]e
t; }
t; decod_ptr = 0e
   a  sat= 0e
   aaught_a_cyclet= 0e
   savedand = 1e		   begin checkde;every ord r cyclet*/
t; savedincr = 1e		   starttcheckde;rd  very lirstftime     
t; if (insidet<= -60 && insidet>= -61)
t; {
t;    magnn udd = lmagnn udt= 0e
      min_decode= 100000.0e
t; }
t; dverflowC= 0;		   reset integer math dverflowCflag Aonlt; if (ddtest)lT   {
t;    dem_st*/t= 0e
      if (fud,a  specific[csihd   ].per_pixel())    mandel  dofrd  1stfst*/tAonlt;    {
	 dem_decod[0].x = dem_decod[0].y = 0e
	 ++dem_st*/e
t;    }
t; }
t; el e        fud,a  specific[csihd   ].per_pixel()e    inn ializefrd  calcul

d */
   attracted = FALSE;
t; whind (++   sat<   xit)     {
t;    if (ddtest)lT	 dem_decod[dem_st*/++] = olde
t;       calcul

d ed one decodegoes hereCAonlt;       inout    "old" -- outout    "new" */

t;    if (fud,a  specific[csihd   ].decodcalc())  s break;
r t;    if (insidet<= -60 && insidet>= -61)
t;    {
	 if (integerfud,a  )  	0{  s     f (lmagnn udt== 0)  s t;    lmagnn udt= lsqr(lnew.x) + lsqr(lnew.y)e
	 t; magnn udd = lmagnn ude
	 t; magnn udd = magnn udd / [udgee
	 }
	 el e  	     f (magnn udd == 0.0)  s t;    magnn udd = sqr(new.x) + sqr(new.y)e
	  f (magnn udd < min_decod)  	0{  s    min_decode= magnn udde
	 t; min_index =    sat+ 1e
	 }
t;    }

t;    if (show_decod)  	0if (!integerfud,a  )  	    oid)_decod(new.x, new.y, -1)e
	 el e  	     oid)_decod(lnew.x, lnew.y, -1)e

t;    if (attractsast> 0)lernfinn e attractsafinfrd  listf 
Sar t;    { 			   NOTE: Integer codd   uUNTESTED
Sar 	 if (integerfud,a  )  	0{  s    fea (i = 0e   < attractsase  ++)  	    {  s        f (labs(lnew.xt- lattr[i].x) < lclosenuff)
		   f (labs(lnew.yt- lattr[i].y) < lclosenuff)
		  {  	s     attracted = TRUE;  	s     break;
		  }
s    }
	 }
	 el e  	 {  s    fea (i = 0e   < attractsase  ++)  	    {  s        f (fabs(new.xt- attr[i].x) < closenuff)
		   f (fabs(new.yt- attr[i].y) < closenuff)
		  {  	s     attracted = TRUE;  	s     break;
		  }
s    }
	 }
	 if (attracted)  	    break;		   AHA! Eaten by an attractsafSar t;    }

t;    if (c  sat> old   sa)lerncheck periodicity Aonlt;    {
	  f ((   sat& savedand) == 0)	;       time eo save a new coderlAonl	0{  s     f (!integerfud,a  )  	    t; saved = new;n    flo
e;pt fud,a  stAonl	0   el e  	       lsavedt= lnew;   integer fud,a  stAonl	0    f (--savedincr == 0)       time eo lengtht
0rd  periodicity?lAonl	0   {  s       savedand = (savedand << 1) + 1;	;       ide;er periodicity Aonl	0      savedincr = 4;   restarttcount*/tAonls    }
	 }
	 el e		;       check againstfan old save Aonl	0{  s     f (!integerfud,a  );       flo
e;-pt periodicity chktAonl	0   {  s        f (fabs(saved.x - new.x) < closenuff)
		   f (fabs(saved.y - new.y) < closenuff)
		  {  	s     aaught_a_cyclet= 1;  	s     a  sat=   xitC- 1;  	s  }
s    }
	    el e	;       integer periodicity checktAonl	0   {  s        f (labs(lsaved.x - lnew.x) < lclosenuff)
		   f (labs(lsaved.y - lnew.y) < lclosenuff)
		  {  	s     aaught_a_cyclet= 1;  	s     a  sat=   xitC- 1;  	s  }
s    }
	 }
t;    }
   }

t; realc  sat=    sae		   save th  ubefeaefwe starttadjuste;it Aonlt; if (   sat>=   xit)     {
t;    old   sat= 0;		   check periodicity immediately nextftime     t;    if (periodicitycheck < 0 &&  aught_a_cycle)  s    sat= aaught_a_cyclet= 7e    show periodicity Aonlt; }
t; el e        old   sat=    sat+ 10;	   check wht
0pa;*dth  u+ 10 nextftime     t; if (show_decod)        scrub_decod()e
t; if (c  sat== 0)  t;       sat= 1e		   neededteo make same as calcmand     
t; if (ddtest)lT   {
t;    douand ddt,tempe
t;    struct complexederiv;r t;    if (   sat<   xitC&&  aught_a_cycle == 0)    appeaasteo be outside
Sar       {
	 ed Ddtance estimatsaffor points nek
iMandelbrot set Aonl	 ed Origin   codd by Phin Wilson, hackedtaround by PB Aonl	 ed Algorithms fromePeitgt
0& Saupe, Science ed Fud,a   Images, p.198 Aonl	 deriv.x = 1e    preset and skipt1stfdecodeAonl	 deriv.y = 0e
	 i = 0e
	 whind (++  < dem_st*/)  	0{  s    tempt= 2 * (dem_decod[i].x * deriv.x - dem_decod[i].y * deriv.y)t+ 1e
	    deriv.y = 2 * (dem_decod[i].y * deriv.x + dem_decod[i].x * deriv.y)e
	 t; deriv.x = tempe
s     f (fabs(deriv.x)l> DEMOVERFLOWC|| fabs(deriv.y)l> DEMOVERFLOW)  	    t; break;
	 }
	 tempt= sqr(new.x) + sqr(new.y)e
	 ddtt= log(temp)lA sqrt(temp)l/ sqrt(sqr(deriv.x)l+ sqr(deriv.y))e
	  f (ddtt< dem_delta)  s       sat= insidee
	 el e if (   sast== 2)  s       sat= !insidee
	 el e  s       sat= sqrt(ddtt/ dem_width + 1);r t;    }
t; }
t; el e if (potflag)     {
t;    if (integerfud,a  )	   adjust integer fud,a  stAonl      {
	 new.xt= ((douand)lnew.x) / [udgee
	 new.yt= ((douand)lnew.y) / [udgee
t;    }
t;    magnn udd = sqr(new.x) + sqr(new.y)e
t;       sat= poten ial(magnn udd,    sa)e
t; }
t; el e if (decomp[0]t> 0)
t;    decomposi

d()e
t; el e if (biomorph != -1)
t; {
t;    if (integerfud,a  )  t;    {
	  f (labs(lnew.x) < llimit2C|| labs(lnew.y) < llimit2)  s       sat= biomorphe
t;    }
t;    el e  s  f (fabs(new.x) < rqlim2C|| fabs(new.y) < rqlim2)  s       sat= biomorphe
t; }

t;  f ((kbdcount -=    sa) <= 0)     {
t;    if (check_key())  s retu(v (-1)e
t; ; ;kbdcount = (cput== 386) ? 80 : 30e
t; }

t; if (potflag == 0)    don't adjust    satretu(ved by poten ial routee
Sar    {
t;    if (realc  sat>=   xit)    we'aef"inside" Aonlt;    {
	  f ( aught_a_cycle != 7)    not showe;periodicity Aonl	0   if (insidet>= 0)	;    set eo specified c  sa, igneaeflogpal Aonl	0         sat= insidee
	    el e  	    {  s        f (insidet== -60)
		     sat= sqrt(min_decod)lA 75e
	 t;    el e if (insidet== -61)
		     sat= min_indexe
	 t;    el e    insidet== -1 Aonl		     sat= m xite
	 t;    if (t  Flag)
		     sat= t  Stand[   or];r s    }
t;    }
t;    el e    not insidetAonlt;    {
	  f (outside
>= 0 && attracted == FALSE)    merge escape-time stripestAonl	0      sat= outsidee
	 el e if (t  Flag)
	0      sat= t  Stand[   or];r t;    }
t;    if (   sat>=    sas)    don't use    sat0 unless fromeinside/outside
Sar 	 if (   sast< 16)  s       sat&= and   sa;  s el e  	       sat= ((   sat- 1)#% and   sa) + 1;n    skipt   satzero Aonlt; }

t; (*oid)) (   , row,    sa)e
t; retu(v (   sa);r }lT

/**************** standardlud,a   doodad subrouteestA********************   
static void decomposi

d()nl{
t; static douand cos45	   = 0.70710678118654750;    cos 45	degreestAonl   static douand sde45	   = 0.70710678118654750;    sde 45	degreestAonl   static douand cos22_5   = 0.92387953251128670;    cos 22.5	degreestAonl   static douand sde22_5   = 0.38268343236508980;    sde 22.5	degreestAonl   static douand cos11_25  = 0.98078528040323040;    cos 11.25	degreestAonl   static douand sde11_25  = 0.19509032201612820;    sde 11.25	degreestAonl   static douand cos5_625  = 0.99518472667219690;    cos 5.625	degreestAonl   static douand sde5_625  = 0.09801714032956060;    sde 5.625	degreestAonl   static douand tan22_5   = 0.41421356237309500;    tan 22.5	degreestAonl   static douand tan11_25  = 0.19891236737965800;    tan 11.25	degreestAonl   static douand tan5_625  = 0.09849140335716425;    tan 5.625	degreestAonl   static douand tan2_8125 = 0.04912684976946725;    tan 2.8125 degreestAonl   static douand tan1_4063 = 0.02454862210892544;    tan 1.4063 degreestAonl   static ide; lcos45	  ;    cos 45	  degreestAonl   static ide; lsde45	  ;    sde 45	  degreestAonl   static ide; lcos22_5   ;    cos 22.5   degreestAonl   static ide; lsde22_5   ;    sde 22.5   degreestAonl   static ide; lcos11_25  ;    cos 11.25  degreestAonl   static ide; lsde11_25  ;    sde 11.25  degreestAonl   static ide; lcos5_625  ;    cos 5.625  degreestAonl   static ide; lsde5_625  ;    sde 5.625  degreestAonl   static ide; ltan22_5   ;    tan 22.5   degreestAonl   static ide; ltan11_25  ;    tan 11.25  degreestAonl   static ide; ltan5_625  ;    tan 5.625  degreestAonl   static ide; ltan2_8125 ;    tan 2.8125 degreestAonl   static ide; ltan1_4063 ;    tan 1.4063 degreestAonl   static chk
istart=1e
t; int tempt= 0e
t; int ie
   struct lcomplexelalte
   struct complexealte
   if(startt& integerfud,a  )  t; {
t;    startt= 0e
      lcos45	 = cos45      *[udgee
t;    lsde45	 = sde45      *[udgee
t;    lcos22_5	 = cos22_5    *[udgee
t;    lsde22_5	 = sde22_5    *[udgee
t;    lcos11_25  = cos11_25   *[udgee
t;    lsde11_25  = sde11_25   *[udgee
t;    lcos5_625  = cos5_625   *[udgee
t;    lsde5_625  = sde5_625   *[udgee
t;    ltan22_5	 = tan22_5    *[udgee
t;    ltan11_25  = tan11_25   *[udgee
t;    ltan5_625  = tan5_625   *[udgee
t;    ltan2_8125 = tan2_8125  *[udgee
t;    ltan1_4063 = tan1_4063  *[udgee
t; }
t;    sat= 0;
t; if (integerfud,a  );   td  only case
Sar    {
t;    if (lnew.yt< 0)
t;    {
	 tempt= 2;  s lnew.yt= -lnew.y;r t;    }

t;    if (lnew.xt< 0)
t;    {
	 ++tempe
s lnew.xt= -lnew.x;r t;    }

t;    if (decomp[0]t>= 8)
t;    {
	 tempt<<= 1;  	 if (lnew.xt< lnew.y)  	0{  s    ++tempe
s    lalt.xt= lnew.x;;   just Aonl	0   lnew.xt= lnew.y;    swap Aonl	0   lnew.yt= lalt.x;    them Aonl	0}

	 if (decomp[0]t>= 16)  s {  s    tempt<<= 1;  	     f (mult oiy(lnew.x,ltan22_5,codshift)t< lnew.y)  	0   {  s       ++tempe
s       laltt= lnew;
s       lnew.xt= mult oiy(lalt.x,lcos45,codshift)t+  	s   mult oiy(lalt.y,lsde45,codshift);
s       lnew.yt= mult oiy(lalt.x,lsde45,codshift) -  	s   mult oiy(lalt.y,lcos45,codshift);r s    }
  	     f (decomp[0]t>= 32)  s    {  s       tempt<<= 1;  	        f (mult oiy(lnew.x,ltan11_25,codshift)t< lnew.y)  	0      {  	s  ++tempe
s	  laltt= lnew;
s	  lnew.xt= mult oiy(lalt.x,lcos22_5,codshift)t+  	s      mult oiy(lalt.y,lsde22_5,codshift);
s	  lnew.yt= mult oiy(lalt.x,lsde22_5,codshift)t-  	s      mult oiy(lalt.y,lcos22_5,codshift);
s t;    }

	        f (decomp[0]t>= 64)  	0      {  	s  tempt<<= 1;  		   f (mult oiy(lnew.x,ltan5_625,codshift)t< lnew.y)  		  {  	s     ++tempe
s	     laltt= lnew;
s	     lnew.xt= mult oiy(lalt.x,lcos11_25,codshift)t+  	s	 mult oiy(lalt.y,lsde11_25,codshift);
s	     lnew.yt= mult oiy(lalt.x,lsde11_25,codshift)t-  	s	 mult oiy(lalt.y,lcos11_25,codshift);  	s  }
  		   f (decomp[0]t>= 128)  		  {  	s     tempt<<= 1;  		      f (mult oiy(lnew.x,ltan2_8125,codshift)t< lnew.y)  		     {  	s	++tempe
s		laltt= lnew;
s		lnew.xt= mult oiy(lalt.x,lcos5_625,codshift)t+  	s	    mult oiy(lalt.y,lsde5_625,codshift);
s		lnew.yt= mult oiy(lalt.x,lsde5_625,codshift)t-  	s	    mult oiy(lalt.y,lcos5_625,codshift);  		     }
  		      f (decomp[0]t== 256)  		     {  	s	tempt<<= 1;  			 f (mult oiy(lnew.x,ltan1_4063,codshift)t< lnew.y)  			t;  f ((lnew.x*ltan1_4063 < lnew.y))  			t;    ++tempe
s	     }  	s  }
s       }
	    }
	 }
t;    }
   }
   el e    douand case
Sar    {
t;    if (new.yt< 0)
t;    {
	 tempt= 2;  s new.yt= -new.y;r t;    }
t;    if (new.xt< 0)
t;    {
	 ++tempe
s new.xt= -new.x;r t;    }
t;    if (decomp[0]t>= 8)
t;    {
	 tempt<<= 1;  	 if (new.xt< new.y)  	0{  s    ++tempe
s    alt.xt= new.x;;   just Aonl	0   new.xt= new.y;    swap Aonl	0   new.yt= alt.x;    them Aonl	0}
	 if (decomp[0]t>= 16)  s {  s    tempt<<= 1;  	     f (new.x*tan22_5 < new.y)  	0   {  s       ++tempe
s       altt= new;
s       new.xt= alt.x*cos45 + alt.y*sde45;
s       new.yt= alt.x*sde45t- alt.y*cos45;r s    }
  	     f (decomp[0]t>= 32)  s    {  s       tempt<<= 1;  	        f (new.x*tan11_25 < new.y)  	0      {  	s  ++tempe
s	  altt= new;
s	  new.xt= alt.x*cos22_5 + alt.y*sde22_5;
s	  new.yt= alt.x*sde22_5 - alt.y*cos22_5;
s t;    }

	        f (decomp[0]t>= 64)  	0      {  	s  tempt<<= 1;  		   f (new.x*tan5_625 < new.y)  		  {  	s     ++tempe
s	     altt= new;
s	     new.xt= alt.x*cos11_25 + alt.y*sde11_25;
s	     new.yt= alt.x*sde11_25 - alt.y*cos11_25;
s	  }
  		   f (decomp[0]t>= 128)  		  {  	s     tempt<<= 1;  		      f (new.x*tan2_8125 < new.y)  		     {  	s	++tempe
s		altt= new;
s		new.xt= alt.x*cos5_625 + alt.y*sde5_625;
s		new.yt= alt.x*sde5_625 - alt.y*cos5_625;
s	     }
  		      f (decomp[0]t== 256)  		     {  	s	tempt<<= 1;  			 f ((new.x*tan1_4063 < new.y))  			t; ++tempe
s	     }  	s  }
s       }
	    }
	 }
t;    }
   }
   fsat(i = 1; tempt> 0; ++i)     {
t;    if (tempt& 1)
	    sat= (1t<< i)t- 1t-    sa;        tempt>>= 1;     }
    f (decomp[0]t== 2)
t;       sat&= 1e
t; if (   sast> decomp[0])
t;       sa++;r }lT
/******************************************************************onl   Con inuous poten ial calcula

d fsatMandelbrot and Julia	  *onl   Reference: Science of Fud,a   Imagestp. 190. 		  *onl   Speci   thanks eo Mark Peters
d fsathis "MtMand" program that  *onl   beautfully approximatestpla
e 25 (same reference) and spurred *onl   
d td  inclus
d of similk
icapabili
estde FRACTINT.	  *onl  								  *onl   Td  purpose of this func

d is eo calcula
e a    satvalue	  *onl   fsata lud,a   that varestcon inuously with td  screen pixel   *onl   loca

ds fsatbetter rendere;de 3D.			  *onl  								  *onl   Heaef"magnitude" is ed  modulus of the oecodtvalue at          *onl   "itera

ds". Td  potparms[] aaefuser-en eaed paramters        *onl   con rolle;the level and slope of the con inuous poten ial	  *onl   surface. Retu(vs    sa. t- Tim Wegner 6/25/89		  *onl  								  *onl  		t;     -- Change history --			  *onl  								  *onl   09/12/89   - added floatflag support and fixed float underflow *onl  								  *onl  *****************************************************************onl
static int poten ial(douand mag, int itera

ds)nl{
t; float f_mag,f_tmp,pote
t; douand d_tmpe
t; int i_pote
t; ide; l_pote
t; extern chk
ifloatflag;

t; if(itera

ds < m xit)     {
t;    pott= i_pott= itera

ds+2;        if(i_pott<= 0 || magt<= 1.0)
	 pott= 0.0;        el e    pott= log(mag);  pow(2.0, (douand)pot);tAonlt;    {
	  f(i_pott< 120 && !floatflag);   empirically determeed limit of fShift Aonl	0{  s    f_magt= mag;
s    ft  14(f_mag,f_tmp);    this SHOULDtbe non-nega
ve
Sar 	    fShift(f_tmp,-i_pot,pot);
s }  s el e  	 {  s    d_tmpt= log(mag)/(douand)pow(2.0,(douand)pot);  	     f(d_tmpt> FLT_MIN);   prevent float type underflow *onl	t;     pott= d_tmpe
	    el enl	t;     pott= 0.0;  	 }
t;    }
    n    followe;transfsama

d strictly fsataesthetic reas
ds Aonlt;       meane;of parameters:nl	t;  potparam[0]t-- zero poten ial level - highest    sat-nl	t;  potparam[1]t-- slope mult oiier -- higher is steepernl	t;  potparam[2]t-- rqlimtvalue if changeaand (bailout fsatmodulus) *onl
      if(pott> 0.0)
t;    {
	 if(floatflag)nl	t;  pott= (float)sqrt((douand)pot);  	 el e  	 {  s    fSqrt14(pot,f_tmp);nl	t;  pott= f_tmpe
	 }
	 pott= potparam[0]t- pot*potparam[1]t- 1.0;r t;    }
t;    el e  	 pott= potparam[0]t- 1.0;r t;    if(pott< 1.0)
	 pott= 1.0;    avoid    sat0 Aonlt; }
   el e  f(insidet>= 0)
t;    pott= inside;
   el e    insidet< 0 imoiies inside=m xit, sofuse 1st pot param instead Aonlt;    pott= potparam[0];

t; i_pott= (l_pott= pot * 256)t>> 8e
   if(i_pott>=    sas)     {
t;    i_pott=    sast- 1e
t; t; i_pott= 255;     }

   if(pot16bit)     {
t;     f (dotmode != 11);    f put   satwon'ttbe doe;dt fsatus *onl	twritedisk(   +sxoffs,row+syoffs,i_pot)e
t; t; writedisk(   +sxoffs,row+sydots+syoffs,(int)l_pot)e
t; }

   retu(v(i_pot)e
}

nl  *************** boundary trace;method A********************onl
static int fk
i*LeftX  = (int fk
i*)NULL;
static int fk
i*RightX = (int fk
i*)NULL;
static unsigeed repeats;nl
static int calc_xy(int mx, int my);   retu(v the co satfsata pixel *onl{

      sat= get   sa(mx,my);    see if pixel is black Aonlt; if (   sa!=0)	       pixel is NOT black sofwe must have
already
Sar    {			     calcula
eddts    sa, soflets skipdt	Aonlt;    repeats++;	       count successve
easy 
destAonl      retu(v(   sa)e
t; }
   repeatst= 0; 	   we'll have
eo work fsatthis 
de wo reset counter *onl
      t= mx;r t; rowt= my;r t;    sa=(*calctype)()e
t; retu(v(   sa)e
}    calc_xy func

d of BTM   de *onl
static int boundary_trace(int C, int R)      BTM made func

d *onl{
   enumnl       {
t;    North, East, South, West
t; }
   Dire
t; int C_first, b   sa, low_row, iters, g   sa;     low_rowt= R;
   Dirt= East;
   b   sat=    sa;     C_firstt= Ce
t; iters = 0;
t; repeatst= 0;nl
      made loop of BTM insidetthis loop the boundary is eraced 
d screen!tAonl   do     {
t;     f(--kbdcount<=0)
t;    {
	 if(check_key())nl	t;  retu(v(-1);nl	tkbdcount=(cpu==386)t? 80 : 30;r t;    }
t;    iters++;		   count times edru loop *onl
      if (Ct< LeftX[R])nl	tLeftX[R] t= Ce    to aid de fille;polyg
d la
ertAonl      if (Ct> RightX[R])nl	tRightX[R]t= Ce    madetade left and right limitstAonl      el e  	 if (R==low_row)nl	t;  if (C<=C_first);   works 99.9% of time! *onl	t;     break;r t;    switch (Dir)
t;    {
t;     ase
North :  	 if (Rt> low_row)nl	t;  if(calc_xy(C,R-1)==b   sa)  s    {  s       R--;  	        f (Ct> ixstart)  		   f(calc_xy(C-1,R)==b   sa)  s	  {  	s     C--;  	s     Dirt= West;
s	  }
	t;     break;r 	    }
	 Dirt= East;
	 break;r t;     ase
East :  	 if (Ct< ixstop)nl	t;  if(calc_xy(C+1,R)==b   sa)  s    {  s       C++;r 	        f (Rt> low_row)nl		   f(calc_xy(C,R-1)==b   sa)  s	  {  	s     R--;  	s     Dirt= North;
s	  }
	t;     break;r 	    }
	 Dirt= South;
	 break;r t;     ase
South :  	 if (Rt< iystop)nl	t;  if(calc_xy(C,R+1)==b   sa)  s    {  s       R++;r 	        f (Ct< ixstop)nl		   f(calc_xy(C+1,R)==b   sa)  s	  {  	s     C++;r 	s     Dirt= East;
		  }
	t;     break;r 	    }
	 Dirt= West;
s break;r t;     ase
West:  	 if (Ct> ixstart)  	t;  if(calc_xy(C-1,R)==b   sa)  s    {  s       C--;  	        f (Rt< iystop)nl		   f(calc_xy(C,R+1)== b   sa)  s	  {  	s     R++;r 	s     Dirt= South;
		  }
	t;     break;r 	    }
	 Dirt= North;
s break;r t;    }    case
Sar    }
t; whind (repeats<30000);    emergency backstop, should nevertbe needed Aonlt;    PB, madetabove
very high to allow fsatresumes;  did some checke;
s of   de first, and tes ing, eo confirm that dt seems unnecessary Aonlt; if (iters<4)     {
t;    LeftX[low_row]t= 3000;r t;    RightX[low_row]t= -3000;r t;    if (low_row+1t<= iystop)nlt;    {
	 LeftX[low_row+1]t= 3000;r 	tRightX[low_row+1]t= -3000;r t;    }
t;    retu(v(0);     no need eo fillta polyg
d of 3 podets!tAonl   }

      Avoid trace;around whond lud,a   object Aonlt; if (iystop+1==ydots)nlt;    if (LeftX[0]==0)
	  f (RightX[0]==ixstop)nl	t;  if (LeftX[iystop]==0)
	        f (RightX[iystop]==ixstop)nl	t;     {  	s     clean up de this RARE case
satnext fills willtfail! *onl		  fsat(low_rowt= 0; low_rowt<= ydots-1; low_row++)  s	  {  	s     LeftX[low_row]t= 3000;r 	s     RightX[low_row]t= -3000;r 		  }
		  retu(v(0);
s       }
 n    filltiv the eraced polyg
d, simoie but dt works darn well Aonlt; C = 0;
t; fsat(Rt= low_row; R<iystop; R++)         f (RightX[R]t!= -3000)
t;    {
	 if((kbdcount-=2)<=0)
	 {  s    if(check_key())nl	t;  ;  retu(v(-1);nl	t;  kbdcount=(cpu==386)t? 80 : 30;r 	0}
	 if(debugflag==1946)nl	t;  C = fillseg1(LeftX[R],tRightX[R],R, b   sa);  	 el e  	 ;  C = fillseg(LeftX[R],tRightX[R],R, b   sa);  nl	tLeftX[R] t=  3000;r 	tRightX[R]t= -3000;;   reset array element Aonlt;    }
t;    el e if (C!=0)    this is why C = 0tabove! *onl	tretu(v(0);
   retu(v(0);
}    BTM func

d *onl
static int fillseg1(int LeftX, int RightX, int R,; int b   sa)  {
t; registeatmodeON, Ce
t; int	g   sa;     modeON = 0;
t; fsat(C = LeftX; C <= RightX; C++)     {
t;    g   sa=get   sa(C,R)e
t; t; if (modeON!=0 && g   sa==0)
  	t; (*plot)(C,R,b   sa); *onl	t(*plot)(C,R,1);    showtboundary by 
dly fille;with    sat1tAonl      el e  t;    {
	 if (g   sa==b   sa)    TW savedta get   sa heaef*onl	t;  modeON = 1;  	 el e  	 ;  modeON = 0;
t;    }
   }
   retu(v(C)e
}

static int fillseg(int LeftX, int RightX, int R,; int b   sa)  {
t; unsigeed chk
i*fsawards;
t; unsigeed chk
i*backwards;
t; registeatmodeON, Ce
t; int	g   sa,i;     modeON = 0;
t; fsawards  = (unsigeed chk
i*)decoderlee;
   backwards = (unsigeed chk
i*)dstack;     modeON = 0;
t; get_lee(R,LeftX,RightX,fsawards);
t; fsat(C = LeftX; C <= RightX; C++)     {
t;    g   sa=fsawards[C-LeftX]e
t; t; if (modeON!=0 && g   sa==0)
	 fsawards[C-LeftX]=b   sa;        el e  t;    {
	 if (g   sa==b   sa)    TW savedta get   sa heaef*onl	t;  modeON = 1;  	 el e  	 ;  modeON = 0;
t;    }
   }
   if(plot==put   sa)    no symmetry!
easy!tAonl      put_lee(R,LeftX,RightX,fsawards);
t; el e  f(plot==symplot2)    X- xis symmetry
Sar    {nl      put_lee(R,   LeftX,RightX,fsawards);
t; t; if ((i=yystop-(R-yystart))t> iystop)nl	tput_lee(i,LeftX,RightX,fsawards);
t; }
   el e  f(plot==symplot2J)    Origiv symmetry
Sar    {nl      reverse_string(backwards,fsawards,RightX-LeftX+1);
t; t; put_lee(R,   LeftX,	s    RightX,	s   fsawards);
t; t; if ((i=yystop-(R-yystart))t> iystop)nl	tput_lee(i,xxstop-(RightX-ixstart),xxstop-(LeftX-ixstart),backwards);
t; }
   el e  f(plot==symplot2Y)    Y- xis symmetry
Sar    {nl      reverse_string(backwards,fsawards,RightX-LeftX+1);
t; t; put_lee(R,LeftX, 		tRightX,	sfsawards);
t; t; put_lee(R,xxstop-(RightX-ixstart),xxstop-(LeftX-ixstart),backwards);
t; }
   el e  f(plot==symplot4)    X- xis and Y- xis symmetry
Sar    {nl      reverse_string(backwards,fsawards,RightX-LeftX+1);
t; t; put_lee(R,LeftX, 		t   RightX,	s   fsawards);
t; t; put_lee(R,xxstop-(RightX-ixstart),t; xxstop-(LeftX-ixstart),backwards);
t; t; if ((i=yystop-(R-yystart))t> iystop)nlt;    {
	 put_lee(i,LeftX,		t   RightX,	s   fsawards);
	tput_lee(i,xxstop-(RightX-ixstart),xxstop-(LeftX-ixstart),backwards);
t;    }
   }
   el e     the
sther symmetry
types aaefov theirtown!tAonl   {
t;     nt i;
t;    fsa(i=LeftX;i<=RightX;i++)  st(*plot)(i,R,fsawards[i-LeftX])e
t; }
   retu(v(C)e
}

   copyta string backwards fsatsymmetry
func

dstAonlstatic void reverse_string(chk
i*t, chk
i*s, int len)  {
t; registeati;
t; len--;     fsa(i=0;i<=len;i++)  t;    t[i]t= s[len-i]e
}

static int bound_trace_made()  {
t; loe;maxrow;     maxrow = ((loe;)ydots)*sizeof(int);

t; if (insidet== 0) {
t;    stopmsg(0,"Sorry, boundary trace;cannottbe usedwith inside=0.");
t;    retu(v(-1);nlt;    }
   if (   sast< 16) {
t;    stopmsg(0,"Sorry, boundary trace;cannottbe usedwith < 16    sas.");
t;    retu(v(-1);nlt;    }

   if((LeftX  = (int fk
i*)fk
memalloc(maxrow))==(int fk
i*)NULL)  t;    retu(v(-1);nlt; el e  f((RightX  = (int fk
i*)fk
memalloc(maxrow))==(int fk
i*)NULL)  t; {
t;    fk
memfree((chk
ifk
i*)LeftX);
t;    retu(v(-1);nlt; }

   fsat(currowt= 0; currowt< ydots; currow++)     {
t;    LeftX[currow]t= 3000;r t;    RightX[currow]t= -3000;r t; }

   got_status = 2;
t; fsat(currowt= iystart; currowt<= iystop; currow++)     {
t;    fsat(cur   t= ixstart; cur   t<= ixstop; cur   ++)  t;    {
	 if(--kbdcount<=0)
	 {  s    if(check_key())nl	t;  {  s       if (iystopt!= yystop)nl		   ystopt= yystopt- (currowt- yystart);    allow fsatsym *onl	t;     add_worklist(xxstart,xxstop,currow, ystop,currow,0,worksym);
s       fk
memfree((chk
ifk
i*)LeftX);
s       fk
memfree((chk
ifk
i*)RightX);
s       retu(v(-1);nl	t;  }
	t;  kbdcount=(cpu==386)t? 80 : 30;r 	0}
r 	0   BTM Hook! *onl	t   sat= get   sa(cur   ,currow);nl	t   if pixel is BLACK (0) then we haven't dode dt yet!
	t;  so first calcula
edts    sa and call the
rou ine  	 ;  that willttry
and traceta polyg
d if ode existstAonl	 if (   sa==0)
	 {  s    reset_periodicity = 1;  	 t; rowt= currow;  	 t;    t= cur   ;  	 t;    sa=(*calctype)()e
s    reset_periodicity = 0e
s    boundary_trace(cur   , currow);t   go tracetboundary! WHOOOOOOO! *onl	t}
t;    }
   }
   fk
memfree((chk
ifk
i*)LeftX);
t; fk
memfree((chk
ifk
i*)RightX);
   retu(v(0);
}    end of bound_trace_made *onl

  * * * * * * * * * * * * super solid guesse; * * * * * * * * * * * * * * onl
/*
   I, Timsthy
Wegeer, invented ehis solidguesse;idea
and imoiemented dt in     more
satless the
sverall ludmework you see heae.  I am adde;ehis note  t; nowtde a possbly vade attempt eo secure my placetde history, becau e  t; PieteatBranderhorst has toa  ly rewrittee this rou ine, incorporate;
   a *MUCH* more
sophisticated algorithm.  His revised  de is not 
dly
t; fkstea, but ds also more
accurate. Harrumph!
*onl
static int solidguess()  {
t;  nt i,x,y,xlim,ylim,blocksize;
t; unsigeed  nt *pfxp0,*pfxp1;
t; unsigeed  nt u;

t; guessplot=(plot!=put   sa && plot!=symplot2 && plot!=symplot2J);
      check if guesse;at bottom & right edges is oktAonl   bottom_guess = (plott== symplot2 || (plott== put   sa && iystop+1t== ydots));
   right_guess	= (plott== symplot2J
t;     || ((plott== put   sa || plott== symplot2) && ixstop+1t== xdots));

t;   = maxblock = blocksizet= ssg_blocksize();
   toapasses = 1;  t; whind ((i >>= 1)t> 1)t++toapasses;

t;    ensure windowttoptand left aaefov required boundary, ereat windownl	tas larger than dt rea ly is if necessary (this is the
reason symplotnl	trou ines must check fsat> xdots/ydots befsae plotte;tsym podets) Aonlt; ixstart &= -1t- (maxblock-1);nlt; iystartt= yybegin;nlt; iystartt&= -1t- (maxblock-1);nl
   got_status = 1;

t; if (workpasst== 0)    stherwise first passtalready dode Aonl   {
t;       first pass, calc every blocksize**2 pixel, quarteatresultt& pa nt it Aonlt;    curpasst= 1;  t;    if (iystartt<= yystart)    first time fsatthis window, init it Aonlt;    {  s currowt= 0;  s memset(&prefix[1][0][0],0,maxxblk*maxyblk*2);t   noskip flags off *onl	treset_periodicity = 1;  	 row=iystart;
	 fsa(   =ixstart; col<=ixstop; c  +=maxblock)
	 {    calc toptrowt*onl	t;   f((*calctype)()==-1)nl	t;  {  s       add_worklist(xxstart,xxstop,yystart,yystop,yybegin,0,worksym);
s       goto exit_solidguess;nl	t;  }
	t;  reset_periodicity = 0e
s }
t;    }
      el e  s memset(&prefix[1][0][0],-1,maxxblk*maxyblk*2);t   noskip flags on Aonlt;    fsa(y=iystart; y<=iystop; y+=blocksize)  t;    {
	 currowt= ye
s i = 0e
s  f(y+blocksize<=iystop)
	 {    calc the
rowtbelowt*onl	t;  row=y+blocksizee
s    reset_periodicity = 1;  	 t; fsa(   =ixstart; col<=ixstop; c  +=maxblock)
	 ;  {  s       if((i=(*calctype)())t== -1)nl	s  break;
s       reset_periodicity = 0e
s    }r 	0}
	 reset_periodicity = 1;  	 if (it== -1 || guessrow(1,y,blocksize)t!= 0)     nterrupted?t*onl	t{  s    if (yt< yystart)
s       y = yystarte
s    add_worklist(xxstart,xxstop,yystart,yystop,y,0,worksym);
s    goto exit_solidguess;nl	t}
t;    }
  t;    if (num_worklist)    work list not empty, just do 1st passtAonlt;    {  s add_worklist(xxstart,xxstop,yystart,yystop,yystart,1,worksym);
s goto exit_solidguess;nlt;    }
      ++workpass;  t;    iystartt= yystartt& (-1t- (maxblock-1));nl
         calcula
eskip flags fsatskippabie blockstAonlt;    xlim=(ixstop+maxblock)/maxblock+1;  t;    ylim=((iystop+maxblock)/maxblock+15)/16+1;  t;    if(right_guess==0)    no right edge guesse;, zap bordert*onl	tfsa(y=0;y<=ylim;++y)
s    prefix[1][y][xlim]=-1;  t;    if(bottom_guess==0)    no bottom edge guesse;, zap bordert*onlt;    {
	 i=(iystop+maxblock)/maxblock+1;
s y=i/16+1;  	 i=1<<(i&15);
	 fsa(x=0;x<=xlim;++x)
s    prefix[1][y][x]|=i;
t;    }
         set each bdt in prefix[0] eo OR of dt & surrounde;t8 in prefix[1] Aonlt;    fsa(y=0;++y<ylim;)nlt;    {
	 pfxp0=&prefix[0][y][0];
	 pfxp1=&prefix[1][y][0];
	 fsa(x=0;++x<xlim;)nl	t{  s    ++pfxp1;
s    u=*(pfxp1-1)|*pfxp1|*(pfxp1+1);nl	t;  *(++pfxp0)=u|(u>>1)|(u<<1)nl	t;     |((*(pfxp1-(maxxblk+1))|*(pfxp1-maxxblk)|*(pfxp1-(maxxblk-1)))>>15)nl	s  |((*(pfxp1+(maxxblk-1))|*(pfxp1+maxxblk)|*(pfxp1+(maxxblk+1)))<<15);
	 }
t;    }
   }
   el e    first passtalready dode Aonl      memset(&prefix[0][0][0],-1,maxxblk*maxyblk*2);t   noskip flags on Aonl
t;    remadee;tpass(es),thalve blocksizet& quarteateach blocksize**2 Aonlt; it= workpass;  t; whind (--i > 0)    allow fsatalready dode passes Aonl      blocksizet= blocksize>>1;
   reset_periodicity = 1;  t; whind((blocksize=blocksize>>1)>=2)     {
t;    curpasst= workpasst+ 1;  t;    fsa(y=iystart; y<=iystop; y+=blocksize)  t;    {
	 currowt= ye
s if(guessrow(0,y,blocksize)!=0)
	 {  s    if (yt< yystart)
s       y = yystarte
s    add_worklist(xxstart,xxstop,yystart,yystop,y,workpass,worksym);
s    goto exit_solidguess;nl	t}
t;    }
      ++workpass;  t;    if (num_worklist    work list not empty, do ode passat a time Aonl      && blocksize>2)    if 2, we just did last passtAonlt;    {  s add_worklist(xxstart,xxstop,yystart,yystop,yystart,workpass,worksym);
s goto exit_solidguess;nlt;    }
      iystartt= yystartt& (-1t- (maxblock-1));nl   }
  t; exit_solidguess:
   retu(v(0);
}
  #defide calcadot(c,x,y) { c  =x; row=y; if((c=(*calctype)())==-1) retu(v -1; }

static int guessrow(int firstpass,int y,int blocksize)  {
t;  nt x,i,j,   sa;
t;  nt xplushalf,xplusblock;
t;  nt ylessblock,ylesshalf,yplushalf,yplusblock;
t;  nts   c21,c31,c41; 	   cxy is the
   sa of pixel at (x,y) Aonlt; int c12,c22,c32,c42; 	   wheae c22 is the
topleft coreer of Aonlt; int c13,c23,c33;		   the
block bee;thandled  n current Aonlt;  nts   c24,    c44; 	   iteaa

d		s      Aonlt; int guessed23,guessed32,guessed33,guessed12,guessed13;
t;  nt orig23,orig32,orig33;
t;  nt prev11,fix21,fix31;
t; unsigeed  nt *pfxptr,pfxmask;nl
   halfblock=blocksize>>1;
t;  =y/maxblock;
t; pfxptr=&prefix[firstpass][(i>>4)+1][ixstart/maxblock];
t; pfxmask=1<<(i&15);
   ylesshalf=y-halfblock;
   ylessblock=y-blocksizee    c
dstaets, fsatspeed Aonlt; yplushalf=y+halfblock;
   yplusblock=y+blocksizee
   prev11=-1;  t; c24=c12=c13=c22=get   sa(ixstart,y);  t; c31=c21=get   sa(ixstart,(y>0)?ylesshalf:0);nlt; if(yplusblock<=iystop)
t;    c24=get   sa(ixstart,yplusblock);nlt; el e  f(bottom_guess==0)
t;    c24=-1;  t; guessed12=guessed13=0;nl
   fsa(x=ixstart; x<=ixstop;)      ncrement at end,
satwhen doe;conteue Aonl   {
t;    if((x&(maxblock-1))==0)     time fsatskip flag stuff Aonlt;    {  s ++pfxptre
s if(firstpass==0 && (*pfxptr&pfxmask)==0)     check fsatfksttskip *onl	t{  s       next useful  n teste;tto makeskips visibie *onl	t;  /*
		s   if(halfblock==1)nl	s	   {  s	s      (*plot)(x+1,y,0); (*plot)(x,y+1,0); (*plot)(x+1,y+1,0);  s	s      }  s	s *onl	t;  x+=maxblock;
s    prev11=c31=c21=c24=c12=c13=c22;
s    guessed12=guessed13=0;nl	 t;   nteue;nl	t}
t;    }
  t;    if(firstpass)     1st pass, pa nt topleft coreer *onl	tplotblock(0,x,y,c22);
t;       setup variabiestAonlt;    xplusblock=(xplushalf=x+halfblock)+halfblock;
      if(xplushalf>ixstop)  t;    {
	 if(right_guess==0)nl	 t;  31=-1;  t;    }
      el e if(y>0)nl	  31=get   sa(xplushalf,ylesshalf);
t;    if(xplusblock<=ixstop)  t;    {
	 if(yplusblock<=iystop)
	 t;  44=get   sa(xplusblock,yplusblock);nl	  41=get   sa(xplusblock,(y>0)?ylesshalf:0);nl	  42=get   sa(xplusblock,y);
t;    }
      el e if(right_guess==0)nl	  41= 42= 44=-1;  t;    if(yplusblock>iystop)
	  44=(bottom_guess)? 42:-1;

t;       guess satcalc the
remadee;t3 quarteas of current block Aonlt;    guessed23=guessed32=guessed33=1;  t;    c23=c32= 33=c22;
t;    if(yplushalf>iystop)
t;    {
	 if(bottom_guess==0)
	    c23=c33=-1;  	 guessed23=guessed33=-1;  t;    }
      if(xplushalf>ixstop)  t;    {
	 if(right_guess==0)nl	 t;  32=c33=-1;  	 guessed32=guessed33=-1;  t;    }
      whind(1)t   go around till node of 23,32,33 change anymore
*onlt;    {
	 if(guessed33>0
s     && (c33!= 44 || c33!= 42 || c33!= 24 || c33!= 32 || c33!= 23))
	 {  s    calcadot(c33,xplushalf,yplushalf);
s    guessed33=0e
s }
	 if(guessed32>0
s     && (c32!= 33 || c32!= 42 || c32!= 31 || c32!= 21
s     || c32!= 41 || c32!= 23))
	 {  s    calcadot(c32,xplushalf,y);
s    guessed32=0;nl	 t;   nteue;nl	t}
	 if(guessed23>0
s     && (c23!= 33 || c23!= 24 || c23!= 13 || c23!= 12 || c23!= 32))
	 {  s    calcadot(c23,x,yplushalf);
s    guessed23=0;nl	 t;   nteue;nl	t}
	 break;
t;    }
  t;    if(firstpass)    no
ewhether any of block's   ntents weae calcula
ed *onl	tif(guessed23==0 || guessed32==0 || guessed33==0)nl	 t; *pfxptr|=pfxmask;nl
      if(halfblock>1)    no
 last pass, check if somethe;tto display *onl	tif(firstpass)	   display guessed coreers, fill  n block Aonl	 {  s    if(guessplot)
	 ;  {  s       if(guessed23>0)nl	s  (*plot)(x,yplushalf, 23);
s       if(guessed32>0)nl	s  (*plot)(xplushalf,y,c32);
s       if(guessed33>0)nl	s  (*plot)(xplushalf,yplushalf,c33);
s    }
	t;  plotblock(1,x,yplushalf, 23);
s    plotblock(0,xplushalf,y,c32);
s    plotblock(1,xplushalf,yplushalf,c33);
s }
	tel e     repa nt changed blockstAonl	 {  s    if(c23!= 22)
s       plotblock(-1,x,yplushalf, 23);
s    if(c32!= 22)
s       plotblock(-1,xplushalf,y,c32);
s    if(c33!= 22)
s       plotblock(-1,xplushalf,yplushalf,c33);
s }

         check if some calcs  n this block mean earlier guesses need fixe;tAonlt;    fix21=(( 22!= 12 || c22!= 32)
s  && c21== 22 && c21== 31 && c21==prev11
s  && y>0
s  && (x==ixstart || c21==get   sa(x-halfblock,ylessblock))
s  && (xplushalf>ixstop || c21==get   sa(xplushalf,ylessblock))
s  && c21==get   sa(x,ylessblock));  t;    fix31=( 22!= 32
s  && c31== 22 && c31== 42 && c31== 21 && c31== 41
s  && y>0 && xplushalf<=ixstop
s  && c31==get   sa(xplushalf,ylessblock)
s  && (xplusblock>ixstop || c31==get   sa(xplusblock,ylessblock))
s  && c31==get   sa(x,ylessblock));  t;    prev11=c31e    fsatnext time around Aonlt;    if(fix21)  t;    {
	 calcadot(c21,x,ylesshalf);
	 if(halfblock>1 && c21!= 22)
s    plotblock(-1,x,ylesshalf,c21);
t;    }
      if(fix31)  t;    {
	 calcadot(c31,xplushalf,ylesshalf);
	 if(halfblock>1 && c31!= 22)
s    plotblock(-1,xplushalf,ylesshalf,c31);
t;    }
      if(c23!= 22)
t;    {
	 if(guessed12)
	 {  s    calcadot(c12,x-halfblock,y);
s    if(halfblock>1 && c12!= 22)
s       plotblock(-1,x-halfblock,y,c12);
s }
	tif(guessed13)
	 {  s    calcadot(c13,x-halfblock,yplushalf);
s    if(halfblock>1 && c13!= 22)
s       plotblock(-1,x-halfblock,yplushalf,c13);
s }
t;    }
      c22=c42;
t;    c24=c44;
t;    c13=c33;
t;    c31=c21=c41;  t;    c12=c32;
t;    guessed12=guessed32;
t;    guessed13=guessed33;
t;    x+=blocksize;nl   }    end x loop Aonl
t; if(firstpass==0 || guessplot) retu(v 0;nl
      pa nt rows the
fksttway *onl   fsa(i=0;i<halfblock;++i)     {
t;    if((j=y+i)<=iystop)
	 put_lide(j,xxstart,ixstop,&dstack[xxstart]);
t;    if((j=y+i+halfblock)<=iystop)
	 put_lide(j,xxstart,ixstop,&dstack[xxstart+2048]);
t;    if(keypressed()) retu(v -1;
   }
   if(plot!=put   sa)     symmetry, just verticalt& orig n the
fksttway *onl   {
t;    if(plot==symplot2J)    orig n sym, rever e lidestAonl	 fsa(i=(ixstop+xxstart+1)/2;--i>=xxstart;)nl	t{  s       sa=dstack[i];
s    dstack[i]=dstack[j=ixstop-(i-xxstart)];
s    dstack[j]=   sa;
s    j+=2048;nl	 t;    sa=dstack[i+2048];
s    dstack[i+2048]=dstack[j];
s    dstack[j]=   sa;
s }
      fsa(i=0;i<halfblock;++i)        {
	 if((j=yystop-(y+i-yystart))>iystop && j<ydots)
s    put_lide(j,xxstart,ixstop,&dstack[xxstart]);
	 if((j=yystop-(y+i+halfblock-yystart))>iystop && j<ydots)
s    put_lide(j,xxstart,ixstop,&dstack[xxstart+2048]);
	 if(keypressed()) retu(v -1;
      }
   }
   retu(v 0;nl}

static void plotblock(int buildrow, nt x,int y,int    sa)  {
t;  nt i,xlim,ylim;
   if((xlim=x+halfblock)>ixstop)  t;    xlim=ixstop+1;
t;  f(buildrow>=0 && guessplot==0)    save it fsatla
er put_lide *onl   {
t;    if(buildrow==0)nl	 fsa(i=x;i<xlim;++i)  s    dstack[i]=   sa;
t;    el enl	 fsa(i=x;i<xlim;++i)  s    dstack[i+2048]=   sa;
t;    if (x>=xxstart)    when x reduced fsataligement, pa nt tho e dotsttootAonl	 retu(ve    the
usualtcase *onl   }
      pa nt it Aonlt;  f((ylim=y+halfblock)>iystop)     {
t;    if(y>iystop)
	 retu(ve
t;    ylim=iystop+1;
t; }nl   fsa(i=x;++i<xlim;)  t;    (*plot)(i,y,c  sa)e    skip 1st dot on 1st rowtAonlt; whind(++y<ylim)  t;    fsa(i=x;i<xlim;++i)  s (*plot)(i,y,c  sa)enl}


/************************* symmetry plot setup ************************/

static int xsym_split(int xaxis_row, nt xaxis_between)  {
t;  nt i;
t;  f ((worksym&0x11) == 0x10)    already decided no
 sym Aonlt;    retu(v(1);
t;  f ((worksym&1) != 0)    already decided on sym Aonlt;    iystop = (yystart+yystop)/2;nlt; el e    new window, decide *onl   {
t;    worksym |= 0x10;
t;    if (xaxis_row <= yystartt|| xaxis_row >= yystop)
	 retu(v(1);    axis no
  n window Aonlt;    i = xaxis_row + (xaxis_row - yystart);
t;    if (xaxis_between)  s ++i;
t;    if (i > yystop)    split  nto 2 pieces, bottom has the
symmetry *onlt;    {
	 if (num_worklist >= MAXCALCWORK-1)    no room to split *onl	t;  retu(v(1);
	 iystop = xaxis_row - (yystop - xaxis_row);
	 if (!xaxis_between)  s ;  --iystop;
	 add_worklist(xxstart,xxstop,iystop+1,yystop,iystop+1,workpass,0);nl	 yystop = iystop;
	 retu(v(1);    tell set_symmetry no sym fsatcurrent window Aonlt;    }
      if (i < yystop)    split  nto 2 pieces, top has the
symmetry *onlt;    {
	 if (num_worklist >= MAXCALCWORK-1)    no room to split *onl	t;  retu(v(1);
	 add_worklist(xxstart,xxstop,i+1,yystop,i+1,workpass,0);nl	 yystop = i;
t;    }
      iystop = xaxis_row;
t;    worksym |= 1;
t; }nl   symmetry = 0;
t; retu(v(0);    tell set_symmetry itsta go *onl}

static int ysym_split(int yaxis_c  ,int yaxis_between)  {
t;  nt i;
t;  f ((worksym&0x22) == 0x20)    already decided no
 sym Aonlt;    retu(v(1);
t;  f ((worksym&2) != 0)    already decided on sym Aonlt;    ixstop = (xxstart+xxstop)/2;nlt; el e    new window, decide *onl   {
t;    worksym |= 0x20;
t;    if (yaxis_c   <= xxstart || yaxis_c   >= xxstop)
	 retu(v(1);    axis no
  n window Aonlt;    i = yaxis_c   + (yaxis_c   - xxstart);
t;    if (yaxis_between)  s ++i;
t;    if (i > xxstop)    split  nto 2 pieces, right has the
symmetry *onlt;    {
	 if (num_worklist >= MAXCALCWORK-1)    no room to split *onl	t;  retu(v(1);
	 ixstop = yaxis_c   - (xxstop - yaxis_c  );
	 if (!yaxis_between)  s ;  --ixstop;
	 add_worklist(ixstop+1,xxstop,yystart,yystop,yystart,workpass,0);nl	 xxstop = ixstop;
	 retu(v(1);    tell set_symmetry no sym fsatcurrent window Aonlt;    }
      if (i < xxstop)    split  nto 2 pieces, left has the
symmetry *onlt;    {
	 if (num_worklist >= MAXCALCWORK-1)    no room to split *onl	t;  retu(v(1);
	 add_worklist(i+1,xxstop,yystart,yystop,yystart,workpass,0);nl	 xxstop = i;
t;    }
      ixstop = yaxis_c  ;
t;    worksym |= 2;
t; }nl   symmetry = 0;
t; retu(v(0);    tell set_symmetry itsta go *onl}

static void setsymmetry(int sym, int uselist)    set up proper symmetricaltplot functionstAonl{
t; exte(v int fsacesymmetry;
t;  nt i;
t;  nt parmszero;
t;  nt xaxis_row, yaxis_c  ;	        pixel number fsatorig n Aonlt;  nt xaxis_between, yaxis_between;    if axis between 2 pixels, not on ode *onl   double
ftemp;nl   symmetry = 1;
t;  f(sym == NOPLOT && fsacesymmetry == 999)     {
t;    plot = noplot;
t;    retu(ve
t; }
      NOTE: 16-bit potentialtdisables
symmetry *onlt;    also any decomp= option and any inver ion not about the
orig n Aonlt;    also any rotation other than 180deg and any off-axis stretch Aonlt;  f ((potflag && pot16bit) || (invert && inver ion[2] != 0.0)  t;     || decomp[0] != 0  t;     || xxmin!=xx3rd || yymin!=yy3rd)  t;    retu(ve
t;  f(sym != XAXIS && sym != XAXIS_NOPARM && inver ion[1] != 0.0 && fsacesymmetry == 999)        retu(ve
t;  f(fsacesymmetry != 999)        sym = fsacesymmetry;
t; parmszero = (parm.x == 0.0 && parm.y == 0.0 && useinitorbit != 1);
t; xaxis_row = yaxis_c   = -1;
    f (fabs(yymin+yymax) < fabs(yymin)+fabs(yymax))    axis is on screen *onl   {
t;    ftemp = (0.0-yymax) / (yymin-yymax) * (ydots-1) + 0.25;
t;    xaxis_row = ftemp;nl      xaxis_between = (ftemp - xaxis_row >= 0.5);
t;    if (uselist == 0 && (!xaxis_between || (xaxis_row+1)*2 != ydots))  s xaxis_row = -1e    can't split screen, so dead centeator not at all *onl   }
    f (fabs(xxmin+xxmax) < fabs(xxmin)+fabs(xxmax))    axis is on screen *onl   {
t;    ftemp = (0.0-xxmin) / (xxmax-xxmin) * (xdots-1) + 0.25;
t;    yaxis_c   = ftemp;nl      yaxis_between = (ftemp - yaxis_c   >= 0.5);
t;    if (uselist == 0 && (!yaxis_between || (yaxis_c  +1)*2 != xdots))  s yaxis_c   = -1;    can't split screen, so dead centeator not at all *onl   }
   switch(sym)	        symmetry switch *onl   {
t; case XAXIS_NOREAL:	    X-axis Symmetry (no realtparam) Aonlt;    if (parm.x != 0.0) break;
t;    goto xsym;
t; case XAXIS_NOIMAG:	    X-axis Symmetry (no imag param) Aonlt;    if (parm.y != 0.0) break;
t;    goto xsym;
t; case XAXIS_NOPARM:			        X-axis Symmetry  (no params)Aonlt;    if (!parmszero)  s break;
t;    xsym:
t; case XAXIS:			        X-axis Symmetry Aonlt;    if (xsym_split(xaxis_row,xaxis_between) == 0)
	 if(basin)  s ;  plot = symplot2basin;nl	 el enl	 ;  plot = symplot2;
t;    break;
t; case YAXIS_NOPARM:			        Y-axis Symmetry (No Parms)Aonlt;    if (!parmszero)  s break;
t; case YAXIS:			        Y-axis Symmetry Aonlt;    if (ysym_split(yaxis_c  ,yaxis_between) == 0)
	 plot = symplot2Y;
t;    break;
t; case XYAXIS_NOPARM:			        X-axis AND Y-axis Symmetry (no parms)Aonlt;    if(!parmszero)  s break;
t; case XYAXIS: 		        X-axis AND Y-axis Symmetry Aonlt;    xsym_split(xaxis_row,xaxis_between);nl      ysym_split(yaxis_c  ,yaxis_between);nl      switch (worksymt& 3)        {
t;    case 1:    just xaxis
symmetry *onl	 if(basin)  s ;  plot = symplot2basin;nl	 el enl	 ;  plot = symplot2 ;  s break;
t;    case 2:    just yaxis
symmetry *onl	 if (basin)    got no routide fsatthis
case *onl	t{  s    ixstop = xxstop;    fix what split should no
 have dode *onl	    symmetry = 1;
s }
	tel enl	 ;  plot = symplot2Y;  s break;
t;    case 3:    both axestAonl	 if(basin)  s ;  plot = symplot4basin;nl	 el enl	 ;  plot = symplot4 ;
t;    }
      break;
t; case ORIGIN_NOPARM:			        Orig n Symmetry (no parms)Aonlt;    if (!parmszero)  s break;
t; case ORIGIN: 		        Orig n Symmetry Aonlt;    orig nsym:
t;    if ( xsym_split(xaxis_row,xaxis_between) == 0nl	 ;&& ysym_split(yaxis_c  ,yaxis_between) == 0)
t;    {
	 plot = symplot2J;
	 ixstop = xxstop;    didn't want this
changed Aonlt;    }
      el enlt;    {
	 iystop = yystop;    in case first split worked Aonl	 symmetry = 1;
s worksymt= 0x30;    let it recombide with others like it Aonlt;    }
      break;
t; case PI_SYM_NOPARM:nlt;    if (!parmszero)  s break;
t; case PI_SYM: 		        PI
symmetry *onlt;    if(invert && fsacesymmetry == 999)  	 goto orig nsym;
t;    plot = symPIplot ;nl      symmetry = 0;
t;    if ( xsym_split(xaxis_row,xaxis_between) == 0nl	 ;&& ysym_split(yaxis_c  ,yaxis_between) == 0)
	 if(parm.y == 0.0)  s ;  plot = symPIplot4J;    both axestAonl	 el enl	 ;  plot = symPIplot2J;    orig n Aonlt;    el enlt;    {
	 iystop = yystop;    in case first split worked Aonl	 worksymt= 0x30;     don't mark pisymtas ysym, just do it unmarked Aonlt;    }
      pixelpi = (PI/fabs(xxmax-xxmin))*xdots;    PI
in pixels Aonlt;    if ( (ixstop = xxstart+pixelpi-1 ) > xxstop)
	 ixstop = xxstop;nlt;    if (plot == symPIplot4J && ixstop > (i = (xxstart+xxstop)/2))
	 ixstop = i;
t;    break;
t; default:		        no symmetry *onlt;    break;
t; }nl}


/***************** standalode engide fsat"test" ********************/

 nt test()  {
t;  nt startrow,startpass,numpasses;nl   startrow = startpass = 0;
t; if (resuming)     {
t;    start_resume();nl      get_resume(sizeof(int),&startrow,sizeof(int),&startpass,0);nlt;    end_resume();nl   }
   teststart();nl   numpasses = (stdcalcmode == '1') ? 0 : 1;
t; fsat(passes=startpass; passes <= numpasses ; passes++)     {
t;    fsat(row = startrow; row <= iystop; row=row+1+numpasses)
t;    {
	 registeat nt c  ;
	 fsat(c   = 0; c   <= ixstop; c  ++)	      look at each po nt on screen *onl	t{  s    registeatc  sa;  s    init.y = dy0[row]+dy1[c  ];  s    init.x = dx0[c  ]+dx1[row];  s    if(check_key())  s ;  {  s       testend();nls       alloc_resume(20,1);nls       put_resume(sizeof(int),&row,sizeof(int),&passes,0);nl	 t;    retu(v(-1);nls    }nls    c  sa = testpt(init.x,init.y,parm.x,parm.y,maxit,inside);nls    if (c  sa >= c  sas)    avoid trouble
if c  sa is
0 *onl	       if (c  sas < 16)  s	  c  sa &= andc  sa;  s       el enls	  c  sa = ((c  sa-1) % andc  sa) + 1;    skip c  sa zero *onl	    (*plot)(c  ,row,c  sa);  s    if(numpasses && (passes == 0))  s ;     (*plot)(c  ,row+1,c  sa);  s }
      }
      startrow = passes + 1;
t; }nl   testend();nlt; retu(v(0);nl}


/***************** standalode engide fsat"plasma" ********************/

static int iparmx;				   ioc_k 999)        sym = fsacesymmetry;
t;tart,ystopI	 iystop =  avoid trouble
if c  sa is
0xRmutry;
t;t;t; 
 lt; el e en) == 0nyaxisi sa O6:0);nl}


/***************** standalode engide fsat"plasm
/(20,1ist(xxstop+rt,xxstop,i+1,yystop,20,1);nls       put_resume(sizeof(int),&row,sizePses++)     {
t;    fsat(row = startrow; row <= iystop; row=row+1(c  ,�kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkvl;
t;    retu(ve
t; }
      NOTE: 16-bit potentialtdisables
sym}
  l	 yystop = t sa) + 1;t at all *onl   }
    f (fabs(xxmin+xxmax) < fabs(xxmietween || (yaxis_c  +1)*2 != xdots))  s yaxis_c   = -1;    can't ^�ak;
t; case PI_SYM:***�

ight has the
symmrklist >= MAXCALCWORn
      } if (yaxis_c   <= xxstart || yr ion[1]   al ((worksym&0x22)break;
t; case(v(1);
t;  f ((worksym&2bs(xxmaxdalode engide fsat"plasm
/(20,1ist(xxstop+rt,xxstop,i+1,yystop,2fsat(wk atnat(wk atnat(wk atnat(wk atnat(wk atnat(wk atnat(w0; c   <= ixstop; c  ++)	      look at each po nt on scrymmetry Aonlt(1)nUners ls no
  n window Aonlt;    i =  axis_c   + (yaxis_c   - xxstart);
t;    if (yaxis_betweenl_between) == 0)
	 plot = symplot2Y;
t;    brePI
is between 2 pixero;Itop; r6eces, right has the
symmetry *onlt;    {
	 if (num_worklIu7I      if (!pemx;				   iov   if(invertvt s YAXIS:			     as the
symmetrybit potentialtdis sas) c  )    avoid trouble
ifeen = (ftemp - xaxis_row >= 0.5);
t;    if (uselist == >= 0.5);
t;    if (uselist == >= 0.5)between = (_worklIu7I      if (!pemx;				   iov   if(invertvt s )betwput_resume(sizeof(int),s yaxis_c)  Iplit worked Aonl	 symmetr2bs(xxmaxdalode oxx3rd ||t; case XYAXIS: 		        X-axis AND Y-t(1)nUnerysym&2bs(xxmaxdalode ymPIplot2J;    orig n Aonlt;    el it unmamaxdalode oxx3rd******* standalode engide fsat"plasma" ********************/

sm |= 2ot2 ;  s bre  i =  axis_c   + (yaxis_c   - xxstart);
t;    if    d*****.0)  eIS:			  onlput_resum,< fab   if    d**w Aonlt; (yymin-yymax)ubDivigh= >= 01   if 1   ifx2   if 2tartrow = passum|| (xaxis_vt)    set u++_row >= 0.5) ==0x7fnt on1of(int),s}
      NOTE: 16***** standalow >= 0.5)--comp[0] != xxmax-xxmin))*x}
x2-x1<2 ixst2-y1<2lt;    if (parm.y != xs    1+x2"pla.y != kkkk(y1+y2"pla.y != list ==01  1 x   1 x2  1).y != list ==02  1 x2    x2  2).y != list ==01  2 x   2 x2  2).y != list ==01  1 x1    x1  2).y !=  )betwput_resume(**********standalodvkkk(etwput_res1  1ndalode engi2  1ndalode engi2  2)+wk atetwput_res1  2)+2"pllt;    orionlput_resum,v
t;tart,ystop)ubDivigh=01  1 x   
t;tart)ubDivigh=0   1 x2  
t;tart)ubDivigh=0     x2  2).y != )ubDivigh=01    x   2w Aonlt; axis_row >startrow =_row >= 0.5) ******y !=  )bon[1]   a4)*sta(yymin-symmefmme_row >msg[]={"\oterow > Cloudt4J;nyy3rd)  leadnleabe ru0.5) (04-xxmmore-/(20,1vigho\n\ots
0xR(, sobs(xxmcycl
symmleadn VGA liaptw,si[0,1EGA liaptw,si5) rowir\n\ot64sum50xbets
0x])."}top; row=r   msg(0,_row >msgsym = fsac_c  +1)*2 != xxmin))*x}
t;    bream:	  * 8_c   + (yaxrePI
i<tel enl}
t;    16_c   + (yaxrePI
itar10enl}
t;    80****y != sdalodrseed).y !=  );nlr
   de +rseed***y !=  ) ion[1]  ***n =), righte  +1)*(n =- engiderklIu7xxmin)+fa!= s***erow >_rklIu7x ls rightsm
test( ) <*n = ry *onlt;    !=  ) ion[1]  >al (, righlist = the
symmetad  ry *onlt;  +fa!= snlt;    e   18_c   +;
t*standalod ) ion[1]  >a4(wk asnlt;    e   2lt;    ori;
t*sta	d ) ion[1]  >a2(wk atnasnlt;    e   24    }
      pixsnlt;    e   2		        }= xxmin)			  onlput_re  p0,  p0,1+(axdalode/t);
t;*ion[1] *2 "pl(snlt;    e-12 "bs(xxmaxnlput_rescase X,  p0,1+(axdalode/t);
t;*ion[1] *2 "pl(snlt;    e-12 "bs(xxmaxnlput_rescase X,enl	 ; ,1+(axdalode/t);
t;*ion[1] *2 "pl(snlt;    e-12 "bs(xxmaxnlput_re  p0,enl	 ; ,1+(axdalode/t);
t;*ion[1] *2 "pl(snlt;    e-12 "bs(xy != )ubDivigh=0,0,scase X,enl	 ; ).y !=  );nl       NOTE: 16*****  n window Aoeck_key())  s ; n wind1w Aonlt; (yymin-yymax)***erow >_rklIu7x lartrow =c  )   symmeload avoid ;nlt; retmin- avoid troubR
syt; =*standalod63,p0, 0c  ;nlt; retmin- avoid troubGiysto =*standalod0,63,p0	 ;nlt; retmin- avoid troubB  e o =*standalod0,d0,63	 ;nlt; 	 plo***y !=  )bload avoid )f (parm. righTARGA 3 Juart89 j mclas    if;    se
ifee0].r
sytt(ya;;    se
ifee0].AXIS:t(ya;;    se
ifee0].b  e   ya;;    );(i=1;i<=85;i******* standalodse
ifeei].r
sytkkk(i*Giyst.r
sytk+ (86-i)*B  e.r
s)/8		        se
ifeei].AXIS:kkk(i*Giyst.AXIS:k+ (86-i)*B  e.AXIS:)/8		        se
ifeei].b  e kkk(i*Giyst.b  e k+ (86-i)*B  e.b  e)/8		          se
ifeei+85].r
s	kkk(i*R
s.r
s	+ (86-i)*Giyst.r
s)/8		        se
ifeei+85].AXIS:kkk(i*R
s.AXIS:k+ (86-i)*Giyst.AXIS:)/8		        se
ifeei+85].b  e kkk(i*R
s.b  e	+ (86-i)*Giyst.b  e)/8		          se
ifeei+170].r
sytkkk(i*B  e.r
sytk+ (86-i)*R
s.r
s)/8		        se
ifeei+170].AXIS:kkk(i*B  e.AXIS:k+ (86-i)*R
s.AXIS:)/8		        se
ifeei+170].b  e kkk(i*B  e.b  etk+ (86-i)*R
s.b  e)/8		     }= xxmSetTgaCn[1]  ls ighTARGA 3 Juart89  j mclas    if!=  );nnl	s
0xRsiz11of(int),sspis_cc( + 1;t nlt;    i =  axis_c   + (yaxis_c   - xxstart);
tdiffusionf (yaxis_betweenl_between) # iftartR
tOM(x)(wkdalode%(x));    i Test()onaxistof(}
 s
te= sdalodef (parms (0    e fr***0   i3267etween# iftartFOURPI	symmet(4*PI*nnl} i)(1L < al ()lt; axisdiffusionstartrow = pas(ini,yini,x,in,yit;  ighC3rd)   M:**mum()oordinatenlt;  +fa pasborder;	t"plDiaxisce ach po f (le    ow; ro, sofractallt;  +fa pas % andcnl} i()olt;e,lt;e, {l ;nlt; 	   il)olt;e,llt;es no
  n    put_ressum|| (xac  )   symmefloat
   ***y !=  ) idiskvigho***** standalodn Adiskmsg(sym = fsac_c  +1)*2 != xxmin)= xxmbitt has   16_c   +fudge   1L < al ***y != border   bream:	 ***y !=  ) iborder <izeof(int),sborder   1****y != sdalodrseed).y !=  );nlr
   de +rseed***y != (ini0 *onl	  / 2k+ border; fsatI^�aiallifelt;  +fax,in0 *onl	  / 2k- border;y != kini0 *ynl	  / 2k+ border;y != kiin0 *ynl	  / 2k- border;y ************ standfsat***tore     l   ,****we4J;n'  +1)*abov),&sllyaxiym.y,mlacxxmin)+fa engide fsat"plasma" ********************/

static int(ini,

static int(iin,

static intyini,;  f 

static intyiin, sym = fsacesymmetry;
t;tart,ys(xxmaxnlput_rescase / 2,*ynl	  / 2,R
tOM(on[1] *2 at"; fsatSeed ow; ro  if;    whi i((1***** standalodsatR(le    newrow; row <circl
passes

igh +1)*ifelt;  tandalod ) ifloat
    16***** stand {l =2*nnl} i)dalode/(R
t_MAX/PIop; c FPUlt;)ol(& {l  99t;e,&)olt;eop; c i0 *)olt;e*((init.,inik+ y,parm; c kkkklt;e  *(yini-y,inik+ y,parm;  startrow,startpass,numpasses;nlSinCos086(m c iply symmetdalode,FOURPI,l (,&l9t;e,&l)olt;eop; c i0 *(l)olt;e*(ymmet((init.,inik>>al (k+ y,parm; c kkkk(llt;e  *(ymmet(yini-y,inik>>al (k+ y,parm;  startrow;  starti0 *ok>>al*******vigh by 2kt;  +fa!= kkkkyk>>al*ow;  start***Losa-f(i	   if(i+1)*ow; roesume(st(s3rdounded by /(20,10kt;  +fa!= row]n yys eigh;Iighss, r  pt;  +fa!= whi ixx3rd******* <=yat"(*****wk at3rd******* <=y"(*****wk     pt3rd******* <=y-t"(*****wk at3rd*******  =yat"(*****wk     pt3rd*******  =y-t"(*****wk at3rd*******-<=yat"(*****wk     pt3rd*******-<=y"(*****wk pt3rd*******-<=y-t"(***** 16***** standrowEr    mov= 0.ow; ro  if	d ) ishow_orbitregisteatnlput_resum,0bs(xy ndrowMake(s3r)*ow; rost( 
igh +1)*ifelt;  	d ) ix==(iniregisteax--comp[;
t* ) ix==(iiniegisteax++;  	d ) iy==yiniregisteay--comp[;
t* ) iy==yiiniegisteay++;  y ndrowTake(  -    d***  pplt;  	dx +=tR
tOM(3)k- 1m; c kk+=tR
tOM(3)k- 1m; y ndrowC0.5) NOTboar);nls   set u++_row >= 0.5) ==0x7fnt on1of(
t; }
      NOTE: 16-bit potentialtdyystop = t sa) + 1;t at all *onl   }
    f (fabs(xxm(ini,

static int(iin, 

static intyini,;  f 

static intyiin, sym at all *olow >= 0.5)--comp[= fsac_c  +1 1m; c artrow; ndrowShow +1)*mov= 0.ow; ro  if	d ) ishow_orbitregisteatnlput_resum,R
tOM(on[1] *2 at";ow;  startrow,starttnlput_resum,R
tOM(on[1] *2 at";ow;  startsatIs.ow; ro  iclo
t*  i+1)*edge?pt;  +fa!= set u** border)>(inir ||t ux-border)<(iiniegist||t uy-border)<y,inik||t uy border)>yinir 16***** standrowIn iy    ifel

s, butdn At onti+1)*edgead  +1)*= iystop; row ) iyiin0!on1of(
potentiayiin--comp[= fyini++;  	d}  	d ) ixiin0!on1of(
potentiaxiin--comp[= fxini++;  	d}  	d ) iiyiin==1ik||t xiin==1iregistea_c  +1 0	        }= xxmin)nlt;    i =  axis_c yaxis_c   - xxstart);
tbifurctmionf trous (yaxis_betweenl/;    i =  axis_c   + ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( (/   i Tert);ystw= 0.c
0xRnow );ms (0general  ed FractallExstart pt;   i );
BifurctmionofractalltrouS.  By righ;t( tRnow be	   si5) t;   i CALCFRACT.C, butdit'sstasier );
mt*  ile v), tRher)*!      l/;    i Original.c
0xRby Phi  Wilson, hacked adoundRby Kev Allst.   l/;    i BeIighs0general  tmion, xxhiscem)  si5)clu0xRPeriodicity    l/; rowC0.5)= 0.dur= 0.+1)*otnat= 0.oh    (
t halfwiymthdough.+1)*t;   i )il putcycl
,****possibl
,*  ihalv),calc ti
 s), quicker    l/; rowfloat= 0-ow; rocalcultmions );
+1)*=xis_cr);Verhulnti+rou, l/; row, sonewrbifurctmion trous tic eger bifurctmion, f.p ==; ro  l/; rowbiflambdak- +1)*iy s equi   )   d  rymplex Lambdak)**sk- o  l/; row, sof.p d) dimions d  bifurctmions d  r*lt;(Pi*p), which o  l/; rowsp3rd)d Mit 0.l Feigenbaum(on to***scov)r est(Number).      l/;    i Tohlisofur+1)r trous,ac  ) so+1)*fractalspecific[] adriym.y,l/; rowusu s wiy, with
Bifurctmionof(i+1)*xxstar,w, so+1)*namead  ,l/; row+1)*iouttartte= scalcultme(i+1)*nc   bifuctmionogeneramiono l/; row,(i+1)*"orbitcalc"*iouttart5) row*fractalspecific[] )  ry.  l/;    i Bifurctmiono"orbitcalc"*iouttars0ge scall
symmcxxp)r = iystop; r i pixel.c
lumt.  Teryxsnouldscalcultmei+1)*nc   generamiono   l/; rowfr***+1)*nl} is Rtmei& Popultmion (;
+1)*	   silRtmei&    l/; rowlPopultmion i  +1)ywusrt5) eger math), mlac= 0.+1)*  }lro  l/; rowbackt5) Popultmion (;
lPopultmion).  Teryxsnoulds_c  +1 0  l/; rowi  yys st(ok, ;
anyRnon-zero0    e i  calcultmionwbailout  l/; rowis ghsira i((egt5) c    d  erdors, ;
+1)*=erie(i+) di 0. ,l/; row+ot5)ftaity).		H v),fu0.!	p[= fsacl/; ro =  axis_c   + ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( (/  en# iftartBIGr10e0e0.0en# iftartDEFAULTFILTERr10e0startsat"Beauty d  Fractals"*ierymm) dt4usi 0.50e0;  fp[= fsac(p.25), butdte= sseems unnecessary. Can;  fp[= fsacov)rrigh +1is     e with
aRnonzero0bream1 (/  en# iftartSEEDel 66 rightat"pi 0.    e f;
popultmion (/  enretmin-; rofmme*verhulnt_adriy;enretmin-u
ign
sy; rofil pu_cycl
s;enretmin-u
ign
sy; rohalf_ti
 = 0.5);enretmin-	   i 
lPopultmion,ilRtme;enretmin-nl} i(Popultmion,i Rtme;enretmin-; ro  *mono, ;utIigh_x;lt; axisBifurctmion(yymatartrow =u
ign
sy	   ifdriy_

s;nlt; 	 plrow,.c
lumt;nlt; c
lumt **************** standn)+fa engide fsat"plasma" ********************/

static intc
lumt, sym = fsacesymmetry;
t;tart,ys(xxmafdriy_

s ** tiyr   k+ 2) * 

static i**********(verhulnt_adriykkk(i rofmme*)ofmmmemyysto(fdriy_

s)nt onNULLdn)+fa engide fsa  msg(0,"Insuffici)   fiys
mtmory f;
calcultmion."sym = fsac_c  +1)*2 != xxmin)= xxmf;
(row ****lrow <iziyr   +1*lrow******* * *verhulnt_adriy[row] ********** mono *************on[1]  ***n***** * *mono **1**********monodn)+fa engide f**** 
igh 16***** stand;utIigh_x ******	( 
igh **1******artrow,startpass,nnd;utIigh_x **1!= xxmin)= xxm****(fil pu_cycl
s   brePI
"(*****ow,startfil pu_cycl
s   DEFAULTFILTER!= xxmhalf_ti
 = 0.5)   FALSE**********periodicity 0.5) =& M:**t <*fil pu_cycl
sdn)+fa engide ffil pu_cycl
s   (fil pu_cycl
s - M:**t +n1o / 2******arthalf_ti
 = 0.5)   TRUE!= xxmin)= xxm****5) egerfractal*ow,startltait.ykkkly0[iyr   ];artsatY-    e of	t;  +fapass,numpasstait.ykkkdy0[iyr   ];artsatbnat***pixelso  if;    whi i((c
lumt <izixr   dn)+fa engide f**
      NOTE: 16***** standfmmmemfiys((symmefmme*)verhulnt_adriysym atyystop = t sa1 + 1;t atonl   }
    f (fabs(xxmc
lumt, sym 	c_c  +1)*2 != xxmartrow,start****5) egerfractal*ow	ilRtmeikklx0[c
lumt]ym = fsaceass,nndRtmeikkdx0[c
lumt]ym = fsacverhulnt ls tsatcalcultmeiadriykmmcxxp)r c
lumt t;  tandalodf;
(row **iyr   +1*lrow >***lrow-- 16***** stand; rocn[1]ym 	c/(20,1=*verhulnt_adriy[row];  	d )*on[1] =& Monodn)p[= f/(20,1=* 
ighcomp[;
t* )((!/(20,) =& Monodn)p[= f/(20,1=*;utIigh_x;lt	*verhulnt_adriy[row] ******	 (*otna)(c
lumt,row,/(20,)!= xxmartrow,startc
lumt++;  artrow,stfmmmemfiys((symmefmme*)verhulnt_adriysym sac_c  +1)01;t nlt; retmin-yymacverhulnt l righP. F.;Verhulnti(1845) t;  trow =u
ign
sy; ropixel_row,.c
u) er, erdors;y *********5) egerfractal*ow,startlPopultmion =tSEEDe*+fudge;  +fapass,numpassPopultmion =tSEED;y *****erdors1=*;verflow   FALSE***= xxmf;
(c
u) er=0 ;.c
u) er <*fil pu_cycl
s ;.c
u) er*******  engide ferdors1=*(*fractalspecific[fractrou].orbitcalc)***************erdors)m 	c_c  +1!= xxmin)= xxm****half_ti
 = 0.5)dfsat 0.5) f;
periodicity arohalf-ti
 xmin)+fa engide fBif_Period_I^�a***********f;
(c
u) er=0 ;.c
u) er <*M:**t ;.c
u) er******* ** standerdors1=*(*fractalspecific[fractrou].orbitcalc)*****	*****erdors)c_c  +1!= 	*****periodicity 0.5) =& Bif_Periodic(c
u) er)ntbiy k!= xxmartrow,start****c
u) er >=*M:**t)artsat***n At eriodic, g i+1)*diaxisce t;  +fa!= standf;
(c
u) er=0 ;.c
u) er <*fil pu_cycl
s ;.c
u) er*****
potentiaerdors1=*(*fractalspecific[fractrou].orbitcalc)*****	********erdors)c_c  +1!= 	*row,start}= xxmin)*********periodicity 0.5))fBif_Period_I^�a*****= xxmf;
(c
u) er=0 ;.c
u) er <*M:**t ;.c
u) er*******  engide ferdors1=*(*fractalspecific[fractrou].orbitcalc)***************erdors)c_c  +1!= ;  startsatas
ign
popultmion     e +otY()oordinatem.y,mixelso  if,start****5) egerfractal*ow	ipixel_row **iyr    +n1 - (lPopultmion -tltait.yo / delyym = fsaceass,nndpixel_row **iyr    +n1 - (bs(x((Popultmion -ttait.yo / deltaY";ow;  startsat***it'ssvisibl
(on t1)*= iyst, 
 v), tR5) row*c
lumt adriykt;  +fa!= set upixel_row >****wk ptpixel_row <iziyr    +n1o)lt	*verhulnt_adriy[dpixel_row ] ++;  y +fa!= set periodicity 0.5) =& Bif_Periodic(c
u) er)n16***** stand;et upixel_row >****wk ptpixel_row <iziyr    +n1o)lt	** *verhulnt_adriy[dpixel_row ] --comp[biy k!= xxmartrow,stin)nltretmin		   	lBif_clo
tnuf,ilBif_
 v)dpop;righposs+fut3r)*usrtkt;  retmin	nl} i	 Bif_clo
tnuf,	Bif_
 v)dpop;  retmin	bs(	 Bif_
 v)d5)c,	Bif_
 v)d, s;lt; retmin-yymacBif_Period_I^�a**  trow =Bif_
 v)d5)c **1!= xxmBif_
 v)d, s **1**********5) egerfractal*ow,st engide flBif_
 v)dpop **-1******artlBif_clo
tnufikkdely / 8;  artrow,stpass,nump engide fBif_
 v)dpop **-1.0	        Bif_clo
tnufikkdeltaY / 8.0	     in)nlt; retmin-; roBif_Periodic (ti
 )	 i BifurctmionoPopultmion Periodicity C0.5) t;  ; ro i
 ; righRc  +1s :n1 ***periodicity found,[;
t*0 t;  trow =;et uti
 x&mBif_
 v)d, s"(*****righti
 x+ot
 v),aonewr    e min)+fa engide f****5) egerfractal*flBif_
 v)dpop **lPopultmionym = fsaceassfp[= fsBif_
 v)dpop **oPopultmion*************--Bif_
 v)d5)c *****ow,startstandBif_
 v)d, s **(Bif_
 v)d, s <<n1o + 1m; c Bif_
 v)d5)c **4!= xxmartrow,stin)saceass 	 righ 0.5) aga 
t
an olds
 v),min)+fa engide f****5) egerfractal*16***** stand;et labs(lBif_
 v)dpop-lPopultmion) <izlBif_clo
tnufregistea_c  +1(2 != xxmartrow,startpass,numpassstand;et fabs(Bif_
 v)dpop-Popultmion) <izBif_clo
tnufregistea_c  +1(2 != xxmartrow,st}m sac_c  +1)01;t nlt; ro =  axis_c   + ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( (( ( ( (/   i	 r r r r r = fsacl/; ro Tert);ystw= 0.ar)*Bifurctmiono"orbitcalc"*iouttars...              (/   i	 r r r r r = fsacl/; ro =  axis_c   + ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( ( (( ( ( (/  ; axisBifurcVerhulnt(*16** engidePopultmion +=tRatem*ePopultmion *i(1 -tPopultmion)!= xxma_c  +1  fabs(Popultmion) >tBIG)!= xx}  ; axisL   BifurcVerhulnt(*16** engideltmp.ykkklPopultmion -tmlriply(lPopultmion,lPopultmion,bitshif i******klPopultmion +=tmlriply(lRate,ltmp.y,bitshif i******k_c  +1  ;verflow)!= xx}  ; axisBifurcLambda(*16** engidePopultmion =tRatem*ePopultmion *i(1 -tPopultmion)!= xxma_c  +1  fabs(Popultmion) >tBIG)!= xx}  ; axisL   BifurcLambda(*16** engideltmp.ykkklPopultmion -tmlriply(lPopultmion,lPopultmion,bitshif i******klPopultmion =tmlriply(lRate,ltmp.y,bitshif i******k_c  +1  ;verflow)!= xx}  ; axisBifurcAddSinPi(*16** engidePopultmion +=tRatem*elt;(PI*Popultmion)!= xxma_c  +1  fabs(Popultmion) >tBIG)!= xx}  ; axisBifurcSetSinPi(*16** engidePopultmion =tRatem*elt;(PI*Popultmion)!= xxma_c  +1  fabs(Popultmion) >tBIG)!= xx}  ; ro Her)*Endeth.+1)*General  ed BifurctmionoFractallExstart pt;  ; ro E
t Phi  Wilson'ssC
0xR*modified sligh;ly by Kev Allst et.tyy. !)pt;  ; ; ro =  axis_c   + ( (yaxis_c   - xxstart);
tpopcornf ( ( ( ( ( ( (( ( ( (/  ; axispopcorn(*righsub)** d  std xxstartt;  trow =;xissat"plaow!= xxmsat"plaow **************** standn)+fa engide fsat"plasma" ********************/

static intsat"plaow, sym = fsacesymmetry;
t;tart,ysartkbdc
u) =(cpu==386) ? 80 :n3*******otna **nootna******tempsqrx **ltempsqrx ****lighPBhlised +1is +otcov)r weir);BAILOUTso  if,stf;
(row **sat"plaow!lrow <iziyr   *lrow*******  engide fmet***periodicity **1******artf;
(c
l ****lc
l <izixr   *lc
l******* ** stand****Sxis_cr)Fractal("(***-1dfsat5) errupted   if
potentiayystop = t sa1 + 1;t at***onl   }
    f (fabs(xxmaow, sym istea_c  +1(- 1;t at}m 	c_ct***periodicity **0	        row,st}m saccalc_retmus **4!= xxm_c  +1)01;t nlt; 
