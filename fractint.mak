
# Note that frachelp.mak and fractint.mak can't be combined into a single
# make file because with MSC6 we need to use "NMK", to have enough memory
# available for the compiler.  NMK would not trigger subsequent recompiles
# due to a rebuild of helpdefs.h file if we used a single step.

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : fractint.exe

.asm.obj:
	masm /ML $*;

.c.obj:
	  $(CC) /AM /W1 /FPi /c $(OptT) $*.c

Optsize = $(CC) /AM /W1 /FPi /c $(OptS) $*.c

lorenz.obj : lorenz.c fractint.h fractype.h

lsys.obj : lsys.c fractint.h

plot3d.obj : plot3d.c fractint.h fractype.h

3d.obj : 3d.c fractint.h

fractals.obj : fractals.c fractint.h fractype.h mpmath.h helpdefs.h

calcfrac.obj : calcfrac.c fractint.h mpmath.h

fracsubr.obj : fracsubr.c fractint.h

fracsuba.obj : fracsuba.asm

parser.obj : parser.c fractint.h mpmath.h

calcmand.obj : calcmand.asm

cmdfiles.obj : cmdfiles.c fractint.h
	$(Optsize)

loadfile.obj : loadfile.c fractint.h fractype.h
	$(Optsize)

loadfdos.obj : loadfdos.c fractint.h helpdefs.h
	$(Optsize)

decoder.obj : decoder.c fractint.h

diskvid.obj : diskvid.c fractint.h

encoder.obj : encoder.c fractint.h fractype.h

fr8514a.obj : fr8514a.asm

hgcfra.obj : hgcfra.asm

fractint.obj : fractint.c fractint.h fractype.h helpdefs.h
	$(Optsize)

video.obj : video.asm

general.obj : general.asm

gifview.obj : gifview.c fractint.h

tgaview.obj : tgaview.c fractint.h targa_lc.h port.h

help.obj : help.c fractint.h helpdefs.h helpcom.h
	$(Optsize)

intro.obj : intro.c fractint.h helpdefs.h
	$(Optsize)

line3d.obj : line3d.c fractint.h

newton.obj : newton.asm
	masm /e /ML newton;

printer.obj : printer.c fractint.h
	$(Optsize)

printera.obj : printera.asm

prompts.obj : prompts.c fractint.h fractype.h helpdefs.h
	$(Optsize)

rotate.obj : rotate.c fractint.h helpdefs.h
	$(Optsize)

editpal.obj : editpal.c fractint.h
	$(Optsize)

testpt.obj: testpt.c fractint.h

targa.obj : targa.c targa.h fractint.h

loadmap.obj : loadmap.c targa.h fractint.h
	$(Optsize)

yourvid.obj : yourvid.c

fpu387.obj : fpu387.asm

fpu087.obj : fpu087.asm
	masm /e /ML fpu087;

f16.obj : f16.c targa_lc.h

mpmath_c.obj : mpmath_c.c mpmath.h

mpmath_a.obj : mpmath_a.asm

jb.obj : jb.c fractint.h helpdefs.h

zoom.obj : zoom.c fractint.h
	$(Optsize)

miscres.obj : miscres.c fractint.h fractype.h helpdefs.h
	$(Optsize)

miscovl.obj : miscovl.c fractint.h fractype.h helpdefs.h
	$(Optsize)

realdos.obj : realdos.c fractint.h helpdefs.h
	$(Optsize)

tplus.obj : tplus.c tplus.h

tplus_a.obj : tplus_a.asm

slideshw.obj : slideshw.c
	$(Optsize)

fractint.exe : fractint.obj help.obj loadfile.obj encoder.obj gifview.obj \
     general.obj calcmand.obj fractals.obj calcfrac.obj testpt.obj \
     decoder.obj rotate.obj yourvid.obj prompts.obj parser.obj \
     diskvid.obj line3d.obj 3d.obj newton.obj cmdfiles.obj \
     intro.obj slideshw.obj \
     targa.obj loadmap.obj printer.obj printera.obj fracsubr.obj fracsuba.obj \
     video.obj tgaview.obj f16.obj fr8514a.obj loadfdos.obj \
     hgcfra.obj fpu087.obj fpu387.obj mpmath_c.obj mpmath_a.obj \
     lorenz.obj plot3d.obj jb.obj zoom.obj miscres.obj miscovl.obj \
     fractint.hlp \
     realdos.obj lsys.obj editpal.obj tplus.obj tplus_a.obj
#	link /ST:4096 /CO /NOE /SE:200 /PACKC /F /EXEPACK @fractint.lnk
	link /ST:4096 /SE:200 /PACKC /F /EXEPACK /NOE @fractint.lnk
	hc /a
