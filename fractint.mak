.c.obj:	
	cl /AM /W1 /FPi /Gs /c /Oait $*.c

.asm.obj:
	masm /MX $*;


3d.obj : 3d.c fractint.h

fractals.obj : fractals.c fractint.h

calcfrac.obj : calcfrac.c fractint.h

calcmand.obj : calcmand.asm

cmdfiles.obj : cmdfiles.c fractint.h

config.obj : config.c fractint.h

decoder.obj : decoder.c fractint.h

diskvid.obj : diskvid.c fractint.h

encoder.obj : encoder.c fractint.h

farmsg.obj : farmsg.asm

farvideo.obj : farvideo.asm

fr8514a.obj : fr8514a.asm

hgcfra.obj : hgcfra.asm

fractint.obj : fractint.c fractint.h

video.obj : video.asm

general.obj : general.asm

gifview.obj : gifview.c fractint.h

tgaview.obj : tgaview.c fractint.h targa_lc.h port.h

help.obj : help.c fractint.h

line3d.obj : line3d.c fractint.h

newton.obj : newton.asm
	masm /e newton;

printer.obj : printer.c fractint.h

rotate.obj : rotate.c fractint.h

log.obj : log.c

testpt.obj: testpt.c fractint.h

targa.obj : targa.c targa.h fractint.h

loadmap.obj : loadmap.c targa.h

tgasubs.obj : tgasubs.c targa.h

yourvid.obj : yourvid.c

fmath.obj : fmath.c fmath.h

fmath086.obj : fmath086.asm

f16.obj : f16.c targa_lc.h

fractint.exe : fractint.obj help.obj config.obj encoder.obj gifview.obj \
     general.obj calcmand.obj fractals.obj calcfrac.obj testpt.obj \
     decoder.obj rotate.obj yourvid.obj \
     diskvid.obj line3d.obj 3d.obj newton.obj farmsg.obj cmdfiles.obj \
     targa.obj loadmap.obj tgasubs.obj printer.obj fmath.obj fmath086.obj \
     log.obj video.obj tgaview.obj f16.obj farvideo.obj fr8514a.obj \
     hgcfra.obj
	link /ST:4096 /NOE @fractint.lnk
