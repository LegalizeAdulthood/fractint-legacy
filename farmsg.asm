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

helpmessagemain 	dw	offset	helpmessagemain1
			dw	seg	helpmessagemain1
			dw	offset	helpmessagemain2
			dw	seg	helpmessagemain2
			dw	offset	helpmessagemain3
			dw	seg	helpmessagemain3
			dw	0,0

helpmessagecycling	dw	offset	helpmessagecycling1
			dw	seg	helpmessagecycling1
			dw	offset	helpmessagecycling2
			dw	seg	helpmessagecycling2
			dw	0,0

helpmessagexhair	dw	offset	helpmessagexhair1
			dw	seg	helpmessagexhair1
			dw	offset	helpmessagexhair2
			dw	seg	helpmessagexhair2
			dw	0,0

helpmessagemouse	dw	offset	helpmessagemouse1
			dw	seg	helpmessagemouse1
			dw	0,0

helpmessagecmdline	dw	offset	helpmessagecmdline1
			dw	seg	helpmessagecmdline1
			dw	offset	helpmessagecmdline2
			dw	seg	helpmessagecmdline2
			dw	offset	helpmessagecmdline3
			dw	seg	helpmessagecmdline3
			dw	offset	helpmessagecmdline4
			dw	seg	helpmessagecmdline4
			dw	offset	helpmessagecmdline5
			dw	seg	helpmessagecmdline5
			dw	offset	helpmessagecmdline6
			dw	seg	helpmessagecmdline6
			dw	offset	helpmessagecmdline7
			dw	seg	helpmessagecmdline7
			dw	0,0

helpmessagefractals	dw	offset	helpmessagefractals1
			dw	seg	helpmessagefractals1
			dw	offset	helpmessagefractals2
			dw	seg	helpmessagefractals2
			dw	offset	helpmessagefractals3
			dw	seg	helpmessagefractals3
			dw	offset	helpmessagefractals4
			dw	seg	helpmessagefractals4
			dw	offset	helpmessagefractals5
			dw	seg	helpmessagefractals5
			dw	offset	helpmessagefractals6
			dw	seg	helpmessagefractals6
			dw	offset	helpmessagefractals7
			dw	seg	helpmessagefractals7
			dw	offset	helpmessagefractals8
			dw	seg	helpmessagefractals8
			dw	offset	helpmessagefractals9
			dw	seg	helpmessagefractals9
			dw	offset	helpmessagefractals10
			dw	seg	helpmessagefractals10
			dw	offset	helpmessagefractals11
			dw	seg	helpmessagefractals11
			dw	offset	helpmessagefractals12
			dw	seg	helpmessagefractals12
			dw	0,0

helpmessageformoreinfo	dw	offset	helpmessageformoreinfo1
			dw	seg	helpmessageformoreinfo1
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
	public	inversionmessage

	public	helpmessage

	public	initifs, initifs3d

helpmessagetitle	db	"FRACTINT    Version 14.0  "
 db 10,0

helpmessageauthors	db	10
 db "------------------  Primary Authors (this changes over time)  -----------------",10
 db "Bert Tyler          - Compuserve (CIS) ID: [73477,433]   BIX ID: btyler",10
 db "Timothy Wegner      - CIS ID: [71320,675]   Internet: twegner@mwunix.mitre.org",10
 db "Mark Peterson       - CIS ID: [70441,3353]",10
 db "Pieter Branderhorst - CIS ID: [72611,2257]",10
 db "---------  Contributing Authors (SPACEBAR toggles scrolling off/on)  ----------",10
 db 10
 db 10		     ; room for sixteen authors here
 db 10
 db 10
 db 10
 db 10
 db 10
 db 10
 db 10
 db 10
 db 10
 db 10
 db 10
 db 10
 db 10
 db "Please press one of the Function Keys to select a video mode and begin an image",10
 db "(or press the >> F1 << (NOT the 'h') key now or at any other time for help)"
 db 0

helpmessagecredits	db	32
 db  "                 ...",10
 db "Michael Abrash  - 360x480x256, 320x400x256 VGA video modes",10
 db "Kevin Allen     - Finite attractor and bifurcation engine",10
 db "Steve Bennett   - restore-from-disk logic",10
 db "Rob Beyer       - [71021,2074] Barnsley IFS, Lorenz fractals",10
 db "Mike Burkey     - 376x564x256, 400x564x256, and 832x612x256 VGA video modes",10
 db "John Bridges    - [73307,606] superVGA support, 360x480x256 mode",10
 db "Brian Corbino   - [71611,702] Tandy 1000 640x200x16 video mode",10
 db "Lee Crocker     - [73407,2030] Fast Newton, Inversion, Decomposition..",10
 db "Monte Davis     - [71450,3542] Documentation",10
 db "Richard Finegold- [76701,153] 8/16/../256-Way Decomposition option",10
 db "Lawrence Gozum  - [73437,2372] Tseng 640x400x256 Video Mode",10
 db "David Guenther  - [70531,3525] Boundary Tracing algorithm",10
 db "Mike Kaufman    - [71610,431] mouse support, other features",10
 db "Adrian Mariano  - [theorem@blake.acs.washington.edu] Diffusion fractal type",10
 db "Chris Martin    - Paintjet printer support",10
 db "Joe McLain      - [75066,1257] TARGA Support, color-map files",10
 db "Bob Montgomery  - [73357,3140] (Author of VPIC) Fast text I/O routines",10
 db "Bret Mulvey     - plasma clouds",10
 db "Marc Reinig     - [72410,77] Lots of 3D options",10
 db "Kyle Powell     - [76704,12] 8514/A Support",10
 db "Matt Saucier    - [72371,3101] Printer Support",10
 db "Herb Savage     - [71640,455] 'inside=bof60', 'inside=bof61' options",10
 db "Lee Skinner     - Tetrate, Spider, Mandelglass fractal types and more",10
 db "Dean Souleles   - [75115,1671] Hercules Support",10
 db "Kurt Sowa       - [73467,2013] Color Printer Support",10
 db "Scott Taylor    - [72401,410] KAM Torus, many trig function types",10
 db "Paul Varner     - [73237,411] Floating-point fractal algorithms",10
 db "Dave Warker     - Integer Mandelbrot Fractals concept",10
 db "Phil Wilson     - [76247,3145] Distance Estimator, Bifurcation fractals",10
 db "Richard Wilton  - Tweaked VGA Video modes",10
 db "                  ...",10
 db "Byte Magazine   - Tweaked VGA Modes",10
 db "MS-Kermit       - Keyboard Routines",10
 db "PC Magazine     - Sound Routines",10
 db "PC Tech Journal - CPU, FPU Detectors",10
 db 0

helpmessagemenu db	10
 db 10
 db "The following help screens are available",10
 db 10
 db "1   - Commands available at the initial Credits Screen and main command level",10,10
 db "2   - Commands available at the Color-Cycling command level",10,10
 db "3   - Commands available at the Cross-Hair command level",10,10
 db "4   - The SSTOOLS.INI file and Command-Line arguments",10,10
 db "5   - Descriptions of currently available fractal types",10,10
 db "6   - List of Available Video Modes and the keys that select them",10
 db "      ( >>> and if you're at the initial Credits Screen, NO FRACTAL IMAGES",10
 db "      are going to get drawn UNTIL AFTER you select a video mode <<< )",10,10
 db "7   - Contacting the authors / obtaining the latest versions of FRACTINT",10,10
 db "8   - Using FRACTINT with a mouse",10,10
 db "Please press one of the above keys (or press ESCAPE to exit Help Mode)"
 db 0

helpmessagemain1	db	10
 db "The useful keys you can hit while this program is running (the commands marked",10
 db "                    with an '*' are also available at the credits screen) are:",10
 db 10
 db "* F1 or ?           HELP! (Enter help mode and display this screen)",10
 db "* F2,F3,F4,F5...    Select a new Video Mode and THEN Redraw",10
 db "                    (see the Video-modes HELP screens for the full modes list)",10
 db "* t or T            Select a new fractal type and parameters",10
 db "* x or X            Set any of a number of options ('eXtensions')",10
 db "  PageUp, PageDown  Shrink or Expand the Zoom Box",10
 db "  Cursor Keys       Pan (Move) the Zoom Box across the screen",10
 db "  Ctrl-Cursor Keys  Fast-Pan the Zoom Box (may require an enhanced keyboard)",10
 db "  End or Enter      Redraw the Screen or area inside the Zoom Box",10
 db "  Tab               Display the current fractal image information",10
 db "  Home              Redraw Previous screen (you can 'back out' recursively)",10
 db "  s or S            Save the current screen image to disk (restart with 'r')",10
 db "* r or R or 3 or o  Restart from a saved (or .GIF) file ('3' or 'o' for 3-D)",10
 db 10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",10
 db "   If the screen finishes before you hit a key, it will beep and wait for you."
 db 0

helpmessagemain2	db	10
 db "The useful keys you can hit while this program is running (the commands marked",10
 db "                    with an '*' are also available at the credits screen) are:",10
 db 10
 db "  p or P            Print the screen (command-line options set printer type)",10
 db "  c or C or + or -  Enter Color-Cycling Mode (see Color-Cycling Help screen)",10
 db "  Spacebar          Mandelbrot/Julia Set toggle (read FRACTINT.DOC first)",10
 db "* Delete or Esc     Stop the program and return to MSDOS",10
 db "* Insert            Restart the program (at the credits screen)",10
 db "* d or D            Shell to DOS (type 'exit' at the DOS prompt to return)",10
 db "  o or O            toggles 'orbits' option on and off during image generation",10
 db "  Ctrl- Enter       'Zoom-out' - expands the image so that your current",10
 db "                    image is positioned inside the current zoom-box location.",10
 db "  Ctrl- Pad+/Pad-   Rotate the Zoom Box",10
 db "  Ctrl- PgUp/PgDn   Change Zoom Box vertical size (change its aspect ratio)",10
 db "  Ctrl- Home/End    Change Zoom Box shape",10
 db "  Ctrl- Ins/Del     Change Zoom Box color",10
 db 10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",10
 db "   If the screen finishes before you hit a key, it will beep and wait for you."
 db 0

helpmessagemain3	db	10
 db "The useful keys you can hit while this program is running (the commands marked",10
 db "                    with an '*' are also available at the credits screen) are:",10
 db 10
 db "  b or B            Add the current fractal description to FRABATCH.BAT",10
 db "* 1 or 2 or g       Select Single-Pass, Dual-Pass, or Solid-Guessing mode",10
 db "  < or >            Lower or Raise the Iteration Limit (display with Tab key)",10
 db "* f or F            toggle the floating-point algorithm option ON or OFF.",10
 db "* n or N or l or L  Select Normal (the default) or Logarithmic Palettes",10
 db "  i or I            apply inversion to the current fractal image",10
 db "  q or Q            apply Decomposition (2, 4, .. 256-way) to the current",10
 db "                    fractal image.  See FRACTINT.DOC for details.",10
 db "  a or A            Convert the current image into a fractal 'starfield'",10
 db "                    (the 'astrologer' option).",10
 db "* e or E            Edit the parameters for the Barnsley IFS fractal-types",10
 db "                    (this option ONLY affects Barnsley IFS and IFS3D fractals)",10
 db 10
 db 10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",10
 db "   If the screen finishes before you hit a key, it will beep and wait for you."
 db 0

helpmessagecycling1	db	10
 db "Command Keys that are available in Color-Cycling mode (which is the mode you",10
 db "are in if you have hit the 'c' key and are now paused in Color-Cycling mode",10
 db "with a white overscan (border) area, or you hit the '+' or '-' keys and",10
 db "the the colors are now cycling on your screen).  Commands marked with an '*'",10
 db "are available only on VGA systems (the others also work on EGA systems).",10
 db 10
 db "  F1 or ?          HELP! (Enter help mode and display this screen)",10
 db "  + or -           (re)-set the direction of the color-cycling",10
 db "  Right/Left Arrow (re)-set the direction of the color-cycling (just like +/-)",10
 db "  Up/Down Arrow    SpeedUp/SlowDown the color cycling process",10
 db "  F2 thru F10      Select Short--Medium--Long (randomly-generated) color bands",10
 db "  1  thru 9        Cycle through 'nn' colors between screen updates (default=1)",10
 db "  Enter            Randomly (re)-select all new colors  [TRY THIS ONE!]",10
 db "  Spacebar         Pause until another key is hit (the overscan area is set",10
 db "                   to white as a visual indicator of a Color-Cycling pause)",10
 db "* SF1 thru AF10    Pause and re-set the Palette to one of 30 fixed sequences",10
 db "* r or g or b or   force a pause and Lower (lower case) or Raise (upper case)",10
 db "* R or G or B      the Red, Green, or Blue component of the fractal image",10
 db 0

helpmessagecycling2	db	10
 db "Command Keys that are available in Color-Cycling mode (which is the mode you",10
 db "are in if you have hit the 'c' key and are now paused in Color-Cycling mode",10
 db "with a white overscan (border) area, or you hit the '+' or '-' keys and",10
 db "the the colors are now cycling on your screen).  Commands marked with an '*'",10
 db "are available only on VGA systems (the others also work on EGA systems).",10
 db 10
 db "  d or D or a or A pause and load the palette from DEFAULT.MAP or ALTERN.MAP",10
 db "  m or M           pause, prompt for a palette map filename (default",10
 db "                   filetype is .MAP), and load the palette from that map file",10
 db "  s or S           pause, prompt for a palette map filename (default",10
 db "                   filetype is .MAP), and save the palette to that map file",10
 db "* x or X           Enter Cross-hair palette-manipulation mode, where you can",10
 db "                   modify the screen colors a palette at a time (see the",10
 db "                   Cross-Hair Help screen for details)",10
 db "  (any other key)  Exit Color-Cycling and return to main command level",10
 db 0

helpmessagexhair1	db	10
 db "Command Keys that are available in Cross-Hair mode (which is the mode you",10
 db "are in if you have hit the 'x' key in Color-Cycling mode and are now",10
 db "looking at a screen with a white overscan (border) area and a Cross-Hair",10
 db "cursor on the screen).  Cross-Hair mode is available only on VGA systems.",10
 db 10
 db "  F1 or ?          HELP! (Enter help mode and display this screen)",10
 db "  Cursor Keys      Move the cross-hair cursor around the screen.  The",10
 db "                   Control-Cursor keys move the cross-hair around faster.",10
 db "                   A mouse can also be used to move around.",10
 db "  r or g or b or   Lower (lower case) or Raise (upper case) the Red, Green,",10
 db "  R or G or B      or Blue component of the palette color of the pixel",10
 db "                   in the center of the Cross-Hair cursor (and all of the",10
 db "                   other pixels that use the same palette value)",10
 db "  + or -           Change the RGB values of the palette of the pixel in",10
 db "                   the center of the Cross-Hair cursor (and all of its",10
 db "                   sister pixels) to that of the next higher (or lower)",10
 db "                   palette value.  Useful for 'erasing' bands of color.",10
 db 0

helpmessagexhair2	db	10
 db "Command Keys that are available in Cross-Hair mode (which is the mode you",10
 db "are in if you have hit the 'x' key in Color-Cycling mode and are now",10
 db "looking at a screen with a white overscan (border) area and a Cross-Hair",10
 db "cursor on the screen).  Cross-Hair mode is available only on VGA systems.",10
 db 10
 db "  Ctrl-Ins         Change the palette value (color) of the Cross-Hair",10
 db "    or Ctrl-Del    Cursor.  Useful when the cursor gets 'lost'.",10
 db "                   Holding down the Right mouse-button and moving the",10
 db "                   mouse forward or backward also changes the cursor color.",10
 db "  Enter, Ctrl-Enter, the other keys used for Zoom Box functions, and",10
 db "                   the equivalent mouse buttons, all do nothing in",10
 db "                   Cross-Hair mode.",10
 db "  (any other key)  Exit Cross-Hair mode and return to Color-Cycling mode",10
 db 0

helpmessagemouse1	db	10
 db "Using FRACTINT with a Mouse",10,10
 db "Left Button:   Brings up and sizes the Zoom Box.   While holding down the",10
 db "               left button, push the mouse forward to shrink the Zoom Box,",10
 db "               and pull it back to expand it.",10
 db "               Double-clicking the left button performs the Zoom.",10
 db 10
 db "Right Button:  While holding the right button held down, move the mouse",10
 db "               from side to side to 'rotate' the Zoom Box.  Move the mouse",10
 db "               forward or back to change the Zoom Box color.",10
 db "               Double-clicking the right button performs a 'Zoom-Out'.",10
 db 10
 db "Both Buttons:  (or the middle button, if you have three of them) While",10
 db "               holding down both buttons, move the mouse up and down to",10
 db "               stretch/shrink the height of the Zoom Box, or side to side",10
 db "               to 'squish' the Zoom Box into a non-rectangular shape.",10
 db 10
 db "Zoom and Pan using the mouse typically consists of pushing in the left",10
 db "button, sizing the zoom box, letting go of the button, panning to the",10
 db "general area, then double-clicking the left button to perform the Zoom.",10
 db 0

helpmessagecmdline1	db	10
 db "If FRACTINT locates a file called 'SSTOOLS.INI' in the DOS PATH, it reads",10
 db "initialization variables from it.  These variables can also be initialized",10
 db "on the command-line as arguments, or from other files referenced on the",10
 db "command line using an '@' notation.  The variables currently available are:",10
 db 10
 db "@filename                  Read more cmd-line arguments from 'filename'",10
 db "                           (this option only available on the command-line)",10
 db "[filename=]filename        Start with this saved file (one saved by FRACTINT",10
 db "                           or a generic GIF file [treated as a plasma cloud])",10
 db "                           ('filename=' is mandatory inside '@' or .INI files)",10
 db "savename=filename          Save files using this name (instead of FRACT001)",10
 db "video=xxx                  Begin with this video mode (Example: Video=F2)",10
 db "                           See the video-modes HELP screen for a full list",10
 db "type=fractaltype           Perform this Fractal Type (Default = mandel)",10
 db "                           See the fractaltypes HELP screen for a full list",10
 db "params=xxx[/xxx[/xxx]]...  Begin with these extra Parameter values",10
 db "                           (Examples:  params=4   params=-0.480/0.626)",10
 db "corners=xmin/xmax/ymin/ymax[/x3rd/y3rd]  Begin with these X, Y Coordinates",10
 db "                           (Example: corners=-0.739/-0.736/0.288/0.291)",10
 db 0

helpmessagecmdline2	db	10
 db "The initialization variables available, and their formats are:",10
 db 10
 db "passes=x (x = 1, 2, g, or b)  Select Single-Pass, Dual-Pass, Solid-Guessing",10
 db "                           or the Boundary-Tracing drawing algorithms",10
 db "potential=nn[/nn[/nn]]     Continuous Potential options (see FRACTINT.DOC)",10
 db "logmap=yes|old             Maps logarithm of iteration to color. Updated ",10
 db "                           calculation in version 14; 'old' selects old method.",10
 db "maxiter=nnn                Maximum number of iterations (default = 150)",10
 db "iterincr=nnn               Iteration inc/decrement stepsize (default = 50)",10
 db "inside=nnn                 Fractal interior color (inside=0 for black)",10
 db "outside=nnn                Fractal exterior color (forces two-color images)",10
 db "finattract=yes             Look for finite attractor in julia types",10
 db "map=filename               (VGA or TARGA) get the color map from 'filename'",10
 db "warn=yes                   Tells FRACTINT to avoid over-writing existing files",10
 db "batch=yes                  Batch mode run (display image, save-to-disk, exit)",10
 db "savetime=nnn               Autosave image every nnn minutes of calculation",10
 db "batch=config               Batch mode run to generate a 'fractint.cfg' file",10
 db "cyclelimit=nnn             color-cycler speed-limit (1 to 256, default = 55)",10
 db "float=yes                  For most functions changes from integer math to fp",10
 db 0

helpmessagecmdline3	db	10
 db "The initialization variables available, and their formats are:",10
 db 10
 db "preview=yes                Turns on 3D 'preview' default mode",10
 db "showbox=yes                Turns on 3D 'showbox' default mode",10
 db "sphere=yes                 Turns on 3D sphere mode",10
 db "longitude=nn/nn            Longitude minumim and maximum",10
 db "latitude=nn/nn             Latitude minimum and maximum",10
 db "radius=nn                  Radius scale factor",10
 db "rotation=nn[/nn[/nn]]      Rotation abount x,y, and z axes",10
 db "scalexyz=nn/nn/nn          X, Y, and Z scale facytors",10
 db "roughness=nn               Same as Z scale factor",10
 db "waterline=nn               Colors this number and below will be 'inside' color",10
 db "filltype=nn                3D filltype",10
 db "perspective=nn             Perspective viewer distance (100 is at the edge)",10
 db "xyshift=nn/nn              Shift image in x and y directions (alters viewpoint)",10
 db "lightsource=nn/nn/nn       The coordinates of the light source vector",10
 db "smoothing=nn               Smooths rough images in light source mode",10
 db "invert=nn/nn/nn            Turns on inversion - turns images 'inside out'.",10
 db "transparent=mm/nn          Sets colors 'mm' to 'nn as transparent",10
 db 0

helpmessagecmdline4	db	10
 db "The initialization variables available, and their formats are:",10
 db 10
 db "coarse=nnn                 Sets Preview 'coarseness' default value",10
 db "stereo=nnn                 Sets Stereo (R/B 3D) option:  0 = none,",10
 db "                           1 = alternate, 2 = superimpose, 3 = photo",10
 db "interocular=nnn            Sets 3D Interocular distance default value",10
 db "converge=nnn               Sets 3D Convergence default value",10
 db "crop=nnn/nnn/nnn/nnn       Sets 3D red-left, red-right, blue-left,",10
 db "                           and blue-right cropping default valuess",10
 db "bright=nnn/nnn             Sets 3D red and blue brightness defaults,",10
 db "xyadjust=nnn/nnn           Sets 3D X and Y adjustment defaults,",10
 db "rseed=nnnnn                Forces reproducable Plasma Clouds.  The 'rseed='",10
 db "                           value is listed as part of the <TAB> display",10
 db "decomp=nn[/nnnnn]          'Decomposition' toggle.  First value 2 to 256,",10
 db "                           2nd is bailout limit.  See FRACTINT.DOC for details",10
 db "biomorph=nnn               Turns on Biomorph Coloring (try with the mansinzexpd",10
 db "                           and Julsinzexpd fractal types)",10
 db "bailout=nnnn               Use this as the iteration bailout value (rather than",10
 db "                           the default value of [for most fractal types] 4.0)",10
 db 0

helpmessagecmdline5	db	10
 db "distest=nnn                Turns on Distance Estimator Method,",10
 db "                           see FRACTINT.DOC for details",10
 db "ifs=filename               Define an IFS map for the Barnsley IFS fractals",10
 db "                           Read the IFS section of FRACTINT.DOC for details",10
 db "symmetry=xxxx              Force symmetry to None, Xaxis, Yaxis, XYaxis,",10
 db "                           Origin, or Pi symmetry.  Useful for debugging.",10
 db "formulafile=filename       Find the 'type=formula' fractals in this file",10
 db "                           instead of the default file (FRACTINT.FRM).",10
 db "formulaname=formulaname    Have the 'type=formula' fractals use this",10
 db "                           formula (instead of the first one in the file).",10
 db "printer=type[/res[/lpt#]]  Set the printer type (HP-Laserjet, IBM, Epson, or,",10
 db "                           Color [Star Micronix]), dots/inch, and port#",10
 db "                           (1-3 for LPTn, 11-14 for COMn)",10
 db "askvideo=no                Disable 'Is This Mode OK?' prompt if you have a ",10
 db "                           FRACTINT.CFG file restricted to legal video modes.",10
 db "    ;                      indicates the rest of the line is a comment",10
 db "                           (IE, 'fractint type=plasma ; use plasma clouds')",10
 db "sound=off                  (nobody ever plays with fractals at work, do they?)",10
 db 0

helpmessagecmdline6	db	10
 db "function=fn1/fn2/fn3/fn4   Allows specification of transcendental functions ",10
 db "                           with types using variable functions. Possible values",10
 db "                           are sin, cos, sinh, cosh, exp, log, and sqr.",10
 db "periodicity=[no|show|nnn]  Controls periodicity checking. 'no' turns checking",10
 db "                           off; entering a number nnn controls the tightness",10
 db "                           of checking (1 is default, higher is more stringent)",10
 db "                           'show' or a neg value colors 'caught' points white.",10
 db "center-mag=[Xctr/Yctr/Mag] Allows an alternative method of entering corners.",10
 db "                           With no parameters causes 'b' command to output ",10
 db "                           'center-mag=' instead of corners.",10
 db "randomize=nnn              smoothes 3d color transitions between elevations",10
 db "fullcolor=yes              allows creation of full color .TGA image with",10
 db "                           light source fill types. See the Docs.",10
 db "ambient=nnn                sets depth of shadows and contrast when using",10
 db "                           light source fill types",10
 db "haze=nnn                   sets amount of haze for distant objct if fullcolor=1",10
 db "lightname=filename         <full path optional> file name which will be used to",10
 db "                           save the full color files with full color option.",10
 db 0

helpmessagecmdline7	db	10
 db "gif87a=yes                 Forces Fractint to save all GIF files in the older",10
 db "                           GIF87a format (with no FRACTINT extension blocks)",10
 db "initorbit=nnn/nnn          Sets the value used to initialize Mandelbrot orbits",10
 db "                           to the given complex number (real and imag parts).",10
 db "initorbit=pixel            Sets the value used to initialize Mandelbrot orbits",10
 db "                           to the complex number corresponding to the screen ",10
 db "                           pixel. This is the default for most types.",10
 db 0

helpmessagefractals1	db	10
 db "mandel     = 'Classic' Mandelbrot fractals using 32-bit integer math for speed.",10
 db "             z(0) = 0; z(n+1) = z(n)^2 + C, where C = Xcoord + i * Ycoord.",10
 db "             Two optional params: real and imaginary pertibations of z(0).",10
 db "julia      = 'Classic' Julia set fractals using 32-bit integer math for speed.",10
 db "             z(0) = Xcoord + i * Ycoord; z(n+1) = z(n)^2 + C.",10
 db "             Two params required: real and imaginary parts of C.",10
 db "newton,    = Newton Domains-of-attraction (only the coloring schemes are",10
 db "newtbasin    different).  First param:  the power (from 3 to 10) of the eqn.",10
 db "             If param=4, the eqn is z(n+1) = (3*z(n)^4+1)/(4*z(n)^3).",10
 db "             Second param is flag to turn on alternating stripes in basin mode.",10
 db "complexnewton, = Newton's fractal type extended to complex numbers. ",10
 db "complexbasin     Calculates roots of z^a + b, where both 'a' and 'b' complex.",10
 db "lambda     = Classic Lambda fractal.   z(0) = Xcoord + i * Ycoord;  ",10
 db "             z(n+1) = Lambda*z(n)*(1 - z(n)^2).  Two params: real, imaginary",10
 db "             parts of Lambda. 'Julia' variant of Mandellambda",10
 db "mandellambda= 'Mandelbrot-Equivalent' for the lambda fractal. z(0) = .5;",10
 db "             lambda = Xcoord + i * Ycoord; z(n+1) = lambda*z(n)*(1 - z(n)^2).",10
 db "             Parameters are pertubations of z(0).",10
 db 0

helpmessagefractals2	db	10
 db "Types taken from Michael Barnsley's 'Fractals Everywhere'",10
 db 10
 db "barnsleym1 = Alternative 'Mandelbrot'. z(0) = Xcoord + i * Ycoord;",10
 db "             z(n+1) = (z-1)*C if Real(z) >= 0, else = (z+1)*modulus(C)/C, ",10
 db "             where C = Xcoord + i * Ycoord. Params are pertubations of z(0).",10
 db "barnsleyj1 = 'Julia' corresponding to barnsleym1. z(0) = Xcoord + i * Ycoord;",10
 db "             z(n+1) = (z-1)*C if Real(z) >= 0, else = (z+1)*modulus(C)/C.",10
 db "             Two params required: real and imaginary parts of C.",10
 db "barnsleym2 = Another alternative 'Mandelbrot'. z(0) = 0; z(n+1) = (z-1)*C",10
 db "             if Real(z)*Imag(C) + real(C)*imag(z) >= 0, else z(n+1) =",10
 db "             (z+1)*C, where C = Xcoord + i * Ycoord. Parameters are ",10
 db "             pertubations of z(0).",10
 db "barnsleyj2 = 'Julia' corresponding to barnsleym2. z(0) = Xcoord + i * Ycoord;",10
 db "             z(n+1) = (z-1)*C if Real(z)*Imag(C) + real(C)*imag(z) >= 0,",10
 db "             else = z(n+1) = (z+1)*C. Two params: real and imaginary parts of C.",10
 db "barnsleym3,= Another Barnsley 'Mandelbrot', 'Julia' pair.",10
 db "barnsleyj3   See FRACTINT.DOC for details.",10
 db 0

helpmessagefractals3	db	10
 db "Barnsley types (continued)",10
 db "sierpinski = Sierpinski gasket - a Julia set that produces a 'Swiss cheese",10
 db "             triangle'. z(n+1) = (2*x,2*y -1) if y > .5; else (2*x-1,2*y)",10
 db "             if (x > .5); else (2*x,2*y). No parameters.",10
 db "ifs        = Barnsley IFS Fractal (a fern unless an alternate IFS map has ",10
 db "             been defined using the 'ifs=' command-line option).",10
 db "ifs3d      = Barnsley 3D IFS Fractal (a fern unless an alternate IFS map has ",10
 db "             been defined using the 'ifs3d=' command-line option).",10
 db 0


helpmessagefractals4	db	10
 db "Mark Peterson's Types",10
 db 10
 db "marksmandel= A variant of the mandel-lambda fractal.  z(0) = 0;",10
 db "             z(n+1) = ((Xcoord+i*Ycoord)^exp)*z(n) + (Xcoord+i*Ycoord).",10
 db "             Parameters are pertubations of z(0).",10
 db "marksjulia = A variant of the julia-lambda fractal. ",10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = (z(0)^exp)*z(n) + z(0).",10
 db "cmplxmarksjul, = The above two types generalized with 'exp' a complex rather",10
 db "cmplxmarksmand  than a real number",10
 db "julibrot   = 'Julibrot' 4-dimensional fractals.  Read FRACTINT.DOC for",10
 db "             an description of these fractals (and a description of",10
 db "             the prompts involved in invoking them).",10
 db "unity      = Mark Peterson's 'Unity' fractal type.  Truly Wierd - ",10
 db "             See FRACTINT.DOC for the description of this one!",10
 db "formula    = Formula interpreter - write your own formulas as text files!",10
 db "             See FRACTINT.DOC for instructions on using this one.",10
 db 0

helpmessagefractals5	db	10
 db "lambdafn   = lambda-fn fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",10
 db "             lambda * fn(z(n)).  Two params: real, imag portions of lambda.",10
 db "             'Julia' variant of Mandelfn. 'fn' is a variable function, ",10
 db "             and may be one of sin, cos, sinh, cosh, exp, log, or sqr.",10
 db "mandelfn =   'Mandelbrot-Equivalent' for the lambda-fn fractal.",10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = z(0)*fn(z(n)). Parameters",10
 db "             are pertubations of z(0). 'fn' is a variable function.",10
 db "mandel4    = Fourth-power 'Mandelbrot' fractals using 32-bit integer math.",10
 db "             z(0) = 0; z(n+1) = z(n)^4 + C, where C = Xcoord + i * Ycoord.",10
 db "             Two optional params: real and imaginary parts of z(0) (if not 0).",10
 db "julia4     = Fourth-power Julia set fractals using 32-bit integer math.",10
 db "             z(0) = Xcoord + i * Ycoord; z(n+1) = z(n)^4 + C.",10
 db "             Two params required: real and imaginary parts of C.",10
 db "test       = 'test' point letting us (and you!) easily add fractal types.",10
 db "             Currently, the 'Distance Estimator' M'brot/Julia Set algorithm.",10
 db "             two optional parameters - if none given, uses the M'brot Set",10
 db "             If given, they are the the same as the Julia Set parameters.",10
 db 0

helpmessagefractals6	db	10
 db "plasma     = plasma clouds - random, cloud-like formations.  Requires four or",10
 db "             more colors.  One param: 'graininess' (.5 to 50, default = 2)",10
 db "julfn+zsqrd= Julia Biomorph fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",10
 db "             fn(z(n)) + z(n)^2 + C.  Two params: real, imag portions of C.",10
 db "manfn+zsqrd= 'Mandelbrot-Equivalent' for the Julfn+zsqrd fractal.",10
 db "             z(0) = Xcoord + i * Ycoord0; z(n+1) = fn(z(n)) + z(n)^2 +  ",10
 db "             (Xcoord + i * Ycoord). Parameters are pertubations of z(0).",10
 db "manzpower  = 'Mandelbrot-Equivalent' for julzpower. z(n+1) = z(n)^m + C.",10
 db "             Parameters are real and imaginary pertubations, exponent m.",10
 db "julzpower  = Juliazpower fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",10
 db "             z(n)^m + C.  Three params: real, imag portions of C, exponent m.",10
 db "manzzpwr  = 'Mandelbrot-Equivalent' for the julzzpwr fractal.  Use the ",10
 db "             Space-bar to select julzzpwr fractals a/la Mandel/Julia. ",10
 db "             z(n+1) = z(n)^z(n) + z(n)^m + C. Parameters are real pertubation,",10
 db "             imaginary pertubation, and exponent m.",10
 db "julzzpwr  =  z(0) = Xcoord + i * Ycoord;  z(n+1) = z(n)^z(n) + z(n)^m + C.",10
 db "             Three params: real, imag portions C, and the exponent m.",10
 db 0

helpmessagefractals7	db	10
 db "manfn+exp = 'Mandelbrot-Equivalent' for the julfn+exp fractal.  Use the ",10
 db "             Space-bar to select julfn+exp fractals a/la Mandel/Julia. ",10
 db "             z(n+1) = fn(z(n)) + e^z(n) + C. Parameters are real pertubation, ",10
 db "             and imaginary pertubation of z(0).",10
 db "julfn+exp =  julia fn+exp fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = ",10
 db "             fn(z(n)) + e^z(n) + C.  Two params: real, imag portions C.",10
 db "popcorn   =  orbits of x(n+1) = x(n) - h*sin(y(n) + tan(3*y(n)) and",10
 db "             y(n+1) = y(n) - h*sin(x(n) + tan(3*x(n)) plotted for EACH ",10
 db "             screen pixel and superimposed. ",10
 db "popcornjul=  Julia set from popcorn formula. z(0) = Xcoord + i * Ycoord.",10
 db "             Orbit calculated as in popcorn.",10
 db 0

helpmessagefractals8	db	10
 db "bifurcation = 'Bifurcation' fractal. Pictoral representation of a population",10
 db "             growth model.  Let P = new population, p = oldpopulation,",10
 db "             r = growth rate.   The model is: P = p +  r*p*(1-p).",10
 db "biflambda =  Bifurcation variation: model is: P =      r*p*(1-p)P.",10
 db "bif+sinpi =  Bifurcation variation: model is: P =  p + r*sin(PI*p).",10
 db "bif=sinpi =  Vifurcation variation: model is: P =      r*sin(PI*p).",10
 db "lorenz    =  Lorenz attractor - orbit in three dimensions defined by:",10
 db "             xnew = x + (-a*x*dt) + (a*y*dt)",10
 db "             ynew = y + (b*x*dt) - (y*dt) - (z*x*dt)",10
 db "             znew = z + (-c*z*dt) + (x*y*dt)",10
 db "             Parameters are dt, a, b, and c.",10
 db "lorenz3d  =  3D Lorenz attractor with 3D perspective.",10
 db "rossler3D =  Orbit in three dimensions defined by:",10
 db "             xnew = x - y*dt -   z*dt",10
 db "             ynew = y + x*dt + a*y*dt",10
 db "             znew = z + b*dt + x*z*dt - c*z*dt",10
 db "             Parameters are dt, a, b, and c.",10
 db 0

helpmessagefractals9	db	10
 db "henon     =  Orbit in two dimensions defined by:",10
 db "             xnew = 1 + y - a*x*x",10
 db "             ynew = b*x",10
 db "             Parameters are a and b",10
 db "pickover  =  Orbit in three dimensions defined by:",10
 db "             xnew = sin(a*y) - z*cos(b*x)",10
 db "             ynew = z*sin(c*x) - cos(d*y)",10
 db "             znew = sin(x)",10
 db "             Parameters are a, b, c, and d.",10
 db "gingerbread = Orbit in two dimensions defined by:",10
 db "             x <- 1 - y + |x|",10
 db "             y <- x",10
 db "diffusion =  Diffusion Limited Aggregation.  Randomly moving points",10
 db "             accumulate.  One param:  border width (default 10)",10
 db 0

helpmessagefractals10	db	10
 db "Scott Taylor/Lee Skinner fractal types - 'fn' can be replaced by sin,cos, etc.",10
 db 10
 db "fn(z*z)   =  z(0) = Xcoord + i * Ycoord; z(n+1) = fn(z(n)*z(n))",10
 db "fn*fn     =  z(0) = Xcoord + i * Ycoord; z(n+1) = fn(n)*fn(n)",10
 db "fn*z+z    =  z(0) = Xcoord + i * Ycoord; z(n+1) = p1*fn(z(n))+p2*z(n)",10
 db "             Parameters are real and imaginary components of p1 and p2.",10
 db "fn+fn     =  z(0) = Xcoord + i * Ycoord; z(n+1) = p1*fn1(z(n))+p2*fn2(z(n))",10
 db "             Parameters are real and imaginary components of p1 and p2.",10
 db "sqr(1/fn) =  z(0) = Xcoord + i * Ycoord; z(n+1) = (1/fn(z(n))*(1/fn(z(n))",10
 db "sqr(fn)   =  z(0) = Xcoord + i * Ycoord; z(n+1) = fn(z(n))*fn(z(n))",10
 db "spider    =  c(0) = z(0) = Xcoord + i * Ycoord; z(n+1) = z(n)*z(n) + c(n);",10
 db "             c(n+1) = c(n)/2 + z(n+1)",10
 db "manowar   =  c = z1(0) = z(0) = Xcoord + i*Ycoord; z(n+1) = z(n)^2 + z1(n) + c;",10
 db "             z1(n+1) = z(n); ",10
 db "tetrate   =  z(0) = c = Xcoord + i * Ycoord; z(n+1) = c^z(n)",10
 db 0

helpmessagefractals11	db	10
 db "kamtorus  =  series of orbits superimposed. x(0) = y(0) = orbit/3; ",10
 db "             x(n+1) = x(n)*cos(a) + (x(n)*x(n)-y(n))*sin(a) ",10
 db "             y(n+1) = x(n)*sin(a) - (x(n)*x(n)-y(n))*cos(a) ",10
 db "             After each orbit, 'orbit' is incremented by a step size. Parmeters",10
 db "             are a, step size, stop value for 'orbit', and points per orbit.",10
 db "kamtorus3d=  Same as kamtorus but in 3D with 'orbit' the z dimension.",10
 db 0

helpmessagefractals12	db	10
 db "magnetj1  =  z(0) = Xcoord + i * Ycoord;",10
 db "                        /  z(n)^2 + (c-1) \",10
 db "              z(n+1) = |  ---------------  | ^ 2",10
 db "                        \  2*z(n) + (c-2) /",10
 db "             Parameters are real and imaginary parts of c.",10
 db "magnetm1  =  'Mandelbrot' variant with c = Xcoord + i * Ycoord and z(0) = 0.",10
 db "             Orbit formula same as magnetj1",10
 db "             Parameters are pertubations of z(0).",10
 db "magnetj2  =  z(0) = Xcoord + i * Ycoord;",10
 db "                       /      z(n)^3 + 3*(C-1)*z(n) + (C-1)*(C-2)      \",10
 db "             z(n+1) = |  --------------------------------------------   | ^ 2",10
 db "                       \  3*(z(n)^2) + 3*(C-2)*z(n) + (C-1)*(C-2) - 1  /",10
 db "             Parameters are real and imaginary parts of c.",10
 db "magnetm2  =  'Mandelbrot' variant with c = Xcoord + i * Ycoord and z(0) = 0.",10
 db "             Orbit formula same as magnetj2",10
 db "             Parameters are pertubations of z(0).",10
 db 0

helpmessageformoreinfo1 db	10
 db 10
 db "Virtually all of the FRACTINT authors can be found on the Compuserve",10
 db "network in the COMART ('COMputer ART') forum in S 15 ('Fractals').",10
 db "Several of us can also be found on BIX in the GRAPHIC.DISP/FRACTALS area.",10
 db 10
 db "In addition, several of the authors have agreed to the listing of their",10
 db "home addresses at the end of the FRACTINT.DOC file.",10
 db 10
 db "New versions of FRACTINT are uploaded (as self-extracting archive files)",10
 db "to the Compuserve and BIX networks, and make their way to other systems",10
 db "from those points.  The latest version of the program can usually be found",10
 db "in the following locations:",10
 db 10
 db "FRAINT.EXE - (Executable/Docs)  Compuserve: COMART DL 15 and IBMNEW DL 5",10
 db "              BIX: GRAPHIC.DISP/LISTINGS and IBM.PC/LISTINGS",10
 db 10
 db "FRASRC.EXE - (Complete Source)  Compuserve: COMART DL 15",10
 db "              BIX: GRAPHIC.DISP/LISTINGS and IBM.PC/LISTINGS",10
 db 10
 db "(What's the latest version?  Well, THIS one was, way back when we uploaded it!)",10
 db 0

helpmessagemoretext	db	10
 db ">>ANOTHER HELP PAGE IS AVAILABLE<< -- Press the ENTER key to see it."
 db 0;

helpmessageendtext	db	10
 db "Press ESCAPE to exit Help mode, or F1 to see the help menu. ",10
 db "Pressing any other key passes that keypress back to your program. "
 db 0;

helpmessagevideo	db	" "
 db "   (Notation:  F1, SF1, CF1, AF1 == Normal-, Shift-, Control- or Alt-F1) ",10
 db "    'B' after #-of-colors means video access is via the BIOS (s-l-o-w-l-y)",10
 db 10
 db ">>ANOTHER HELP PAGE IS AVAILABLE<< -- Press the ENTER key to see it.",10
 db "Press ESCAPE to exit Help mode, or F1 to see the help menu.",10
 db "Pressing any other key passes that keypress back to your program. "
 db 0

runningontarga		db	10
 db "                FRACTINT Running On TrueVision TARGA Card"
 db 10,0

plasmamessage		db	10
 db 10
 db "I'm sorry, but because of their random-screen-access algorithms, Plasma",10
 db "Clouds, Diffusion and Barnsley IFS fractal images cannot be created using a",10
 db "Disk-based 'Video' mode.",10
 db 10
 db "Also, Plasma Clouds can currently only be run in a 4-or-more-color video",10
 db "mode (and color-cycled only on VGA adapters [or EGA adapters in their",10
 db "640x350x16 mode]).",10
 db 10,10,10
 db "Either press a function key (like F2 thru F5) that selects one of",10
 db "those modes, or press the 't' key to select a new fractal type.",10
 db 10,0

argerrormessage 	db	10
 db "(see the Startup Help screens or FRACTINT.DOC for a complete"
 db 10
 db " argument list with descriptions):"
 db 10
 db 10,0

goodbyemessage		db	10
 db 10
 db 10
 db "Thank You for using FRACTINT"
 db 10
 db 10,0

inversionmessage	db	10
 db 10
 db  " Please enter inversion parameters that apply.  Note",10
 db  "  that the inversion option requires a fixed radius and ",10
 db  "  center for zooming to make sense - if you want to zoom,",10
 db  "  do not use default values, but specify radius and center",10
 db 0

;	IFS fractal of a fern
;	      a     b	  c	d     e     f	  p

initifs dd 0.00, 0.00, 0.00, 0.16, 0.00, 0.00, 0.01
	dd 0.85, 0.04, -.04, 0.85, 0.00, 1.60, 0.85
	dd 0.20, -.26, 0.23, 0.22, 0.00, 1.60, 0.07
	dd -.15, 0.28, 0.26, 0.24, 0.0,  0.44, 0.07
	dd 28*7 dup(0.0)
	dd 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 1.00

;		IFS3D fractal of a fern
;		a     b     c	  d	e     f     g	  h	i     j     k	  l	p
initifs3d dd  0.00, 0.00, 0.00, 0.00, 0.18, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.01
	  dd  0.85, 0.00, 0.00, 0.00, 0.85, 0.10, 0.00,-0.10, 0.85, 0.00, 1.60, 0.00, 0.85
	  dd  0.20,-0.20, 0.00, 0.20, 0.20, 0.00, 0.00, 0.00, 0.30, 0.00, 0.80, 0.00, 0.07
	  dd -0.20, 0.20, 0.00, 0.20, 0.20, 0.00, 0.00, 0.00, 0.30, 0.00, 0.80, 0.00, 0.07
	  dd  28*13 dup(0.0)
	  dd  0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 1.00

newlinestr db 13,10

.code

helpmessage proc uses ds di, message:far ptr byte
		lds	di, message
msgnext:	sub	bx, bx			; init length to zero
msgscan:	mov	al, byte ptr [bx+di]	; is next byte a special?
		or	al, al
		jz	endfragment		; 0, end of msg
		cmp	al, 10
		je	endfragment		; 10, newline
		inc	bx
		cmp	bx, 2050		; max chars
		jl	msgscan
endfragment:	or	bx, bx			; empty fragment?
		jz	chkspecial
		mov	dx, di			; dx points to msg for DOS
		add	di, bx			; di now points past fragment
		mov	cx, bx			; need cx=length for DOS
		mov	ah, 40h 		; DOS write to file
		mov	bx, 1			; stdout
		int	21h
		jc	reterr
chkspecial:	cmp	byte ptr [di], 10	; fragment end with newline?
		je	newline 		; yup, go do it
		xor	ax, ax			; assume end of entire msg,
		jmp	short msgret		; return zero
newline:	inc	di			; skip the newline in source
		push	ds
		mov	cx, 2			; length of newline string
		mov	bx, 1			; stdout
		mov	ax, seg newlinestr	; load ds:dx with addr of string
		mov	ds, ax
		mov	dx, offset newlinestr
		mov	ah, 40h 		; DOS write to file
		int	21h			; dx was already set earlier
		pop	ds			; restore registers
		jmp	msgnext 		; go do next fragment
reterr: 	mov	ax, 1
msgret: 	ret
helpmessage endp

		end




