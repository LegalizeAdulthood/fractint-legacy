
       ***   FRACT386 (Version 3.1)   ******   by Bert Tyler   ***


This program generates Mandelbrot and Julia set fractal images using 32-bit
integer arithmetic specific to the 386 microprocessor.   It will politely
refuse to execute if it finds itself on any earlier Intel processors such
as the 8086, 80186 or 80286.   The program will work with CGA, EGA, MCGA,
VGA and many popular super-rez adapters, and can switch display modes
on-the-fly for those adapters capable of multiple emulation.  For instance,
I can run this program in any of nine IBM CGA/EGA/MCGA/VGA modes on my
PS/2 model 80.   The program does not perform any floating point arithmetic
in its image generation routines, and does not require an FPU.

To start the program, simply type its name (FRACT386) without any parameters.
When the program fires up, it displays a startup screen explaining how it
works, and then waits for you to hit a Function Key (F1, F2, etc) to select
an initial video mode (see the table below for a partial list of supported
adapters and video modes).   You can also hit the ENTER (Return) key to see
a complete list of the video adapters and video modes currently supported
by the program and the function keys you can hit to activate them.   As soon
as you select a video mode, the program begins drawing an initial display
of the full Mandelbrot set.   From this point on, and AT ANY TIME, you can
hit any of the following keys to select a function:

 Key          Function
 ===========  ===============================================================

 PageUp         Display and Shrink the Zoom Box ("Zoom In")
 PageDown       Display and Expand the Zoom Box ("Zoom Out")
 Cursor Keys    Display and Move the Zoom Box Left, Right, Up, or Down ("Pan")
 Ctrl-Cursor-Keys   Pan like the normal Cursor-Keys, but (five times) faster
                (NOTE:  Fast-Panning works only with an Enhanced Keyboard BIOS)
 End or Enter   Redraw the area inside the Zoom Box as a full-screen image
                (If there is no Zoom Box, just re-draw the current screen)
 F1,F2,F3,F4... Select a new Video Mode and THEN perform a Screen Redraw
                Currently supported video modes include:
                F1 = 320x200 16-color EGA/VGA   F2 = 640x350 16-color EGA/VGA
                F3 = 320x200 256-color MCGA/VGA F4 = 640x480 16-color VGA
                F5 = 320x200 4-color CGA        F6 = 640x200 B&W CGA
                F7 = 640x350 B&W EGA/VGA        F8 = 640x480 B&W VGA
                F9 = 640x200 16-color EGA/VGA   ..etc...etc..
                (For a complete list, hit the ENTER key on the Startup Screen)
 Home           Redraw the Previous Screen (the program tracks 100 screens)
                (this gives you the ability to "back out" screen-by-screen)
 Tab            Display the Current Screen or Zoom-Box coordinates
                (this gives you the ability to track where you are and
                re-start the program at your current position later on)
 Spacebar       Toggles between Mandelbrot images and their corresponding
                Julia set images (read the Julia set notes below before
                trying this option if you want to see anything interesting)
 Insert         Restart the Program all over again at the Startup Screen
                (in case you forgot the instructions or need to see the
                Video Mode list again)
 Delete or Esc  Stop the Program and Return to MSDOS

Remember, you do NOT have to wait for the program to finish generating the
full screen display (a process that can take from 30 seconds in "Low-Rez" EGA
or MCGA mode to several minutes in full VGA mode) before hitting one of the
above keys.   If you hit a keyboard key while the program is generating a
screen image, it will simply stop and process the key  (it will NOT finish
the display, though). If, say, you see an interesting spot you want to zoom
in on, don't wait --  do it!   If the program finishes a display before you
hit any keys, it will simply beep and wait for you to hit one.

Hints:  The most interesting areas are the border areas where the colors are
changing rapidly.   Zoom in on them for the best results.   The areas closer
to the outside of the fractal "egg" tend to involve fewer iterations and
display more quickly than those closer to the inside.  The solid blue
interior is the slowest region of all to display -- in fact, it's where
the program has hit its iteration maximum (150) and given up.

Another hint:  The time it takes to generate a fractal image is directly
proportional to the resolution of the selected video mode (more dots = more
calculations).   I always select a low-resolution mode (I use F3 for MCGA)
on the startup screen, and at some later point hit a function key instead
of the END key to switch to a higher resolution mode (I use F4 for VGA)
when things get interesting.   That gets me into the interesting stuff in
detailed resolution quickly -- I've seen that first screen too often to
get excited about it.   Actually, with its 256 colors (the program is
"only" using the first 150), MCGA mode can give you some pretty spectacular
effects on its own.


       ***** Toggling between Mandelbrot and Julia Sets *****

   With Version 3.0, FRACT386 can be toggled between Mandelbrot images and
their corresponding Julia sets.   What is a Julia set?   I don't really know
either, but I can describe how the program generates them.  Let's start with
a diversionary tactic and describe how the program generates the Mandelbrot
set.   The Mandelbrot set is generated by assuming that each pixel on the
screen represents a point on the complex-number "C plane", (X + i*Y), and
calculating the following function until the "size" of Z(n) is greater
than 2 :

Start with a complex-number constant            C = xcoord + i * ycoord
use as the initial value of Z                   Z(0) = 0
and iterate using this function                 Z(n+1) = Z(n)**2 + C

   Julia sets use a slightly different tactic, picking out a specific point
on the "C plane" and assuming that each pixel on the screen represents
an initial point on the "Z plane".   Once everything has been started up,
however, the calculations are the same:

Start with a USER-SPECIFIED value of C          C = Creal + i * Cimaginary
use as the initial value of Z                   Z(0) = xcoord + i * ycoord
and iterate using this function                 Z(n+1) = Z(n)**2 + C


   In either case, the pixel's color is arbitrarily determined by the
number of iterations it took the program to get Z "large" enough to
bail out of the loop.


   Generating Julia sets are different from generating Mandelbrot sets in
several important ways:

   (1) There is only one Mandelbrot set but, given that there are an infinite
number of values available for the complex-number C, there are an infinite
number of Julia sets.

   (2) Although there are an infinite number of Julia sets, a lot of them
are pretty boring.   Only certain ranges of C result in interesting
Julia set displays - values too "small" generate a simple circular display,
and values that are too "big" generate something that looks like scattered
dust.

   (3) It turns out, however, that the coordinates of the most interesting
portions of the Mandelbrot image, where the colors are changing rapidly,
are the VERY SAME values that generate the most interesting Julia sets.
(There is a very sound mathematical reason for this.   I haven't the 
vaguest idea what it is, though.)


   What FRACT386 does is begin with the full Mandelbrot set, give you the 
capability to zoom and pan around generating interesting Mandelbrot images,
and then AT ANY POINT hit the spacebar toggle to "flip" to a full Julia set
with startup constant C set to the coordinates at the center of the Mandelbrot
image that you last generated.   From that point, you are zooming and
panning around in a Julia set "Z plane" (you can always hit the spacebar
toggle again to get your Mandelbrot set back).   You can think of it this way:
all those fantastic Mandelbrot images you have been generating are just
a way to select an initial value for Julia sets you can play with!
Holy infinity, Batman!


  ******   Optional Parameters (re-generating your favorite images)  ******


   FRACT386 accepts optional parameters which tell it to fire up at something
other than the full Mandelbrot set.   The full FRACT386 command line is
actually:

         FRACT386 [Creal Cimaginary] [ Xmin Xmax Ymin Ymax ]

   The first two parameters, if given, are floating point numbers that
tell the program to fire up using a Julia set rather than the default
Mandelbrot set, and to use the two values as the real and imaginary
portions of the startup constant C.

   The final four parameters, if given, are floating point numbers that
specify the X and Y coordinates of the corners of the screen that is
initially displayed.

   These parameters are all the same values (in the same order) that display
on the screen when you hit the TAB key after generating a fractal image.
The optional parameters, in combination with the TAB key function, are
designed to give you the capability to quickly re-generate interesting
displays that you might want to see again (or show to somebody else), and
the floating point notation is the one commonly used by other fractal
programs, so you can compare pictures with your buddy's MAC-II. 

   The following examples are from screens that I have played with:

                       (Mandelbrot sets)

         FRACT386 -0.7389  -0.7364  0.2883   0.2907     
         FRACT386 -0.7276  -0.7270  0.36204  0.36263    
         FRACT386 -0.727322554 -0.727313176 0.362338984 0.362347448

                       (Julia sets)

         FRACT386 .385 .382 .1161 .1490 .5034 .5344
         FRACT386 -.523 .688 -.08 .08 -.08 .08
         FRACT386 -.480 .626 .258 .598 -.501 -.167
         FRACT386 -.480 .626                    (generates a full Julia set)


          *** Support for Third-Party Hi-Rez Video Adapters ***

   FRACT386 uses a Video Adapter Table in the "C" program to reference
everything it needs to know about any particular adapter/mode combination.
This table can contain information for up to fifty adapter/mode combinations,
and is automatically tied to fifty Function Keys (F1-F10, their Control/
Shift/Alt variants, and Alt-0 thru Alt-9) when the program is running.
The table entries, and the function keys they are tied to, are displayed
on your screen if you hit the ENTER key at FRACT386's startup screen.
This table makes adding support for various third-party video cards and
their high-rez modes much easier, at least for the ones that pretend to be
a standard adapter with more dots and/or colors.   

   The assembler object code (FRASM386.OBJ) is included in the ARC files to
make it easier for those of you that have a fancy video adapter and want to
modify FRACT386 to support it.   You will at a minimum need a "C" compiler
that supports "Medium" model object code (Microsoft or Borland will work)
and enough information about your adapter to throw it into one of its unique
modes via INT 10.

   [Chris Green didn't have a "C" compiler handy, so he modified the
program's video adapter table using DEBUG when he was testing the Paradise
adapter entries -- but that's an act of desperation.   It worked, though.
And I don't run the .EXE file through EXEPACK anymore, so it's easier now.]

   The table as currently distributed begins with nine standard IBM video
modes that have been exercised successfully with a PS/2 model 80.   These
entries, coupled with the descriptive comments in the table definition
and the knowledge you have about throwing your adapter into its unique modes,
should be all you need to see to be able to add your own entries.

   UNTESTED (note the emphasis) support for a number of popular high-rez
video adapters has been added to the table from an information sheet that
shows up sporadically on USENET.   Because the information on the sheet is
somewhat sparse, I was forced to use the BIOS method for reading/writing dots
for each of these adapters.   Specs for the Everex EVGA board are courtesy
of Tom Moran (tmoran on BIX).   Specs for the Video-7 board are courtesy of
Ira Emus (irae on BIX).   I tried calling up each of the "major" video
board manufacturers (defined as those paying for full-page ads in magazines
like BYTE and PC) several times to get the specs for their boards, but
never got through to any technical support people - unless they were the
ones playing the MUSAK while I was on hold.   And boy, was I on hold.

   If you have a favorite adapter that you would like added to the standard
releases of FRACT386, just give me the table entry that you added to get
it working.   You will be credited in the comments which display with the
initial list of adapters ("Courtesy of John Smith").   If you have a favorite
adapter that you would like added, but don't have the equipment to add it
yourself, just provide me with the following information:

      The name of the Adapter/Video mode combination
      The resolution (pixels across, pixels down, number of colors)
      How you get it into its special mode (hopefully it's a variant
            of BIOS interrupt 10H, with AX/BX/CX/DX = some specific values)
      How the assembler routine should access it in its write-a-dot
            and read-a-dot routines.  Current options are:
                  1) use the BIOS (INT 10H, AH=12/13, AL=color) ((SLOW))
                  2) pretend it's a super-res EGA/VGA
                  3) pretend it's a super-res MCGA
            (If you're not sure, have me use the BIOS or get me a programmer's
            manual.  Note that using the BIOS to read/write dots can make it
            tough to keep up with the keyboard during zoom/pan box operations)

I will add the adapter to my list, with a status of "UNTESTED:  may not work",
and will change the status to "Tested OK by John Smith" on future releases of
FRACT386 after getting confirmation from you or someone else that it works.
Which brings up another point:   If you can confirm that a particular video
adapter/mode works (or that it doesn't), and the program says it is UNTESTED,
please get that information to me.   Thanks.


                  ***** Limitations and Uglies *****

This program uses 32-bit integer math to generate fractals quickly (I do not
have an FPU, and "normal" fractal packages take hours to draw on my machine).
The advantage of this option is speed:  quite simply, this is by far the
fastest fractal package that I have ever seen on a 386-class machine.   The
disadvantage, aside from the fact that it cannot run on 80286-and-below
processors, is accuracy.   To keep as much accuracy as possible, the program
represents numbers like 1.00 as 32-bit integers of the form [1.00 * (2**29)]
(approximately 500,000,000).   This yields over 8 significant digits of
accuracy, and works just great -- until the initial values of the fractal
calculations on consecutive pixels differ only in the ninth decimal place.
At that point, the program does the best it can do -- it switches to its
minimal drawing mode, with consecutive pixels in both directions having
initial values differing by 1 (really 0.000000002) and disables zooming
and panning.   This happens more often than you might think, only because
it's so easy (and fascinating) to zoom in on a tiny piece of the previous
screen -- and you can force this situation with your seventh consecutive
"maximum zoom", each of which zooms in on about 1% of the previous screen.
If it's any consolation, remember that this situation occurs when you are
attempting to draw an area over your full screen that is approximately
1/(10**13)th [~0.0000000000001] of the area of the full Mandelbrot set ***
-- and you can always hit the "Home" key to get the previous screen(s) back.

***  Or, you can think of it this way:   First, draw the full Mandelbrot set
     in full VGA mode.   Then zoom in on an area represented by a SINGLE
     PIXEL (which you can't do with the current program) and re-draw it as
     a full-screen image.   Then zoom in on an area represented by a single
     pixel of THAT screen and re-draw IT as a full-screen image.   Your
     screen is now displaying an area representing ((1/(640*480))**2)th
     [~0.00000000001] of the area of the area of the full Mandelbrot set -
     not yet in minimal drawing mode.   Try it a THIRD time, though, and
     you'll reach it - but not if you can contain yourself and zoom in on
     an area no smaller than 1/100th of the third screen.

Also, this being a public domain program and being true to that spirit, the
program makes no attempt to verify that your video adapter can run in the
mode you specify, or even that it's really there, before writing to it.
It also assumes that every EGA adapter has a full 256K of memory (and can
therefore display sixteen simultaneous colors in 640x350 resolution), but
does nothing in particular to verify that fact before throwing pixels at it.


                           *** Background ***

   This program started out as a modification of the Mandelbrot set program
distributed (on BIX) by Doug Klein as DKMANDEL.ARC.   I have also downloaded
and read the excellent description on Fractal calculations that is part of
Tron Hvaring's FRACT12.ARC program (also on BIX).   I kept adding this and
replacing that to Doug's code, until one day I looked around and couldn't
find any of his code lying around anymore, so I guess it's now an original
program...

   I have a PS/2 model 80 without an FPU, and all the Mandelbrot programs
I could find on BIX ran slow as molasses using FPU emulation, so I
modified Doug's code to use 386-specific arithmetic along with a few other
tricks to speed thing up.   If I say so myself, I think this version sets
new speed records.   Clock the basic mandelbrot set (map 26, 150 iterations -
that's the first screen the program draws) in MCGA mode on a 16MHZ PS/2
model 80 without an FPU in 26 seconds, or in full VGA mode in 125 seconds!

   I would like to be able to claim that re-writing what is traditionally
Floating Point arithmetic as scaled 32-bit integer arithmetic was my idea,
but the truth is that I first saw it done by Dave Warker of HDS about a 
year ago when he wrote a Mandelbrot generator for a TI34010-based video
display that HDS was going to throw into a graphics terminal.   When I
got the idea to do the same thing on a PS/2 model 80, I first called Dave to
find out how he accomplished it.   I may not be very creative, but I know
good code when I steal--uh--see it.

   (No, Dave doesn't mind -- in fact, he was one of the guys responsible
for getting me the Scientific American articles on Julia sets and then egging
me on to adding them to FRACT386.   The TAB key display was also added
at his request so that, in his words, "someone could compare screens with,
say, something on his MAC II".   Dave uses a MAC-II as his "home" computer.)

   After I got it running real fast and started showing it around to
everybody (see that!  that's mine!  WOW, that's fast!) I discovered that
nobody was particularly interested in playing with it, primarily because
the original user interface was less than spectacular (re-read the examples
that use four or more floating-point numbers to start FRACT386 on a particular
piece of the fractal plane -- then picture that as the ONLY way to select an
area to draw).   That's when I added the zoom-and-pan interface. It has passed
a pretty robust ease-of-use test -- my seven year old likes to play with it.
She wants me to modify it to work with a mouse, though (sigh).


   This code was compiled using Microsoft C (version 5.1) and Microsoft
Assembler (also version 5.1).   Note that the assembler code uses the
"C" model option added to version 5.1 (just because I thought it was neat),
and must be assembled with the /MX switch to link with the "C" code.
The "C" code is pretty simple (and simple-minded).   The assembler code,
however, is tweaked to the hilt for performance reasons.



             *** Distribution and Contribution Policies ***

   This is public domain software.   There is no warranty or acceptance
of liability either expressed or implied with it.   Use it, modify it,
distribute it as you wish.   Your uploading it to other bulletin boards and
the like is specifically encouraged.

Contribution policy:    Don't want money.   Got money.   Want admiration.


            Bert Tyler                    btyler on BIX
            Tyler Software                [73477,433] on Compuserve
            124 Wooded Lane
            Villanova, Pa  19085
            (215) 525-6355
