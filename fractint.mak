fractint.obj : fractint.c fractint.h
	cl /c fractint.c

help.obj : help.c fractint.h
	cl /c help.c

config.obj : config.c fractint.h
	cl /c config.c

encoder.obj : encoder.c fractint.h
	cl /c encoder.c

rotate.obj : rotate.c fractint.h
	cl /c rotate.c

gifview.obj : gifview.c
	cl /c gifview.c

decoder.obj : decoder.c
	cl /c decoder.c

calcmand.obj : calcmand.asm
	masm calcmand;

calcfrac.obj : calcfrac.c
	cl /c calcfrac.c

testpt.obj: testpt.c
	cl /c testpt.c

general.obj : general.asm
	masm general;

fractint.exe : fractint.obj help.obj config.obj encoder.obj gifview.obj \
     decoder.obj rotate.obj general.obj calcmand.obj calcfrac.obj testpt.obj
	link fractint help encoder config gifview decoder rotate general \
     calcmand calcfrac testpt;
