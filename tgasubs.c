/* tsubs.c */

#define TARGA_DATA

#include	"targa.h"

/*******************************************************************/

void
VCenterDisplay( nLines )
int nLines;
{
int	lines;
int top, bottom;
long color;

	lines  = nLines >> 1;					/* half value of last line 0..x */
	top		 = 140 - (lines >> 1);
	bottom = top + lines;
	SetVBorder( top, bottom );
	SetVertShift( 255 - lines );	/* skip lines we're not using */

	if( targa->boardType == 16 )
		color = (12 << 10) | (12 << 5) | 12;
	else
		color = (80 << 16) | (80 << 8) | 80;
	SetBorderColor( (long *)&color );
}


/*****************************************************************/

void
SetDispReg(reg, value)
int reg, value;
{
	targa->DisplayRegister[reg] = value;	

	TSetMode(targa->mode&MSK_REGWRITE);  /* select Index Register write */
	OUTPORTB(DRREG, reg);	/* select sync register */
	/* 
	 *		Set Mask register to write value to
	 *		display register and to set Bit 9 in the DR
	 */
	TSetMode( ((targa->mode|(~MSK_REGWRITE))     /* turn on write bit */
		                   & MSK_BIT9      )     /* turn off Bit 9 */
                           | (value&0x0100)>>1); /* set bit 9 for value */
	OUTPORTB(DRREG, value);	/* select sync register */
	return;
 }

/*****************************************************************/

#define   WAITCOUNT  20000

int
VWait()
{
int	rasterreg, GiveUp;
	
	rasterreg = RASTERREG;

	GiveUp= WAITCOUNT;
	/*
 	 *	If beyond bottom of frame wait for next field
	 */
	if ( GetLine(rasterreg) == 0 ) {
		while ( (GiveUp>0) && (GetLine(rasterreg)==0) ) GiveUp--;
	}
	/*
	 *	Wait for the bottom of the border
	 */
	while ( (GiveUp>0) && (GetLine(rasterreg) >0) ) GiveUp--;

	if ( GiveUp > 0 )  return(0);
	else			   return(-1);
}


/*****************************************************************/

void
SetVBorder(top, bottom)
int top, bottom;
{
	/* top border */
	if ( top < MIN_TOP ) top=MIN_TOP;	
	SetDispReg(TOPBORDER,top);
	/* bottom border */
	if ( bottom > MAX_BOTTOM ) bottom=MAX_BOTTOM;
	SetDispReg(BOTTOMBORDER,bottom);

	SetDispReg(DR10,top);
	SetDispReg(DR11,bottom);
}


/*****************************************************************/

void
SetRGBorCV(type)
int	type;
{
	/*  set the contrast level */
	targa->RGBorCV = type;
	targa->VCRCon = ( targa->VCRCon  & MSK_RGBORCV ) | 
					(targa->RGBorCV<<SHF_RGBORCV) ;
	OUTPORTB(VCRCON, targa->VCRCon );
	return;
}

/*****************************************************************/

void
SetVCRorCamera(type)
int	type;
{
	targa->VCRorCamera = type&1;
	targa->VCRCon = ( targa->VCRCon  & MSK_VCRORCAMERA ) | 
					(targa->VCRorCamera<<SHF_VCRORCAMERA) ;
	OUTPORTB(VCRCON, targa->VCRCon );

}


/*****************************************************************/

void
SetBorderColor(color)
long *color;
{
	targa->BorderColor = *color;
	OUTPORTB(BORDER, (int)(0x0000ffffL&(*color)));
	OUTPORTB((BORDER+2), (int)(*(color)>>16));

	return;
}

/*****************************************************************/

void
SetMask(mask)
int mask;
{
	/* mask to valid values and output to mode register */
	targa->Mask = mask;
	OUTPORTB(MASKREG, mask);
	return;
}


/*****************************************************************/

void
SetVertShift(preshift)
int	preshift;
{
	/*  set the Vertical Preshift count  level */
	targa->VertShift = preshift;
	OUTPORTB(VERTPAN, preshift);
	return;
}


/*****************************************************************/

static   long  black =0L;

void
SetOverscan(mode)
int mode;
{
int temp;
long tempColor;

	targa->ovrscnOn = mode;
	if ( mode == 0 ) {
			INPORTB(UNDERREG);   /*  select underscan mode */
	 		SetHBorder(	(DEF_LEFT+targa->xOffset),
						(DEF_RIGHT+targa->xOffset));
			SetDispReg(4,352);
			SetDispReg(5,1);
			SetBorderColor(&targa->BorderColor);
	}
	else	{
			INPORTB(OVERREG);   /*  select overrscan mode */
			SetDispReg(0,64);	/*  Set four of the display registers */
			SetDispReg(1,363);  /*  to values required for Overscan */
			SetDispReg(4,363);
			SetDispReg(5,17);
			tempColor = targa->BorderColor;
			SetBorderColor(&black);
			targa->BorderColor = tempColor;
	}
}


/*****************************************************************/

void
SetInterlace(type)
int type;
{
int pageMode;

	pageMode= GetPageMode();
	targa->InterlaceMode= type & MSK_INTERLACE;
	SetDispReg(INTREG, targa->InterlaceMode);
	/*
	 *	SET THE INTERLACE BIT TO MATCH THE INTERLACE MODE AND 
	 *	SCREEN RESOLUTION -  SCREEN PAGE
	 */
	if ( ( targa->InterlaceMode >= 2 ) && 
		 ( pageMode> 1 )  &&
		 ( (pageMode&1) != 0 )    )     TSetMode(targa->mode|(~MSK_IBIT) );
	else								TSetMode(targa->mode& MSK_IBIT);
	return;
}


/*****************************************************************/

void
SetBlndReg(value)
int	value;
{
	/*  set the Vertical Preshift count  level */
	if ( targa->boardType == 32 ) {
		targa->VCRCon = (targa->VCRCon&0xfe) | value;
		OUTPORTB(BLNDREG, value);
		}
	return;
}


/*****************************************************************/

void
TSetMode(mode)
int mode;	/*   Mode Register Value */
{
	/* mask to valid values and output to mode register */
	OUTPORTB(MODEREG, mode );
	targa->mode = mode;
}


/*****************************************************************/

void
SetContrast(level)
int	level;
{
	/*  set the contrast level */
	targa->Contrast = level &((~MSK_CONTRAST)>>SHF_CONTRAST);
	targa->VCRCon = ( targa->VCRCon  & MSK_CONTRAST ) | 
			(targa->Contrast<<SHF_CONTRAST) ;
	OUTPORTB(VCRCON, targa->VCRCon );
	return;
}


/*****************************************************************/

void
SetHue(level)
int	level;
{
		/*  set the hue level -  Mask to valid value */
		targa->Hue = level&((~MSK_HUE)>>SHF_HUE);
					/* mask to valid range */
		targa->SatHue = (targa->SatHue&MSK_HUE) | 
			       (targa->Hue<<SHF_HUE);
		OUTPORTB(SATHUE, targa->SatHue );
	return;
}


/*****************************************************************/

void
SetSaturation(level)
int	level;
{
	/*  set the saturation level */

		targa->Saturation= level&( (~MSK_SATURATION)>>SHF_SATURATION);
		targa->SatHue =  (targa->SatHue&MSK_SATURATION) |
				(targa->Saturation<<SHF_SATURATION);
		OUTPORTB(SATHUE , targa->SatHue );
	return;
}


/*****************************************************************/

int
GetPageMode()
{
	return(targa->PageMode);
}


/*************************************************************/

void
SetPageMode(pageMode)
int	pageMode;
{

	pageMode &= 0x07;
	targa->PageMode = pageMode;
	VWait();
	TSetMode( (targa->mode)&(MSK_RES) |((pageMode<<SHF_RES)&(~MSK_RES)) ) ;
	if ( ( targa->DisplayRegister[20] >= 2 ) && 
		 ( pageMode> 1 )  &&
		 ( (pageMode&1) != 0 )    )     TSetMode(targa->mode|(~MSK_IBIT) );
	else
		TSetMode(targa->mode& MSK_IBIT);
	return;
}


/*****************************************************************/

void
SetHBorder(left, right)
int left, right;	/* left & right border positions */
{
	SetDispReg(LEFTBORDER, left);	/* set horizontal left border */
	SetDispReg(RIGHTBORDER,right);	/* set horizontal right border */
/*
 *					Set DR 8 and 9 since they
 *					default to tracking DR0 and DR 1
 */	
	SetDispReg(DR8,left);
	SetDispReg(DR9,left);

	return;
}


/*****************************************************************/

void
SetGenlock(OnOrOff)
int	OnOrOff;
{
	TSetMode( (targa->mode)&(MSK_GENLOCK)
			|((OnOrOff<<SHF_GENLOCK)&(~MSK_GENLOCK)) );
}


/*****************************************************************/
/* was asm, TC fast enough on AT */

int
GetLine( port )
int	port;
{
int cnt;
int val1, val2;
	
	val1 = INPORTB( port );
	for( cnt = 0; cnt < 20; cnt++ ) {
		val2 = INPORTB( port );
		if( val1 == val2 )
			break;
		val1 = val2;
	}
	return( val1 );
}

/**********************************************************************
                          TFIND
**********************************************************************/

/*****************************************************************
 	FindTarga : seek and find the first targa
 
	Usage:
		int   ret=0;
		ret = FindTarga(type, ioBase, memLoc)
		int	*type;		 targa type (8, 16, 24, 32)
   	int	*ioBase;	 io base location	
		int	*memLoc;	 memory base location

		ret = 0 if no targa is found or the TARGA is an invalid type
              and the  arguments are not changed
*****************************************************************/

#define    IOPOSSIBLE      16
#define    MEMPOSSIBLE      8

static unsigned ioPossible[] = {
	0x220, 0x230, 0x200, 0x210, 0x240, 
	0x250, 0x260, 0x270, 0x220, 0x290, 
	0x2a0, 0x2b0, 0x2c0, 0x2d0, 0x2e0,
	0x2f0
};

static unsigned memPossible[] = {
	0xa000, 0xd000, 0x8000, 0x9000, 
	0xc000, 0xe000, 0xf000, 0xb000
};


int
FindTarga(type, ioBase, memLoc)
int	*type;		/* targa type (8, 16, 24, 32)	*/
int	*ioBase;	/* io base location		*/
int	*memLoc;	/* memory base location		*/
{
int	i;
int	j;

int	huntType;	/* type we hunt for	*/
int	targaFound;
int	firstFound;
int	tmpMemLoc;
int	tmpIoBase;
int	tmpType;
int	possMemLoc;
int	possIoBase;
int	possType;

	/*
	 *	Find its I/O Space by searching for raster counter.
	 *
	 *	Look for the finger print of an I/O port register that
	 *	cycles between 0 and 242 and its companion which is always
	 *	flipping between 0 and 1
	 */

	targaFound = FALSE;
	firstFound = TRUE;
	huntType = *type;

	targaFound = FALSE;
	for(i = 0; i < IOPOSSIBLE; i++){ /* look for matching type	*/
		if(CheckRCounter(ioPossible[i]) == 0){
			continue;
		}
		else{
			possIoBase = ioPossible[i];
		}

		/*
		 *	find the memory address
		 *
		 *	Look for a memory segment that responds
		 *	to the memory segment.
		 */

		for(j = 0; (j < MEMPOSSIBLE) &&
			(CheckMemLoc(possIoBase, memPossible[j]) == 0); j++);
			if(j == MEMPOSSIBLE){ 
				continue;
			}
			else{
				possMemLoc = memPossible[j]; 
			}

		/*
		 *	See if this is a TARGA 8, M8,  16, 24, 32
		 */
		if ( (possType = FindBoardType(possIoBase, possMemLoc))== -1 ) 
			continue;

		targaFound = TRUE;

		if(firstFound){
			tmpMemLoc = possMemLoc;
			tmpIoBase = possIoBase;
			tmpType = possType;
			firstFound = FALSE;
		}

		if((possType == huntType) && targaFound) {
			*memLoc = possMemLoc;
			*ioBase = possIoBase;
			*type = possType;
			return(1);
		}
	}

	if(targaFound) {
		if(possType != huntType){
			*memLoc = tmpMemLoc;
			*ioBase = tmpIoBase;
			*type = tmpType;
		}
	}

	if(targaFound)
		return(1);
	else
		return(0);
}


/*****************************************************************
	CheckRCounter --  See if raster counter at this base IO address

	usage:     ret = CheckRCounter(ioBase)
			unsigned ioBase;
				
		ret ==  0   if not at this address
            1   if fingerprint found

*****************************************************************/

#define LINES	1000	/*  Pulled out of the air			*/

#define CHANGES  20	/*  This is the number of valid changes		*/

int
CheckRCounter(ioBase)
unsigned	ioBase;
{
int	i;					/* array index */
int	changes;		/*  number of valid transitions */
int	new;				/*  temporary variable */
int	old;				/*  last valid raster value */
int	rasterReg;	/*  value of the raster Register */

	old =  0;	/*  insure that we don't blow out when starting */
	changes =  0;		/*  zero change counter */
	rasterReg = ioBase + (signed)0xC00 ;   /*  add offset to I/O base */

	/*
	*	Read LINES raster values and check if each is valid wrt
	*	previous value.  Quit and return 0 if invalid 
	*/

	for (i=0; i<LINES; i++){
		if ((new = INPORTB(rasterReg)) > old ){
			if(new > 243){ return(0);}   /* return if illegal */
			old = new;
			changes++;
		}
		else if(new != old){	/*   if new is less than old */
			if(new != 0)  {
				/*  This is only valid if new == 0 */
				return(0);
			}
			/* quit otherwise */
			old = 0;
			changes++;
		}
	}
	if(changes > CHANGES) 	
		return(1);
	else
		return(0);
}


/*****************************************************************

	CheckMemLoc  --  See if specified Memory Segment is associated with
					the specified IO space

	usage:   ret = CheckMemLoc(ioBase,memAddr)
				
		ret  =  0   if not valid memory
			1   i valid memory/IO pair
*****************************************************************/

CheckMemLoc(ioBase,memAddr)
	unsigned ioBase;	/*  previously determine ioSpace */
	unsigned memAddr;	/*   proposed memory segment */
{
	int	modeReg,  maskReg;
	int	saveValue, temp;
	int	loop;

	modeReg = ioBase + (signed)0xC00;
	maskReg = ioBase + (signed)0x800;
	OUTPORTB(modeReg, (signed)0x01);   	/*   Select display memory */
	OUTPORTW(maskReg, (signed)0x0000);	/* clear the mask for valid read */

	PEEK(0, memAddr, &saveValue, 2 );

	/*	loop and see if this is real or just junk on the bus
	*	assume open bus if junk 
	*/


	for(loop = 0; loop<20; loop++){

		PEEK(0, memAddr, &temp, 2 );
		if(saveValue != temp){
			OUTPORTB(modeReg, 0);	/*   Deselect display memory */
			return(0);
		}
	}

	/*
	*	if savevalue is -1 then we can't check using mask w/o
	*	first writing out new value 
	*/

	if(saveValue == (signed)0xFFFF){
		disable();

		temp=0;
		POKE(0, memAddr, &temp,2);

		/*	If write did not work , enable ints, and fail */

		PEEK(0, memAddr, &temp, 2 );
		if( temp != 0){
			enable();
			OUTPORTB(modeReg, 0);	/*   Deselect display memory */
			return(0);
		}
	}

	/*	set mask and see if it returns 0xffff	*/

	temp = (signed)0xffff;
	OUTPORTW(maskReg, temp);

	PEEK(0, memAddr, &temp, 2 );
	if(temp == (signed)0xFFFF)  loop=1;
	else				loop=0;

	OUTPORTW(maskReg,0);

	if(saveValue == (signed)0xFFFF){
		POKE(0, memAddr, &saveValue, 2);
		enable();
	}

	OUTPORTB(modeReg, 0);	/*   Deselect display memory */
	return(loop);
}


/*****************************************************************
  FindBoardType  -  find out what type o board is assoicated with
			the TARGA located at a specied IO amd memory
			space

	Usage:   ret = FindBoardType(ioBase,memAddr);

			unsigned  ioBase;    TARGA's base io address
			unsigned  memAddr;	TARGA's memory space

			ret = 8 , 16, 24, 32 ----  Targa type
				-1	Unknown or invalid board
********************************************************************/

FindBoardType(ioBase,memAddr)
	unsigned    ioBase;
	unsigned    memAddr;
{
	int	modeReg,  maskReg;
	int	saveVal1, saveVal2;
	int	test1, test2;
	int	type, temp;


	modeReg = ioBase + (signed)0xC00;
	maskReg = ioBase + (signed)0x800;
	OUTPORTB(modeReg, 0x01);   	/*   Select display memory */
	OUTPORTW(maskReg, 0x0000);	/* clear the mask for valid read */

	PEEK(0, memAddr, &saveVal1, 2 );
	PEEK(2, memAddr, &saveVal2, 2 );

	/* write ff's to the first four bytes */

	temp =(signed)0xffff;
	POKE(0, memAddr, &temp, 2);
	POKE(2, memAddr, &temp, 2);

	OUTPORTW(maskReg, (signed)0x7FFe);	/* set write to known pattern */
	temp =0;
	POKE(0, memAddr, &temp, 2);	/*  and determine is 8, 16, 24, or 32 */
	POKE(2, memAddr, &temp, 2);	/*  from pattern read */

	OUTPORTW(maskReg, 0);
	PEEK(0, memAddr, &test1, 2 );

	type = -1;
	if(test1 == (signed)0x7ffe) type = TYPE_16;
	if(test1 == (signed)0xfffc){
		PEEK(2, memAddr, &test2, 2 );
		if(test2 == (signed)0x7fff) type = TYPE_32;
		if(test2 == (signed)0xffff) type = TYPE_24;
	}

	OUTPORTW(maskReg, 0x0000);	/* clear the mask for valid read */
	POKE(0, memAddr, &saveVal1, 2);
	POKE(2, memAddr, &saveVal2, 2);
	OUTPORTB(modeReg, 0);	/*   Deselect display memory */
	return(type);
}

/**********************************************************************
                          TINIT
**********************************************************************/

GraphInit( board )
int board;
{
int	cnt;
int bottom, top;
int  mode, tBoard;
	
	board = board;

	memset( &tstructure, 0, sizeof(tstructure) );
	targa = &tstructure;

	targa->boardType = -1;										/*  assume the resident board */
	targa->xOffset = 0;  targa->yOffset = 0;  /* default to no offset */
	targa->LinesPerField= DEF_ROWS/2; 				/*  number of lines per field */
	targa->AlwaysGenLock = DEF_GENLOCK;				/* default to genlock off */
	targa->PageMode = 0;
	targa->InterlaceMode = DEF_INT;   					/* Defalut:  Interlace Mode 0 */
	targa->Contrast= DEF_CONTRAST;
	targa->Saturation = DEF_SATURATION;
	targa->Hue = DEF_HUE;
	targa->RGBorCV = CV;											/*  default to Composite video */
	targa->VCRorCamera = CAMERA;
	targa->PanXOrig = 0; targa->PanYOrig=0;
	targa->PageUpper= 0xffff;					/*  set the bank flags to illega& values */
	targa->PageLower= 0xffff;   			/*  so that they will be set the first time */
	targa->ovrscnAvail = 0;       			/*  Assume no Overscan option */
	targa->ovrscnOn = 0;
	targa->iobase = TIOBASE;
	targa->memloc = TSEG;

	targa->boardType = TYPE_16;		/* default to looking for a 16 */
	for( cnt = 0; ;cnt++ ) {
		if( FindTarga(&targa->boardType, &targa->iobase, &targa->memloc) )
			break;										/* if multiple, first card only */
		else if( cnt == 5 )
			return( -1 );		/* not found */
	}

	if ( targa->boardType == TYPE_16 ) {
			targa->MaxBanks = 16;
			targa->BytesPerPixel = 2;
    }
	if ( targa->boardType == TYPE_24 ) {
			targa->MaxBanks = 32;
			targa->BytesPerPixel = 3;
    }
	if ( targa->boardType == TYPE_32 ) {
			targa->MaxBanks = 32;
			targa->BytesPerPixel = 4;
    }

		/******	Compute # of rows per 32K bank  ********/
	targa->RowsPerBank = 512/(targa->MaxBanks);
	targa->AddressShift =  targa->MaxBanks>>4;

		/*	if initializing CVA:  set these before we quit	*/
	SetSaturation(targa->Saturation);
	SetHue(targa->Hue);
	SetContrast( targa->Contrast);
	
		/*	Set Genlock bit if always genlocked */
		/*	Set before flipping and jerking screen */
	TSetMode( (targa->AlwaysGenLock<<SHF_GENLOCK) | DEF_MODE);

	SetBlndReg(0);		/*  disable blend mode on TARGA 32 */

	SetInterlace(targa->InterlaceMode);
	SetFixedRegisters();
	SetOverscan( 0 );

	top 		= 140 - (targa->LinesPerField / 2);
	bottom = top + targa->LinesPerField;
	SetVBorder(top,bottom);
	SetVertShift(256-targa->LinesPerField);

	SetMask(DEF_MASK);
	SetRGBorCV (targa->RGBorCV );
	SetVCRorCamera(targa->VCRorCamera);

	/*	See if the raster register is working correctly and
	    return error flag if its not	 */
	if ( VWait() == -1)
		return(-1);
	else
		return(0);
}


/**************************************************************/
void
GraphEnd()
{
	TSetMode( (targa->mode)&MSK_MSEL );	/*  disable memory */
	return;
}


/**************************************************************/
/* Set the registers which have required values */

#define	FIXED_REGS	10

static int FixedRegs[] = {
	DR6,DR7,DR12,DR13,DR14,DR15,DR16,DR17,DR18,DR19
};

static int FixedValue[] = {
	DEF_DR6,DEF_DR7,DEF_DR12,DEF_DR13,DEF_DR14,
	DEF_DR15,DEF_DR16,DEF_DR17,DEF_DR18,DEF_DR19
};

SetFixedRegisters()
{
int reg;

	for ( reg=0; reg<FIXED_REGS; reg++)
		SetDispReg(FixedRegs[reg],FixedValue[reg]);
	return(0);
}



