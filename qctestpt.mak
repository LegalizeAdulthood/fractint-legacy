PROJ	=QCTESTPT
DEBUG	=1
CC	=qcl
CFLAGS_G	= /AM /W3 /Ze 
CFLAGS_D	= /Zi /Zr /Od 
CFLAGS_R	= /O /Ot /DNDEBUG 
CFLAGS	=$(CFLAGS_G) $(CFLAGS_D)
LFLAGS_G	= /CP:0xffff /NOI /SE:0x80 /ST:0x1000 
LFLAGS_D	= /CO 
LFLAGS_R	= 
LFLAGS	=$(LFLAGS_G) $(LFLAGS_D)
RUNFLAGS	=
OBJS_EXT = 	fractint.obj encoder.obj config.obj gifview.obj decoder.obj rotate.obj \
	general.obj calcmand.obj calcfrac.obj diskvid.obj line3d.obj 3d.obj newton.obj cmdfiles.obj \
	fr8514a.obj help.obj farmsg.obj targa.obj loadmap.obj tgasubs.obj printer.obj tiwview.obj \
	fmath086.obj farvideo.obj f16.obj tgaview.obj
LIBS_EXT = 	

all:	$(PROJ).exe

testpt.obj:	testpt.c

fmath.obj:	..\ffloat\fmath.c

$(PROJ).exe:	testpt.obj fmath.obj $(OBJS_EXT)
	echo >NUL @<<$(PROJ).crf
testpt.obj +
fmath.obj +
$(OBJS_EXT)
$(PROJ).exe

$(LIBS_EXT);
<<
	ilink -a -e "link $(LFLAGS) @$(PROJ).crf" $(PROJ)

run: $(PROJ).exe
	$(PROJ) $(RUNFLAGS)

