.c.obj:
#	qcl /AM /W1 /FPi /c /Olt $*.c
#	cl /AM /W1 /FPi /qc /c /Zi $*.c
#	cl /AM /W1 /FPi /c /Oait $*.c
	cl /AM /W1 /FPi /c /Oait /Gs $*.c
#	cl /DWAITE /AM /W1 /FPi /c /Oait /Gs $*.c
#	cl /AM /W1 /FPi /qc /c $*.c

.asm.obj:
#	masm /ML /Zi $*;
	masm /ML $*;
#	masm /DWAITE /ML $*;
 

lorenz.obj : lorenz.c fractint.h fractype.h

lsys.obj : lsys.c fractint.h

plot3d.obj : plot3d.c fractint.h fractype.h
#	cl /AL /W1 /FPi /c /Oait /Gs plot3d.c

3d.obj : 3d.c fractint.h

fractals.obj : fractals.c fractint.h fractype.h

calcfrac.obj : calcfrac.c fractint.h mpmath.h

fracsubr.obj : fracsubr.c fractint.h

fracsuba.obj : fracsuba.asm

parser.obj : parser.c fractint.h

calcmand.obj : calcmand.asm

cmdfiles.obj : cmdfiles.c fractint.h

loadfile.obj : loadfile.c fractint.h fractype.h

loadfdos.obj : loadfdos.c fractint.h

decoder.obj : decoder.c fractint.h

diskvid.obj : diskvid.c fractint.h

encoder.obj : encoder.c fractint.h fractype.h

helpmsg.obj : helpmsg.asm

fr8514a.obj : fr8514a.asm

hgcfra.obj : hgcfra.asm

fractint.obj : fractint.c fractint.h fractype.h

video.obj : video.asm

general.obj : general.asm

gifview.obj : gifview.c fractint.h

tgaview.obj : tgaview.c fractint.h targa_lc.h port.h

help.obj : help.c fractint.h

line3d.obj : line3d.c fractint.h

newton.obj : newton.asm
	masm /e /ML newton;

printer.obj : printer.c fractint.h

prompts.obj : prompts.c fractint.h fractype.h

rotate.obj : rotate.c fractint.h

editpal.obj : editpal.c fractint.h

testpt.obj: testpt.c fractint.h

targa.obj : targa.c targa.h fractint.h

loadmap.obj : loadmap.c targa.h

tgasubs.obj : tgasubs.c targa.h

yourvid.obj : yourvid.c

fpu387.obj : fpu387.asm

fpu087.obj : fpu087.asm
	masm /e /ML fpu087;

f16.obj : f16.c targa_lc.h

mpmath_c.obj : mpmath_c.c mpmath.h

mpmath_a.obj : mpmath_a.asm

jb.obj : jb.c fractint.h

zoom.obj : zoom.c fractint.h

miscres.obj : miscres.c fractint.h fractype.h

miscovl.obj : miscovl.c fractint.h fractype.h

realdos.obj : realdos.c fractint.h

fractint.exe : fractint.obj help.obj loadfile.obj encoder.obj gifview.obj \
     general.obj calcmand.obj fractals.obj calcfrac.obj testpt.obj \
     decoder.obj rotate.obj yourvid.obj prompts.obj parser.obj \
     diskvid.obj line3d.obj 3d.obj newton.obj helpmsg.obj cmdfiles.obj \
     targa.obj loadmap.obj tgasubs.obj printer.obj fracsubr.obj fracsuba.obj \
     video.obj tgaview.obj f16.obj fr8514a.obj loadfdos.obj \
     hgcfra.obj fpu087.obj fpu387.obj mpmath_c.obj mpmath_a.obj \
     lorenz.obj plot3d.obj jb.obj zoom.obj miscres.obj miscovl.obj \
     realdos.obj lsys.obj editpal.obj
#	link /ST:4096 /CO /NOE /MAP @fractint.lnk
	link /ST:4096 /EXEPACK @fractint.lnk
