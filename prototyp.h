/* includes needed to define the prototypes */

#include <stdio.h>
#include "mpmath.h"
#include "port.h"
#include "fractint.h"
#include "helpcom.h"

/*  calcmand -- assembler file prototypes */

extern int cdecl calcmandasm();

/*  calmanfp -- assembler file prototypes */

extern void cdecl calcmandfpasmstart();
extern int  cdecl calcmandfpasm();

/*  fpu087 -- assembler file prototypes */

extern void cdecl FPUcplxmul(_CMPLX *, _CMPLX *, _CMPLX *);
extern void cdecl FPUcplxdiv(_CMPLX *, _CMPLX *, _CMPLX *);
extern void cdecl FPUsincos(double *, double *, double *);
extern void cdecl FPUsinhcosh(double *, double *, double *);
extern void cdecl FPUcplxlog(_CMPLX *, _CMPLX *);
extern void cdecl SinCos086(long , long *, long *);
extern void cdecl SinhCosh086(long , long *, long *);
extern long far cdecl r16Mul(long , long );
extern long far cdecl RegFloat2Fg(long , int );
extern long cdecl Exp086(long);
extern unsigned long far cdecl ExpFudged(long , int );
extern long far cdecl RegDivFloat(long , long );
extern long far cdecl LogFudged(unsigned long , int );
extern long far cdecl LogFloat14(unsigned long );
#ifndef XFRACT
extern long far cdecl RegFg2Float(long, char);
extern long far cdecl RegSftFloat(long, char);
#else
extern long far cdecl RegFg2Float(long , int );
extern long far cdecl RegSftFloat(long , int );
#endif

/*  fpu387 -- assembler file prototypes */

extern void cdecl FPUaptan387(double *, double *, double *);
extern void cdecl FPUcplxexp387(_CMPLX *, _CMPLX *);

/*  fracsuba -- assembler file prototypes */

extern int cdecl  longbailout( void );
extern int FManOWarfpFractal( void );
extern int FJuliafpFractal( void );
extern int FBarnsley1FPFractal( void );
extern int FBarnsley2FPFractal( void );
extern int FLambdaFPFractal( void );

/*  general -- assembler file prototypes */

extern  long   cdecl multiply(long, long, int);
extern  long   cdecl divide(long, long, int);
extern  int    cdecl getakey(void);
extern  void   cdecl buzzer(int);
extern  void   cdecl farmemfree(VOIDFARPTR );
extern  int    cdecl far_strlen( char far *);
extern  int    cdecl far_strnicmp(char far *, char far *,int);
extern  void   cdecl far_strcpy( char far *, char far *);
extern  int    cdecl far_strcmp( char far *, char far *);
extern  void   cdecl far_stricmp(char far *, char far *);
extern  void   cdecl far_strcat( char far *, char far *);
extern  void   cdecl far_memset( VOIDFARPTR , int      , int);
extern  void   cdecl far_memcpy( VOIDFARPTR , VOIDFARPTR , int);
extern  int    cdecl far_memcmp( VOIDFARPTR , VOIDFARPTR , int);
extern  void   cdecl far_memicmp(VOIDFARPTR , VOIDFARPTR , int);
extern  void   cdecl emmdeallocate(unsigned int);
extern  void   cdecl emmgetpage(unsigned int, unsigned int);
extern  void   cdecl emmclearpage(unsigned int, unsigned int);
extern  int    cdecl keypressed(void);
extern  long   cdecl readticker( void );
extern  void   cdecl emmdeallocate(unsigned int);
extern  void   cdecl xmmdeallocate(unsigned int);
extern  void   cdecl snd( int );
extern  void   cdecl nosnd( void );
extern  void   cdecl initasmvars( void );

#ifndef __BORLANDC__
extern  void   cdecl enable( void );
extern  void   cdecl disable( void );
extern  void   cdecl delay( int );
#endif

extern  int    cdecl farread(int, VOIDFARPTR, unsigned);
extern  int    cdecl farwrite(int, VOIDFARPTR, unsigned);
extern  long   cdecl normalize(char far *);
extern  unsigned int cdecl xmmmoveextended(struct XMM_Move *);
extern  void   cdecl erasesegment(int, int);
extern  int    cdecl IITCoPro( void );
extern  int    cdecl F4x4Check( void );
extern  int    cdecl F4x4Lock( void );
extern  void   cdecl F4x4Free( void );
extern  int    cdecl getakeynohelp( void );
extern  unsigned int cdecl cmpextra( unsigned int, char *, int );
extern  unsigned int cdecl fromextra( unsigned int, char *, int );
extern  unsigned int cdecl toextra( unsigned int, char *, int );
extern  void   cdecl load_mat(double (*)[4]);
extern  VOIDFARPTR cdecl farmemalloc(long);
extern  unsigned int *cdecl xmmquery(void);
extern  BYTE far *cdecl emmquery(void);
extern  unsigned int cdecl emmgetfree(void);
extern  unsigned int cdecl emmallocate(unsigned int);
extern  unsigned int cdecl emmallocate(unsigned int);
extern  unsigned int cdecl xmmallocate(unsigned int);
extern  void   mult_vec_iit(VECTOR);

/*  lsysa -- assembler file prototypes */

extern void lsys_doplus(long);
extern void lsys_doplus_pow2(long);
extern void lsys_dominus(long);
extern void lsys_dominus_pow2(long);
extern void lsys_dopipe_pow2(long);
extern void lsys_dobang(long);

#ifndef XFRACT
extern void lsys_doslash_386(long);
extern void lsys_dobslash_386(long);
extern void lsys_doat_386(long);
extern void lsys_dosizegf_386(long);
extern void lsys_dodrawg_386(long);
#endif

/*  mpmath_a -- assembler file prototypes */

extern struct MP * MPmul086(struct MP , struct MP );
extern struct MP * MPdiv086(struct MP , struct MP );
extern struct MP * MPadd086(struct MP , struct MP );
extern int         MPcmp086(struct MP , struct MP );
extern struct MP * d2MP086(double );
extern double    * MP2d086(struct MP );
extern struct MP * fg2MP086(long , int );
extern struct MP * MPmul386(struct MP , struct MP );
extern struct MP * MPdiv386(struct MP , struct MP );
extern struct MP * MPadd386(struct MP , struct MP );
extern int         MPcmp386(struct MP , struct MP );
extern struct MP * d2MP386(double );
extern double    * MP2d386(struct MP );
extern struct MP * fg2MP386(long , int );
extern double *    MP2d(struct MP );
extern int         MPcmp(struct MP , struct MP );
extern struct MP * MPmul(struct MP , struct MP );
extern struct MP * MPadd(struct MP , struct MP );
extern struct MP * MPdiv(struct MP , struct MP );
extern struct MP * d2MP(double );  /* Convert double to type MP */
extern struct MP * fg2MP(long , int ); /* Convert fudged to type MP */

/*  newton -- assembler file prototypes */

extern int cdecl    NewtonFractal2( void );
extern void cdecl   invertz2(_CMPLX *);

/*  tplus_a -- assembler file prototypes */

extern void WriteTPlusBankedPixel(int, int, unsigned long);
extern unsigned long ReadTPlusBankedPixel(int, int);

/*  video -- assembler file prototypes */

extern void   cdecl adapter_detect(void);
extern void   cdecl clearbox(void);
extern void   cdecl dispbox(void);
extern void   cdecl setvideotext(void);
extern void   cdecl setnullvideo(void);
extern void   cdecl setfortext(void);
extern void   cdecl setforgraphics(void);
extern void   cdecl swapnormwrite(void);
extern void   cdecl setclear(void);
extern int    cdecl SetupShadowVideo(void);
extern int    cdecl ShadowVideo(int);
extern int    cdecl keycursor(int,int);
extern void   cdecl swapnormread(void);
extern void   cdecl setvideomode(int, int, int, int);
extern void   cdecl movewords(int,BYTE far*,BYTE far*);
extern void   cdecl movecursor(int, int);
extern void   cdecl get_line(int, int, int, BYTE *);
extern void   cdecl put_line(int, int, int, BYTE *);
extern void   cdecl setattr(int, int, int, int);
extern void   cdecl putstring(int,int,int,CHAR far *);
extern void   cdecl spindac(int, int);
extern void   cdecl find_special_colors(void);
extern char   cdecl get_a_char(void);
extern void   cdecl put_a_char(int);
extern void   cdecl scrollup(int, int);
extern void   cdecl home(void);
extern BYTE far *cdecl  findfont(int);
extern int _fastcall getcolor(int, int);
extern void _fastcall putcolor(int, int, int);
extern int  out_line(BYTE *, int);
extern void   (*swapsetup)(void);

/*  3d -- C file prototypes */

extern void identity(MATRIX);
extern void mat_mul(MATRIX,MATRIX,MATRIX);
extern void scale(double ,double ,double ,MATRIX);
extern void xrot(double ,MATRIX);
extern void yrot(double ,MATRIX);
extern void zrot(double ,MATRIX);
extern void trans(double ,double ,double ,MATRIX);
extern int cross_product(VECTOR,VECTOR,VECTOR);
extern int normalize_vector(VECTOR);
extern int vmult(VECTOR,MATRIX,VECTOR);
extern void mult_vec_c(VECTOR);
extern int perspective(VECTOR);
extern int longvmultpersp(LVECTOR,LMATRIX,LVECTOR,LVECTOR,LVECTOR,int);
extern int longpersp(LVECTOR,LVECTOR,int );
extern int longvmult(LVECTOR,LMATRIX,LVECTOR,int );

/*  calcfrac -- C file prototypes */

extern void calcfrac_overlay(void);
extern int calcfract(void);
extern int calcmand(void);
extern int calcmandfp(void);
extern int StandardFractal(void);
extern int test(void);
extern int plasma(void);
extern int diffusion(void);
extern int Bifurcation(void );
extern int BifurcLambda(void);
extern int BifurcSetTrigPi(void);
extern int LongBifurcSetTrigPi(void);
extern int BifurcAddTrigPi(void);
extern int LongBifurcAddTrigPi(void);
extern int BifurcMay(void);
extern int BifurcMaySetup(void);
extern int LongBifurcMay(void);
extern int BifurcLambdaTrig(void);
extern int LongBifurcLambdaTrig(void);
extern int BifurcVerhulstTrig(void);
extern int LongBifurcVerhulstTrig(void);
extern int BifurcStewartTrig(void);
extern int LongBifurcStewartTrig(void);
extern int popcorn(void);
extern int lyapunov(void);
extern int lya_setup(void);
extern int cellular(void);
extern int CellularSetup(void);
extern int calcfroth(void);
extern int froth_setup(void);
extern int demowalk(void);

/*  cmdfiles -- C file prototypes */

extern void cmdfiles_overlay(void);
extern int cmdfiles(int ,char **);
extern int load_commands(FILE *);
extern void set_3d_defaults(void);

/*  decoder -- C file prototypes */

extern short decoder(short );

/*  diskvid -- C file prototypes */

extern int startdisk(void);
extern int pot_startdisk(void);
extern int targa_startdisk(FILE *,int );
extern void enddisk(void);
#ifndef XFRACT
extern int readdisk(unsigned int, unsigned int );
extern void writedisk(unsigned int, unsigned int, unsigned int );
#else
extern int readdisk(int, int );
extern void writedisk(int, int, int );
#endif
extern void targa_writedisk(unsigned int ,unsigned int ,BYTE ,BYTE ,BYTE );
extern void dvid_status(int ,char *);

/*  editpal -- C file prototypes */

extern void EditPalette(void );
extern VOIDPTR mem_alloc(unsigned size);
void putrow(int x, int y, int width, char *buff);
void getrow(int x, int y, int width, char *buff);
void mem_init(VOIDPTR block, unsigned size);
void hline(int x, int y, int width, int color);
int Cursor_WaitKey(void);
void Cursor_CheckBlink(void);
#ifdef XFRACT
void Cursor_StartMouseTracking(void);
void Cursor_EndMouseTracking(void);
#endif
void clip_putcolor(int x, int y, int color);
int clip_getcolor(int x, int y);
BOOLEAN Cursor_Construct (void);
void    Cursor_Destroy   (void);
void    Cursor_SetPos    (int x, int y);
void    Cursor_Move	    (int xoff, int yoff);
int	   Cursor_GetX	    (void);
int	   Cursor_GetY	    (void);
void    Cursor_Hide	    (void);
void    Cursor_Show	    (void);

/*  encoder -- C file prototypes */

extern void encoder_overlay(void);
extern int savetodisk(char *);
extern int encoder(void);

/*  f16 -- C file prototypes */

extern FILE *t16_open(char *,int *,int *,int *,U8 *);
extern int t16_getline(FILE *,int ,U16 *);

/*  fracsubr -- C file prototypes */

extern void calcfracinit(void);
extern void adjust_corner(void);
#ifndef XFRACT
extern int put_resume(int ,... );
extern int get_resume(int ,... );
#else
extern int put_resume();
extern int get_resume();
#endif
extern int alloc_resume(int ,int );
extern int start_resume(void);
extern void end_resume(void);
extern void sleepms(long );
extern void iplot_orbit(long ,long ,int );
extern void plot_orbit(double ,double ,int );
extern void scrub_orbit(void);
extern int add_worklist(int ,int ,int ,int ,int ,int ,int );
extern void tidy_worklist(void);
extern void get_julia_attractor(double ,double );
extern int ssg_blocksize(void);
extern void _fastcall symPIplot(int ,int ,int );
extern void _fastcall symPIplot2J(int ,int ,int );
extern void _fastcall symPIplot4J(int ,int ,int );
extern void _fastcall symplot2(int ,int ,int );
extern void _fastcall symplot2Y(int ,int ,int );
extern void _fastcall symplot2J(int ,int ,int );
extern void _fastcall symplot4(int ,int ,int );
extern void _fastcall symplot2basin(int ,int ,int );
extern void _fastcall symplot4basin(int ,int ,int );
extern void _fastcall noplot(int ,int ,int );

/*  fractals -- C file prototypes */

extern void FloatPreCalcMagnet2(void);
extern void cpower(_CMPLX *,int ,_CMPLX *);
extern int lcpower(_LCMPLX *,int ,_LCMPLX *,int );
extern int complex_mult(_CMPLX ,_CMPLX ,_CMPLX *);
extern int complex_div(_CMPLX ,_CMPLX ,_CMPLX *);
extern int lcomplex_mult(_LCMPLX ,_LCMPLX ,_LCMPLX *,int );
extern int MPCNewtonFractal(void);
extern int Barnsley1Fractal(void);
extern int Barnsley1FPFractal(void);
extern int Barnsley2Fractal(void);
extern int Barnsley2FPFractal(void);
extern int JuliaFractal(void);
extern int JuliafpFractal(void);
extern int LambdaFPFractal(void);
extern int LambdaFractal(void);
extern int SierpinskiFractal(void);
extern int SierpinskiFPFractal(void);
extern int LambdaexponentFractal(void);
extern int LongLambdaexponentFractal(void);
extern int FloatTrigPlusExponentFractal(void);
extern int LongTrigPlusExponentFractal(void);
extern int MarksLambdaFractal(void);
extern int MarksLambdafpFractal(void);
extern int UnityFractal(void);
extern int UnityfpFractal(void);
extern int Mandel4Fractal(void);
extern int Mandel4fpFractal(void);
extern int floatZtozPluszpwrFractal(void);
extern int longZpowerFractal(void);
extern int longCmplxZpowerFractal(void);
extern int floatZpowerFractal(void);
extern int floatCmplxZpowerFractal(void);
extern int Barnsley3Fractal(void);
extern int Barnsley3FPFractal(void);
extern int TrigPlusZsquaredFractal(void);
extern int TrigPlusZsquaredfpFractal(void);
extern int Richard8fpFractal(void);
extern int Richard8Fractal(void);
extern int PopcornFractal(void);
extern int LPopcornFractal(void);
extern int MarksCplxMand(void );
extern int SpiderfpFractal(void );
extern int SpiderFractal(void );
extern int TetratefpFractal(void);
extern int ZXTrigPlusZFractal(void);
extern int ScottZXTrigPlusZFractal(void);
extern int SkinnerZXTrigSubZFractal(void);
extern int ZXTrigPlusZfpFractal(void);
extern int ScottZXTrigPlusZfpFractal(void);
extern int SkinnerZXTrigSubZfpFractal(void);
extern int Sqr1overTrigFractal(void);
extern int Sqr1overTrigfpFractal(void);
extern int TrigPlusTrigFractal(void);
extern int TrigPlusTrigfpFractal(void);
extern int ScottTrigPlusTrigFractal(void);
extern int ScottTrigPlusTrigfpFractal(void);
extern int SkinnerTrigSubTrigFractal(void);
extern int SkinnerTrigSubTrigfpFractal(void);
extern int TrigXTrigfpFractal(void);
extern int TrigXTrigFractal(void);
extern int TryFloatFractal(int (*)());
extern int TrigPlusSqrFractal(void);
extern int TrigPlusSqrfpFractal(void);
extern int ScottTrigPlusSqrFractal(void);
extern int ScottTrigPlusSqrfpFractal(void);
extern int SkinnerTrigSubSqrFractal(void);
extern int SkinnerTrigSubSqrfpFractal(void);
extern int TrigZsqrdfpFractal(void);
extern int TrigZsqrdFractal(void);
extern int SqrTrigFractal(void);
extern int SqrTrigfpFractal(void);
extern int Magnet1Fractal(void);
extern int Magnet2Fractal(void);
extern int LambdaTrigFractal(void);
extern int LambdaTrigfpFractal(void);
extern int LambdaTrigFractal1(void);
extern int LambdaTrigfpFractal1(void);
extern int LambdaTrigFractal2(void);
extern int LambdaTrigfpFractal2(void);
extern int ManOWarFractal(void);
extern int ManOWarfpFractal(void);
extern int MarksMandelPwrfpFractal(void);
extern int MarksMandelPwrFractal(void);
extern int TimsErrorfpFractal(void);
extern int TimsErrorFractal(void);
extern int CirclefpFractal(void);
extern int long_julia_per_pixel(void);
extern int long_richard8_per_pixel(void);
extern int long_mandel_per_pixel(void);
extern int julia_per_pixel(void);
extern int marks_mandelpwr_per_pixel(void);
extern int mandel_per_pixel(void);
extern int marksmandel_per_pixel(void);
extern int marksmandelfp_per_pixel();
extern int marks_mandelpwrfp_per_pixel(void);
extern int mandelfp_per_pixel(void);
extern int juliafp_per_pixel(void);
extern int MPCjulia_per_pixel(void);
extern int otherrichard8fp_per_pixel(void);
extern int othermandelfp_per_pixel(void);
extern int otherjuliafp_per_pixel(void);
extern int trigmandelfp_per_pixel(void);
extern int trigjuliafp_per_pixel(void);
extern int trigXtrigmandelfp_per_pixel(void);
extern int trigXtrigjuliafp_per_pixel(void);
extern int MarksCplxMandperp(void );
extern int MandelSetup(void);
extern int MandelfpSetup();
extern int JuliaSetup(void);
extern int NewtonSetup(void);
extern int StandaloneSetup(void);
extern int UnitySetup(void);
extern int JuliafpSetup(void);
extern int MandellongSetup(void);
extern int JulialongSetup(void);
extern int TrigPlusSqrlongSetup(void);
extern int TrigPlusSqrfpSetup(void);
extern int TrigPlusTriglongSetup(void);
extern int TrigPlusTrigfpSetup(void);
extern int FnPlusFnSym(void);
extern int ZXTrigPlusZSetup(void);
extern int LambdaTrigSetup(void);
extern int JuliafnPlusZsqrdSetup(void);
extern int SqrTrigSetup(void);
extern int FnXFnSetup(void);
extern int MandelTrigSetup(void);
extern int MarksJuliaSetup(void);
extern int MarksJuliafpSetup();
extern int SierpinskiSetup(void);
extern int SierpinskiFPSetup(void);
extern int StandardSetup(void);
extern int LambdaTrigOrTrigFractal(void);
extern int LambdaTrigOrTrigfpFractal(void);
extern int LambdaTrigOrTrigSetup(void);
extern int JuliaTrigOrTrigFractal(void);
extern int JuliaTrigOrTrigfpFractal(void);
extern int JuliaTrigOrTrigSetup(void);
extern int ManlamTrigOrTrigSetup(void);
extern int MandelTrigOrTrigSetup(void);
extern int HalleySetup(void);
extern int HalleyFractal(void);
extern int Halley_per_pixel(void);
extern int MPCHalleyFractal(void);
extern int MPCHalley_per_pixel(void);
extern int dynamfloat(double *,double *,double*);
extern int mandelcloudfloat(double *,double *,double*);
extern int dynam2dfloatsetup(void);
extern int dynam2dfloat(void);
extern int QuaternionFPFractal(void);
extern int quaternionfp_per_pixel(void);
extern int quaternionjulfp_per_pixel(void);
extern int LongPhoenixFractal(void);
extern int PhoenixFractal(void);
extern int PhoenixSetup(void);
extern int long_phoenix_per_pixel(void);
extern int phoenix_per_pixel(void);
extern int MandPhoenixSetup(void);
extern int long_mandphoenix_per_pixel(void);
extern int mandphoenix_per_pixel(void);
extern int HyperComplexFPFractal(void);

/*  fractint -- C file prototypes */

extern void main(int ,char **);
extern int key_count(int );
extern int cmp_line(BYTE *,int );
extern void flip_image(int kbdchar);
extern int pot_line(BYTE *,int );
extern void reset_zoom_corners(void);
extern void clear_zoombox(void);
extern int sound_line(BYTE *,int );

/*  gifview -- C file prototypes */

extern int get_byte(void);
extern int get_bytes(BYTE *,int );
extern int gifview(void);

/*  help -- C file prototypes */

extern int _find_token_length(char far *,unsigned int ,int *,int *);
extern int find_token_length(int ,char far *,unsigned int ,int *,int *);
extern int find_line_width(int ,char far *,unsigned int );
extern int process_document(PD_FUNC ,PD_FUNC ,VOIDPTR );
extern void help_overlay(void );
extern int help(int );
extern int read_help_topic(int ,int ,int ,VOIDFARPTR );
extern int makedoc_msg_func(int ,int );
extern void print_document(char *,int (*)(int ,int ),int );
extern int init_help(void );
extern void end_help(void );

/*  intro -- C file prototypes */

extern void intro_overlay(void );
extern void intro(void );

/*  jb -- C file prototypes */

extern int JulibrotSetup(void );
extern int JulibrotfpSetup(void );
extern int jb_per_pixel(void );
extern int jbfp_per_pixel(void );
extern int zline(long ,long );
extern int zlinefp(double ,double );
extern int Std4dFractal(void );
extern int Std4dfpFractal(void );

/*  line3d -- C file prototypes */

extern void line3d_overlay(void);
extern int line3d(BYTE *,unsigned int );
extern int _fastcall targa_color(int ,int ,int );
extern int targa_validate(char *);

/*  loadfdos -- C file prototypes */

extern int get_video_mode(struct fractal_info *);

/*  loadfile -- C file prototypes */

extern void loadfile_overlay(void);
extern int read_overlay(void);
extern void set_if_old_bif(void);

/*  loadmap -- C file prototypes */

extern void SetTgaColors(void);
extern int ValidateLuts(char *);
extern int SetColorPaletteName(char *);

/*  lorenz -- C file prototypes */

extern int orbit3dlongsetup(void);
extern int orbit3dfloatsetup(void);
extern int lorenz3dlongorbit(long *,long *,long *);
extern int lorenz3d1floatorbit(double *,double *,double *);
extern int lorenz3dfloatorbit(double *,double *,double *);
extern int lorenz3d3floatorbit(double *,double *,double *);
extern int lorenz3d4floatorbit(double *,double *,double *);
extern int henonfloatorbit(double *,double *,double *);
extern int henonlongorbit(long *,long *,long *);
extern int inverse_julia_orbit(double *,double *,double *);
extern int Minverse_julia_orbit(void);
extern int Linverse_julia_orbit(void);
extern int inverse_julia_per_image(void);
extern int rosslerfloatorbit(double *,double *,double *);
extern int pickoverfloatorbit(double *,double *,double *);
extern int gingerbreadfloatorbit(double *,double *,double *);
extern int rosslerlongorbit(long *,long *,long *);
extern int kamtorusfloatorbit(double *,double *,double *);
extern int kamtoruslongorbit(long *,long *,long *);
extern int hopalong2dfloatorbit(double *,double *,double *);
extern int martin2dfloatorbit(double *,double *,double *);
extern int orbit2dfloat(void);
extern int orbit2dlong(void);
extern int orbit3dlongcalc(void);
extern int orbit3dfloatcalc(void);
extern int funny_glasses_call(int (*)());
extern int ifs(void);
extern int ifs2d(void);
extern int orbit3dfloat(void);
extern int orbit3dlong(void);
extern int ifs3d(void);
extern int iconfloatorbit(double *, double *, double *);  /* dmf */

/*  lsys -- C file prototypes */

extern int Lsystem(void);
extern int LLoad(void);

/*  miscovl -- C file prototypes */

extern void miscovl_overlay(void);
extern void make_batch_file(void);
extern void shell_to_dos(void);
extern void showfreemem(void);
extern int edit_text_colors(void);
extern int select_video_mode(int );

/*  miscres -- C file prototypes */

#ifndef XFRACT
extern int timer(int ,int (*)(),... );
extern int matherr(struct exception *);
#else
extern int timer();
#endif
extern void restore_active_ovly(void);
extern void findpath(char *,char *);
extern void notdiskmsg(void);
extern int cvtcentermag(double *,double *,double *);
extern void updatesavename(char *);
extern int check_writefile(char *,char *);
extern int check_key(void);
extern void showtrig(char *);
extern int set_trig_array(int ,char *);
extern void set_trig_pointers(int );
extern int tab_display(void);
extern int endswithslash(char *);
extern int ifsload(void);
extern int find_file_item(char *,char *,FILE **);
extern int file_gets(char *,int ,FILE *);
extern void roundfloatd(double *);

/*  mpmath_c -- C file prototypes */

extern struct MP *MPsub(struct MP ,struct MP );
extern struct MP *MPsub086(struct MP ,struct MP );
extern struct MP *MPsub386(struct MP ,struct MP );
extern struct MP *MPabs(struct MP );
extern struct MPC MPCsqr(struct MPC );
extern struct MP MPCmod(struct MPC );
extern struct MPC MPCmul(struct MPC ,struct MPC );
extern struct MPC MPCdiv(struct MPC ,struct MPC );
extern struct MPC MPCadd(struct MPC ,struct MPC );
extern struct MPC MPCsub(struct MPC ,struct MPC );
extern struct MPC MPCpow(struct MPC ,int );
extern int MPCcmp(struct MPC ,struct MPC );
extern _CMPLX MPC2cmplx(struct MPC );
extern struct MPC cmplx2MPC(_CMPLX );
extern void setMPfunctions(void );
extern _CMPLX ComplexPower(_CMPLX ,_CMPLX );
extern void SetupLogTable(void );
extern long far ExpFloat14(long );
extern int ComplexNewtonSetup(void );
extern int ComplexNewton(void );
extern int ComplexBasin(void );
extern int GausianNumber(int ,int );

/*  msccos -- C file prototypes */

extern double _cos(double );

/*  parser -- C file prototypes */

extern unsigned int SkipWhiteSpace(char *);
extern unsigned long NewRandNum(void );
extern void lRandom(void );
extern void dRandom(void );
extern void mRandom(void );
extern void SetRandFnct(void );
extern void RandomSeed(void );
extern void lStkSRand(void );
extern void mStkSRand(void );
extern void dStkSRand(void );
extern void dStkAbs(void );
extern void mStkAbs(void );
extern void lStkAbs(void );
extern void dStkSqr(void );
extern void mStkSqr(void );
extern void lStkSqr(void );
extern void dStkAdd(void );
extern void mStkAdd(void );
extern void lStkAdd(void );
extern void dStkSub(void );
extern void mStkSub(void );
extern void lStkSub(void );
extern void dStkConj(void );
extern void mStkConj(void );
extern void lStkConj(void );
extern void dStkZero(void );
extern void mStkZero(void );
extern void lStkZero(void );
extern void dStkReal(void );
extern void mStkReal(void );
extern void lStkReal(void );
extern void dStkImag(void );
extern void mStkImag(void );
extern void lStkImag(void );
extern void dStkNeg(void );
extern void mStkNeg(void );
extern void lStkNeg(void );
extern void dStkMul(void );
extern void mStkMul(void );
extern void lStkMul(void );
extern void dStkDiv(void );
extern void mStkDiv(void );
extern void lStkDiv(void );
extern void StkSto(void );
extern void StkLod(void );
extern void dStkMod(void );
extern void mStkMod(void );
extern void lStkMod(void );
extern void StkClr(void );
extern void dStkFlip(void );
extern void mStkFlip(void );
extern void lStkFlip(void );
extern void dStkSin(void );
extern void mStkSin(void );
extern void lStkSin(void );
extern void dStkTan(void );
extern void mStkTan(void );
extern void lStkTan(void );
extern void dStkTanh(void );
extern void mStkTanh(void );
extern void lStkTanh(void );
extern void dStkCoTan(void );
extern void mStkCoTan(void );
extern void lStkCoTan(void );
extern void dStkCoTanh(void );
extern void mStkCoTanh(void );
extern void lStkCoTanh(void );
extern void dStkRecip(void );
extern void mStkRecip(void );
extern void lStkRecip(void );
extern void StkIdent(void );
extern void dStkSinh(void );
extern void mStkSinh(void );
extern void lStkSinh(void );
extern void dStkCos(void );
extern void mStkCos(void );
extern void lStkCos(void );
extern void dStkCosXX(void );
extern void mStkCosXX(void );
extern void lStkCosXX(void );
extern void dStkCosh(void );
extern void mStkCosh(void );
extern void lStkCosh(void );
extern void dStkLT(void );
extern void mStkLT(void );
extern void lStkLT(void );
extern void dStkGT(void );
extern void mStkGT(void );
extern void lStkGT(void );
extern void dStkLTE(void );
extern void mStkLTE(void );
extern void lStkLTE(void );
extern void dStkGTE(void );
extern void mStkGTE(void );
extern void lStkGTE(void );
extern void dStkEQ(void );
extern void mStkEQ(void );
extern void lStkEQ(void );
extern void dStkNE(void );
extern void mStkNE(void );
extern void lStkNE(void );
extern void dStkOR(void );
extern void mStkOR(void );
extern void lStkOR(void );
extern void dStkAND(void );
extern void mStkAND(void );
extern void lStkAND(void );
extern void dStkLog(void );
extern void mStkLog(void );
extern void lStkLog(void );
extern void FPUcplxexp(_CMPLX *,_CMPLX *);
extern void dStkExp(void );
extern void mStkExp(void );
extern void lStkExp(void );
extern void dStkPwr(void );
extern void mStkPwr(void );
extern void lStkPwr(void );
extern void (*mtrig0)(void);
extern void (*mtrig1)(void);
extern void (*mtrig2)(void);
extern void (*mtrig3)(void);
extern void EndInit(void );
extern struct ConstArg far *isconst(char *,int );
extern void NotAFnct(void );
extern void FnctNotFound(void );
extern int whichfn(char *,int );
#ifndef XFRACT
extern void (far *isfunct(char *,int ))(void );
#else
extern void (far *isfunct(char *,int ))();
#endif
extern void RecSortPrec(void );
extern int ParseStr(char *);
extern int Formula(void );
extern int form_per_pixel(void );
extern char *FindFormula(char *);
extern int RunForm(char *);
extern int fpFormulaSetup(void );
extern int intFormulaSetup(void );
extern void init_misc(void);
extern void free_workarea(void);

/*  plot3d -- C file prototypes */

extern void _fastcall draw_line(int ,int ,int ,int ,int );
extern void _fastcall plot3dsuperimpose16b(int ,int ,int );
extern void _fastcall plot3dsuperimpose16(int ,int ,int );
extern void _fastcall plot3dsuperimpose256(int ,int ,int );
extern void _fastcall plotIFS3dsuperimpose256(int ,int ,int );
extern void _fastcall plot3dalternate(int ,int ,int );
extern void plot_setup(void);

/*  printer -- C file prototypes */

extern void printer_overlay(void);
extern void Print_Screen(void);

/*  prompts -- C file prototypes */

extern void prompts_overlay(void);
extern int fullscreen_prompt(char far*,int ,char far **,struct fullscreenvalues *,int ,int ,char far *);
extern int get_fracttype(void);
extern int get_fract_params(int );
extern int get_fract3d_params(void);
extern int get_3d_params(void);
extern int get_toggles(void);
extern int get_toggles2(void);
extern int get_view_params(void);
extern int get_starfield_params(void );
extern int get_commands(void);
extern void goodbye(void);
extern int getafilename(char *,char *,char *);

/*  realdos -- C file prototypes */

extern int stopmsg(int ,CHAR far *);
extern int texttempmsg(char *);
extern int showtempmsg(char *);
extern void cleartempmsg(void);
extern void blankrows(int ,int ,int );
extern void helptitle(void);
extern int putstringcenter(int ,int ,int ,int ,char far *);
extern void stackscreen(void);
extern void unstackscreen(void);
extern void discardscreen(void);
extern int fullscreen_choice(int ,char far*,char far*,char far*,int ,char **,int *,int ,int ,int ,int ,void (*)(),char *,int (*)(),int (*)());
#ifndef XFRACT /* Unix should have this in string.h */
extern int strncasecmp(char *,char *,int );
#endif
extern int main_menu(int );
extern int input_field(int ,int ,char *,int ,int ,int ,int (*)(int));
extern int field_prompt(int ,char *,char *,char *,int ,int (*)(int));
extern int thinking(int ,char *);
extern void clear_screen(void );
extern int savegraphics(void);
extern void restoregraphics(void);
extern void discardgraphics(void);
extern int load_fractint_cfg(int );
extern void bad_fractint_cfg_msg(void);
extern void load_videotable(int );
extern int check_vidmode_key(int ,int );
extern int check_vidmode_keyname(char *);
extern void vidmode_keyname(int ,char *);

/*  rotate -- C file prototypes */

extern void rotate_overlay(void);
extern void rotate(int );
extern void save_palette(void);
extern void load_palette(void );

/*  slideshw -- C file prototypes */

extern int slideshw(void);
extern int startslideshow(void);
extern void stopslideshow(void);
extern void recordshw(int );

/*  targa -- C file prototypes */

extern void WriteTGA(int ,int ,int );
extern int ReadTGA(int ,int );
extern void EndTGA(void );
extern void StartTGA(void);
extern void ReopenTGA(void);

/*  testpt -- C file prototypes */

extern int teststart(void);
extern void testend(void);
extern int testpt(double ,double ,double ,double ,int ,int );

/*  tgaview -- C file prototypes */

extern int tgaview(void);
extern int outlin16(BYTE*,int );

/*  tp3d -- C file prototypes */

extern char far *TrueColorAutoDetect(void );
extern void TranspPerPixel(int ,union Arg far *,union Arg far *);
extern int Transp3DFnct(void);
extern void ShadowPutColor(unsigned int ,unsigned int ,unsigned int );
extern void AntiAliasPass(void );

/*  tplus -- C file prototypes */

extern void WriteTPWord(unsigned int ,unsigned int );
extern void WriteTPByte(unsigned int ,unsigned int );
extern unsigned int ReadTPWord(unsigned int );
extern BYTE ReadTPByte(unsigned int );
extern void DisableMemory(void );
extern void EnableMemory(void );
extern int TargapSys(int ,unsigned int );
extern int _SetBoard(int );
extern int TPlusLUT(BYTE far *,unsigned int ,unsigned int ,unsigned int );
extern int SetVGA_LUT(void );
extern int SetColorDepth(int );
extern int SetBoard(int );
extern int ResetBoard(int );
extern int CheckForTPlus(void );
extern int SetTPlusMode(int ,int ,int ,int );
extern int FillTPlusRegion(unsigned int ,unsigned int ,unsigned int ,unsigned int ,unsigned long );
extern void BlankScreen(unsigned long );
extern void UnBlankScreen(void );
extern void EnableOverlayCapture(void );
extern void DisableOverlayCapture(void );
extern void ClearTPlusScreen(void );
extern int MatchTPlusMode(unsigned int ,unsigned int ,unsigned int ,unsigned int ,unsigned int );
extern void TPlusZoom(int );

/*  yourvid -- C file prototypes */

extern int startvideo(void);
extern int endvideo(void);
extern void writevideo(int ,int ,int );
extern int readvideo(int ,int );
extern int readvideopalette(void);
extern int writevideopalette(void);
#ifdef XFRACT
extern void readvideoline(int ,int, int, BYTE * );
extern void writevideoline(int ,int, int, BYTE * );
#endif

/*  zoom -- C file prototypes */

extern void drawbox(int );
extern void moveboxf(double ,double );
extern void resizebox(int );
extern void chgboxi(int ,int );
extern void zoomout(void);
extern void aspectratio_crop(float ,float );
extern int init_pan_or_recalc(int );
