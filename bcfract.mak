.AUTODEPEND

#               *Translator Definitions*
# bcfract.cfg - optimize for speed
# bcfract2.cfg - optimize for size
#
CC = bcc +BCFRACT.CFG
CCsize = bcc +BCFRACT2.CFG
TASM = TASM /ml /zi /o /jJUMPS /m2
TLIB = tlib
TLINK = tlink
# put your libpath here
LIBPATH = ..\lib
# and include here
INCLUDEPATH = ..\include


#               *Implicit Rules*
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

#               *List Macros*
Link_Exclude =  \
 fractint.hlp

Link_Include =  \
 3d.obj \
 ant.obj \
 bigflt.obj \
 bigfltc.obj \
 biginit.obj \
 bignum.obj \
 calcfrac.obj \
 decoder.obj \
 diskvid.obj \
 encoder.obj \
 f16.obj \
 fracsubr.obj \
 fractalb.obj \
 fractalp.obj \
 fractals.obj \
 fractint.obj \
 framain2.obj \
 frasetup.obj \
 gifview.obj \
 hcmplx.obj \
 jb.obj \
 jiim.obj \
 line3d.obj \
 loadmap.obj \
 lorenz.obj \
 lsys.obj \
 lsysf.obj \
 miscfrac.obj \
 miscovl.obj \
 miscres.obj \
 mpmath_c.obj \
 parser.obj \
 plot3d.obj \
 printer.obj \
 realdos.obj \
 rotate.obj \
 slideshw.obj \
 stereo.obj \
 targa.obj \
 testpt.obj \
 tgaview.obj \
 tplus.obj \
 yourvid.obj \
 zoom.obj \
 cmdfiles.obj \
 editpal.obj \
 help.obj \
 intro.obj \
 loadfdos.obj \
 loadfile.obj \
 parserfp.obj \
 prompts1.obj \
 prompts2.obj \
 bigflta.obj \
 bignuma.obj \
 calcmand.obj \
 calmanfp.obj \
 fpu087.obj \
 fpu387.obj \
 fr8514a.obj \
 fracsuba.obj \
 general.obj \
 hgcfra.obj \
 lsysa.obj \
 lsysaf.obj \
 lyapunov.obj \
 mpmath_a.obj \
 newton.obj \
 parsera.obj \
 tplus_a.obj \
 video.obj

#               *Explicit Rules*
fractint.exe: bcfract.lnk $(Link_Include) $(Link_Exclude)
  $(TLINK) /L$(LIBPATH)/o @bcfract.lnk



#               *Individual File Dependencies*
hc.exe: hc.obj
  $(TLINK) /v-/i/c/P=48000/L$(LIBPATH)/o @bchc.lnk

fractint.hlp: help.src help2.src help3.src help4.src help5.src hc.exe
  hc /c help.src

hc.obj: hc.c helpcom.h
  $(CC) -ml -c hc.c


# first all these

3d.obj: bcfract.mak bcfract.cfg fractint.hlp 3d.c

ant.obj: bcfract.mak bcfract.cfg fractint.hlp ant.c

bigflt.obj: bcfract.mak bcfract.cfg fractint.hlp bigflt.c

bigfltc.obj: bcfract.mak bcfract.cfg fractint.hlp bigfltc.c

biginit.obj: bcfract.mak bcfract.cfg fractint.hlp biginit.c

bignum.obj: bcfract.mak bcfract.cfg fractint.hlp bignum.c

bignumc.obj: bcfract.mak bcfract.cfg fractint.hlp bignumc.c

calcfrac.obj: bcfract.mak bcfract.cfg calcfrac.c

decoder.obj: bcfract.mak bcfract.cfg decoder.c

diskvid.obj: bcfract.mak bcfract.cfg diskvid.c

encoder.obj: bcfract.mak bcfract.cfg encoder.c

f16.obj: bcfract.mak bcfract.cfg f16.c

fracsubr.obj: bcfract.mak bcfract.cfg fracsubr.c

fractalb.obj: bcfract.mak bcfract.cfg fractalb.c

fractalp.obj: bcfract.mak bcfract.cfg fractalp.c

fractals.obj: bcfract.mak bcfract.cfg fractals.c

fractint.obj: bcfract.mak bcfract.cfg fractint.c

framain2.obj: bcfract.mak bcfract.cfg framain2.c

frasetup.obj: bcfract.mak bcfract.cfg frasetup.c

gifview.obj: bcfract.mak bcfract.cfg gifview.c

hcmplx.obj: bcfract.mak bcfract.cfg hcmplx.c

jb.obj: bcfract.mak  bcfract.cfg jb.c

jiim.obj: bcfract.mak  bcfract.cfg jiim.c

line3d.obj: bcfract.mak  bcfract.cfg line3d.c

loadmap.obj: bcfract.mak  bcfract.cfg loadmap.c

lorenz.obj: bcfract.mak  bcfract.cfg lorenz.c

lsys.obj: bcfract.mak  bcfract.cfg lsys.c

lsysf.obj: bcfract.mak  bcfract.cfg lsysf.c

miscfrac.obj: bcfract.mak  bcfract.cfg miscfrac.c

miscovl.obj: bcfract.mak  bcfract.cfg miscovl.c

miscres.obj: bcfract.mak  bcfract.cfg miscres.c

mpmath_c.obj: bcfract.mak  bcfract.cfg mpmath_c.c

parser.obj: bcfract.mak  bcfract.cfg parser.c

plot3d.obj: bcfract.mak  bcfract.cfg plot3d.c

printer.obj: bcfract.mak  bcfract.cfg printer.c

realdos.obj: bcfract.mak  bcfract.cfg realdos.c

rotate.obj: bcfract.mak  bcfract.cfg rotate.c

slideshw.obj: bcfract.mak  bcfract.cfg slideshw.c

stereo.obj: bcfract.mak bcfract.cfg stereo.c

targa.obj: bcfract.mak  bcfract.cfg targa.c

testpt.obj: bcfract.mak  bcfract.cfg testpt.c

tgaview.obj: bcfract.mak  bcfract.cfg tgaview.c

tplus.obj: bcfract.mak  bcfract.cfg tplus.c

yourvid.obj: bcfract.mak  bcfract.cfg yourvid.c

zoom.obj: bcfract.mak  bcfract.cfg zoom.c


# then the ones optimized for size

cmdfiles.obj: bcfract.mak bcfract2.cfg cmdfiles.c
  $(CCsize) -c {cmdfiles.c }

editpal.obj: bcfract.mak bcfract2.cfg editpal.c
  $(CCsize) -c {editpal.c }

help.obj: bcfract.mak bcfract2.cfg help.c
  $(CCsize) -c {help.c }

intro.obj: bcfract.mak  bcfract2.cfg intro.c
  $(CCsize) -c {intro.c }

loadfdos.obj: bcfract.mak  bcfract2.cfg loadfdos.c
  $(CCsize) -c {loadfdos.c }

loadfile.obj: bcfract.mak  bcfract2.cfg loadfile.c
  $(CCsize) -c {loadfile.c }

parserfp.obj: bcfract.mak  bcfract2.cfg parserfp.c
  $(CCsize) -c {parserfp.c }

prompts1.obj: bcfract.mak  bcfract2.cfg prompts1.c
  $(CCsize) -c {prompts1.c }

prompts2.obj: bcfract.mak  bcfract2.cfg prompts2.c
  $(CCsize) -c {prompts2.c }



bigflta.obj: bcfract.mak  bcfract.cfg bigflta.asm
	$(TASM)  bigflta.ASM,bigflta.OBJ

bignuma.obj: bcfract.mak  bcfract.cfg bignuma.asm
	$(TASM)  bignuma.ASM,bignuma.OBJ

calcmand.obj: bcfract.mak  bcfract.cfg calcmand.asm
	$(TASM)  CALCMAND.ASM,CALCMAND.OBJ

calmanfp.obj: bcfract.mak  bcfract.cfg calmanfp.asm
	$(TASM)  CALMANFP.ASM,CALMANFP.OBJ

fpu087.obj: bcfract.mak  bcfract.cfg fpu087.asm
	$(TASM)  FPU087.ASM,FPU087.OBJ

fpu387.obj: bcfract.mak  bcfract.cfg fpu387.asm
	$(TASM)  FPU387.ASM,FPU387.OBJ

fr8514a.obj: bcfract.mak  bcfract.cfg fr8514a.asm
	$(TASM)  FR8514A.ASM,FR8514A.OBJ

fracsuba.obj: bcfract.mak  bcfract.cfg fracsuba.asm
	$(TASM)  FRACSUBA.ASM,FRACSUBA.OBJ

general.obj: bcfract.mak  bcfract.cfg general.asm
	$(TASM)  GENERAL.ASM,GENERAL.OBJ

hgcfra.obj: bcfract.mak  bcfract.cfg hgcfra.asm
	$(TASM)  HGCFRA.ASM,HGCFRA.OBJ

lsysa.obj: bcfract.mak  bcfract.cfg lsysa.asm
	$(TASM)  LSYSA.ASM,LSYSA.OBJ

lsysaf.obj: bcfract.mak  bcfract.cfg lsysaf.asm
	$(TASM)  LSYSAF.ASM,LSYSAF.OBJ

lyapunov.obj: bcfract.mak  bcfract.cfg lyapunov.asm
	$(TASM)  LYAPUNOV.ASM,LYAPUNOV.OBJ

mpmath_a.obj: bcfract.mak  bcfract.cfg mpmath_a.asm
	$(TASM)  MPMATH_A.ASM,MPMATH_A.OBJ

newton.obj: bcfract.mak  bcfract.cfg newton.asm
	$(TASM)  NEWTON.ASM,NEWTON.OBJ

parsera.obj: bcfract.mak  bcfract.cfg parsera.asm
	$(TASM)  PARSERA.ASM,PARSERA.OBJ

tplus_a.obj: bcfract.mak  bcfract.cfg tplus_a.asm
	$(TASM)  TPLUS_A.ASM,TPLUS_A.OBJ

video.obj: bcfract.mak  bcfract.cfg video.asm
	$(TASM)  VIDEO.ASM,VIDEO.OBJ


