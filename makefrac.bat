@echo off
rem ** The SET commands are here to ensure no options are accidentally set
set CL=
set MASM=
set LINK=

rem ** now un-comment out the goto that applies to you...
rem goto msc7debug
    goto msc7
rem goto msc6
rem goto msc5
rem goto quickc

:msc7debug
rem ** Microsoft C7.00 with CodeView Debugging
echo Building Fractint using MSC 7 using Debug
nmake "CC=cl /Zi" "AS=masm /ML /Zi" "LINKER=link /CO " "OptT= " "C7=YES" frachelp.mak
if errorlevel 1 goto exit
nmake "CC=cl /Zi" "AS=masm /ML /Zi" "LINKER=link /CO" "OptT= " "OptS= " "OptN= " "DEBUG=YES" "C7=YES" fractint.mak
goto exit

:msc7
rem ** Microsoft C7.00 or Visual C++ (normal case)
echo Building Fractint using MSC 7 or Visual C++
nmake "CC=cl /Gs" "AS=masm /ML" "LINKER=link" "OptT=/Oilg" "C7=YES" frachelp.mak
if errorlevel 1 goto exit
nmake "CC=cl /Gs /DC6" "AS=masm /ML" "LINKER=link" "OptT=/Oecilgtaz" "OptS=/Osleazcg" "OptN=/Oilg" "C7=YES" fractint.mak
goto exit

:msc6
rem ** Microsoft C6.00A
nmk "CC=cl /Gs" "AS=masm /ML" "LINKER=link" "OptT=/Oecilgtaz" frachelp.mak
if errorlevel 1 goto exit
nmk "CC=cl /Gs /DC6 /qc" "AS=masm /ML" "LINKER=link" "OptT=/Oecilgtaz" "OptS=/Osleazcg" "OptN=/Oeilg" fractint.mak
goto exit

:msc5
rem ** Microsoft C5.1
make "CC=cl" "AS=masm /ML" "LINKER=link" "OptT=/Oait" frachelp.mak
if errorlevel 1 goto exit
make "CC=cl /Gs" "AS=masm /ML" "LINKER=link" "OptT=/Oait" "OptS=/Oais"  "OptN=/Oais" fractint.mak
goto exit

:quickc
rem ** Use for QuickC 2.50
nmake "CC=qcl" "AS=masm /c /ML" "LINKER=link" "OptT=" frachelp.mak
if errorlevel 1 goto exit
nmake "CC=qcl /Gs" "AS=masm /c /ML" "LINKER=link" "OptT=/Olt" "OptS=/Ols" "OptN=/Ols" fractint.mak

:exit
