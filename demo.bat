echo off
:top
cls
echo ************************************************************
echo *                                                          *
echo *  The following demonstration of Fractint will only work  *
echo *  with a VGA or higher resolution video system.           *
echo *  The demo should be run in a directory containing all    *
echo *  the Fractint release files.                             *
echo *                                                          *
echo ************************************************************
echo.
echo 0:  Exit
echo 1:  Basic commands demo
echo 2:  New stuff in version 19
echo 3:  Advanced commands demo
echo 4:  All three demos
sschoice
echo.
if errorlevel 5 goto top
if errorlevel 4 goto allthree
if errorlevel 3 goto advanced
if errorlevel 2 goto newin19
if errorlevel 1 goto basic
if errorlevel 0 goto end
goto top

:advanced
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=advanced.key
goto top
:newin19
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=new19.key
del new19???.gif
goto top
:basic
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=basic.key
del basic001.gif
goto top
:allthree
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=basic.key
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=new19.key
fractint savename=.\ filename=.\ curdir=yes @demo.par/Mandel_Demo autokey=play autokeyname=advanced.key
del basic001.gif
del new19???.gif
goto top
:end
echo Have a nice day!
