;
;	HELP literals are in a code segment so that:
;	  1) we don't chew up half of the 64k near data on just these literals
;	  2) we can include the literals with help.c in an overlay


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
			dw	offset	helpmessagexhair3
			dw	seg	helpmessagexhair3
			dw	offset	helpmessagexhair4
			dw	seg	helpmessagexhair4
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
			dw	offset	helpmessagecmdline8
			dw	seg	helpmessagecmdline8
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
			dw	offset	helpmessagefractals13
			dw	seg	helpmessagefractals13
			dw	0,0

helpmessageformoreinfo	dw	offset	helpmessageformoreinfo1
			dw	seg	helpmessageformoreinfo1
			dw	offset	helpmessageformoreinfo2
			dw	seg	helpmessageformoreinfo2
			dw	0,0

helpmessageloadfile	dw	offset	helpmessageloadfile1
			dw	seg	helpmessageloadfile1
			dw	0,0

helpmessageview 	dw	offset	helpmessageview1
			dw	seg	helpmessageview1
			dw	0,0

helpmessagezoom 	dw	offset	helpmessagezoom1
			dw	seg	helpmessagezoom1
			dw	0,0

.code HELP_TEXT ; to combine with help.c for TC++ overlays

	public	helpmessageauthors
	public	helpmessagecredits
	public	helpmessagemenu
	public	helpmessagemenuinstr
	public	helpmessagemain
	public	helpmessagecycling
	public	helpmessagexhair
	public	helpmessagemouse
	public	helpmessagecmdline
	public	helpmessagefractals
	public	helpmessageformoreinfo
	public	helpmessageendtext
	public	helpmessagenexttext
	public	helpmessageprevtext
	public	helpmessagenextprevtext
	public	helpmessagevideo
	public	helpmessageloadfile
	public	helpmessageview
	public	helpmessagezoom

helpmessageauthors label byte
 db " Primary Authors",10
 db " Bert Tyler           CompuServe (CIS) ID: [73477,433]   BIX ID: btyler",10
 db " Timothy Wegner       CIS ID: [71320,675]   Internet: twegner@mwunix.mitre.org",10
 db " Mark Peterson        CIS ID: [70441,3353]",10
 db " Pieter Branderhorst  CIS ID: [72611,2257]",10
 db " Contributing Authors",10
 db 10
 db 10		     ; room for 14 authors at a time here
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
 db " SPACEBAR toggles scrolling off/on",10
 db "   Copyright (C) 1990 The Stone Soup Group.  Fractint may be freely copied",10
 db "   and distributed but may not be sold.  See help for more information.",10
 db 0

helpmessagecredits label byte
 db "                  ...",10
 db " Michael Abrash   360x480x256, 320x400x256 VGA video modes",10
 db " Joseph Albrecht  Tandy video, CGA video speedup",10
 db " Kevin Allen      Finite attractor and bifurcation engine",10
 db " Steve Bennett    restore-from-disk logic",10
 db " Rob Beyer        [71021,2074] Barnsley IFS, Lorenz fractals",10
 db " Mike Burkey      376x564x256, 400x564x256, and 832x612x256 VGA video modes",10
 db " John Bridges     [73307,606] superVGA support, 360x480x256 mode",10
 db " Brian Corbino    [71611,702] Tandy 1000 640x200x16 video mode",10
 db " Lee Crocker      [73407,2030] Fast Newton, Inversion, Decomposition..",10
 db " Monte Davis      [71450,3542] Documentation",10
 db " Chuck Ebbert     [76306,1226] compressed,sqrt log palette",10
 db " Richard Finegold [76701,153] 8/16/../256-Way Decomposition option",10
 db " Mike Gelvin      [73337,520] Mandelbrot speedups",10
 db " Lawrence Gozum   [73437,2372] Tseng 640x400x256 Video Mode",10
 db " David Guenther   [70531,3525] Boundary Tracing algorithm",10
 db " Mike Kaufman     [71610,431] mouse support, other features",10
 db " Adrian Mariano   [theorem@blake.acs.washington.edu] Diffusion & L-Systems",10
 db " Chris Martin     Paintjet printer support",10
 db " Joe McLain       [75066,1257] TARGA Support, color-map files",10
 db " Bob Montgomery   [73357,3140] (Author of VPIC) Fast text I/O routines",10
 db " Bret Mulvey      plasma clouds",10
 db " Ethan Nagel      [70022,2552] Palette editor",10
 db " Marc Reinig      [72410,77] Lots of 3D options",10
 db " Kyle Powell      [76704,12] 8514/A Support",10
 db " Matt Saucier     [72371,3101] Printer Support",10
 db " Herb Savage      [71640,455] 'inside=bof60', 'inside=bof61' options",10
 db " Lee Skinner      Tetrate, Spider, Mandelglass fractal types and more",10
 db " Dean Souleles    [75115,1671] Hercules Support",10
 db " Kurt Sowa        [73467,2013] Color Printer Support",10
 db " Scott Taylor     [72401,410] (DGWM18A) PostScript, Kam Torus, many fn types.",10
 db " Paul Varner      [73237,411] Floating-point fractal algorithms",10
 db " Dave Warker      Integer Mandelbrot Fractals concept",10
 db " Phil Wilson      [76247,3145] Distance Estimator, Bifurcation fractals",10
 db " Richard Wilton   Tweaked VGA Video modes",10
 db "                  ...",10
 db " Byte Magazine    Tweaked VGA Modes",10
 db " MS-Kermit        Keyboard Routines",10
 db " PC Magazine      Sound Routines",10
 db " PC Tech Journal  CPU, FPU Detectors",10
 db 0

helpmessagemenu label byte
 db "Index of available Help Screens",10
 db 10
 db "1   - Commands available at the initial Credits Screen and main command level",10,10
 db "2   - Zoom Box commands",10,10
 db "3   - Commands available in Color-Cycling mode",10,10
 db "4   - Commands available in Palette-Editing mode",10,10
 db "5   - Descriptions of currently available fractal types",10,10
 db "6   - List of Available Video Modes and the keys that select them",10,10
 db "7   - Using FRACTINT with a mouse",10,10
 db "8   - The SSTOOLS.INI file and Command-Line arguments",10,10
 db "9   - Distribution of FRACTINT / Contacting the Authors",10,10
 db 0
helpmessagemenuinstr label byte
 db "  Please press one of the above keys (or press ESCAPE to exit Help Mode)"
 db 0

helpmessagemain1 label byte
 db "Command Keys available while in Display Mode",10
 db 10
 db "* F1                HELP! (Enter help mode and display this screen)",10
 db "* F2,F3,F4,F5...    Select a new Video Mode and THEN Redraw",10
 db "                    (see the Video-modes HELP screens for the full modes list)",10
 db "* t                 Select a new fractal type and parameters",10
 db "* x                 Set a number of options and doodads",10
 db "* y                 Set extended options and doodads",10
 db "  Tab               Display the current fractal image information",10
 db "  s                 Save the current screen image to disk (restart with 'r')",10
 db "* r or 3 or o       Restart from a saved (or .GIF) file ('3' or 'o' for 3-D)",10
 db "  p                 Print the screen (command-line options set printer type)",10
 db "  Esc or m          Go to main menu",10
 db "* Insert            Restart the program (at the credits screen)",10
 db "  Enter             Continue an interrupted calculation (e.g. after a save)",10
 db 10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",10
 db "If the screen finishes before you hit a key, it will beep and wait for you.",10
 db "Commands marked with an '*' are also available at the credits screen.",10
 db 0

helpmessagemain2 label byte
 db "Command Keys available while in Display mode",10
 db 10
 db "  c or + or -       Enter Color-Cycling Mode (see Color-Cycling help)",10
 db "  e                 Enter Palette-Editing Mode (see Palette-Editing help)",10
 db "* f                 toggle the floating-point algorithm option ON or OFF.",10
 db "  b                 Add the current fractal description to FRABATCH.BAT",10
 db "  Spacebar          Mandelbrot/Julia Set toggle.",10
 db "  \                 Redraw Previous screen (you can 'back out' recursively)",10
 db "* d                 Shell to DOS (type 'exit' at the DOS prompt to return)",10
 db "  o                 toggles 'orbits' option on and off during image generation",10
 db "  a                 Convert the current image into a fractal 'starfield'",10
 db "                    (the 'astrologer' option).",10
 db "* i                 Edit the parameters for the Barnsley IFS fractal-types",10
 db "                    or the parameters for 3D fractal types",10
 db "* v                 Set view window parameters (reduction, aspect ratio)",10
 db 10
 db 10
 db "Hit any of these keys while drawing a fractal to immediately do the command.",10
 db "If the screen finishes before you hit a key, it will beep and wait for you.",10
 db "Commands marked with an '*' are also available at the credits screen.",10
 db 0

helpmessagecycling1 label byte
 db "Command Keys available while in Color-Cycling mode",10
 db 10
 db "Color-cycling mode is entered with the 'c', '+', or '-' keys from an image,",10
 db "or with the 'c' key from Palette-Editing mode.",10
 db "When paused in color-cycling mode, the screen has a white border. When not",10
 db "paused, the colors cycle. Commands marked with an '*' are available only on",10
 db "VGA systems; the others also work on EGA systems.",10
 db 10
 db "  F1               HELP! (Enter help mode and display this screen)",10
 db "  Esc              Exit from color-cycling mode",10
 db "  + or -           (re)-set the direction of the color-cycling",10
 db "  Right/Left Arrow (re)-set the direction of the color-cycling (just like +/-)",10
 db "  Up/Down Arrow    SpeedUp/SlowDown the color cycling process",10
 db "  F2 thru F10      Select Short--Medium--Long (randomly-generated) color bands",10
 db "  1  thru 9        Cycle through 'nn' colors between screen updates (default=1)",10
 db "  Enter            Randomly (re)-select all new colors  [TRY THIS ONE!]",10
 db "  Spacebar         Pause until another key is hit (the overscan area is set",10
 db "                   to white as a visual indicator of a Color-Cycling pause)",10
 db 0

helpmessagecycling2 label byte
 db "Command Keys available while in Color-Cycling mode",10
 db 10
 db "Color-cycling mode is entered with the 'c', '+', or '-' keys from an image,",10
 db "or with the 'c' key from Palette-Editing mode.",10
 db "When paused in color-cycling mode, the screen has a white border. When not",10
 db "paused, the colors cycle. Commands marked with an '*' are available only on",10
 db "VGA systems; the others also work on EGA systems.",10
 db 10
 db "* SF1 thru AF10    Pause and re-set the Palette to one of 30 fixed sequences",10
 db "  d or a           pause and load the palette from DEFAULT.MAP or ALTERN.MAP",10
 db "  l                pause, prompt for a palette map filename (default",10
 db "                   filetype is .MAP), and load the palette from that map file",10
 db "  s                pause, prompt for a palette map filename (default",10
 db "                   filetype is .MAP), and save the palette to that map file",10
 db "* r or g or b or   force a pause and Lower (lower case) or Raise (upper case)",10
 db "* R or G or B      the Red, Green, or Blue component of the fractal image",10
 db 0

helpmessagexhair1 label byte
 db "Command Keys available while in Palette-Editing mode",10,10
 db "This mode is entered by using the 'e' key.",10
 db "It is only available on MCGA and VGA systems.",10
 db 10
 db "When this mode is entered, an empty palette frame is displayed. Use the",10
 db "cursor keys to position the frame, Pageup and Pagedown keys to size it,",10
 db "then hit Enter to display the palette and continue.",10
 db 10
 db "Note that the palette frame shows R(ed) G(reen) and B(lue) values for two",10
 db "color registers at the top.  The active color register has a solid frame,",10
 db "the inactive register's frame is dotted.  Within the active register, the",10
 db "active color component is framed.",10
 db 10
 db "Once the palette frame is displayed, the follow commands are available:",10
 db 10
 db "  F1               HELP! (Enter help mode and display this screen)",10
 db "  Esc              Exit back to color-cycling mode",10
 db "  h                Hide the palette frame to see full image; the cross-hair",10
 db "                   remains visible and all functions remain enabled; hit 'h'",10
 db "                   again to restore the palette display",10
 db 0

helpmessagexhair2 label byte
 db "Command Keys available while in Palette-Editing mode",10,10
 db "  Cursor keys      Move the cross-hair cursor around. In 'auto' mode (the",10
 db "                   default) the center of the cross-hair selects the active",10
 db "                   color register. Control-Cursor keys move the cross-hair",10
 db "                   faster. A mouse can also be used to move around.",10
 db "  r or g or b      Select the the Red, Green, or Blue component of the",10
 db "                   active color register for subsequent commands",10
 db "  Insert or Delete Select previous or next color component in active register",10
 db "  + or -           Increase or decrease the active color component by 1",10
 db "                   Numeric keypad (gray) + and - keys do the same.",10
 db "  Pageup or Pagedn Increase or decrease the active color component by 5;",10
 db "                   Moving the mouse up/down with left button held is the same",10
 db "  0 1 2 3 4 5 6    Set active color component to 0 10 20 ... 60",10
 db "  Space            Select the other color register as the active one",10
 db "                   (In 'auto' mode this results in both registers set to the",10
 db "                   color under the cursor till you move it.)",10
 db "  , or .           Rotate the palette one step",10
 db "  < or >           Rotate the palette continuously (until next keystroke)",10
 db "  c                Enter Color-Cycling Mode (see Color-Cycling help)",10
 db 0

helpmessagexhair3 label byte
 db "Command Keys available while in Palette-Editing mode",10,10
 db "  =                Create a smoothly shaded range of colors between the",10
 db "                   two color registers",10
 db "  d                Duplicate the inactive color register in active color",10
 db "  t                Stripe-shade; create a smoothly shaded range of colors",10
 db "                   between the two color registers, setting only every Nth",10
 db "                   register;  after hitting 't', hit a number from 2 to 9",10
 db "                   which is used as N",10
 db "  Shift-F2,F3,..F9 Store the current palette in a temporary save area",10
 db "                   associated with the function key; these save palettes",10
 db "                   are remembered only until you exit Palette-Editing mode",10
 db "  F2,F3,...,F9     Restore the palette from a temporary save area",10
 db "  \                Move or resize the palette frame.  The frame outline is",10
 db "                   drawn; it can then be moved and sized with the cursor",10
 db "                   keys, Pageup, and Pagedown.  Hit Enter when done moving/",10
 db "                   sizing",10
 db "  i                Invert frame colors, useful with dark colors",10
 db 0

helpmessagexhair4 label byte
 db "Command Keys available while in Palette-Editing mode",10,10
 db "  l                prompt for a palette map filename (default file type",10
 db "                   is .MAP), and load the palette from that map file",10
 db "  s                prompt for a palette map filename (default file type",10
 db "                   is .MAP), and save the palette to that map file",10
 db "  a                Toggle 'auto' mode on or off - when on, the active color",10
 db "                   register follows the cursor; when off, Enter must be hit",10
 db "                   to set the register to the color under the cursor",10
 db "  Enter            Only useful when 'auto' is off, as described above; double",10
 db "                   clicking the left mouse button is the same as Enter",10
 db "  x                Toggle 'exclude' mode on or off - when toggled on, only",10
 db "                   the active color is displayed",10
 db "  y                Toggle 'exclude' range on or off - when on, only colors",10
 db "                   in the range of the two color registers are shown",10
 db 0

helpmessagemouse1 label byte
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

helpmessagecmdline1 label byte
 db "Command Line Parameters, General",10,10
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
 db "type=fractaltype           Perform this Fractal Type (Default = mandel)",10
 db "                           See the fractaltypes HELP screen for a full list",10
 db "params=xxx[/xxx[/xxx]]...  Begin with these extra Parameter values",10
 db "                           (Examples:  params=4   params=-0.480/0.626)",10
 db "corners=xmin/xmax/ymin/ymax[/x3rd/y3rd]  Begin with these X, Y Coordinates",10
 db "                           (Example: corners=-0.739/-0.736/0.288/0.291)",10
 db "center-mag=[Xctr/Yctr/Mag] Allows an alternative method of entering corners.",10
 db "                           With no parameters causes 'b' command to output ",10
 db "                           'center-mag=' instead of corners.",10
 db 0

helpmessagecmdline2 label byte
 db "Command Line Parameters, General",10,10
 db "passes=1|2|g|b             Select Single-Pass, Dual-Pass, Solid-Guessing",10
 db "                           or the Boundary-Tracing drawing algorithms",10
 db "float=yes                  For most functions changes from integer math to fp",10
 db "maxiter=nnn                Maximum number of iterations (default = 150)",10
 db "inside=nnn                 Fractal interior color (inside=0 for black)",10
 db "outside=nnn                Fractal exterior color (forces two-color images)",10
 db "bailout=nnnn               Use this as the iteration bailout value (rather than",10
 db "                           the default value of [for most fractal types] 4.0)",10
 db "initorbit=nnn/nnn          Sets the value used to initialize Mandelbrot orbits",10
 db "                           to the given complex number (real and imag parts).",10
 db "initorbit=pixel            Sets the value used to initialize Mandelbrot orbits",10
 db "                           to the complex number corresponding to the screen ",10
 db "                           pixel. This is the default for most types.",10
 db "potential=nn[/nn[/nn[/16bit]]]  Continuous Potential",10
 db "logmap=yes|old|nn          Yes maps logarithm of iteration to color. Old uses",10
 db "                           pre vsn 14 logic. >1 compresses, <-1 for quadratic.",10
 db "distest=nnn                Turns on Distance Estimator Method,",10
 db "decomp=nn                  'Decomposition' toggle, value 2 to 256.",10
 db "invert=radius/xcenter/ycenter  Turns on Inversion",10
 db 0

helpmessagecmdline3 label byte
 db "Command Line Parameters, General",10,10
 db "biomorph=nnn               Turns on Biomorph Coloring (try with the mansinzexpd",10
 db "                           and Julsinzexpd fractal types)",10
 db "finattract=yes             Look for finite attractor in julia types",10
 db "function=fn1/fn2/fn3/fn4   Allows specification of transcendental functions ",10
 db "                           with types using variable functions. Possible values",10
 db "                           are sin, cos, sinh, cosh, exp, log, and sqr.",10
 db "formulafile=filename       File for type=formula, default FRACTINT.FRM",10
 db "formulaname=formulaname    Have the 'type=formula' fractals use this",10
 db "                           formula (instead of the first one in the file).",10
 db "ifs=filename               Define an IFS map for the Barnsley IFS fractals",10
 db "ifs3d=filename             Define 3D IFS map for the Barnsley IFS fractals",10
 db "lfile=filename             File for type=lsystem, default FRACTINT.L",10
 db "lname=lsystem-name         Have the 'type=lsystem' fractals use this",10
 db "                           fractal (instead of the first one in the file).",10
 db "periodicity=[no|show|nnn]  Controls periodicity checking. 'no' turns checking",10
 db "                           off; entering a number nnn controls the tightness",10
 db "                           of checking (1 is default, higher is more stringent)",10
 db "                           'show' or a neg value colors 'caught' points white.",10
 db 0

helpmessagecmdline4 label byte
 db "Command Line Parameters, General",10,10
 db "sound=off|x|y|z            Nobody ever plays with fractals at work, do they?",10
 db "                           x|y|z can be used with attractor fractals",10
 db "hertz=nnn                  Base frequency for attractor sound effects",10
 db "savename=filename          Save files using this name (instead of FRACT001)",10
 db "overwrite=no|yes           Don't over-write existing files",10
 db "gif87a=yes                 Save GIF files in the older GIF87a format (with",10
 db "                           no FRACTINT extension blocks)",10
 db "batch=yes                  Batch mode run (display image, save-to-disk, exit)",10
 db "savetime=nnn               Autosave image every nnn minutes of calculation",10
 db "map=filename               (VGA or TARGA) get default color map from 'filename'",10
 db "rseed=nnnnn                Forces reproducable Plasma Clouds.  The 'rseed='",10
 db "                           value is listed as part of the <TAB> display",10
 db "cyclelimit=nnn             color-cycler speed-limit (1 to 256, default = 55)",10
 db "batch=config               Do a batch mode run to generate 'fractint.cfg' file",10
 db "symmetry=xxxx              Force symmetry to None, Xaxis, Yaxis, XYaxis,",10
 db "                           Origin, or Pi symmetry.  Useful for debugging.",10
 db ";                          indicates the rest of the line is a comment",10
 db "                           (e.g. 'fractint type=plasma ; use plasma clouds')",10
 db 0

helpmessagecmdline5 label byte
 db "Command Line Parameters, Video",10,10
 db "video=xxx                  Begin with this video mode (Example: Video=F2)",10
 db "                           See the video-modes HELP screen for a full list",10
 db "askvideo=no                Skip the prompt for video mode when restoring",10
 db "                           files when possible (but your adapter had better",10
 db "                           support the required mode...)",10
 db "adapter=cga|ega|mcga|vga   Skip the autodetect logic, assume this kind of",10
 db "                           adapter is present",10
 db "textsafe=no                If return from menu, prompt, or help to an image",10
 db "                           garbles the image, try specifying this",10
 db "textsafe=yes               To get rid of the irritating flicker on EGA/VGA",10
 db "                           during FRACTINT startup, try specifying this.",10
 db "                           Remove it if you find that images are scrambled",10
 db "                           when returning to them from the menu",10
 db "exitmode=nn                Sets the bios-supported videomode to use upon exit",10
 db "                           (if not mode 3) - nn is the mode in hexadecimal",10
 db "textcolors=aa/bb/cc/...    Set text screen colors.",10
 db "textcolors=mono            Set text screen colors to simple black and white",10
 db 0

helpmessagecmdline6 label byte
 db "Command Line Parameters, Printer",10,10
 db "printer=type[/res[/lpt#]]  Set the printer type, dots/inch, and port#",10
 db "                           types: IBM, EPSON, CO (Star Micronix),",10
 db "                                  HP (LaserJet), PA (Paintjet),",10
 db "                                  PSH (PostScript portrait), PSL (landscape)",10
 db "                           port# 1-3 for LPTn, 11-14 for COMn",10
 db "                           Optional 4th parameter /-1 prints to file instead",10
 db "                           of port; default first filename is fract001.prn",10
 db "printfile=filename         Print to file, specified filename",10
 db "epsf=1|2|3|...             Forces print to file; default filename fract001.eps;",10
 db "                           forces PostScript mode; lower numbers force",10
 db "                           stricter adherence to EPS",10
 db "title=yes                  Print a title with the output",10
 db "translate=yes|nnn          PostScript only; yes prints negative image;",10
 db "                           >0 reduces image colors; <0 color reduce+negative",10
 db "halftone=freq/angle/style  PostScript only; defines halftone screen;",10
 db "comport=port/baud/options  COM port initialization. Port=1,2,3,etc.",10
 db "                           baud=115,150,300,600,1200,2400,4800,9600",10
 db "                           options 7,8 | 1,2 | e,n,o (any order)",10
 db "                           Example: comport=1/9600/n71",10
 db 0

helpmessagecmdline7 label byte
 db "Command Line Parameters, 3D",10,10
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

helpmessagecmdline8 label byte
 db "Command Line Parameters, 3D",10,10
 db "coarse=nnn                 Sets Preview 'coarseness' default value",10
 db "stereo=nnn                 Sets Stereo (R/B 3D) option:  0 = none,",10
 db "                           1 = alternate, 2 = superimpose, 3 = photo",10
 db "interocular=nnn            Sets 3D Interocular distance default value",10
 db "converge=nnn               Sets 3D Convergence default value",10
 db "crop=nnn/nnn/nnn/nnn       Sets 3D red-left, red-right, blue-left,",10
 db "                           and blue-right cropping default valuess",10
 db "bright=nnn/nnn             Sets 3D red and blue brightness defaults,",10
 db "xyadjust=nnn/nnn           Sets 3D X and Y adjustment defaults,",10
 db "randomize=nnn              smoothes 3d color transitions between elevations",10
 db "fullcolor=yes              allows creation of full color .TGA image with",10
 db "                           light source fill types. See the Docs.",10
 db "ambient=nnn                sets depth of shadows and contrast when using",10
 db "                           light source fill types",10
 db "haze=nnn                   sets amount of haze for distant objct if fullcolor=1",10
 db "lightname=filename         <full path optional> file name which will be used to",10
 db "                           save the full color files with full color option.",10
 db 0

helpmessagefractals1 label byte
 db "Fractal Types",10,10
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

helpmessagefractals2 label byte
 db "Fractal Types",10,10
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
 db "             else = z(n+1) = (z+1)*C. Two params: real & imaginary parts of C",10
 db "barnsleym3 = Another Barnsley 'Mandelbrot', 'Julia' pair.",10
 db "barnsleyj3 ",10
 db 0

helpmessagefractals3 label byte
 db "Fractal Types",10,10
 db "Barnsley types (continued)",10
 db "sierpinski = Sierpinski gasket - a Julia set that produces a 'Swiss cheese",10
 db "             triangle'. z(n+1) = (2*x,2*y -1) if y > .5; else (2*x-1,2*y)",10
 db "             if (x > .5); else (2*x,2*y). No parameters.",10
 db "ifs        = Barnsley IFS Fractal (a fern unless an alternate IFS map has ",10
 db "             been defined using the 'ifs=' command-line option).",10
 db "ifs3d      = Barnsley 3D IFS Fractal (a fern unless an alternate IFS map has ",10
 db "             been defined using the 'ifs3d=' command-line option).",10
 db 0


helpmessagefractals4 label byte
 db "Fractal Types",10,10
 db "Mark Peterson's Types",10
 db 10
 db "marksmandel= A variant of the mandel-lambda fractal.  z(0) = 0;",10
 db "             z(n+1) = ((Xcoord+i*Ycoord)^exp)*z(n) + (Xcoord+i*Ycoord).",10
 db "             Parameters are pertubations of z(0).",10
 db "marksjulia = A variant of the julia-lambda fractal. ",10
 db "             z(0) = Xcoord + i * Ycoord;  z(n+1) = (z(0)^exp)*z(n) + z(0).",10
 db "cmplxmarksjul, = The above two types generalized with 'exp' a complex rather",10
 db "cmplxmarksmand  than a real number",10
 db "julibrot   = 'Julibrot' 4-dimensional fractals.",10
 db "unity      = Mark Peterson's 'Unity' fractal type.  Truly Wierd - ",10
 db "formula    = Formula interpreter - write your own formulas as text files!",10
 db 0

helpmessagefractals5 label byte
 db "Fractal Types",10,10
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

helpmessagefractals6 label byte
 db "Fractal Types",10,10
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

helpmessagefractals7 label byte
 db "Fractal Types",10,10
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

helpmessagefractals8 label byte
 db "Fractal Types",10,10
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

helpmessagefractals9 label byte
 db "Fractal Types",10,10
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

helpmessagefractals10 label byte
 db "Fractal Types",10,10
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

helpmessagefractals11 label byte
 db "Fractal Types",10,10
 db "kamtorus  =  series of orbits superimposed. x(0) = y(0) = orbit/3; ",10
 db "             x(n+1) = x(n)*cos(a) + (x(n)*x(n)-y(n))*sin(a) ",10
 db "             y(n+1) = x(n)*sin(a) - (x(n)*x(n)-y(n))*cos(a) ",10
 db "             After each orbit, 'orbit' is incremented by a step size. Parmeters",10
 db "             are a, step size, stop value for 'orbit', and points per orbit.",10
 db "kamtorus3d=  Same as kamtorus but in 3D with 'orbit' the z dimension.",10
 db 0

helpmessagefractals12 label byte
 db "Fractal Types",10,10
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

helpmessagefractals13 label byte
 db "Fractal Types",10,10
 db "lsystem   =  Using a turtle-graphics control language and starting with an",10
 db "             initial string, the axiom, carry out string substitutions",10
 db "             the specified number of times.  (This is the order).",10
 db "                F   Draw forward         D   Draw forward",10
 db "                G   Move forward         M   Move forward",10
 db "                +   Increase angle       \nn Increase angle nn degrees",10
 db "                -   Decrease angle       /nn Decrease angle nn degrees",10
 db "                |   Reverse              !   Reverse angles",10
 db "                Cnn Select color nn      []  Push and pop",10
 db "                <nn Increase color nn    @nn Change scale by nn",10
 db "                >nn Decrease color nn        I inverse  Q square root",10
 db 0

helpmessageformoreinfo1 label byte
 db "Distribution, Contacting the Authors",10,10
 db "The Stone Soup Group is a loosely associated group of fanatic programmers.",10
 db "See FRACTINT.DOC for the story behind the 'Stone Soup' name.",10
 db 10
 db "FRACTINT is freeware.",10
 db "Contribution policy: Don't want money. Got money. Want admiration.",10
 db "Conditions on use:",10
 db " FRACTINT may be freely copied and distributed but may not be sold.",10
 db " It may be used personally or in a business - if you can do your job better",10
 db " by using FRACTINT, or use images from it, that's great!",10
 db " It may be given away with commercial products under the following conditions:",10
 db "   It must be clearly stated that Fractint does not belong to the vendor",10
 db "   and is included as a free give-away.",10
 db "   It must be a complete and unmodified release of FRACTINT.",10
 db " There is no warranty of FRACTINT's suitability for any purpose, nor any",10
 db " acceptance of liability, express or implied.",10
 db 10
 db "Source code for FRACTINT is also freely available. See the FRACTSRC.DOC file",10
 db "included with it for conditions on use. (In most cases we just want credit.)",10
 db 0

helpmessageformoreinfo2 label byte
 db "Distribution, Contacting the Authors",10,10
 db "Virtually all of the FRACTINT authors can be found on the CompuServe",10
 db "network in the COMART ('COMputer ART') forum in Section 15 ('Fractals').",10
 db "FRACTINT development occurs in the COMART forum - most of the authors have",10
 db "never met except there.  New members are always welcome!",10
 db "Several of us can also be found on BIX in the GRAPHIC.DISP/FRACTALS area.",10
 db 10
 db "New versions of FRACTINT are uploaded to the CompuServe and BIX networks,",10
 db "and make their way to other systems from those points.",10
 db 10
 db "FRACTINT is available as two self-extracting archive files:",10
 db "  FRAINT.EXE (executable & documentation) and FRASRC.EXE (source code)",10
 db 10
 db "The latest version can usually be found in the following locations:",10
 db "  CompuServe:  COMART library 15 (both files)",10
 db "               IBMNEW library 5  (executable and documentation only)",10
 db "  BIX:         GRAPHIC.DISP/LISTINGS and IBM.PC/LISTINGS",10
 db 10
 db "(What's the latest version? Well, THIS one was, way back when we uploaded it!)",10
 db 0

helpmessageloadfile1 label byte
 db "Selecting a video mode when loading a file",10
 db 10
 db "The most suitable video modes for the file are listed first.",10
 db 10
 db "The 'err' column in the video mode information indicates:",10
 db "  blank  mode seems perfect for this image",10
 db "  v      image smaller than screen, will be loaded in a <v>iew window",10
 db "  c      mode has more colors than image needs",10
 db "  *      a major problem, one or more of the following is also shown:",10
 db "   C     mode has too few colors",10
 db "   R     image larger than screen, Fractint will reduce the image, possibly",10
 db "         into a <v>iew window, and maybe with aspect ratio a bit wrong",10
 db "   A     mode has the wrong shape of pixels for this image",10
 db 0

helpmessageview1 label byte
 db "View Parameters (Reduction and Aspect Ratio)",10,10
 db "Preview Display:    Set to yes to display in preview mode - the image is",10
 db "      reduced and/or squeezed as required by the other view parameters.",10
 db "Reduction factor:   Sizes the window, useful for fast previews.",10
 db "Final aspect ratio: The y/x value of the overall dimensions of the intended",10
 db "      final output.  This value has some effects even when preview display",10
 db "      is turned off - see example below.",10
 db "Crop coordinates:   Controls whether the prior image is squeezed/stretched",10
 db "      when changing aspectratio, vs being cropped to preserve aspect ratio",10
 db "      This also affects initial image when selecting a new fractal type.",10
 db "Explicit size:      Should only be used when production of a GIF with",10
 db "      particular non-standard dimensions is desired.",10
 db 10
 db "E.g. of complex use:  you might want a 4 x 8 high-res PostScript printout.",10
 db "(Or some other media if you have a way to get a GIF file displayed on it.)",10
 db "Set aspect ratio 2, crop yes, preview yes, reduction 3, then zoom around and",10
 db "pick an image.  When you've picked it, change preview to no and re-generate in",10
 db "a high-res mode.  Then print - the aspect ratio will be correct.  The printer",10
 db "size parameters will need fiddling to scale it.",10
 db 0

helpmessagezoom1 label byte
 db "Zoom Box commands",10,10
 db "Zoom Box functions can be invoked while an image is being generated or when",10
 db "it has been completely drawn.  Zooming is supported for most fractal types,",10
 db "but not all.  Also see the 'Using FRACTINT with a mouse' help information.",10
 db 10
 db "  PageUp            When no Zoom Box is active, bring one up",10
 db "                    When active already, shrink it",10
 db "  PageDown          Expand the Zoom Box",10
 db "                    Expanding past the screen size cancels the Zoom Box",10
 db "  Cursor Keys       Pan (Move) the Zoom Box",10
 db "  Ctrl-Cursor Keys  Fast-Pan the Zoom Box (may require an enhanced keyboard)",10
 db "  Enter             Redraw the Screen or area inside the Zoom Box",10
 db "  Ctrl- Enter       'Zoom-out' - expands the image so that your current",10
 db "                    image is positioned inside the current zoom-box location.",10
 db "  Ctrl- Pad+/Pad-   Rotate the Zoom Box",10
 db "  Ctrl- PgUp/PgDn   Change Zoom Box vertical size (change its aspect ratio)",10
 db "  Ctrl- Home/End    Change Zoom Box shape",10
 db "  Ctrl- Ins/Del     Change Zoom Box color",10
 db 0

helpmessageendtext label byte
 db "   Press ESCAPE to exit Help mode, or F1 for help menu.",0
helpmessagenexttext label byte
 db "   More information is available, press PageDown for next page.",0
helpmessageprevtext label byte
 db "   Press PageUp for previous page.",0
helpmessagenextprevtext label byte
 db "   Press PageDown for next page, PageUp for previous page.",0

helpmessagevideo label byte
 db "   (Notation:  F1, SF1, CF1, AF1 == Normal-, Shift-, Control- or Alt-F1) ",10
 db "    'B' after #-of-colors means video access is via the BIOS (s-l-o-w-l-y)",10
 db 0

	end

