/*
    zoom.c - routines for zoombox manipulation and for panning

*/

#include <float.h>
#include "fractint.h"

/* screen dimensions here are (1.0,1.0) corresponding to (xdots-1,ydots-1) */
extern double zbx,zby;		   /* topleft of unrotated zoombox  */
extern double zwidth,zdepth,zskew; /* zoombox size & shape	    */
extern int zrotate;		   /* * 2.5 degree increments	    */
extern int boxcount,boxx[],boxy[]; /* co-ords of each zoombox pixel */
extern int xdots,ydots,sxdots,sydots,sxoffs,syoffs;
extern double dxsize,dysize;	   /* xdots-1, ydots-1		    */
extern double xxmin,yymin,xxmax,yymax,xx3rd,yy3rd;
extern double sxmin,symin,sxmax,symax,sx3rd,sy3rd;
/* top	  left	corner of screen is (xxmin,yymax) */
/* bottom left	corner of screen is (xx3rd,yy3rd) */
/* bottom right corner of screen is (xxmax,yymin) */
extern double plotmx1,plotmx2,plotmy1,plotmy2;

extern int  calc_status;	   /* status of calculations */
extern int  fractype;		   /* fractal type */
extern char stdcalcmode;	   /* '1', '2', 'g', 'b', or 't' */
extern int  num_worklist;	   /* resume worklist for standard engine */
extern struct workliststuff worklist[MAXCALCWORK];
extern char dstack[4096];	   /* common temp, used for get_line/put_line */
extern int  StandardFractal();
extern int  calcmand();
extern int  calcmandfp();
extern int  potflag;
extern int  pot16bit;
extern float finalaspectratio;

struct coords {
    int x,y;
    };

#define PIXELROUND 0.00001

static void _fastcall drawlines(struct coords, struct coords, int, int);
static void _fastcall addbox(struct coords);
static void _fastcall zmo_calc(double, double, double *, double *);
static int  check_pan();
static void fix_worklist();
static void _fastcall move_row(int fromrow,int torow,int col);

void drawbox(int drawit)
{   struct coords tl,bl,tr,br; /* dot addr of topleft, botleft, etc */
    double tmpx,tmpy,dx,dy,rotcos,rotsin,ftemp1,ftemp2;
    double fxwidth,fxskew,fydepth,fyskew,fxadj;

    if (zwidth==0) { /* no box to draw */
	if (boxcount!=0) { /* remove the old box from display */
	    clearbox();   /* asm routine */
	    boxcount = 0; }
	reset_zoom_corners();
	return; }

    ftemp1 = PI*zrotate/72; /* convert to radians */
    rotcos = cos(ftemp1);   /* sin & cos of rotation */
    rotsin = sin(ftemp1);

    /* do some calcs just once here to reduce fp work a bit */
    fxwidth = sxmax-sx3rd;
    fxskew  = sx3rd-sxmin;
    fydepth = sy3rd-symax;
    fyskew  = symin-sy3rd;
    fxadj   = zwidth*zskew;

    /* calc co-ords of topleft & botright corners of box */
    tmpx = zwidth/-2+fxadj; /* from zoombox center as origin, on xdots scale */
    tmpy = zdepth*finalaspectratio/2;
    dx = (rotcos*tmpx - rotsin*tmpy) - tmpx; /* delta x to rotate topleft */
    dy = tmpy - (rotsin*tmpx + rotcos*tmpy); /* delta y to rotate topleft */
    /* calc co-ords of topleft */
    ftemp1 = zbx + dx + fxadj;
    ftemp2 = zby + dy/finalaspectratio;
    tl.x   = ftemp1*(dxsize+PIXELROUND); /* screen co-ords */
    tl.y   = ftemp2*(dysize+PIXELROUND);
    xxmin  = sxmin + ftemp1*fxwidth + ftemp2*fxskew; /* real co-ords */
    yymax  = symax + ftemp2*fydepth + ftemp1*fyskew;
    /* calc co-ords of bottom right */
    ftemp1 = zbx + zwidth - dx - fxadj;
    ftemp2 = zby - dy/finalaspectratio + zdepth;
    br.x   = ftemp1*(dxsize+PIXELROUND);
    br.y   = ftemp2*(dysize+PIXELROUND);
    xxmax  = sxmin + ftemp1*fxwidth + ftemp2*fxskew;
    yymin  = symax + ftemp2*fydepth + ftemp1*fyskew;

    /* do the same for botleft & topright */
    tmpx = zwidth/-2 - fxadj;
    tmpy = 0.0-tmpy;
    dx = (rotcos*tmpx - rotsin*tmpy) - tmpx;
    dy = tmpy - (rotsin*tmpx + rotcos*tmpy);
    ftemp1 = zbx + dx - fxadj;
    ftemp2 = zby + dy/finalaspectratio + zdepth;
    bl.x   = ftemp1*(dxsize+PIXELROUND);
    bl.y   = ftemp2*(dysize+PIXELROUND);
    xx3rd  = sxmin + ftemp1*fxwidth + ftemp2*fxskew;
    yy3rd  = symax + ftemp2*fydepth + ftemp1*fyskew;
    ftemp1 = zbx + zwidth - dx + fxadj;
    ftemp2 = zby - dy/finalaspectratio;
    tr.x   = ftemp1*(dxsize+PIXELROUND);
    tr.y   = ftemp2*(dysize+PIXELROUND);

    if (boxcount!=0) { /* remove the old box from display */
	clearbox();   /* asm routine */
	boxcount = 0; }

    if (drawit) { /* caller wants box drawn as well as co-ords calc'd */
	/* build the list of zoom box pixels */
	addbox(tl); addbox(tr); 	      /* corner pixels */
	addbox(bl); addbox(br);
	drawlines(tl,tr,bl.x-tl.x,bl.y-tl.y); /* top & bottom lines */
	drawlines(tl,bl,tr.x-tl.x,tr.y-tl.y); /* left & right lines */
	dispbox();			      /* asm routine to paint it */
	}
    }

static void _fastcall drawlines(struct coords fr, struct coords to,
				int dx, int dy)
{   int xincr,yincr,ctr;
    int altctr,altdec,altinc;
    struct coords tmpp,line1,line2;

    if (abs(to.x-fr.x) > abs(to.y-fr.y)) { /* delta.x > delta.y */
	if (fr.x>to.x) { /* swap so from.x is < to.x */
	    tmpp = fr; fr = to; to = tmpp; }
	xincr = (to.x-fr.x)*4/sxdots+1; /* do every 1st, 2nd, 3rd, or 4th dot */
	ctr = (to.x-fr.x-1)/xincr;
	altdec = abs(to.y-fr.y)*xincr;
	altinc = to.x-fr.x;
	altctr = altinc/2;
	yincr = (to.y>fr.y)?1:-1;
	line2.x = (line1.x = fr.x) + dx;
	line2.y = (line1.y = fr.y) + dy;
	while (--ctr>=0) {
	    line1.x += xincr;
	    line2.x += xincr;
	    altctr -= altdec;
	    while (altctr<0) {
		altctr	+= altinc;
		line1.y += yincr;
		line2.y += yincr;
		}
	    addbox(line1);
	    addbox(line2);
	    }
	}

    else { /* delta.y > delta.x */
	if (fr.y>to.y) { /* swap so from.y is < to.y */
	    tmpp = fr; fr = to; to = tmpp; }
	yincr = (to.y-fr.y)*4/sydots+1; /* do every 1st, 2nd, 3rd, or 4th dot */
	ctr = (to.y-fr.y-1)/yincr;
	altdec = abs(to.x-fr.x)*yincr;
	altinc = to.y-fr.y;
	altctr = altinc/2;
	xincr = (to.x>fr.x) ? 1 : -1;
	line2.x = (line1.x = fr.x) + dx;
	line2.y = (line1.y = fr.y) + dy;
	while (--ctr>=0) {
	    line1.y += yincr;
	    line2.y += yincr;
	    altctr  -= altdec;
	    while (altctr<0) {
		altctr	+= altinc;
		line1.x += xincr;
		line2.x += xincr;
		}
	    addbox(line1);
	    addbox(line2);
	    }
	}
    }

static void _fastcall addbox(struct coords point)
{
    point.x += sxoffs;
    point.y += syoffs;
    if (point.x >= 0 && point.x < sxdots && point.y >= 0 && point.y < sydots) {
	boxx[boxcount] = point.x;
	boxy[boxcount] = point.y;
	++boxcount;
	}
    }

void moveboxf(double dx, double dy)
{   int align,row,col;
    align = check_pan();
    if (dx!=0.0) {
	if ((zbx += dx) + zwidth/2 < 0)  /* center must stay onscreen */
	    zbx = zwidth/-2;
	if (zbx + zwidth/2 > 1)
	    zbx = 1.0 - zwidth/2;
	if (align != 0
	  && ((col = zbx*(dxsize+PIXELROUND)) & (align-1)) != 0) {
	    if (dx > 0) col += align;
	    col -= col & (align-1); /* adjust col to pass alignment */
	    zbx = (double)col/dxsize; }
	}
    if (dy!=0.0) {
	if ((zby += dy) + zdepth/2 < 0)
	    zby = zdepth/-2;
	if (zby + zdepth/2 > 1)
	    zby = 1.0 - zdepth/2;
	if (align != 0
	  && ((row = zby*(dysize+PIXELROUND)) & (align-1)) != 0) {
	    if (dy > 0) row += align;
	    row -= row & (align-1);
	    zby = (double)row/dysize; }
	}
    }

static void _fastcall chgboxf(double dwidth, double ddepth)
{
    if (zwidth+dwidth > 1)
	dwidth = 1.0-zwidth;
    if (zwidth+dwidth < 0.05)
	dwidth = 0.05-zwidth;
    zwidth += dwidth;
    if (zdepth+ddepth > 1)
	ddepth = 1.0-zdepth;
    if (zdepth+ddepth < 0.05)
	ddepth = 0.05-zdepth;
    zdepth += ddepth;
    moveboxf(dwidth/-2,ddepth/-2); /* keep it centered & check limits */
    }

void resizebox(int steps)
{
    double deltax,deltay;
    if (zdepth*SCREENASPECT > zwidth) { /* box larger on y axis */
	deltay = steps * 0.036 / SCREENASPECT;
	deltax = zwidth * deltay / zdepth;
	}
    else {				/* box larger on x axis */
	deltax = steps * 0.036;
	deltay = zdepth * deltax / zwidth;
	}
    chgboxf(deltax,deltay);
    }

void chgboxi(int dw, int dd)
{   /* change size by pixels */
    chgboxf( (double)dw/dxsize, (double)dd/dysize );
    }

#ifdef C6
#pragma optimize("e",off)  /* MSC 6.00A messes up next rtn with "e" on */
#endif

void zoomout() /* for ctl-enter, calc corners for zooming out */
{   double savxxmin,savyymax,ftemp;
    /* (xxmin,yymax), etc, are already set to zoombox corners;
       (sxmin,symax), etc, are still the screen's corners;
       use the same logic as plot_orbit stuff to first calculate current screen
       corners relative to the zoombox, as if the zoombox were a square with
       upper left (0,0) and width/depth 1; ie calc the current screen corners
       as if plotting them from the zoombox;
       then extend these co-ords from current real screen corners to get
       new actual corners
       */
    ftemp = (yymin-yy3rd)*(xx3rd-xxmin) - (xxmax-xx3rd)*(yy3rd-yymax);
    plotmx1 = (xx3rd-xxmin) / ftemp; /* reuse the plotxxx vars is safe */
    plotmx2 = (yy3rd-yymax) / ftemp;
    plotmy1 = (yymin-yy3rd) / ftemp;
    plotmy2 = (xxmax-xx3rd) / ftemp;
    savxxmin = xxmin; savyymax = yymax;
    zmo_calc(sxmin-savxxmin,symax-savyymax,&xxmin,&yymax); /* new xxmin/xxmax */
    zmo_calc(sxmax-savxxmin,symin-savyymax,&xxmax,&yymin);
    zmo_calc(sx3rd-savxxmin,sy3rd-savyymax,&xx3rd,&yy3rd);
    }

#ifdef C6
#pragma optimize("e",on)  /* back to normal */
#endif

static void _fastcall zmo_calc(double dx, double dy, double *newx, double *newy)
{   double tempx,tempy;
    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft thru (1,1) bottom right */
    tempx = dy * plotmx1 - dx * plotmx2;
    tempy = dx * plotmy1 - dy * plotmy2;
    /* calc new corner by extending from current screen corners */
    *newx = sxmin + tempx*(sxmax-sx3rd) + tempy*(sx3rd-sxmin);
    *newy = symax + tempy*(sy3rd-symax) + tempx*(symin-sy3rd);
    }

void aspectratio_crop(float oldaspect,float newaspect)
{
   double ftemp,xmargin,ymargin;
   if (newaspect > oldaspect) { /* new ratio is taller, crop x */
      ftemp = (1.0 - oldaspect / newaspect) / 2;
      xmargin = (xxmax - xx3rd) * ftemp;
      ymargin = (yymin - yy3rd) * ftemp;
      xx3rd += xmargin;
      yy3rd += ymargin;
      }
   else 		      { /* new ratio is wider, crop y */
      ftemp = (1.0 - newaspect / oldaspect) / 2;
      xmargin = (xx3rd - xxmin) * ftemp;
      ymargin = (yy3rd - yymax) * ftemp;
      xx3rd -= xmargin;
      yy3rd -= ymargin;
      }
   xxmin += xmargin;
   yymax += ymargin;
   xxmax -= xmargin;
   yymin -= ymargin;
}

static int check_pan() /* return 0 if can't, alignment requirement if can */
{   int i,j;
    if (calc_status != 2 && calc_status != 4)
	return(0); /* not resumable, not complete */
    if ( curfractalspecific->calctype != StandardFractal
      && curfractalspecific->calctype != calcmand
      && curfractalspecific->calctype != calcmandfp)
	return(0); /* not a worklist-driven type */
    if (zwidth != 1.0 || zdepth != 1.0 || zskew != 0.0 || zrotate != 0.0)
	return(0); /* not a full size unrotated unskewed zoombox */
    /* can pan if we get this far */
    if (calc_status == 4)
	return(1); /* image completed, align on any pixel */
    if (potflag && pot16bit)
	return(1); /* 1 pass forced so align on any pixel */
    if (stdcalcmode == 'b')
	return(1); /* btm, align on any pixel */
    if (stdcalcmode == 't')
	return(0); /* tesselate, can't do it */
    if (stdcalcmode != 'g' || (curfractalspecific->flags&NOGUESS)) {
	if (stdcalcmode == '2') /* align on even pixel for 2pass */
	   return(2);
	return(1); /* assume 1pass */
	}
    /* solid guessing */
    start_resume();
    get_resume(sizeof(int),&num_worklist,sizeof(worklist),worklist,0);
    /* don't do end_resume! we're just looking */
    i = 9;
    for (j=0; j<num_worklist; ++j) /* find lowest pass in any pending window */
	if (worklist[j].pass < i)
	    i = worklist[j].pass;
    j = ssg_blocksize(); /* worst-case alignment requirement */
    while (--i >= 0)
	j = j>>1; /* reduce requirement */
    return(j);
    }

static void _fastcall move_row(int fromrow,int torow,int col)
/* move a row on the screen */
{   int startcol,endcol,tocol;
    memset(dstack,0,xdots); /* use dstack as a temp for the row; clear it */
    if (fromrow >= 0 && fromrow < ydots) {
	tocol = startcol = 0;
	endcol = xdots-1;
	if (col < 0) {
	    tocol -= col;
	    endcol += col; }
	if (col > 0)
	    startcol += col;
	get_line(fromrow,startcol,endcol,&dstack[tocol]);
	}
    put_line(torow,0,xdots-1,dstack);
    }

int init_pan_or_recalc(zoomout) /* decide to recalc, or to chg worklist & pan */
{   int i,j,row,col,y,alignmask,listfull;
    if (zwidth == 0.0)
	return(0); /* no zoombox, leave calc_status as is */
    /* got a zoombox */
    if ((alignmask=check_pan()-1) < 0) {
	calc_status = 0; /* can't pan, trigger recalc */
	return(0); }
    if (zbx == 0.0 && zby == 0.0) {
	clearbox();
	return(0); } /* box is full screen, leave calc_status as is */
    col = zbx*(dxsize+PIXELROUND); /* calc dest col,row of topleft pixel */
    row = zby*(dysize+PIXELROUND);
    if (zoomout) { /* invert row and col */
	row = 0-row;
	col = 0-col; }
    if ((row&alignmask) != 0 || (col&alignmask) != 0) {
	calc_status = 0; /* not on useable pixel alignment, trigger recalc */
	return(0); }
    /* pan */
    num_worklist = 0;
    if (calc_status == 2) {
       start_resume();
       get_resume(sizeof(int),&num_worklist,sizeof(worklist),worklist,0);
       } /* don't do end_resume! we might still change our mind */
    /* adjust existing worklist entries */
    for (i=0; i<num_worklist; ++i) {
	worklist[i].yystart -= row;
	worklist[i].yystop  -= row;
	worklist[i].yybegin -= row;
	worklist[i].xxstart -= col;
	worklist[i].xxstop  -= col;
	}
    /* add worklist entries for the new edges */
    listfull = i = 0;
    j = ydots-1;
    if (row < 0) {
	listfull |= add_worklist(0,xdots-1,0,0-row-1,0,0,0);
	i = 0 - row; }
    if (row > 0) {
	listfull |= add_worklist(0,xdots-1,ydots-row,ydots-1,ydots-row,0,0);
	j = ydots - row - 1; }
    if (col < 0)
	listfull |= add_worklist(0,0-col-1,i,j,i,0,0);
    if (col > 0)
	listfull |= add_worklist(xdots-col,xdots-1,i,j,i,0,0);
    if (listfull != 0) {
    static char far msg[] = {"\
Tables full, can't pan current image.\n\
Cancel resumes old image, continue pans and calculates a new one."};
	if (stopmsg(2,msg)) {
	    zwidth = 0; /* cancel the zoombox */
	    drawbox(1); }
	else
	    calc_status = 0; /* trigger recalc */
	return(0); }
    /* now we're committed */
    calc_status = 2;
    clearbox();
    if (row > 0) /* move image up */
	for (y=0; y<ydots; ++y) move_row(y+row,y,col);
    else	 /* move image down */
	for (y=ydots; --y>=0;)	move_row(y+row,y,col);
    fix_worklist(); /* fixup any out of bounds worklist entries */
    alloc_resume(sizeof(worklist)+10,1); /* post the new worklist */
    put_resume(sizeof(int),&num_worklist,sizeof(worklist),worklist,0);
    return(0);
    }

static void _fastcall restart_window(int wknum)
/* force a worklist entry to restart */
{   int yfrom,yto,xfrom,xto;
    if ((yfrom = worklist[wknum].yystart) < 0) yfrom = 0;
    if ((xfrom = worklist[wknum].xxstart) < 0) xfrom = 0;
    if ((yto = worklist[wknum].yystop) >= ydots) yto = ydots - 1;
    if ((xto = worklist[wknum].xxstop) >= xdots) xto = xdots - 1;
    memset(dstack,0,xdots); /* use dstack as a temp for the row; clear it */
    while (yfrom <= yto)
	put_line(yfrom++,xfrom,xto,dstack);
    worklist[wknum].sym = worklist[wknum].pass = 0;
    worklist[wknum].yybegin = worklist[wknum].yystart;
}

static void fix_worklist() /* fix out of bounds and symmetry related stuff */
{   int i,j,k;
    struct workliststuff *wk;
    for (i=0; i<num_worklist; ++i) {
	wk = &worklist[i];
	if ( wk->yystart >= ydots || wk->yystop < 0
	  || wk->xxstart >= xdots || wk->xxstop < 0) { /* offscreen, delete */
	    for (j=i+1; j<num_worklist; ++j)
		worklist[j-1] = worklist[j];
	    --num_worklist;
	    --i;
	    continue; }
	if (wk->yystart < 0) /* partly off top edge */
	    if ((wk->sym&1) == 0) /* no sym, easy */
		wk->yystart = 0;
	    else { /* xaxis symmetry */
		if ((j = wk->yystop + wk->yystart) > 0
		  && num_worklist < MAXCALCWORK) { /* split the sym part */
		    worklist[num_worklist] = worklist[i];
		    worklist[num_worklist].yystart = 0;
		    worklist[num_worklist++].yystop = j;
		    wk->yystart = j+1; }
		else
		    wk->yystart = 0;
		restart_window(i); /* restart the no-longer sym part */
		}
	if (wk->yystop >= ydots) { /* partly off bottom edge */
	   j = ydots-1;
	   if ((wk->sym&1) != 0) { /* uses xaxis symmetry */
	      if ((k = wk->yystart + (wk->yystop - j)) < j)
		 if (num_worklist >= MAXCALCWORK) /* no room to split */
		    restart_window(i);
		 else { /* split it */
		    worklist[num_worklist] = worklist[i];
		    worklist[num_worklist].yystart = k;
		    worklist[num_worklist++].yystop = j;
		    j = k-1; }
	      wk->sym &= -1 - 1; }
	   wk->yystop = j; }
	if (wk->xxstart < 0) /* partly off left edge */
	    if ((wk->sym&2) == 0) /* no sym, easy */
		wk->xxstart = 0;
	    else { /* yaxis symmetry */
		if ((j = wk->xxstop + wk->xxstart) > 0
		  && num_worklist < MAXCALCWORK) { /* split the sym part */
		    worklist[num_worklist] = worklist[i];
		    worklist[num_worklist].xxstart = 0;
		    worklist[num_worklist++].xxstop = j;
		    wk->xxstart = j+1; }
		else
		    wk->xxstart = 0;
		restart_window(i); /* restart the no-longer sym part */
		}
	if (wk->xxstop >= xdots) { /* partly off right edge */
	   j = xdots-1;
	   if ((wk->sym&2) != 0) { /* uses xaxis symmetry */
	      if ((k = wk->xxstart + (wk->xxstop - j)) < j)
		 if (num_worklist >= MAXCALCWORK) /* no room to split */
		    restart_window(i);
		 else { /* split it */
		    worklist[num_worklist] = worklist[i];
		    worklist[num_worklist].xxstart = k;
		    worklist[num_worklist++].xxstop = j;
		    j = k-1; }
	      wk->sym &= -1 - 2; }
	   wk->xxstop = j; }
	if (wk->yybegin < wk->yystart) wk->yybegin = wk->yystart;
	if (wk->yybegin > wk->yystop)  wk->yybegin = wk->yystop;
	}
    tidy_worklist(); /* combine where possible, re-sort */
}

