comment {

The following fractals, were written by Adrian Mariano based on The Fractal 
Geometry of Nature by Mandlebrot. 

}


Koch1 {
  Angle 6
  Axiom F--F--F
  F=F+F--F+F
}

Koch2 {
  Angle 12
  Axiom F---F---F---F
  F=-F+++F---F+
}

Koch3 {
  Angle 4
  Axiom F-F-F-F
  F=F-F+F+FF-F-F+F
}

Dragon {
  Angle 8
  Axiom FX
  F=
  y=+FX--FY+
  x=-FX++FY-
}

Peano1 {
  Angle 4
  Axiom F-F-F-F
  F=F-F+F+F+F-F-F-F+F
}

Cesaro {
  Angle 34
  Axiom FX
  F=
  X=----F!X!++++++++F!X!----
}


DoubleCesaro {
  Angle 4
  axiom D\90D\90D\90D\90
  D=\42!D!/84!D!\42
}

FlowSnake {
  angle=6;
  axiom FL
  L=FL-FR--FR+FL++FLFL+FR-",
  R=+FL-FRFR--FR-FL++FL+FR",
  F=
}

CantorDust {
  Angle 6
  Axiom F
  F=FGF
  G=GGG
}

Snowflake2 {
  angle 12
  axiom F
  F=++!F!F--F--F@IQ3|+F!F--
  F=F--F!+++@Q3F@QI3|+F!F@Q3|+F!F
}

SnowflakeColor {
  angle 12
  axiom F
  F=--!F<1!F<1++F<1++F<1@IQ3|-F<1!F<1++
  F=F<1++F<1!---@Q3F<1@QI3|-F<1!F<1@Q3|-F<1!F<1
  <=
}

Island1 {
  angle 4 
  axiom F+F+F+F
  F=FFFF-F+F+F-F[-GFF+F+FF+F]FF
  G=@8G@I8
}


Island2 {
  angle 4
  axiom f+f+f+f
  f=f+gf-ff-f-ff+g+ff-gf+ff+f+ff-g-fff
  g=@6G@I6
}      


Quartet { 
    angle 4
    axiom fb
    A=FBFA+HFA+FB-FA
    B=FB+FA-FB-JFBFA
    F=
    H=-
    J=+
}


SnowFlake1 {
  Angle 12
  Axiom FR
  R=++!FRFU++FU++FU!---@Q3FU|-@IQ3!FRFU!
  U=!FRFU!|+@Q3FR@IQ3+++!FR--FR--FRFU!--
  F=
}


SnowFlake3 {
  angle 12
  axiom fx
  x=++f!x!fy--fx--fy|+@iq3fyf!x!++f!y!++f!y!fx@q3+++f!y!fx
  y=fyf!x!+++@iq3fyf!x!++f!x!++f!y!fx@q3|+fx--fy--fxf!y!++
  f=
}


Tree1 {
  angle=12;
  axiom +++FX
  X=@.6[-FX]+FX
}

 
Peano2 {
  Angle 8
  Axiom FXY++F++FXY++F
  X=XY@Q2-F@IQ2-FXY++F++FXY
  Y=-@Q2F-@IQ2FXY
}


Sierpinski1 {
angle 3
axiom F
F=FXF
X=+FXF-FXF-FXF+
}



Koch4 {
angle 12
axiom f++++f++++f
f=+f--f++f-
}


comment {

These come from The Science of Fractal Images.  I got them 
from Ken Philip.

               -- Adrian
}


Plant7 {
;; !Bush p. 285b TSOFI
axiom Z
z=zFX[+Z][-Z]
x=x[-FFF][+FFF]FX
angle 14
}

Plant8 {
;;  !Bush p, 286 TSOFI!
axiom SLFFF
s=[+++Z][---Z]TS
z=+H[-Z]L
h=-Z[+H]L
t=TL
l=[-FFF][+FFF]F
angle 20
}

Hilbert {
axiom x
x=-YF+XFX+FY-
y=+XF-YFY-FX+
angle 4
}

Sierpinski3 {
;; This parameter list is from Jim Hanan via Corbit
axiom F-F-F
f=F[-F]F
angle 3
}

Peano3 {
axiom x
x=XFYFX+F+YFXFY-F-XFYFX
y=YFXFY-F-XFYFX+F+YFXFY
angle 4
}

Koch5 {
axiom f+F+F+F
f=F+F-F-FFF+F+F-F
angle 4
}


Sierpinski2 {
;; !This parameter list is from the 3rd printing of TSOFI
axiom FXF--FF--FF
f=FF
x=--FXF++FXF++FXF--
angle 6
}


SierpinskiSquare {
axiom F+F+F+F
f=FF+F+F+F+FF
angle 4
}

comment {

    Created by Adrian Mariano

    }


Pentagram {
  angle 10
  axiom fx++fx++fx++fx++fx
;;  f=f[++++@1.618033989f]
  x=[++++@i1.618033989f@.618033989f!x!@i.618033989f]
}

comment {

    The following from the Algorithmic Beauty of Plants.

    -- Adrian
}


 QuadKoch {
;;    Quadratic Koch island
;;    Figure 1.7a pg. 9
angle 4
AXIOM F-F-F-F-
F=F+FF-FF-F-F+F+FF-F-F+F+FF+FF-F
}

Fass1 {
;;   FASS curve (3x3 tiles form macrotile)
;;   Figure 1.16a pg. 17
axiom -l
angle 4
L=LF+RFR+FL-F-LFLFL-FRFR+
R=-LFLF+RFRFR+F+RF-LFL-FR
}

Fass2 {
;;   FASS curve (4x4 tiles form macrotile)
;;   Figure 1.16b pg. 17
angle 4
axiom -l
L=LFLF+RFR+FLFL-FRF-LFL-FR+F+RF-LFL-FRFRFR+
R=-LFLFLF+RFR+FL-F-LF+RFR+FLF+RFRF-LFL-FRFR
}

QuadGosper {
;;   Quadratic Gosper curve
;;   Figure 1.11b pg. 12
angle 4
axiom -Fr
l=FlFl-Fr-Fr+Fl+Fl-Fr-FrFl+Fr+FlFlFr-Fl+Fr+FlFl+Fr-FlFr-Fr-Fl+Fl+FrFr-
r=+FlFl-Fr-Fr+Fl+FlFr+Fl-FrFr-Fl-Fr+FlFrFr-Fl-FrFl+Fl+Fr-Fr-Fl+Fl+FrFr
f=
}


Plant1 {
;;   Plant-like structure, figure 1.24a pg. 25
;;   also p. 285a TSOFI
angle 14
axiom f
f=F[+F]F[-F]F
}


Plant2 {
;;   Plant-like structure, figure 1.24b pg. 25
angle 18
axiom f
f=F[+F]F[-F][F]
}

Plant3 {
;;   Plant-like structure, figure 1.24c pg. 25
angle 16
axiom f
f=FF-[-F+F+F]+[+F-F-F]
}

Plant4 {
;;   Plant-like structure, figure 1.24d pg. 25
angle 18
axiom x
X=F[+X]F[-X]+X
F=FF
}

Plant5 {
;;   Plant-like structure, figure 1.24e pg. 25
angle 14
axiom x
X=f[+X][-X]FX
F=FF
}

plant6 {
;;   Plant-like structure, figure 1.24f pg. 25
angle 16
axiom x
X=F-[[X]+X]+F[+FX]-X
F=FF
}


Curve1 {
;;   curve from figure 1.9a pg. 10 ABoP
angle 4
axiom F-F-F-F-
f=FF-F-F-F-F-F+F
}


Curve2 {
angle 4
axiom F-F-F-F-
f=FF-F+F-F-FF
}


Curve3 {
;;   curve from figure 1.9e pg. 10 ABoP
axiom F-F-F-F-
angle 4
F=F-FF--F-F
}



Leaf1 {

angle 8
axiom x
;;   Compound leaf with alternating branches.
;;   Figure 5.12b pg. 130

a=n
n=o
o=p
p=x
b=e
e=h
h=j
j=y
x=F[+A(4)]Fy
y=F[-B(4)]Fx
F=@1.18F@i1.18
}


Leaf2 {
;;   Compound leaf with alternating branches.
;;   Figure 5.12a pg. 130
angle 8
axiom a
a=f[+x]fb
b=f[-y]fa
x=a
y=b
f=@1.36f@i1.36
}


Bush {
        Angle 16
        Axiom ++++F
        F=FF-[-F+F+F]+[+F-F-F]
}

MyTree {
        Angle 16
        Axiom ++++F
        F=FF-[XY]+[XY]
        X=+FY
        Y=-FX
}

ColorTriangleGasket {
        Angle 6
        Axiom --X
        X=++FXF++FXF++FXF>1
        F=FF
}

SquareGasket {
        Angle 4
        Axiom X
        X=+FXF+FXF+FXF+FXF
        F=FF
}

DragonCurve {
        Angle 4
        Axiom X
        X=X-YF-
        Y=+FX+Y
}

Square {
        Angle 4
        Axiom F+F+F+F
        F=FF+F+F+F+FF
}


KochCurve {
        Angle 6
        Axiom F
        F=F+F--F+F
}

