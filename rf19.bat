@echo off
echo.
echo.
echo        Fractint batch file for FRACT19.PAR created with PARtoBAT 3.3
echo               from Michael Peters (100041.247@compuserve.com)
echo.
echo.
pause

:1
IF EXIST ACHUTE.GIF GOTO 2
FRACTINT VIDEO=%1 @FRACT19.PAR/ACHUTE BATCH=YES SAVENAME=ACHUTE
IF ERRORLEVEL 2 GOTO ABORT
:2
IF EXIST AGUAY.GIF GOTO 3
FRACTINT VIDEO=%1 @FRACT19.PAR/AGUAY BATCH=YES SAVENAME=AGUAY
IF ERRORLEVEL 2 GOTO ABORT
:3
IF EXIST LUMBER.GIF GOTO 4
FRACTINT VIDEO=%1 @FRACT19.PAR/LUMBER BATCH=YES SAVENAME=LUMBER
IF ERRORLEVEL 2 GOTO ABORT
:4
IF EXIST MACULATE.GIF GOTO 5
FRACTINT VIDEO=%1 @FRACT19.PAR/MACULATED_1 BATCH=YES SAVENAME=MACULATE
IF ERRORLEVEL 2 GOTO ABORT
:5
IF EXIST ZORRO.GIF GOTO 6
FRACTINT VIDEO=%1 @FRACT19.PAR/ZORRO BATCH=YES SAVENAME=ZORRO
IF ERRORLEVEL 2 GOTO ABORT
:6
IF EXIST 30_000_F.GIF GOTO 7
FRACTINT VIDEO=%1 @FRACT19.PAR/30,000-FEET BATCH=YES SAVENAME=30_000_F
IF ERRORLEVEL 2 GOTO ABORT
:7
IF EXIST ANT.GIF GOTO 8
FRACTINT VIDEO=%1 @FRACT19.PAR/ANT BATCH=YES SAVENAME=ANT
IF ERRORLEVEL 2 GOTO ABORT
:8
IF EXIST BARNSLEY.GIF GOTO 9
FRACTINT VIDEO=%1 @FRACT19.PAR/BARNSLEYJ2-MANH BATCH=YES SAVENAME=BARNSLEY
IF ERRORLEVEL 2 GOTO ABORT
:9
IF EXIST BARNSLE1.GIF GOTO 10
FRACTINT VIDEO=%1 @FRACT19.PAR/BARNSLEYJ2-MANR BATCH=YES SAVENAME=BARNSLE1
IF ERRORLEVEL 2 GOTO ABORT
:10
IF EXIST CHIP.GIF GOTO 11
FRACTINT VIDEO=%1 @FRACT19.PAR/CHIP BATCH=YES SAVENAME=CHIP
IF ERRORLEVEL 2 GOTO ABORT
:11
IF EXIST FRACTINT.GIF GOTO 12
FRACTINT VIDEO=%1 @FRACT19.PAR/FRACTINT BATCH=YES SAVENAME=FRACTINT
IF ERRORLEVEL 2 GOTO ABORT
:12
IF EXIST NEWTON_R.GIF GOTO 13
FRACTINT VIDEO=%1 @FRACT19.PAR/NEWTON-REAL BATCH=YES SAVENAME=NEWTON_R
IF ERRORLEVEL 2 GOTO ABORT
:13
IF EXIST THREEPLY.GIF GOTO 14
FRACTINT VIDEO=%1 @FRACT19.PAR/THREEPLY BATCH=YES SAVENAME=THREEPLY
IF ERRORLEVEL 2 GOTO ABORT
:14
IF EXIST TILEJULI.GIF GOTO 15
FRACTINT VIDEO=%1 @FRACT19.PAR/TILEJULIA BATCH=YES SAVENAME=TILEJULI
IF ERRORLEVEL 2 GOTO ABORT
:15
IF EXIST TILEMAND.GIF GOTO 16
FRACTINT VIDEO=%1 @FRACT19.PAR/TILEMANDEL BATCH=YES SAVENAME=TILEMAND
IF ERRORLEVEL 2 GOTO ABORT
:16
IF EXIST CAVERNS.GIF GOTO 17
FRACTINT VIDEO=%1 @FRACT19.PAR/CAVERNS_OF_MONGUE BATCH=YES SAVENAME=CAVERNS
IF ERRORLEVEL 2 GOTO ABORT
:17
IF EXIST MANDEL_V.GIF GOTO 18
FRACTINT VIDEO=%1 @FRACT19.PAR/MANDEL-VIRUS BATCH=YES SAVENAME=MANDEL_V
IF ERRORLEVEL 2 GOTO ABORT
:18
IF EXIST NUTCRACK.GIF GOTO 19
FRACTINT VIDEO=%1 @FRACT19.PAR/NUTCRACKERMONSTERS BATCH=YES SAVENAME=NUTCRACK
IF ERRORLEVEL 2 GOTO ABORT
:19
IF EXIST SLICED_T.GIF GOTO 20
FRACTINT VIDEO=%1 @FRACT19.PAR/SLICED-TOMATO BATCH=YES SAVENAME=SLICED_T
IF ERRORLEVEL 2 GOTO ABORT

:20
ECHO RF19.BAT successfully completed.
ECHO.
GOTO END

:ABORT
ECHO RF19.BAT aborted! 

:END
