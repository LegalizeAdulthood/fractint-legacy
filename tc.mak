.asm.obj:
 tasm /zi /ml $*,$*.obj

.c.obj:
 tcc -mm -c -w-aus -w-par -w-voi -w-rch -w-rvl -w-eff -w-def -K -C -Z -d -j5 -g25 $*

fract.exe : 3D.OBJ \
	CALCFRAC.OBJ \
	FRACTALS.OBJ \
	PROMPTS.OBJ \
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
	GENERAL.OBJ \
	VIDEO.OBJ \
	NEWTON.OBJ \
	FR8514A.OBJ \
	HGCFRA.OBJ \
	YOURVID.OBJ \
	CMDFILES.OBJ \
	PRINTER.OBJ \
	PARSER.OBJ \
	MPMATH_C.OBJ \
	MPMATH_A.OBJ \
	LORENZ.OBJ \
	PLOT3D.OBJ \
	FPU387.OBJ \
	FPU087.OBJ \
	JB.OBJ \
	TGAVIEW.OBJ \
	F16.OBJ \
	targa.obj loadmap.obj tgasubs.obj 
 tlink /c /v @tc.lnk
 tdstrip -s fractint

fractint.obj : fractint.c fractint.h






