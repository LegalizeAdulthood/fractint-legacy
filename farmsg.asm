;
;	HELP (and other) strings that have been moved to a FARDATA
;	segment to relieve some of the pressure on the poor little
;	overloaded 64K DATA segment.


;			 required for compatibility if Turbo ASM
IFDEF ??version
MASM51
QUIRKS
ENDIF


.model medium, c

.data

helpmessagemain		dw	offset  helpmessagemain1
			dw	seg     helpmessagemain1
			dw	offset  helpmessagemain2
			dw	seg     helpmessagemain2
			dw	offset  helpmessagemain3
			dw	seg     helpmessagemain3
			dw	0,0

helpmessagecycling	dw	offset  helpmessagecycling1
			dw	seg     helpmessagecycling1
			dw	offset  helpmessagecycling2
			dw	seg     helpmessagecycling2
			dw	0,0

helpmessagexhair	dw	offset  helpmessagexhair1
			dw	seg     helpmessagexhair1
			dw	offset  helpmessagexhair2
			dw	seg     helpmessagexhair2
			dw	0,0

helpmessagemouse	dw	offset  helpmessagemouse1
			dw	seg     helpmessagemouse1
			dw	0,0

helpmessagecmdline	dw	offset  helpmessagecmdline1
			dw	seg     helpmessagecmdline1
			dw	offset  helpmessagecmdline2
			dw	seg     helpmessagecmdline2
			dw	offset  helpmessagecmdline3
			dw	seg     helpmessagecmdline3
			dw	offset  helpmessagecmdline4
			dw	seg     helpmessagecmdline4
			dw	offset  helpmessagecmdline5
			dw	seg     helpmessagecmdline5
			dw	0,0

helpmessagefractals	dw	offset  helpmessagefractals1
			dw	seg     helpmessagefractals1
			dw	offset  helpmessagefractals2
			dw	seg     helpmessagefractals2
			dw	offset  helpmessagefractals3
			dw	seg     helpmessagefractals3
			dw	offset  helpmessagefractals4
			dw	seg     helpmessagefractals4
			dw	offset  helpmessagefractals5
			dw	seg     helpmessagefractals5
			dw	offset  helpmessagefractals6
			dw	seg     helpmessagefractals6
			dw	offset  helpmessagefractals7
			dw	seg     helpmessagefractals7
			dw	offset  helpmessagefractals8
			dw	seg     helpmessagefractals8
			dw	offset  helpmessagefractals9
			dw	seg     helpmessagefractals9
			dw	0,0

helpmessageformoreinfo	dw	offset  helpmessageformoreinfo1
			dw	seg     helpmessageformoreinfo1
			dw	0,0

.code

	public	helpmessagetitle
	public	helpmessageauthors
	public	helpmessagecredits
	public	helpmessagemenu
	public	helpmessagemain
	public	helpmessagecycling
	public	helpmessagexhair
	public	helpmessagemouse
	public	helpmessagecmdline
	public	helpmessagefractals
	public	helpmessageformoreinfo
	public	helpmessageendtext
	public	helpmessagemoretext
	public	helpmessagevideo

	public	runningontarga
	public	plasmamessage
	public	argerrormessage
	public	goodbyemessage

	public	helpmessage

	public	initifs, initifs3d

helpmessagetitle	db	"FRACTINT    Version 12.0"
 db 13,10,0

helpmessageauthors	db	13,10
 db "------------------  Primary Authors (this changes over time)  -----------------",13,10
 db "Bert Tyler      - Programmer-Type obsessed with mind-numbing speed...",13,10
 db "                  Compuserve (CIS) ID: [73477,433]   BIX ID: btyler",13,10
 db "Timothy Wegner  - Mathematician-Type obsessed with 3D and endless options...",13,10
 db "                  CIS ID: [71320,675]   Internet: twegner@mwunix.mitre.org",13,10
 db "Mark Peterson   - Mathematician-Type obsessed with fractal types and speed...",13,10
 db "                  CIS ID: [70441,3353]",13,10
 db "---------  Contributing Authors (SPACEBAR toggles scrolling off/on)  ----------",13,10
 db 13,10
 db 13,10		; room for fourteen authors here
 db 13,10
 db 13,10
 db 13,10
 db 13,10
 db 13,10
 db 13,10
 db 13,10
 db 13,10
 db 13,10
 db 13,10
 db 13,10
 db "Please press one of the Function Keys to select a video mode and begin an image",13,10
 db "(or press the 'h' key now or at any other time for help)"
 db 0

helpmessagecredits	db	32
 db  "                 ...",13,10
 db "Michael Abrash  - 360x480x256, 320x400x256 VGA video modes",13,10
 db "Steve Bennett   - restore-from-disk logic",13,10
 db "Rob Beyer       - [71021,2074] Barnsley IFS, Lorenz fractals",13,10
 db "John Bridges    - [73307,606] superVGA support, 360x480x256 mode",13,10
 db "Lee Crocker     - [73407,2030] Fast Newton, Inversion, Decomposition..",13,10
 db "Monte Davis     - [71450,3542] Documentation",13,10
 db "Richard Finegold- [76701,153] 8/16/../256-Way Decomposition option",13,10
 db "Lawrence Gozum  - [73437,2372] Tseng 640x400x256 Video Mode",13,10
 db "Mike Kaufman    - [71610,431] mouse support, other features",13,10
 db "Joe McLain      - [75066,1257] TARGA Support, color-map files",13,10
 db "Bob Montgomery  - [73357,3140] (Author of VPIC) Fast text I/O routines",13,10
 db "Bret Mulvey     - plasma clouds",13,10
 db "Marc Reinig     - [72410,77] Lots of 3D options",13,10
 db "Kyle Powell     - [76704,12] 8514/A Support",13,10
 db "Matt Saucier    - [72371,3101] Printer Support",13,10
 db "Herb Savage     - [71640,455] 'inside=bof60', 'inside=bof61' options",13,10
 db "Dean Souleles   - [75115,1671] Hercules Support",13,10
 db "Scott Taylor    - [72401,410] type=formula 'Scott...' formulas",13,10
 db "Paul Varner     - [73237,411] Floating-point fractal algorithms",13,10
 db "Dave Warker     - Integer Mandelbrot Fractals concept",13,10
 db "Phil Wilson     - [76247,3145] Distance Estimator, Bifurcation fractals",13,10
 db "Richard Wilton  - Tweaked VGA Video modes",13,10
 db "                  ...",13,10
 db "Byte Magazine   - Tweaked VGA Modes",13,10
 db "MS-Kermit       - Keyboard Routines",13,10
 db "PC Magazine     - Sound Routines",13,10
 db "PC Tech Journal - CPU, FPU Detectors",13,10
 db 0

helpmessagemenu	db	13,10
 db 13,10
 db "The following help screens are available",13,10
 db 13,10
 db "1   - Commands available at the initial Credits Screen and main command level",13,10,13,10
 db "2   - Commands available at the Color-Cycling command level",13,10,13,10
 db "3   - Commands available at the Cross-Hair command level",13,10,13,10
 db "4   - The SSTOOLS.INI file and Command-Line arguments",13,10,13,10
 db "5   - Descriptions of currently available fractal types",13,10,13,10
 db "6   - List of Available Video Modes and the keys that select them",13,10
 db "      ( >>> and if you're at the initial Credits Screen, NO FRACTAL IMAGES",13,10
 db "      are going to get drawn UNTIL AFTER you select a video mode <<< )",13,10,13,10
 db "7   - Contacting the authors / obtaining the latest versions of FRACTINT",13,10,13,10
 db "8   - Using FRACTINT with a mouse",13,10,13,10
 db "Please press one of the above keys (or press ESCAPE to exit Help Mode)"
 db 0

helpmessagemain1	db	13,10
 db "The useful keys you can hit while this program is running (the commands marked",13,10
 db "                    with an '*' are also available at the credits screen) are:",13,10
 db 13,10
 db "* h or H or ?       HELP! (Enter help mode and display this screen)",13,10
 db "  PageUp, PageDown  Shrink or Expand the Zoom Box",13,10
 db "  Cursor Keys       Pan (Move) the Zoom Box across the screen",13,10
 db "  Ctrl-Cursor Keys  Fast-Pan the Zoom Box (may require an enhanced keyboard)",13,10
 db "  End or Enter      Redraw the Screen or area inside the Zoom Box",13,10
 db "* F1,F2,F3,F4...    Select a new Video Mode and THEN Redraw",13,10
 db "                    (see the Video-modes HELP screens for the full modes list)",13,10
 db "* 1 or 2 or g       Select Single-Pass, Dual-Pass, or Solid-Guessing mode",13,10
 db "  c or C or + or -  Enter Color-Cycling Mode (see Color-Cycling Help screen)",13,10
 db "  s or S            Save the current screen image to disk (restart with 'r')",13,10
 db "* r or R or 3 or o  Restart from a saved (or .GIF) file ('3' or 'o' for 3-D)",13,10
 db "* t or T            Select a new fractal type and parameters",13,10
 db 13,10
 db 13,10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",13,10
 db "   If the screen finishes before you hit a key, it will beep and wait for you."
 db 0

helpmessagemain2	db	13,10
 db "The useful keys you can hit while this program is running (the commands marked",13,10
 db "                    with an '*' are also available at the credits screen) are:",13,10
 db 13,10
 db "  p or P            Print the screen (command-line options set printer type)",13,10
 db "  Spacebar          Mandelbrot/Julia Set toggle (read FRACTINT.DOC first)",13,10
 db "  < or >            Lower or Raise the Iteration Limit (display with Tab key)",13,10
 db "* n or N or l or L  Select Normal (the default) or Logarithmic Palettes",13,10
 db "  b or B            Add the current fractal description to FRABATCH.BAT",13,10
 db "  o or O            toggles 'orbits' option on and off during image generation",13,10
 db "  Home              Redraw Previous screen (you can 'back out' recursively)",13,10
 db "  Tab               Display the current fractal image information",13,10
 db "  Control-Enter     'Zoom-out' - expands the image so that your current",13,10
 db "                    image is positioned inside the current zoom-box location.",13,10
 db "* e or E            Edit the parameters for the Barnsley IFS fractal-types",13,10
 db "                    (this option ONLY affects Barnsley IFS and IFS3D fractals)",13,10
 db 13,10
 db 13,10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",13,10
 db "   If the screen finishes before you hit a key, it will beep and wait for you."
 db 0

helpmessagemain3	db	13,10
 db "The useful keys you can hit while this program is running (the commands marked",13,10
 db "                    with an '*' are also available at the credits screen) are:",13,10
 db 13,10
 db "  i or I            apply inversion to the current fractal image",13,10
 db "* f or F            toggle the floating-point option ON or OFF.  The",13,10
 db "                    TAB key display will indicate so if it's ON.",13,10
 db "                    See FRACTINT.DOC for details.",13,10
 db "  q or Q            apply Decomposition (2, 4, .. 256-way) to the current",13,10
 db "                    fractal image.  See FRACTINT.DOC for details.",13,10
 db "  a or A            Convert the current image into a fractal 'starfield'",13,10
 db "                    (the 'astrologer' option).",13,10
 db "* Insert            Restart the program (at the credits screen)",13,10
 db "* d or D            Shell to DOS (type 'exit' at the DOS prompt to return)",13,10
 db "* Delete or Esc     Stop the program and return to MSDOS",13,10
 db 13,10
 db 13,10
 db 13,10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",13,10
 db "   If the screen finishes before you hit a key, it will beep and wait for you."
 db 0

helpmessagecycling1	db	13,10
 db "Command Keys that are available in Color-Cycling mode (which is the mode you",13,10
 db "are in if you have hit the 'c' key and are now paused in Color-Cycling mode",13,10
 db "with a white overscan (border) area, or you hit the '+' or '-' keys and",13,10
 db "the the colors are now cycling on your screen).  Commands marked with an '*'",13,10
 db "are available only on VGA systems (the others also work on EGA systems).",13,10
 db 13,10
 db "  h or H or ?      HELP! (Enter help mode and display this screen)",13,10
 db "  + or -           (re)-set the direction of the color-cycling",13,10
 db "  Right/Left Arrow (re)-set the direction of the color-cycling (just like +/-)",13,10
 db "  Up/Down Arrow    SpeedUp/SlowDown the color cycling process",13,10
 db "  F1 thru F10      Select Short--Medium--Long (randomly-generated) color bands",13,10
 db "  1  thru 9        Cycle through 'nn' colors between screen updates (default=1)",13,10
 db "  Enter            Randomly (re)-select all new colors  [TRY THIS ONE!]",13,10
 db "  Spacebar         Pause until another key is hit (the overscan area is set",13,10
 db "                   to white as a visual indicator of a Color-Cycling pause)",13,10
 db "* SF1 thru AF10    Pause and re-set the Palette to one of 30 fixed sequences",13,10
 db "* r or g or b or   force a pause and Lower (lower case) or Raise (upper case)",13,10
 db "* R or G or B      the Red, Green, or Blue component of the fractal image",13,10
 db 0

helpmessagecycling2	db	13,10
 db "Command Keys that are available in Color-Cycling mode (which is the mode you",13,10
 db "are in if you have hit the 'c' key and are now paused in Color-Cycling mode",13,10
 db "with a white overscan (border) area, or you hit the '+' or '-' keys and",13,10
 db "the the colors are now cycling on your screen).  Commands marked with an '*'",13,10
 db "are available only on VGA systems (the others also work on EGA systems).",13,10
 db 13,10
 db "  d or D or a or A pause and load the palette from DEFAULT.MAP or ALTERN.MAP",13,10
 db "  m or M           pause, prompt for a palette map filename (default",13,10
 db "                   filetype is .MAP), and load the palette from that map file",13,10
 db "  s or S           pause, prompt for a palette map filename (default",13,10
 db "                   filetype is .MAP), and save the palette to that map file",13,10
 db "* x or X           Enter Cross-hair palette-manipulation mode, where you can",13,10
 db "                   modify the screen colors a palette at a time (see the",13,10
 db "                   Cross-Hair Help screen for details)",13,10
 db "  (any other key)  Exit Color-Cycling and return to main command level",13,10
 db 0

helpmessagexhair1	db	13,10
 db "Command Keys that are available in Cross-Hair mode (which is the mode you",13,10
 db "are in if you have hit the 'x' key in Color-Cycling mode and are now",13,10
 db "looking at a screen with a white overscan (border) area and a Cross-Hair",13,10
 db "cursor on the screen).  Cross-Hair mode is available only on VGA systems.",13,10
 db 13,10
 db "  h or H or ?      HELP! (Enter help mode and display this screen)",13,10
 db "  Cursor Keys      Move the cross-hair cursor around the screen.  The",13,10
 db "                   Control-Cursor keys move the cross-hair around faster.",13,10 
 db "                   A mouse can also be used to move around, in which case",13,10
 db "                   holding down the right button speeds up cursor movement",13,10
 db "  r or g or b or   Lower (lower case) or Raise (upper case) the Red, Green,",13,10
 db "  R or G or B      or Blue component of the palette color of the pixel",13,10
 db "                   in the center of the Cross-Hair cursor (and all of the",13,10
 db "                   other pixels that use the same palette value)",13,10
 db "  + or -           Change the RGB values of the palette of the pixel in",13,10
 db "                   the center of the Cross-Hair cursor (and all of its",13,10
 db "                   sister pixels) to that of the next higher (or lower)",13,10
 db "                   palette value.  Useful for 'erasing' bands of color.",13,10 
 db 0

helpmessagexhair2	db	13,10
 db "Command Keys that are available in Cross-Hair mode (which is the mode you",13,10
 db "are in if you have hit the 'x' key in Color-Cycling mode and are now",13,10
 db "looking at a screen with a white overscan (border) area and a Cross-Hair",13,10
 db "cursor on the screen).  Cross-Hair mode is available only on VGA systems.",13,10
 db 13,10
 db "  PageUp or PageDown Change the palette value (color) of the Cross-Hair",13,10
 db "                   Cursor.  Useful when the cursor gets 'lost'.",13,10
 db "                   Holding down the Left mouse-button and moving the",13,10
 db "                   mouse forward and backward also changes the cursor color.",13,10
 db "  Enter            'Do-Nothing' key, added just to keep from accidentally",13,10
 db "                   exiting Cross-Hair mode by pressing both mouse buttons",13,10
 db "                   simultaneously",13,10
 db "  (any other key)  Exit Cross-Hair mode and return to Color-Cycling mode",13,10
 db 0

helpmessagemouse1	db	13,10
 db "Using FRACTINT with a Mouse",13,10,13,10
 db "Left Button:   Brings up and sizes the Zoom Box.   While holding down the",13,10
 db "               left button, push the mouse forward to shrink the Zoom Box,",13,10
 db "               and pull it back to expand it.   Then let go of the button",13,10
 db "               and move the mouse around to 'pan' the Zoom Box (with no",13,10
 db "               buttons held down, you are in 'fast-pan' mode).",13,10
 db 13,10
 db "Right Button:  When the right button is held down, the 'panning' operation",13,10
 db "               switches from 'fast-pan' to 'slow-pan' mode, giving you",13,10
 db "               better control over the location of the Zoom Box.",13,10
 db 13,10
 db "Both Buttons:  (or the middle button, if you have three of them) Redraws",13,10
 db "               the area inside the Zoom Box over your full screen.",13,10
 db 13,10
 db "Zoom and Pan using the mouse typically consists of pushing in the left",13,10
 db "button, sizing the zoom box, letting go of the button, fast-panning to",13,10
 db "the general area, pushing in the right button and slow-panning to the",13,10
 db "exact area you want, and then (still holding down the right button) tapping",13,10
 db "the left button to perform the Zoom.",13,10
 db 0

helpmessagecmdline1	db	13,10
 db "If FRACTINT locates a file called 'SSTOOLS.INI' in the DOS PATH, it reads",13,10
 db "initialization variables from it.  These variables can also be initialized",13,10
 db "on the command-line as arguments, or from other files referenced on the",13,10
 db "command line using an '@' notation.  The variables currently available are:",13,10
 db 13,10
 db "@filename                  Read more cmd-line arguments from 'filename'",13,10
 db "                           (this option only available on the command-line)",13,10
 db "[filename=]filename        Start with this saved file (one saved by FRACTINT",13,10
 db "                           or a generic GIF file [treated as a plasma cloud])",13,10
 db "                           ('filename=' is mandatory inside '@' or .INI files)",13,10
 db "savename=filename          Save files using this name (instead of FRACT001)",13,10
 db "video=xxx                  Begin with this video mode (Example: Video=F2)",13,10
 db "                           See the video-modes HELP screen for a full list",13,10
 db "type=fractaltype           Perform this Fractal Type (Default = mandel)",13,10
 db "                           See the fractaltypes HELP screen for a full list",13,10
 db "params=xxx[/xxx[/xxx]]...  Begin with these extra Parameter values",13,10
 db "                           (Examples:  params=4   params=-0.480/0.626)",13,10
 db "corners=xmin/xmax/ymin/ymax  Begin with these X, Y Coordinates",13,10
 db "                           (Example: corners=-0.739/-0.736/0.288/0.291)",13,10
 db 0

helpmessagecmdline2	db	13,10
 db "The initialization variables available, and their formats are:",13,10
 db 13,10
 db "passes=x (x = 1, 2, or g)  Select Single-Pass, Dual-Pass, or Solid-Guessing",13,10
 db "potential=nn[/nn[/nn]]     Continuous Potential options (see FRACTINT.DOC)",13,10
 db "logmap=yes                 Use a Logarithmic palette map rather than the",13,10
 db "                           default (Continuous) palette map",13,10
 db "maxiter=nnn                Maximum number of iterations (default = 150)",13,10
 db "iterincr=nnn               Iteration inc/decrement stepsize (default = 50)",13,10
 db "inside=nnn                 Mandelbrot Interior color (inside=0 for black)",13,10
 db "map=filename               (VGA or TARGA) get the color map from 'filename'",13,10
 db "warn=yes                   Tells FRACTINT to avoid over-writing existing files",13,10
 db "batch=yes                  Batch mode run (display image, save-to-disk, exit)",13,10
 db "batch=config               Batch mode run to generate a 'fractint.cfg' file",13,10
 db "cyclelimit=nnn             color-cycler speed-limit (1 to 256, default = 55)",13,10
 db "float=yes                  For some functions changes from integer math to fp",13,10
 db "ifs=filename               Define an IFS map for the Barnsley IFS fractals",13,10
 db "                           Read the IFS section of FRACTINT.DOC for details",13,10
 db "printer=type[/res[/lpt#]]  Set the printer type (HP-Laserjet, IBM, Epson),",13,10
 db "                           dots/inch, and port# (1-3 for LPTn, 11-14 for COMn)",13,10
 db 0

helpmessagecmdline3	db	13,10
 db "The initialization variables available, and their formats are:",13,10
 db 13,10
; db "3d=[nn[/nn[/nn]]]...       Generate 'filename' (above) as a 3D image using",13,10
; db "                           'nn/nn...' as default answers to the 3D prompts",13,10
 db "preview=yes                Turns on 3D 'preview' default mode",13,10
 db "showbox=yes                Turns on 3D 'showbox' default mode",13,10
 db "sphere=yes                 Turns on 3D sphere mode",13,10
 db "longitude=nn/nn            Longitude minumim and maximum",13,10
 db "latitude=nn/nn             Latitude minimum and maximum",13,10
 db "radius=nn                  Radius scale factor",13,10
 db "rotation=nn[/nn[/nn]]      Rotation abount x,y, and z axes",13,10
 db "scalexyz=nn/nn/nn          X, Y, and Z scale facytors",13,10
 db "roughness=nn               Same as Z scale factor",13,10
 db "waterline=nn               Colors this number and below will be 'inside' color",13,10
 db "filltype=nn                3D filltype",13,10
 db "perspective=nn             Perspective viewer distance (100 is at the edge)",13,10
 db "xyshift=nn/nn              Shift image in x and y directions (alters viewpoint)",13,10
 db "lightsource=nn/nn/nn       The coordinates of the light source vector",13,10
 db "smoothing=nn               Smooths rough images in light source mode",13,10
 db "invert=nn/nn/nn            Turns on inversion - turns images 'inside out'.",13,10
 db "transparent=mm/nn          Sets colors 'mm' to 'nn as transparent",13,10
 db 0

helpmessagecmdline4	db	13,10
 db "The initialization variables available, and their formats are:",13,10
 db 13,10
 db "coarse=nnn                 Sets Preview 'coarseness' default value",13,10
 db "stereo=nnn                 Sets Stereo (R/B 3D) option:  0 = none,",13,10
 db "                           1 = alternate, 2 = superimpose, 3 = photo",13,10
 db "interocular=nnn            Sets 3D Interocular distance default value",13,10
 db "converge=nnn               Sets 3D Convergence default value",13,10
 db "crop=nnn/nnn/nnn/nnn       Sets 3D red-left, red-right, blue-left,",13,10
 db "                           and blue-right cropping default valuess",13,10
 db "bright=nnn/nnn             Sets 3D red and blue brightness defaults,",13,10
 db "xyadjust=nnn/nnn           Sets 3D X and Y adjustment defaults,",13,10
 db "rseed=nnnnn                Forces reproducable Plasma Clouds.  The 'rseed='",13,10
 db "                           value is listed as part of the <TAB> display",13,10
 db "decomp=nn[/nnnnn]          'Decomposition' toggle.  First value 2 to 256,",13,10
 db "                           2nd is bailout limit.  See FRACTINT.DOC for details",13,10
 db "biomorph=nnn               Turns on Biomorph Coloring (use with the mansinzexpd",13,10
 db "                           and Julsinzexpd fractal types)",13,10
 db "bailout=nnnn               Use this as the iteration bailout value (rather than",13,10
 db "                           the default value of [for most fractal types] 4.0)",13,10
 db 0

helpmessagecmdline5	db	13,10
 db "symmetry=xxxx              Force symmetry to None, Xaxis, Yaxis, XYaxis,",13,10
 db "                           Origin, or Pi symmetry.  Useful for debugging.",13,10
 db "formulafile=filename       Find the 'type=formula' fractals in this file",13,10
 db "                           instead of the default file (FRACTINT.FRM).",13,10
 db "formulaname=formulaname    Have the 'type=formula' fractals use this",13,10
 db "                           formula (instead of the first one in the file).",13,10
 db "askvideo=no                Disable 'Is This Mode OK?' prompt if you have a ",13,10
 db "                           FRACTINT.CFG file restricted to legal video modes.",13,10
 db "    ;                      indicates the rest of the line is a comment",13,10
 db "                           (IE, 'fractint type=plasma ; use plasma clouds')",13,10
 db "sound=off                  (nobody ever plays with fractals at work, do they?)",13,10
 db 0

helpmessagefractals1	db	13,10
 db "Fractal types supported include (see FRACTINT.DOC for full descriptions):",13,10
 db 13,10
 db "mandel     = 'Classic' Mandelbrot fractals using 32-bit integer math for speed.",13,10
 db "             z(0) = 0; z(n+1) = z(n)**2 + C, where C = Xcoord + i * Ycoord.",13,10
 db "             Two optional params: real and imaginary parts of z(0) (if not 0).",13,10
 db "julia      = 'Classic' Julia set fractals using 32-bit integer math for speed.",13,10
 db "             z(0) = Xcoord + i * Ycoord; z(n+1) = z(n)**2 + C.",13,10
 db "             Two params required: real and imaginary parts of C.",13,10
 db "newton,    = Newton Domains-of-attraction (only the coloring schemes are",13,10
 db "newtbasin    different).  First param:  the power (from 3 to 10) of the eqn.",13,10
 db "             If param=4, the eqn is z(n+1) = (3*z(n)**4+1)/(4*z(n)**3).",13,10
 db "             Other Parameters invoke an 'Inversion' option if selected:",13,10
 db "             the Radius and X/Y location of the Inversion point",13,10
 db "plasma     = plasma clouds - random, cloud-like formations.  Requires four or",13,10
 db "             more colors.  One param: 'graininess' (.5 to 50, default = 2)",13,10
 db "mandelsine = 'Mandelbrot-Equivalent' for the lambda-sine fractal.  Use the ",13,10
 db "             Space-bar to select LambdaSine fractals a/la Mandel/Julia. ",13,10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = z(0)*sine(z(n)). No Parms.",13,10
 db 0

helpmessagefractals2	db	13,10
 db "Fractal types supported include (see FRACTINT.DOC for full descriptions):",13,10
 db 13,10
 db "lambdasine = lambda-sine fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             lambda * sine(z(n)).  Two params: real, imag portions of lambda.",13,10
 db "mandelcos  = 'Mandelbrot-Equivalent' for the lambda-cosine fractal.  Use the ",13,10
 db "             Space-bar to select LambdaSine fractals a/la Mandel/Julia. ",13,10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = z(0)*cosine(z(n)). No Parms.",13,10
 db "lambdacos  = lambda-cosine  fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             lambda * cosine(z(n)).  Two params: real, imag portions of lambda.",13,10
 db "mandelexp    'Mandelbrot-Equivalent' for the lambda-exp fractal.  Use the ",13,10
 db "             Space-bar to select LambdaSine fractals a/la Mandel/Julia. ",13,10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = z(0)*exp(z(n)). No Parms.",13,10
 db "lambdaexp  = lambda-exponent fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             lambda * exp(z(n)).  Two params: real, imag portions of lambda.",13,10
 db "barnsleym1 = Michael Barnsley's alternative 'Mandelbrot'. z(0) = 0; z(n+1) =",13,10
 db "             (z-1)*C if Real(z) >= 0, else = (z+1)*modulus(C)/C, where C =",13,10
 db "             Xcoord + i * Ycoord.",13,10
 db 0

helpmessagefractals3	db	13,10
 db "Fractal types supported include (see FRACTINT.DOC for full descriptions):",13,10
 db 13,10
 db "barnsleyj1 = 'Julia' corresponding to barnsleym1. z(0) = Xcoord + i * Ycoord;",13,10
 db "             z(n+1) = (z-1)*C if Real(z) >= 0, else = (z+1)*modulus(C)/C.",13,10
 db "             Two params required: real and imaginary parts of C.",13,10       
 db "barnsleym2 = Another alternative 'Mandelbrot'. z(0) = 0; z(n+1) = (z-1)*C",13,10 
 db "             if Real(z)*Imag(C) + real(C)*imag(z) >= 0, else z(n+1) =",13,10
 db "             (z+1)*C, where C = Xcoord + i * Ycoord.",13,10
 db "barnsleyj2 = 'Julia' corresponding to barnsleym2. z(0) = Xcoord + i * Ycoord;",13,10
 db "             z(n+1) = (z-1)*C if Real(z)*Imag(C) + real(C)*imag(z) >= 0,",13,10
 db "             else = z(n+1) = (z+1)*C. Two params required: real and imaginary",13,10 
 db "             parts of C.",13,10
 db "barnsleym3 = Another alternative 'Mandelbrot' from Barnsley",13,10
 db "             This one has a formula that's just too long for this puny",13,10
 db "             little help file - see FRACTINT.DOC for details",13,10
 db "barnsleyj3 = 'Julia' corresponding to barnsleym3.",13,10
 db "             This one has a formula that's just too long for this puny",13,10
 db "             little help file - see FRACTINT.DOC for details",13,10
 db 0

helpmessagefractals4	db	13,10
 db "Fractal types supported include (see FRACTINT.DOC for full descriptions):",13,10
 db 13,10
 db "sierpinski = Sierpinski gasket - a Julia set that produces a 'Swiss cheese",13,10
 db "             triangle'. z(n+1) = (2*x,2*y -1) if y > .5; else (2*x-1,2*y)",13,10
 db "             if (x > .5); else (2*x,2*y). No parameters.",13,10
 db "mandellambda= 'Mandelbrot-Equivalent' for the lambda fractal.  Use the ",13,10
 db "             Space-bar to select Lambda fractals a/la Mandel/Julia. ",13,10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = z(0)*(z(n)**2). No Parms.",13,10
 db "lambda     = Classic Lambda fractal.   z(0) = Xcoord + i * Ycoord;  ",13,10
 db "             z(n+1) = Lambda*(z(n)**2).  Two params required: real, imaginary",13,10
 db "             parts of Lambda.",13,10       
 db "marksmandel= Mark Peterson's variant of the mandel-lambda fractal.  z(0) = 0;",13,10
 db "             z(n+1) = ((Xcoord+i*Ycoord)**exp)*z(n) + (Xcoord+i*Ycoord).",13,10
 db "marksjulia = Mark Peterson's variant of the julia-lambda fractal. ",13,10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = (z(0)**exp)*z(n) + z(0).",13,10
 db "unity      = Mark Peterson's 'Unity' fractal type.  Truly Wierd - ",13,10
 db "             See FRACTINT.DOC for the description of this one!",13,10
 db "ifs        = Barnsley IFS Fractal (a fern unless an alternate IFS map has ",13,10
 db "             been defined using the 'ifs=' command-line option).",13,10
 db 0

helpmessagefractals5	db	13,10
 db "Fractal types supported include (see FRACTINT.DOC for full descriptions):",13,10
 db 13,10
 db "ifs3d      = Barnsley 3D IFS Fractal (a fern unless an alternate IFS map has ",13,10
 db "             been defined using the 'ifs3d=' command-line option).",13,10
 db "mandel4    = Fourth-power 'Mandelbrot' fractals using 32-bit integer math.",13,10
 db "             z(0) = 0; z(n+1) = z(n)**4 + C, where C = Xcoord + i * Ycoord.",13,10
 db "             Two optional params: real and imaginary parts of z(0) (if not 0).",13,10
 db "julia4     = Fourth-power Julia set fractals using 32-bit integer math.",13,10
 db "             z(0) = Xcoord + i * Ycoord; z(n+1) = z(n)**4 + C.",13,10
 db "             Two params required: real and imaginary parts of C.",13,10
 db "test       = 'test' point letting us (and you!) easily add fractal types.",13,10
 db "             Currently, the 'Distance Estimator' M'brot/Julia Set algorithm.",13,10
 db "             two optional parameters - if none given, uses the M'brot Set",13,10
 db "             If given, they are the the same as the Julia Set parameters.",13,10
 db "mandelsinh = 'Mandelbrot-Equivalent' for the lambda-sinh fractal.  Use the ",13,10
 db "             Space-bar to select LambdaSine fractals a/la Mandel/Julia. ",13,10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = z(0)*sinh(z(n)). No Parms.",13,10
 db "lambdasinh = lambda-sinh fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             lambda * sinh(z(n)).  Two params: real, imag portions of lambda.",13,10
 db 13,10
 db 0

helpmessagefractals6	db	13,10
 db "Fractal types supported include (see FRACTINT.DOC for full descriptions):",13,10
 db 13,10
 db "mandelcosh = 'Mandelbrot-Equivalent' for the lambda-cosh fractal.  Use the ",13,10
 db "             Space-bar to select LambdaCosh fractals a/la Mandel/Julia. ",13,10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = z(0)*cosh(z(n)). No Parms.",13,10
 db "lambdacosh = lambda-cosh  fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             lambda * cosh(z(n)).  Two params: real, imag portions of lambda.",13,10
 db "mansinzsqrd= 'Mandelbrot-Equivalent' for the Julsinzsqrd fractal. Use the ",13,10
 db "             Space-bar to select Julsinzexp a/la Mandel/Julia.  z(0) = 0;",13,10
 db "             z(n+1) =z(n)**2 + sin(z(n)) + (Xcoord + i * Ycoord). No Parms.",13,10
 db "julsinzsqrd= Julia Biomorph fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             z(n)**2 + sin(z(n)) + C.  Two params: real, imag portions of C.",13,10
 db "manzpower  = 'Mandelbrot-Equivalent' for the julzpower fractal.  Use the ",13,10
 db "             Space-bar to select julzpower fractals a/la Mandel/Julia. ",13,10
 db "             z(n+1) = z(n)^m + C. Parameters are real pertubation, ",13,10
 db "             imaginary pertubation, exponent m.",13,10
 db "julzpower  = Juliazpower fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             z(n)^m + C.  Two params: real, imag portions of C.",13,10
 db 0

helpmessagefractals7	db	13,10
 db "manzzpwr  = 'Mandelbrot-Equivalent' for the julzzpwr fractal.  Use the ",13,10
 db "             Space-bar to select julzzpwr fractals a/la Mandel/Julia. ",13,10
 db "             z(n+1) = z(n)^z(n) + z(n)^m + C. Parameters are real pertubation,",13,10
 db "             imaginary pertubation, and exponent m.",13,10
 db "julzzpwr  =  julia*zpower fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             z(n)^z(n) + z(n)^m + C.  Three params: real, imag portions ",13,10
 db "             C, and the exponent m.",13,10
 db "mansinexp = 'Mandelbrot-Equivalent' for the julsinexp fractal.  Use the ",13,10
 db "             Space-bar to select julsinexp fractals a/la Mandel/Julia. ",13,10
 db "             z(n+1) = sin(z(n)) + e^z(n) + C. Parameters are real pertubation, ",13,10
 db "             and imaginary pertubation of z(0).",13,10
 db "julsinexp =  julia sinexp fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",13,10
 db "             sin(z(n)) + e^z(n) + C.  Two params: real, imag portions C.",13,10
 db "popcorn   =  orbits of x(n+1) = x(n) - h*sin(y(n) + tan(3*y(n)) and",13,10
 db "             y(n+1) = y(n) - h*sin(x(n) + tan(3*x(n)) plotted for EACH ",13,10
 db "             screen pixel and superimposed. If symmetry=none, plots Julia",13,10
 db "             set of same equation.",13,10
 db 0

helpmessagefractals8	db	13,10
 db "demm,     =  Mandelbrot and Julia fractal images generated using the",13,10
 db "demj         'Distance Estimator' method.  Same fractal types, same ",13,10
 db "             input parameters, different coloring schemes!",13,10
 db "Bifurcation = 'Bifurcation' fractal. Pictoral representation of a",13,10
 db "             population growth model.  The model is: Newpopulation =",13,10
 db "             growthrate * oldpopulation * (1 - oldpopulation)",13,10
 db "complexnewton, = Newton's fractal type extended to complex numbers. ",13,10
 db "complexbasin     Newton's fractal uses (z**n + 1) - these types use",13,10
 db "             (z**a + b), where both 'a' and 'b' are complex numbers.",13,10
 db "lorenz    =  Lorenz attractor fractal - orbits of differential equation",13,10 
 db "             x = x + (-a * x * dt) + (a * y * dt)",13,10
 db "             y = y + (b * x * dt) - (y * dt) - (z * x * dt)",13,10
 db "             z = z + (-c * z * dt) + (x * y * dt)",13,10
 db "             Parameters are dt, a, b, and c.",13,10
 db "lorenz3d  =  3D Lorenz attractor with 3D perspective.  Run this while",13,10
 db "             using the transformation option of the E(dit) command",13,10
 db "             to change your perspective.",13,10
 db "formula   =  Formula interpreter - write your own formulas as text files!",13,10
 db "             See FRACTINT.DOC for instructions on using this one.",13,10
 db 0

helpmessagefractals9	db	13,10
 db "julibrot  =  'Julibrot' 4-dimensional fractals.  Read FRACTINT.DOC for",13,10
 db "             an description of these fractals (and a description of",13,10
 db "             the prompts involved in invoking them).",13,10
 db 0

helpmessageformoreinfo1	db	13,10
 db 13,10
 db "Virtually all of the FRACTINT authors can be found on the Compuserve",13,10
 db "network in the COMART ('COMputer ART') forum in S 15 ('Fractals').",13,10
 db "Several of us can also be found on BIX in the GRAPHIC.DISP/FRACTALS area.",13,10
 db 13,10
 db "In addition, several of the authors have agreed to the listing of their",13,10
 db "home addresses at the end of the FRACTINT.DOC file.",13,10
 db 13,10
 db "New versions of FRACTINT are uploaded (as self-extracting archive files)",13,10
 db "to the Compuserve and BIX networks, and make their way to other systems",13,10
 db "from those points.  The latest version of the program can usually be found",13,10
 db "in the following locations:",13,10
 db 13,10
 db "FRAINT.EXE - (Executable/Docs)  Compuserve: COMART DL 15 and IBMNEW DL 5",13,10
 db "              BIX: GRAPHIC.DISP/LISTINGS and IBM.PC/LISTINGS",13,10
 db 13,10
 db "FRASRC.EXE - (Complete Source)  Compuserve: COMART DL 15 and IBMPRO DL 3",13,10
 db "              BIX: GRAPHIC.DISP/LISTINGS and IBM.PC/LISTINGS",13,10
 db 13,10
 db "(What's the latest version?  Well, THIS one was, way back when we uploaded it!)",13,10
 db 0

helpmessagemoretext	db	13,10
 db ">>ANOTHER HELP PAGE IS AVAILABLE<< -- Press the ENTER key to see it."
 db 0;

helpmessageendtext	db	13,10
 db "Press ESCAPE to exit Help mode, or 'h' to see the help menu. ",13,10
 db "Pressing any other key passes that keypress back to your program. "
 db 0;

helpmessagevideo	db	" "
 db "   (Notation:  F1, SF1, CF1, AF1 == Normal-, Shift-, Control- or Alt-F1) ",13,10
 db "    'B' after #-of-colors means video access is via the BIOS (s-l-o-w-l-y)",13,10
 db 13,10
 db ">>ANOTHER HELP PAGE IS AVAILABLE<< -- Press the ENTER key to see it.",13,10
 db "Press ESCAPE to exit Help mode, or 'h' to see the help menu.",13,10
 db "Pressing any other key passes that keypress back to your program. "
 db 0

runningontarga		db	13,10
 db "                FRACTINT Running On TrueVision TARGA Card"
 db 13,10,0

plasmamessage		db	13,10
 db 13,10
 db "I'm sorry, but because of their random-screen-access algorithms, Plasma",13,10
 db "Clouds and Barnsley IFS fractal images cannot be created using a",13,10
 db "Disk-based 'Video' mode.",13,10
 db 13,10
 db "Also, Plasma Clouds can currently only be run in a 4-or-more-color video",13,10
 db "mode (and color-cycled only on VGA adapters [or EGA adapters in their",13,10
 db "640x350x16 mode]).",13,10
 db 13,10,13,10,13,10
 db "Either press a function key (like F1 thru F5) that selects one of",13,10
 db "those modes, or press the 't' key to select a new fractal type.",13,10
 db 13,10,0

argerrormessage		db	13,10
 db "(see the Startup Help screens or FRACTINT.DOC for a complete"
 db 13,10
 db " argument list with descriptions):"
 db 13,10
 db 13,10,0

goodbyemessage		db	13,10
 db 13,10
 db 13,10
 db "Thank You for using FRACTINT"
 db 13,10
 db 13,10,0

;	IFS fractal of a fern
;             a     b     c     d     e     f     p 

initifs	dd 0.00, 0.00, 0.00, 0.16, 0.00, 0.00, 0.01
        dd 0.85, 0.04, -.04, 0.85, 0.00, 1.60, 0.85
        dd 0.20, -.26, 0.23, 0.22, 0.00, 1.60, 0.07
        dd -.15, 0.28, 0.26, 0.24, 0.0,  0.44, 0.07
	dd 28*7 dup(0.0)
        dd 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 1.00

;		IFS3D fractal of a fern
;               a     b     c     d     e     f     g     h     i     j     k     l     p 
initifs3d dd  0.00, 0.00, 0.00, 0.00, 0.18, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.01
          dd  0.85, 0.00, 0.00, 0.00, 0.85, 0.10, 0.00,-0.10, 0.85, 0.00, 1.60, 0.00, 0.85
          dd  0.20,-0.20, 0.00, 0.20, 0.20, 0.00, 0.00, 0.00, 0.30, 0.00, 0.80, 0.00, 0.07
          dd -0.20, 0.20, 0.00, 0.20, 0.20, 0.00, 0.00, 0.00, 0.30, 0.00, 0.80, 0.00, 0.07
          dd  28*13 dup(0.0)
          dd  0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 1.00


.code

helpmessage proc uses ds es di, message:far ptr byte
		les 	di, message
		sub 	ax, ax
		mov 	cx, 2050		; max chars
		cld
		repnz	scasb			; find 0 byte
		mov 	ax, 2049
		sub 	ax, cx

		mov 	cx, ax			; need cx=length for DOS
		mov 	ah, 40h 		; DOS write to file
		mov 	bx, 1			; stdout
		lds 	dx, message
		int 	21h

		xor 	ax, ax
		adc 	ax, 0			; return 1 if DOS failed
		ret
helpmessage endp

		end
