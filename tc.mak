.asm.obj:
 tasm /zi $*,$*.obj

.c.obj:
 tcc -v -mm -c -w-aus -w-par -wstv -K -C -Z -d -j5 -g25 $*

fract.exe : 3D.OBJ \
	CALCFRAC.OBJ \
	FRACTALS.OBJ \
	CONFIG.OBJ \
	DECODER.OBJ \
	DISKVID.OBJ \
	ENCODER.OBJ \
	FRACTINT.OBJ \
	GIFVIEW.OBJ \
	HELP.OBJ \
	LINE3D.OBJ \
	ROTATE.OBJ \
	TESTPT.OBJ \
	CALCMAND.OBJ \
	FARMSG.OBJ \
	FARVIDEO.OBJ \
	GENERAL.OBJ \
	VIDEO.OBJ \
	NEWTON.OBJ \
	FR8514A.OBJ \
	HGCFRA.OBJ \
	YOURVID.OBJ \
	CMDFILES.OBJ \
	PRINTER.OBJ \
	LOG.OBJ \
	FMATH.OBJ \
	FMATH086.OBJ \
	TGAVIEW.OBJ \
	F16.OBJ \
	targa.obj loadmap.obj tgasubs.obj 
 tlink /v @tc.lnk
 tdstrip -s fractint

fractint.obj : fractint.c fractint.h






