
# Next is a pseudo-target for nmake/nmk.  It just generates harmless
# warnings with make.

all : hc.exe fractint.hlp

hc.obj : hc.c helpcom.h
	$(CC) /AL /W1 /FPi /c $(OptT) hc.c

hc.exe : hc.obj
	link /ST:4096 /CP:1 /EXEPACK hc;

fractint.hlp : help.src help2.src help3.src help4.src help5.src
	 hc /c
