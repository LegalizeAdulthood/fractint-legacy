@echo off
rem ** The SET commands are here to ensure no options are accidentally set
set CL=
set MASM=
set LINK=

rem ** Use next two for C6.00A
nmk "CC=cl" "AS=masm" "LINKER=link" "OptT=/Oecilgtaz /Gs" frachelp.mak
if errorlevel 1 goto exit
nmk "CC=cl" "AS=masm" "LINKER=link" "OptT=/DC6 /Oecilgtaz /Gs" "OptS=/DC6 /Osleazcg /Gs" fractint.mak

rem ** Use next two for C5.1
rem make "CC=cl" "AS=masm" "LINKER=link" "OptT=/Oait" frachelp.mak
rem if errorlevel 1 goto exit
rem make "CC=cl" "AS=masm" "LINKER=link" "OptT=/Oait /Gs" "OptS=/Oais /Gs" fractint.mak

rem ** Use for QuickC 2.51 where qcl is both the C compiler and the assembler
rem nmake "CC=qcl" "AS=qcl" "LINKER=qlink" "OptT=" frachelp.mak
rem if errorlevel 1 goto exit
rem nmake "CC=qcl" "AS=qcl" "LINKER=qlink" "OptT=/Olt /Gs" "OptS=/Ols /Gs" fractint.mak
:exit
