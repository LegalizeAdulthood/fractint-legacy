@echo off
:begin
cls
echo.
echo            This batch will generate all the images of FRACT19.PAR
echo            ------------------------------------------------------
echo.
echo                         at the desired resolution
echo                         -------------------------
echo.
echo.
echo                            1 - 320 x 200 (F3)
echo                            2 - 640 x 480 (SF5)
echo                            3 - 800 x 600 (SF6)
echo                            4 - 1024 x 768 (SF7)
echo                            5 - exit
echo.
echo.
choice /c:12345 "                       Your selection: "
if errorlevel 5 goto end
if errorlevel 4 goto 1024x768
if errorlevel 3 goto 800x600
if errorlevel 2 goto 640x480
if errorlevel 1 goto 320x200
goto begin

:320x200
RF19 F3
goto end

:640x480
RF19 SF5
goto end

:800x600
RF19 SF6
goto end

:1024x768
RF19 SF7
goto end

:END
