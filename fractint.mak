
# Note that frachelp.mak and fractint.mak can't be combined into a single
# make file because with MSC6 we need to use "NMK", to have enough memory
# available for the compiler.  NMK would not trigger subsequent recompiles
# due to a rebuild of helpdefs.h file if we used a single step.

# the following klooge lets us define an alternate link/def file for 
# the new overlay structure available under MSC7

!ifdef C7
DEFFILE  = fractint.def
LINKFILE = fractint.ln7
LINKER="link /dynamic:1024 $(LINKER)"
!else
DEFFILE  = 
LINKFILE = fractint.lnk
!endif

# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : fractint.exe

.asm.obj:
	$(AS) $*;
# for Quick Assembler
#       $(AS) $*.asm

.c.obj:
	  $(CC) /AM /W1 /FPi /c $(OptT) $*.c

Optsize = $(CC) /AM /W1 /FPi /c $(OptS) $*.c

Optnoalias = $(CC) /AM /W1 /FPi /c $(OptN) $*.c

lorenz.obj : lorenz.c fractint.h fractype.h

lsys.obj : lsys.c fractint.h

lsysa.obj: lsysa.asm

plot3d.obj : plot3d.c fractint.h fractype.h

3d.obj : 3d.c fractint.h

fractals.obj : fractals.c fractint.h fractype.h mpmath.h helpdefs.h

fractalp.obj : fractalp.c fractint.h fractype.h mpmath.h helpdefs.h

calcfrac.obj : calcfrac.c fractint.h mpmath.h

miscfrac.obj : miscfrac.c fractint.h mpmath.h

fracsubr.obj : fracsubr.c fractint.h

jiim.obj : jiim.c

fracsuba.obj : fracsuba.asm

parser.obj : parser.c fractint.h mpmath.h
	$(Optnoalias)

parserfp.obj : parserfp.c fractint.h mpmath.h
	$(Optnoalias)

parsera.obj: parsera.asm
# for MASM
	$(AS) /e parsera;
# for QuickAssembler
#   $(AS) /FPi parsera.asm

calcmand.obj : calcmand.asm

calmanfp.obj : calmanfp.asm
# for MASM
	$(AS) /e calmanfp;
# for QuickAssembler
#   $(AS) /FPi calmanfp.asm

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
	$(AS) /e newton;

printer.obj : printer.c fractint.h
	$(Optsize)

printera.obj : printera.asm

prompts1.obj : prompts1.c fractint.h fractype.h helpdefs.h
	$(Optsize)

prompts2.obj : prompts2.c fractint.h fractype.h helpdefs.h
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
	$(AS) /e fpu087;

f16.obj : f16.c targa_lc.h

mpmath_c.obj : mpmath_c.c mpmath.h

hcmplx.obj : hcmplx.c fractint.h

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

lyapunov.obj : lyapunov.asm
	$(AS) /e lyapunov;

tp3d.obj : tp3d.c fractint.h

slideshw.obj : slideshw.c
	$(Optsize)

fractint.exe : fractint.obj help.obj loadfile.obj encoder.obj gifview.obj \
     general.obj calcmand.obj calmanfp.obj fractals.obj fractalp.obj calcfrac.obj \
     testpt.obj decoder.obj rotate.obj yourvid.obj prompts1.obj prompts2.obj parser.obj \
     parserfp.obj parsera.obj diskvid.obj line3d.obj 3d.obj newton.obj cmdfiles.obj \
     intro.obj slideshw.obj jiim.obj miscfrac.obj \
     targa.obj loadmap.obj printer.obj printera.obj fracsubr.obj fracsuba.obj \
     video.obj tgaview.obj f16.obj fr8514a.obj loadfdos.obj \
     hgcfra.obj fpu087.obj fpu387.obj mpmath_c.obj mpmath_a.obj \
     lorenz.obj plot3d.obj jb.obj zoom.obj miscres.obj miscovl.obj \
     realdos.obj lsys.obj lsysa.obj editpal.obj tplus.obj tplus_a.obj tp3d.obj \
     lyapunov.obj fractint.hlp hcmplx.obj $(DEFFILE) $(LINKFILE)
	$(LINKER) /ST:4096 /SE:200 /PACKC /F /NOE @$(LINKFILE) > foo
!ifdef C7
        @echo (Any overlay_thunks (L4059) warnings from the linker are harmless) >> foo
!endif
	type foo
!ifndef DEBUG
	hc /a
!endif
