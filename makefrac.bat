rem ** The SET commands are here to ensure no options are accidentally set
set CL=
set MASM=
set LINK=
rem ** Use next two for C6.00A
nmk "CC=cl" "OptT=/Oecilgtaz /Gs" frachelp.mak
if errorlevel 1 goto exit
nmk "CC=cl" "OptT=/DC6 /Oecilgtaz /Gs" "OptS=/DC6 /Osleazcg /Gs" fractint.mak
rem ** Use next two for C5.1
REM make "CC=cl" "OptT=/Oait" frachelp.mak
REM if errorlevel 1 goto exit
REM make "CC=cl" "OptT=/Oait /Gs" "OptS=/Oais /Gs" fractint.mak
rem ** Use next two for QuickC, older vsns may need "make" instead of "nmake"
REM nmake "CC=qcl" "OptT=" frachelp.mak
REM if errorlevel 1 goto exit
REM nmake "CC=qcl" "OptT=/Olt /Gs" "OptS=/Ols /Gs" fractint.mak
:exit
