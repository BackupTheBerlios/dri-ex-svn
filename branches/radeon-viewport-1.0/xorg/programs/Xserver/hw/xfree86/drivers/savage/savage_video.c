/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/savage/savage_video.c,v 1.17tsi Exp $ */

#include "Xv.h"
#include "dix.h"
#include "dixstruct.h"
#include "fourcc.h"

#include "savage_driver.h"
#include "savage_streams.h"
#include "savage_regs.h"
#include "savage_bci.h"

#define OFF_DELAY 	200  /* milliseconds */
#define FREE_DELAY 	60000

#define OFF_TIMER 	0x01
#define FREE_TIMER	0x02
#define CLIENT_VIDEO_ON	0x04

#define TIMER_MASK      (OFF_TIMER | FREE_TIMER)

void savageOUTREG( SavagePtr psav, unsigned long offset, unsigned long value );

static XF86VideoAdaptorPtr SavageSetupImageVideo(ScreenPtr);
static void SavageInitOffscreenImages(ScreenPtr);
static void SavageStopVideo(ScrnInfoPtr, pointer, Bool);
static int SavageSetPortAttribute(ScrnInfoPtr, Atom, INT32, pointer);
static int SavageGetPortAttribute(ScrnInfoPtr, Atom ,INT32 *, pointer);
static void SavageQueryBestSize(ScrnInfoPtr, Bool,
	short, short, short, short, unsigned int *, unsigned int *, pointer);
static int SavagePutImage( ScrnInfoPtr, 
	short, short, short, short, short, short, short, short,
	int, unsigned char*, short, short, Bool, RegionPtr, pointer);
static int SavageQueryImageAttributes(ScrnInfoPtr, 
	int, unsigned short *, unsigned short *,  int *, int *);

void SavageResetVideo(ScrnInfoPtr pScrn); 

static void SavageSetColorKeyOld(ScrnInfoPtr pScrn);
static void SavageSetColorKeyNew(ScrnInfoPtr pScrn);
static void SavageSetColorKey2000(ScrnInfoPtr pScrn);
static void (*SavageSetColorKey)(ScrnInfoPtr pScrn) = NULL;

static void SavageSetColorOld(ScrnInfoPtr pScrn );
static void SavageSetColorNew(ScrnInfoPtr pScrn );
static void SavageSetColor2000(ScrnInfoPtr pScrn );
static void (*SavageSetColor)(ScrnInfoPtr pScrn ) = NULL;

static void (*SavageInitStreams)(ScrnInfoPtr pScrn) = NULL;

static void SavageDisplayVideoOld(
    ScrnInfoPtr pScrn, int id, int offset,
    short width, short height, int pitch, 
    int x1, int y1, int x2, int y2,
    BoxPtr dstBox,
    short src_w, short src_h,
    short drw_w, short drw_h
);
static void SavageDisplayVideoNew(
    ScrnInfoPtr pScrn, int id, int offset,
    short width, short height, int pitch, 
    int x1, int y1, int x2, int y2,
    BoxPtr dstBox,
    short src_w, short src_h,
    short drw_w, short drw_h
);
static void SavageDisplayVideo2000(
    ScrnInfoPtr pScrn, int id, int offset,
    short width, short height, int pitch, 
    int x1, int y1, int x2, int y2,
    BoxPtr dstBox,
    short src_w, short src_h,
    short drw_w, short drw_h
);
static void (*SavageDisplayVideo)(
    ScrnInfoPtr pScrn, int id, int offset,
    short width, short height, int pitch, 
    int x1, int y1, int x2, int y2,
    BoxPtr dstBox,
    short src_w, short src_h,
    short drw_w, short drw_h
) = NULL;

/*static void SavageBlockHandler(int, pointer, pointer, pointer);*/

#define XVTRACE	4

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

static Atom xvColorKey, xvBrightness, xvContrast, xvSaturation, xvHue, xvInterpolation;

/* client libraries expect an encoding */
static XF86VideoEncodingRec DummyEncoding[1] =
{
 {
   0,
   "XV_IMAGE",
   1024, 1024,
   {1, 1}
 }
};

#define NUM_FORMATS 5

static XF86VideoFormatRec Formats[NUM_FORMATS] = 
{
  {8, PseudoColor},  {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

#define NUM_ATTRIBUTES 6

static XF86AttributeRec Attributes[NUM_ATTRIBUTES] =
{
   {XvSettable | XvGettable, 0, (1 << 24) - 1, "XV_COLORKEY"},
   {XvSettable | XvGettable, -128, 127, "XV_BRIGHTNESS"},
   {XvSettable | XvGettable, 0, 255, "XV_CONTRAST"},
   {XvSettable | XvGettable, 0, 255, "XV_SATURATION"},
   {XvSettable | XvGettable, -180, 180, "XV_HUE"},
   {XvSettable | XvGettable, 0, 1, "XV_VERTICAL_INTERPOLATION"}
};

#define FOURCC_RV16	0x36315652
#define FOURCC_RV15	0x35315652
#define FOURCC_Y211	0x31313259

/*
 * For completeness sake, here is a cracking of the fourcc's I support.
 *
 * YUY2, packed 4:2:2, byte order: Y0 U0 Y1 V0  Y2 U2 Y3 V2
 * Y211, packed 2:1:1, byte order: Y0 U0 Y2 V0  Y4 U2 Y6 V2
 * YV12, planar 4:1:1, Y plane HxW, V plane H/2xW/2, U plane H/2xW/2
 * I420, planar 4:1:1, Y plane HxW, U plane H/2xW/2, V plane H/2xW/2
 * (I420 is also known as IYUV)
 */
  

static XF86ImageRec Images[] =
{
   XVIMAGE_YUY2,
   XVIMAGE_YV12,
   XVIMAGE_I420,
   {
	FOURCC_RV15,
        XvRGB,
	LSBFirst,
	{'R','V','1','5',
	  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	16,
	XvPacked,
	1,
	15, 0x001F, 0x03E0, 0x7C00,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	{'R','V','B',0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	XvTopToBottom
   },
   {
	FOURCC_RV16,
        XvRGB,
	LSBFirst,
	{'R','V','1','6',
	  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
	16,
	XvPacked,
	1,
	16, 0x001F, 0x07E0, 0xF800,
	0, 0, 0,
	0, 0, 0,
	0, 0, 0,
	{'R','V','B',0,
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	XvTopToBottom
   },
   {
	FOURCC_Y211,
	XvYUV,
	LSBFirst,
	{'Y','2','1','1',
	  0x00,0x00,0x00,0x10,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71},
	6,
	XvPacked,
	3,
	0, 0, 0, 0 ,
	8, 8, 8, 
	2, 4, 4,
	1, 1, 1,
	{'Y','U','Y','V',
	  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	XvTopToBottom
   }
};

#define NUM_IMAGES (sizeof(Images)/sizeof(Images[0]))

typedef struct {
   int		brightness;	/* -128 .. 127 */
   CARD32	contrast;	/* 0 .. 255 */
   CARD32	saturation;	/* 0 .. 255 */
   int		hue;		/* -128 .. 127 */
   Bool		interpolation; /* on/off */

   FBAreaPtr	area;
   RegionRec	clip;
   CARD32	colorKey;
   CARD32	videoStatus;
   Time		offTime;
   Time		freeTime;
   int		lastKnownPitch;
} SavagePortPrivRec, *SavagePortPrivPtr;


#define GET_PORT_PRIVATE(pScrn) \
   (SavagePortPrivPtr)((SAVPTR(pScrn))->adaptor->pPortPrivates[0].ptr)

static
unsigned int GetBlendForFourCC( int id )
{
    switch( id ) {
	case FOURCC_YUY2:
	case FOURCC_YV12:
	case FOURCC_I420:
	    return 1;
	case FOURCC_Y211:
	    return 4;
	case FOURCC_RV15:
	    return 3;
	case FOURCC_RV16:
	    return 5;
        default:
	    return 0;
    }
}

void savageOUTREG( SavagePtr psav, unsigned long offset, unsigned long value )
{
    ErrorF( "MMIO %08lx, was %08lx, want %08lx,", 
	offset, (CARD32)MMIO_IN32( psav->MapBase, offset ), value );
    MMIO_OUT32( psav->MapBase, offset, value );
    ErrorF( " now %08lx\n", (CARD32)MMIO_IN32( psav->MapBase, offset ) );
}

#if 0
static void
SavageClipVWindow(ScrnInfoPtr pScrn)
{
    SavagePtr psav = SAVPTR(pScrn);
  
    if( (psav->Chipset == S3_SAVAGE_MX)  ||
	(psav->Chipset == S3_SUPERSAVAGE) ) {
	if (psav->IsSecondary) {
	    OUTREG(SEC_STREAM2_WINDOW_SZ, 0);
	} else if (psav->IsPrimary) {
	    OUTREG(SEC_STREAM_WINDOW_SZ, 0);
	} else {
	    OUTREG(SEC_STREAM_WINDOW_SZ, 0);
#if 0
	    OUTREG(SEC_STREAM2_WINDOW_SZ, 0);
#endif
  	}
    } else if (psav->Chipset == S3_SAVAGE2000) {
        OUTREG(SEC_STREAM_WINDOW_SZ, 0);
    } else {
	OUTREG( SSTREAM_WINDOW_SIZE_REG, 1);
	OUTREG( SSTREAM_WINDOW_START_REG, 0x03ff03ff);
    }
}
#endif

void SavageInitVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    XF86VideoAdaptorPtr *adaptors, *newAdaptors = NULL;
    XF86VideoAdaptorPtr newAdaptor = NULL;
    SavagePtr psav = SAVPTR(pScrn);
    int num_adaptors;

    xf86ErrorFVerb(XVTRACE,"SavageInitVideo\n");
    if (S3_SAVAGE_MOBILE_SERIES(psav->Chipset))
    {
	newAdaptor = SavageSetupImageVideo(pScreen);
	SavageInitOffscreenImages(pScreen);

	SavageInitStreams = SavageInitStreamsNew;
	SavageSetColor = SavageSetColorNew;
	SavageSetColorKey = SavageSetColorKeyNew;
	SavageDisplayVideo = SavageDisplayVideoNew;
    }
    else if (psav->Chipset == S3_SAVAGE2000)
    {
        newAdaptor = SavageSetupImageVideo(pScreen);
        SavageInitOffscreenImages(pScreen);

	SavageInitStreams = SavageInitStreams2000;
        SavageSetColor = SavageSetColor2000;
        SavageSetColorKey = SavageSetColorKey2000;
        SavageDisplayVideo = SavageDisplayVideo2000;
    }
    else
    {
	newAdaptor = SavageSetupImageVideo(pScreen);
	SavageInitOffscreenImages(pScreen);

	SavageInitStreams = SavageInitStreamsOld;
	SavageSetColor = SavageSetColorOld;
	SavageSetColorKey = SavageSetColorKeyOld;
	SavageDisplayVideo = SavageDisplayVideoOld;
    }
 
    num_adaptors = xf86XVListGenericAdaptors(pScrn, &adaptors);
 
    if(newAdaptor) {
        if(!num_adaptors) {
            num_adaptors = 1;
            adaptors = &newAdaptor;
        } else {
            newAdaptors =  /* need to free this someplace */
        	xalloc((num_adaptors + 1) * sizeof(XF86VideoAdaptorPtr*));
            if(newAdaptors) {
        	memcpy(newAdaptors, adaptors, num_adaptors * 
        				sizeof(XF86VideoAdaptorPtr));
        	newAdaptors[num_adaptors] = newAdaptor;
        	adaptors = newAdaptors;
        	num_adaptors++;
            }
        }
    }
 
    if(num_adaptors)
        xf86XVScreenInit(pScreen, adaptors, num_adaptors);

    if(newAdaptors)
	xfree(newAdaptors);

    if( newAdaptor )
    {
	psav->videoFourCC = 0;
    }
}


void SavageSetColorKeyOld(ScrnInfoPtr pScrn)
{
    SavagePtr psav = SAVPTR(pScrn);
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;
    int red, green, blue;

    /* Here, we reset the colorkey and all the controls. */

    red = (pPriv->colorKey & pScrn->mask.red) >> pScrn->offset.red;
    green = (pPriv->colorKey & pScrn->mask.green) >> pScrn->offset.green;
    blue = (pPriv->colorKey & pScrn->mask.blue) >> pScrn->offset.blue;

    if( !pPriv->colorKey ) {
	OUTREG( COL_CHROMA_KEY_CONTROL_REG, 0 );
	OUTREG( CHROMA_KEY_UPPER_BOUND_REG, 0 );
	OUTREG( BLEND_CONTROL_REG, 0 );
    }
    else {
	switch (pScrn->depth) {
	case 8:
	    OUTREG( COL_CHROMA_KEY_CONTROL_REG,
		0x37000000 | (pPriv->colorKey & 0xFF) );
	    OUTREG( CHROMA_KEY_UPPER_BOUND_REG,
		0x00000000 | (pPriv->colorKey & 0xFF) );
	    break;
	case 15:
	    OUTREG( COL_CHROMA_KEY_CONTROL_REG, 
		0x05000000 | (red<<19) | (green<<11) | (blue<<3) );
	    OUTREG( CHROMA_KEY_UPPER_BOUND_REG, 
		0x00000000 | (red<<19) | (green<<11) | (blue<<3) );
	    break;
	case 16:
	    OUTREG( COL_CHROMA_KEY_CONTROL_REG, 
		0x16000000 | (red<<19) | (green<<10) | (blue<<3) );
	    OUTREG( CHROMA_KEY_UPPER_BOUND_REG, 
		0x00020002 | (red<<19) | (green<<10) | (blue<<3) );
	    break;
	case 24:
	    OUTREG( COL_CHROMA_KEY_CONTROL_REG, 
		0x17000000 | (red<<16) | (green<<8) | (blue) );
	    OUTREG( CHROMA_KEY_UPPER_BOUND_REG, 
		0x00000000 | (red<<16) | (green<<8) | (blue) );
	    break;
	}    

	/* We use destination colorkey */
	OUTREG( BLEND_CONTROL_REG, 0x05000000 );
    }
}

void SavageSetColorKeyNew(ScrnInfoPtr pScrn) 
{
    SavagePtr psav = SAVPTR(pScrn);
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;
    int red, green, blue;

    /* Here, we reset the colorkey and all the controls. */

    red = (pPriv->colorKey & pScrn->mask.red) >> pScrn->offset.red;
    green = (pPriv->colorKey & pScrn->mask.green) >> pScrn->offset.green;
    blue = (pPriv->colorKey & pScrn->mask.blue) >> pScrn->offset.blue;

    if( !pPriv->colorKey ) {
	if (psav->IsSecondary) {
	    OUTREG( SEC_STREAM2_CKEY_LOW, 0 );
	    OUTREG( SEC_STREAM2_CKEY_UPPER, 0 );
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 17) | (8 << 12) ));
	} else if (psav->IsPrimary) {
	    OUTREG( SEC_STREAM_CKEY_LOW, 0 );
	    OUTREG( SEC_STREAM_CKEY_UPPER, 0 );
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 9) | 0x08 ));
	} else {
	    OUTREG( SEC_STREAM_CKEY_LOW, 0 );
	    OUTREG( SEC_STREAM_CKEY_UPPER, 0 );
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 9) | 0x08 ));
#if 0
	    sleep(1);
	    OUTREG( SEC_STREAM2_CKEY_LOW, 0 );
	    OUTREG( SEC_STREAM2_CKEY_UPPER, 0 );
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 17) | (8 << 12) ));
#endif
	}
    }
    else {
	switch (pScrn->depth) {
	case 8:
	    if (psav->IsSecondary) {
	    	OUTREG( SEC_STREAM2_CKEY_LOW, 
		    0x47000000 | (pPriv->colorKey & 0xFF) );
	    	OUTREG( SEC_STREAM2_CKEY_UPPER,
		    0x47000000 | (pPriv->colorKey & 0xFF) );
	    } else if (psav->IsPrimary) {
	    	OUTREG( SEC_STREAM_CKEY_LOW, 
		    0x47000000 | (pPriv->colorKey & 0xFF) );
	    	OUTREG( SEC_STREAM_CKEY_UPPER,
		    0x47000000 | (pPriv->colorKey & 0xFF) );
	    } else {
	    	OUTREG( SEC_STREAM_CKEY_LOW, 
		    0x47000000 | (pPriv->colorKey & 0xFF) );
	    	OUTREG( SEC_STREAM_CKEY_UPPER,
		    0x47000000 | (pPriv->colorKey & 0xFF) );
#if 0
	    	OUTREG( SEC_STREAM2_CKEY_LOW, 
		    0x47000000 | (pPriv->colorKey & 0xFF) );
	    	OUTREG( SEC_STREAM2_CKEY_UPPER,
		    0x47000000 | (pPriv->colorKey & 0xFF) );
#endif
	    }
	    break;
	case 15:
	    if (psav->IsSecondary) {
	    	OUTREG( SEC_STREAM2_CKEY_LOW, 
		    0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
	    	OUTREG( SEC_STREAM2_CKEY_UPPER, 
		    0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
	    } else if (psav->IsPrimary) {
	    	OUTREG( SEC_STREAM_CKEY_LOW, 
		    0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
	    	OUTREG( SEC_STREAM_CKEY_UPPER, 
		    0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
	    } else {
	    	OUTREG( SEC_STREAM_CKEY_LOW, 
		    0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
	    	OUTREG( SEC_STREAM_CKEY_UPPER, 
		    0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
#if 0
	    	OUTREG( SEC_STREAM2_CKEY_LOW, 
		    0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
	    	OUTREG( SEC_STREAM2_CKEY_UPPER, 
		    0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
#endif
	    }
	    break;
	case 16:
	    if (psav->IsSecondary) {
	    	OUTREG( SEC_STREAM2_CKEY_LOW, 
		    0x46000000 | (red<<19) | (green<<10) | (blue<<3) );
	    	OUTREG( SEC_STREAM2_CKEY_UPPER, 
		    0x46020002 | (red<<19) | (green<<10) | (blue<<3) );
	    } else if (psav->IsPrimary) {
	    	OUTREG( SEC_STREAM_CKEY_LOW, 
		    0x46000000 | (red<<19) | (green<<10) | (blue<<3) );
	    	OUTREG( SEC_STREAM_CKEY_UPPER, 
		    0x46020002 | (red<<19) | (green<<10) | (blue<<3) );
	    } else {
	    	OUTREG( SEC_STREAM_CKEY_LOW, 
		    0x46000000 | (red<<19) | (green<<10) | (blue<<3) );
	    	OUTREG( SEC_STREAM_CKEY_UPPER, 
		    0x46020002 | (red<<19) | (green<<10) | (blue<<3) );
#if 0
	    	OUTREG( SEC_STREAM2_CKEY_LOW, 
		    0x46000000 | (red<<19) | (green<<10) | (blue<<3) );
	    	OUTREG( SEC_STREAM2_CKEY_UPPER, 
		    0x46020002 | (red<<19) | (green<<10) | (blue<<3) );
#endif
	    }
	    break;
	case 24:
	    if (psav->IsSecondary) {
	        OUTREG( SEC_STREAM2_CKEY_LOW, 
		    0x47000000 | (red<<16) | (green<<8) | (blue) );
	        OUTREG( SEC_STREAM2_CKEY_UPPER, 
		    0x47000000 | (red<<16) | (green<<8) | (blue) );
	    } else if (psav->IsPrimary) {
	        OUTREG( SEC_STREAM_CKEY_LOW, 
		    0x47000000 | (red<<16) | (green<<8) | (blue) );
	        OUTREG( SEC_STREAM_CKEY_UPPER, 
		    0x47000000 | (red<<16) | (green<<8) | (blue) );
	    } else {
	        OUTREG( SEC_STREAM_CKEY_LOW, 
		    0x47000000 | (red<<16) | (green<<8) | (blue) );
	        OUTREG( SEC_STREAM_CKEY_UPPER, 
		    0x47000000 | (red<<16) | (green<<8) | (blue) );
#if 0
	        OUTREG( SEC_STREAM2_CKEY_LOW, 
		    0x47000000 | (red<<16) | (green<<8) | (blue) );
	        OUTREG( SEC_STREAM2_CKEY_UPPER, 
		    0x47000000 | (red<<16) | (green<<8) | (blue) );
#endif
	    }
	    break;
	}    

	/* We assume destination colorkey */
	if (psav->IsSecondary) {
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 17) | (8 << 12) ));
	} else if (psav->IsPrimary) {
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 9) | 0x08 ));
	} else {
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 9) | 0x08 ));
#if 0
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 17) | (8 << 12) ));
#endif
	}
    }
}

void SavageSetColorKey2000(ScrnInfoPtr pScrn) 
{
    SavagePtr psav = SAVPTR(pScrn);
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;
    int red, green, blue;

    /* Here, we reset the colorkey and all the controls. */

    red = (pPriv->colorKey & pScrn->mask.red) >> pScrn->offset.red;
    green = (pPriv->colorKey & pScrn->mask.green) >> pScrn->offset.green;
    blue = (pPriv->colorKey & pScrn->mask.blue) >> pScrn->offset.blue;

    if( !pPriv->colorKey ) {
        OUTREG( SEC_STREAM_CKEY_LOW, 0 );
	OUTREG( SEC_STREAM_CKEY_UPPER, 0 );
        OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 9) | 0x08 ));
    }
    else {
	switch (pScrn->depth) {
	case 8:
	    OUTREG( SEC_STREAM_CKEY_LOW, 
		0x47000000 | (pPriv->colorKey & 0xFF) );
	    OUTREG( SEC_STREAM_CKEY_UPPER,
		0x47000000 | (pPriv->colorKey & 0xFF) );
	    break;
	case 15:
	    OUTREG( SEC_STREAM_CKEY_LOW, 
		0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
	    OUTREG( SEC_STREAM_CKEY_UPPER, 
		0x45000000 | (red<<19) | (green<<11) | (blue<<3) );
	    break;
	case 16:
	    OUTREG( SEC_STREAM_CKEY_LOW, 
		0x46000000 | (red<<19) | (green<<10) | (blue<<3) );
	    OUTREG( SEC_STREAM_CKEY_UPPER, 
		0x46020002 | (red<<19) | (green<<10) | (blue<<3) );
	    break;
	case 24:
	    OUTREG( SEC_STREAM_CKEY_LOW, 
		0x47000000 | (red<<16) | (green<<8) | (blue) );
	    OUTREG( SEC_STREAM_CKEY_UPPER, 
		0x47000000 | (red<<16) | (green<<8) | (blue) );
	    break;
	}    

	/* We assume destination colorkey */
	OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 9) | 0x08 ));
    }
}

void SavageSetColorOld( ScrnInfoPtr pScrn )
{
    SavagePtr psav = SAVPTR(pScrn);
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;

    xf86ErrorFVerb(XVTRACE, "bright %d, contrast %d, saturation %d, hue %d\n",
	pPriv->brightness, (int)pPriv->contrast, (int)pPriv->saturation, pPriv->hue );

    if( 
	(psav->videoFourCC == FOURCC_RV15) ||
	(psav->videoFourCC == FOURCC_RV16)
    )
    {
	OUTREG( COLOR_ADJUSTMENT_REG, 0 );
    }
    else
    {
        /* Change 0..255 into 0..15 */
	long sat = pPriv->saturation * 16 / 256;
	double hue = pPriv->hue * 0.017453292;
	unsigned long hs1 = ((long)(sat * cos(hue))) & 0x1f;
	unsigned long hs2 = ((long)(sat * sin(hue))) & 0x1f;

	OUTREG( COLOR_ADJUSTMENT_REG, 
	    0x80008000 |
	    (pPriv->brightness + 128) |
	    ((pPriv->contrast & 0xf8) << (12-7)) | 
	    (hs1 << 16) |
	    (hs2 << 24)
	);

    }
}

void SavageSetColorNew( ScrnInfoPtr pScrn )
{
    SavagePtr psav = SAVPTR(pScrn);
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;

    /* Brightness/contrast/saturation/hue computations. */

    double k, dk1, dk2, dk3, dk4, dk5, dk6, dk7, dkb;
    int k1, k2, k3, k4, k5, k6, k7, kb;
    double s = pPriv->saturation / 128.0;
    double h = pPriv->hue * 0.017453292;
    unsigned long assembly1, assembly2, assembly3;

    xf86ErrorFVerb(XVTRACE, "bright %d, contrast %d, saturation %d, hue %d\n",
	pPriv->brightness, (int)pPriv->contrast, (int)pPriv->saturation, pPriv->hue );

    if( psav->videoFourCC == FOURCC_Y211 )
	k = 1.0;	/* YUV */
    else
	k = 1.14;	/* YCrCb */

    /*
     * The S3 documentation must be wrong for k4 and k5.  Their default
     * values, which they hardcode in their Windows driver, have the
     * opposite sign from the results in the register spec.
     */

    dk1 = k * pPriv->contrast;
    dk2 = 64.0 * 1.371 * k * s * cos(h);
    dk3 = -64.0 * 1.371 * k * s * sin(h);
    dk4 = -128.0 * k * s * (0.698 * cos(h) - 0.336 * sin(h));
    dk5 = -128.0 * k * s * (0.698 * sin(h) + 0.336 * cos(h));
    dk6 = 64.0 * 1.732 * k * s * sin(h);	/* == k3 / 1.26331, right? */
    dk7 = 64.0 * 1.732 * k * s * cos(h);	/* == k2 / -1.26331, right? */
    dkb = 128.0 * pPriv->brightness + 64.0;
    if( psav->videoFourCC != FOURCC_Y211 )
	dkb -= dk1 * 14.0;

    k1 = (int)(dk1+0.5) & 0x1ff;
    k2 = (int)(dk2+0.5) & 0x1ff;
    k3 = (int)(dk3+0.5) & 0x1ff;
    assembly1 = (k3<<18) | (k2<<9) | k1;
    xf86ErrorFVerb(XVTRACE+1, "CC1 = %08lx  ", assembly1 );

    k4 = (int)(dk4+0.5) & 0x1ff;
    k5 = (int)(dk5+0.5) & 0x1ff;
    k6 = (int)(dk6+0.5) & 0x1ff;
    assembly2 = (k6<<18) | (k5<<9) | k4;
    xf86ErrorFVerb(XVTRACE+1, "CC2 = %08lx  ", assembly2 );

    k7 = (int)(dk7+0.5) & 0x1ff;
    kb = (int)(dkb+0.5) & 0xffff;
    assembly3 = (kb<<9) | k7;
    xf86ErrorFVerb(XVTRACE+1, "CC3 = %08lx\n", assembly3 );

    if (psav->IsSecondary) {
	OUTREG( SEC_STREAM2_COLOR_CONVERT1, assembly1 );
	OUTREG( SEC_STREAM2_COLOR_CONVERT2, assembly2 );
	OUTREG( SEC_STREAM2_COLOR_CONVERT3, assembly3 );
    } else if (psav->IsPrimary) {
	OUTREG( SEC_STREAM_COLOR_CONVERT3, assembly1 );
	OUTREG( SEC_STREAM_COLOR_CONVERT3, assembly2 );
	OUTREG( SEC_STREAM_COLOR_CONVERT3, assembly3 );
    } else {
	OUTREG( SEC_STREAM_COLOR_CONVERT3, assembly1 );
	OUTREG( SEC_STREAM_COLOR_CONVERT3, assembly2 );
	OUTREG( SEC_STREAM_COLOR_CONVERT3, assembly3 );
#if 0
	sleep(1);
	OUTREG( SEC_STREAM2_COLOR_CONVERT1, assembly1 );
	OUTREG( SEC_STREAM2_COLOR_CONVERT2, assembly2 );
	OUTREG( SEC_STREAM2_COLOR_CONVERT3, assembly3 );
#endif
    }
}

void SavageSetColor2000( ScrnInfoPtr pScrn )
{
    SavagePtr psav = SAVPTR(pScrn);
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;

    /* Brightness/contrast/saturation/hue computations. */

    double k, dk1, dk2, dk3, dk4, dk5, dk6, dk7, dkb;
    int k1, k2, k3, k4, k5, k6, k7, kb;
    double s = pPriv->saturation / 128.0;
    double h = pPriv->hue * 0.017453292;
    unsigned long assembly1, assembly2, assembly3, assembly4;

    xf86ErrorFVerb(XVTRACE, "bright %d, contrast %d, saturation %d, hue %d\n",
		 pPriv->brightness, (int)pPriv->contrast, (int)pPriv->saturation, pPriv->hue );

    if( psav->videoFourCC == FOURCC_Y211 )
      k = 1.0;/* YUV */
    else
      k = 1.14;/* YCrCb */


    dk1 = 128 * k * pPriv->contrast;
    dk2 = 64.0 * 1.371 * k * s * cos(h);
    dk3 = -64.0 * 1.371 * k * s * sin(h);
    dk4 = -128.0 * k * s * (0.698 * cos(h) - 0.336 * sin(h));
    dk5 = -128.0 * k * s * (0.698 * sin(h) + 0.336 * cos(h));
    dk6 = 64.0 * 1.732 * k * s * sin(h);/* == k3 / 1.26331, right? */
    dk7 = 64.0 * 1.732 * k * s * cos(h);/* == k2 / -1.26331, right? */
    dkb = 128.0 * pPriv->brightness + 0.5;
    if( psav->videoFourCC != FOURCC_Y211 )
      dkb = 128 * (pPriv->brightness * k * pPriv->contrast + 0.5); 

    k1 = (int)(dk1 /*+0.5*/) & 0x1ff;
    k2 = (int)(dk2 /*+0.5*/) & 0x1ff;
    assembly1 = (k2<<16) | k1;

    k3 = (int)(dk3 /*+0.5*/) & 0x1ff;
    k4 = (int)(dk4 /*+0.5*/) & 0x1ff;
    assembly2 = (k4<<16) | k3;

    k5 = (int)(dk5 /*+0.5*/) & 0x1ff;
    k6 = (int)(dk6 /*+0.5*/) & 0x1ff;
    assembly3 = (k6<<16) | k5;

    k7 = (int)(dk7 /*+0.5*/) & 0x1ff;
    kb = (int)(dkb /*+0.5*/) & 0xffff;
    assembly4 = (kb<<16) | k7;

    OUTREG( SEC_STREAM_COLOR_CONVERT0_2000, assembly1 );
    OUTREG( SEC_STREAM_COLOR_CONVERT1_2000, assembly2 );
    OUTREG( SEC_STREAM_COLOR_CONVERT2_2000, assembly3 );
    OUTREG( SEC_STREAM_COLOR_CONVERT3_2000, assembly4 );

}

void SavageResetVideo(ScrnInfoPtr pScrn) 
{
    xf86ErrorFVerb(XVTRACE,"SavageResetVideo\n");
    SavageSetColor( pScrn );
    SavageSetColorKey( pScrn );
}


static XF86VideoAdaptorPtr 
SavageSetupImageVideo(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    SavagePtr psav = SAVPTR(pScrn);
    XF86VideoAdaptorPtr adapt;
    SavagePortPrivPtr pPriv;

    xf86ErrorFVerb(XVTRACE,"SavageSetupImageVideo\n");

    if(!(adapt = xcalloc(1, sizeof(XF86VideoAdaptorRec) +
			    sizeof(SavagePortPrivRec) +
			    sizeof(DevUnion))))
	return NULL;

    adapt->type		= XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags	= VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
    adapt->name			= "Savage Streams Engine";
    adapt->nEncodings 		= 1;
    adapt->pEncodings 		= DummyEncoding;
    adapt->nFormats 		= NUM_FORMATS;
    adapt->pFormats 		= Formats;
    adapt->nPorts 		= 1;
    adapt->pPortPrivates = (DevUnion*)(&adapt[1]);
    pPriv = (SavagePortPrivPtr)(&adapt->pPortPrivates[1]);
    adapt->pPortPrivates[0].ptr	= (pointer)(pPriv);
    adapt->pAttributes		= Attributes;
    adapt->nImages		= NUM_IMAGES;
    adapt->nAttributes		= NUM_ATTRIBUTES;
    adapt->pImages		= Images;
    adapt->PutVideo		= NULL;
    adapt->PutStill		= NULL;
    adapt->GetVideo		= NULL;
    adapt->GetStill		= NULL;
    adapt->StopVideo		= SavageStopVideo;
    adapt->SetPortAttribute	= SavageSetPortAttribute;
    adapt->GetPortAttribute	= SavageGetPortAttribute;
    adapt->QueryBestSize	= SavageQueryBestSize;
    adapt->PutImage		= SavagePutImage;
    adapt->QueryImageAttributes	= SavageQueryImageAttributes;

    xvBrightness = MAKE_ATOM("XV_BRIGHTNESS");
    xvContrast   = MAKE_ATOM("XV_CONTRAST");
    xvColorKey   = MAKE_ATOM("XV_COLORKEY");
    xvHue        = MAKE_ATOM("XV_HUE");
    xvSaturation = MAKE_ATOM("XV_SATURATION");
    /* interpolation option only available on "old" streams */
    xvInterpolation = MAKE_ATOM("XV_VERTICAL_INTERPOLATION");

    pPriv->colorKey = 
      (1 << pScrn->offset.red) | 
      (1 << pScrn->offset.green) |
      (((pScrn->mask.blue >> pScrn->offset.blue) - 1) << pScrn->offset.blue); 
    pPriv->videoStatus = 0;
    pPriv->brightness = 0;
    pPriv->contrast = 128;
    pPriv->saturation = 128;
#if 0
    /* 
     * The S3 driver has these values for some of the chips.  I have yet
     * to find any Savage where these make sense.
     */
    pPriv->brightness = 64;
    pPriv->contrast = 16;
    pPriv->saturation = 128;
#endif
    pPriv->hue = 0;
    pPriv->lastKnownPitch = 0;

    pPriv->interpolation = FALSE;

    /* gotta uninit this someplace */
    REGION_NULL(pScreen, &pPriv->clip);

    psav->adaptor = adapt;

    #if 0
    psav->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = SavageBlockHandler;
    #endif

    return adapt;
}


/* SavageClipVideo -  

   Takes the dst box in standard X BoxRec form (top and left
   edges inclusive, bottom and right exclusive).  The new dst
   box is returned.  The source boundaries are given (x1, y1 
   inclusive, x2, y2 exclusive) and returned are the new source 
   boundaries in 16.16 fixed point.
*/

static void
SavageClipVideo(
  BoxPtr dst, 
  INT32 *x1, 
  INT32 *x2, 
  INT32 *y1, 
  INT32 *y2,
  BoxPtr extents,            /* extents of the clip region */
  INT32 width, 
  INT32 height
){
    INT32 vscale, hscale, delta;
    int diff;

    hscale = ((*x2 - *x1) << 16) / (dst->x2 - dst->x1);
    vscale = ((*y2 - *y1) << 16) / (dst->y2 - dst->y1);

    *x1 <<= 16; *x2 <<= 16;
    *y1 <<= 16; *y2 <<= 16;

    diff = extents->x1 - dst->x1;
    if(diff > 0) {
	dst->x1 = extents->x1;
	*x1 += diff * hscale;     
    }
    diff = dst->x2 - extents->x2;
    if(diff > 0) {
	dst->x2 = extents->x2;
	*x2 -= diff * hscale;     
    }
    diff = extents->y1 - dst->y1;
    if(diff > 0) {
	dst->y1 = extents->y1;
	*y1 += diff * vscale;     
    }
    diff = dst->y2 - extents->y2;
    if(diff > 0) {
	dst->y2 = extents->y2;
	*y2 -= diff * vscale;     
    }

    if(*x1 < 0) {
	diff =  (- *x1 + hscale - 1)/ hscale;
	dst->x1 += diff;
	*x1 += diff * hscale;
    }
    delta = *x2 - (width << 16);
    if(delta > 0) {
	diff = (delta + hscale - 1)/ hscale;
	dst->x2 -= diff;
	*x2 -= diff * hscale;
    }
    if(*y1 < 0) {
	diff =  (- *y1 + vscale - 1)/ vscale;
	dst->y1 += diff;
	*y1 += diff * vscale;
    }
    delta = *y2 - (height << 16);
    if(delta > 0) {
	diff = (delta + vscale - 1)/ vscale;
	dst->y2 -= diff;
	*y2 -= diff * vscale;
    }
} 

static void 
SavageStopVideo(ScrnInfoPtr pScrn, pointer data, Bool shutdown)
{
    SavagePortPrivPtr pPriv = (SavagePortPrivPtr)data;
    /*SavagePtr psav = SAVPTR(pScrn); */

    xf86ErrorFVerb(XVTRACE,"SavageStopVideo\n");

    REGION_EMPTY(pScrn->pScreen, &pPriv->clip);   

    if(shutdown) {
      /*SavageClipVWindow(pScrn);*/
 	SavageStreamsOff( pScrn );
	if(pPriv->area) {
	    xf86FreeOffscreenArea(pPriv->area);
	    pPriv->area = NULL;
	}
	pPriv->videoStatus = 0;
    } else {
	if(pPriv->videoStatus & CLIENT_VIDEO_ON) {
	    pPriv->videoStatus |= OFF_TIMER;
	    pPriv->offTime = currentTime.milliseconds + OFF_DELAY;
	}
    }
}


static int 
SavageSetPortAttribute(
    ScrnInfoPtr pScrn, 
    Atom attribute,
    INT32 value, 
    pointer data
){
    SavagePortPrivPtr pPriv = (SavagePortPrivPtr)data;
    SavagePtr psav = SAVPTR(pScrn);

    if(attribute == xvColorKey) {
	pPriv->colorKey = value;
	if( psav->videoFlags & VF_STREAMS_ON)
	    SavageSetColorKey( pScrn );
	REGION_EMPTY(pScrn->pScreen, &pPriv->clip);   
    } 
    else if( attribute == xvBrightness) {
	if((value < -128) || (value > 127))
	    return BadValue;
	pPriv->brightness = value;
	if( psav->videoFlags & VF_STREAMS_ON)
	    SavageSetColor( pScrn );
    }
    else if( attribute == xvContrast) {
	if((value < 0) || (value > 255))
	    return BadValue;
	pPriv->contrast = value;
	if( psav->videoFlags & VF_STREAMS_ON)
	    SavageSetColor( pScrn );
    }
    else if( attribute == xvSaturation) {
	if((value < 0) || (value > 255))
	    return BadValue;
	pPriv->saturation = value;
	if( psav->videoFlags & VF_STREAMS_ON)
	    SavageSetColor( pScrn );
    }
    else if( attribute == xvHue) {
	if((value < -180) || (value > 180))
	    return BadValue;
	pPriv->hue = value;
	if( psav->videoFlags & VF_STREAMS_ON)
	    SavageSetColor( pScrn );
    }
    else if( attribute == xvInterpolation) {
        if((value < 0) || (value > 1))
            return BadValue;
        if (value == 1)
            pPriv->interpolation = TRUE;
	else
	    pPriv->interpolation = FALSE;
    }
    else
	return BadMatch;

    return Success;
}


static int 
SavageGetPortAttribute(
  ScrnInfoPtr pScrn, 
  Atom attribute,
  INT32 *value, 
  pointer data
){
    SavagePortPrivPtr pPriv = (SavagePortPrivPtr)data;

    if(attribute == xvColorKey) {
	*value = pPriv->colorKey;
    }
    else if( attribute == xvBrightness ) {
	*value = pPriv->brightness;
    }
    else if( attribute == xvContrast ) {
	*value = pPriv->contrast;
    }
    else if( attribute == xvHue ) {
	*value = pPriv->hue;
    }
    else if( attribute == xvSaturation ) {
	*value = pPriv->saturation;
    }
    else if( attribute == xvInterpolation ) {
        *value = pPriv->interpolation;
    }
    else return BadMatch;

    return Success;
}

static void 
SavageQueryBestSize(
  ScrnInfoPtr pScrn, 
  Bool motion,
  short vid_w, short vid_h, 
  short drw_w, short drw_h, 
  unsigned int *p_w, unsigned int *p_h, 
  pointer data
){
    /* What are the real limits for the Savage? */

    *p_w = drw_w;
    *p_h = drw_h; 

    if(*p_w > 16384) *p_w = 16384;
}

/* SavageCopyPlanarDataBCI() causes artifacts on the screen when used on savage4. 
 * It's probably something with the BCI.  Maybe we need a waitforidle() or
 * something...
 */
static void
SavageCopyPlanarDataBCI(
    ScrnInfoPtr pScrn,
    unsigned char *srcY, /* Y */
    unsigned char *srcV, /* V */
    unsigned char *srcU, /* U */
    unsigned char *dst,
    int srcPitch, int srcPitch2,
    int dstPitch,
    int h,int w)
{
    SavagePtr psav = SAVPTR(pScrn);
    /* half of the dest buffer for copying the YVU data to it ??? */
    unsigned char *dstCopy = (unsigned char *)(((unsigned long)dst
                                                + 2 * srcPitch * h
                                                + 0x0f) & ~0x0f);
    /* for pixel transfer */
    unsigned long offsetY = (unsigned long)dstCopy - (unsigned long)psav->FBBase;
    unsigned long offsetV = offsetY +  srcPitch * h;
    unsigned long offsetU = offsetV +  srcPitch2 * (h>>1);
    unsigned long dstOffset  = (unsigned long)dst - (unsigned long)psav->FBBase;
    int i;
    
    BCI_GET_PTR;

    /* copy Y planar */
    for (i=0;i<srcPitch * h;i++) {
        dstCopy[i] = srcY[i];
    }

    /* copy V planar */    
    dstCopy = dstCopy + srcPitch * h;
    for (i=0;i<srcPitch2 * (h>>1);i++) {
        dstCopy[i] = srcV[i];
    }

    /* copy U planar */
    dstCopy = dstCopy + srcPitch2 * (h>>1);    
    for (i=0;i<srcPitch2 * (h>>1);i++) {
        dstCopy[i] = srcU[i];        
    }

    /*
     * Transfer pixel data from one memory location to another location
     * and reformat the data during the transfer
     * a. program BCI51 to specify the source information
     * b. program BCI52 to specify the destination information
     * c. program BCI53 to specify the source dimensions 
     * d. program BCI54 to specify the destination dimensions
     * e. (if the data is in YCbCr420 format)program BCI55,BCI56,BCI57 to
     *    locations of the Y,Cb,and Cr data
     * f. program BCI50(command=011) to specify the formatting options and
     *    kick off the transfer
     * this command can be used for color space conversion(YCbCr to RGB)
     * or for oversampling, but not for both simultaneously. it can also be
     * used to do mastered image transfer when the source is tiled
     */

    w = (w+0xf)&0xff0;
    psav->WaitQueue(psav,11);
    BCI_SEND(0x96070051);
    BCI_SEND(offsetY);

    BCI_SEND(dstOffset);

    BCI_SEND(((h-1)<<16)|((w-1)>>3));

    BCI_SEND(dstPitch >> 3);


    BCI_SEND(offsetU);
    BCI_SEND(offsetV);

    BCI_SEND((srcPitch2 << 16)| srcPitch2);

    BCI_SEND(0x96010050);
    BCI_SEND(0x00200003 | srcPitch);
    BCI_SEND(0xC0170000);
}

static void
SavageCopyData(
  unsigned char *src,
  unsigned char *dst,
  int srcPitch,
  int dstPitch,
  int h,
  int w
){
    w <<= 1;
    while(h--) {
	memcpy(dst, src, w);
	src += srcPitch;
	dst += dstPitch;
    }
}

static void
SavageCopyPlanarData(
   unsigned char *src1, /* Y */
   unsigned char *src2, /* V */
   unsigned char *src3, /* U */
   unsigned char *dst1,
   int srcPitch,
   int srcPitch2,
   int dstPitch,
   int h,
   int w
){
   CARD32 *dst = (CARD32*)dst1;
   int i, j;

   dstPitch >>= 2;
   w >>= 1;

   for(j = 0; j < h; j++) {
	for(i = 0; i < w; i++) {
/* Shouldn't this be 'if LITTLEENDIAN'? */
#if 1
	    dst[i] = src1[i << 1] | (src1[(i << 1) + 1] << 16) |
		     (src3[i] << 8) | (src2[i] << 24);
#else
	    dst[i] = (src1[i << 1] << 24) | (src1[(i << 1) + 1] << 8) |
		     (src3[i] << 0) | (src2[i] << 16);
#endif
	}
	dst += dstPitch;
	src1 += srcPitch;
	if(j & 1) {
	    src2 += srcPitch2;
	    src3 += srcPitch2;
	}
   }
}

static FBAreaPtr
SavageAllocateMemory(
    ScrnInfoPtr pScrn,
    FBAreaPtr area,
    int numlines
){
    ScreenPtr pScreen;
    FBAreaPtr new_area;

    if(area) {
	if((area->box.y2 - area->box.y1) >= numlines) 
	   return area;
        
        if(xf86ResizeOffscreenArea(area, pScrn->displayWidth, numlines))
	   return area;

	xf86FreeOffscreenArea(area);
    }

    pScreen = screenInfo.screens[pScrn->scrnIndex];

    xf86PurgeUnlockedOffscreenAreas(pScreen);
    new_area = xf86AllocateOffscreenArea(pScreen, pScrn->displayWidth, 
				numlines, 0, NULL, NULL, NULL);

    if(!new_area) {
	int max_w, max_h;

	xf86QueryLargestOffscreenArea(pScreen, &max_w, &max_h, 0,
			FAVOR_WIDTH_THEN_AREA, PRIORITY_EXTREME);
	
	if((max_w < pScrn->displayWidth) || (max_h < numlines))
	   return NULL;

	xf86PurgeUnlockedOffscreenAreas(pScreen);
	new_area = xf86AllocateOffscreenArea(pScreen, pScrn->displayWidth, 
				numlines, 0, NULL, NULL, NULL);
    }

    return new_area;
}

static void
SavageSetBlend(ScrnInfoPtr pScrn, int id)
{
    SavagePtr psav = SAVPTR(pScrn);

    if( S3_SAVAGE_MOBILE_SERIES(psav->Chipset) ||
        (psav->Chipset == S3_SAVAGE2000) )
    {
	psav->blendBase = GetBlendForFourCC( id );
	xf86ErrorFVerb(XVTRACE+1,"Format %4.4s, blend is %08x\n", (char*)&id, psav->blendBase );
	if (psav->IsSecondary) {
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 17) | (8 << 12) ));
	} else if (psav->IsPrimary) {
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 9) | 0x08 ));
	} else {
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 9) | 0x08 ));
#if 0
	    OUTREG( BLEND_CONTROL, (INREG32(BLEND_CONTROL) | (psav->blendBase << 17) | (8 << 12) ));
#endif
	}
    }
    psav->videoFourCC = id;
}

static void
SavageDisplayVideoOld(
    ScrnInfoPtr pScrn,
    int id,
    int offset,
    short width, short height,
    int pitch, 
    int x1, int y1, int x2, int y2,
    BoxPtr dstBox,
    short src_w, short src_h,
    short drw_w, short drw_h
){
    SavagePtr psav = SAVPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;
    /*DisplayModePtr mode = pScrn->currentMode;*/
    int vgaCRIndex, vgaCRReg, vgaIOBase;
    CARD32 ssControl;
    int scalratio;


    vgaIOBase = hwp->IOBase;
    vgaCRIndex = vgaIOBase + 4;
    vgaCRReg = vgaIOBase + 5;
#if 0
    if ( psav->videoFourCC != id ) {
	SavageSetBlend(pScrn,id);
	SavageResetVideo(pScrn);
    }
#endif
    if( psav->videoFourCC != id )
      SavageStreamsOff(pScrn);

    if( !psav->videoFlags & VF_STREAMS_ON )
      {
        SavageSetBlend(pScrn,id);
	SavageStreamsOn(pScrn);
	SavageResetVideo(pScrn);
      }

    if (S3_MOBILE_TWISTER_SERIES(psav->Chipset)
        && psav->FPExpansion) {
        drw_w = (((float)(drw_w * psav->XExp1)/(float)psav->XExp2)+1);
        drw_h = (float)(drw_h * psav->YExp1)/(float)psav->YExp2+1;
        dstBox->x1 = (float)(dstBox->x1 * psav->XExp1)/(float)psav->XExp2;
        dstBox->y1 = (float)(dstBox->y1 * psav->YExp1)/(float)psav->YExp2;

        dstBox->x1 += psav->displayXoffset;
        dstBox->y1 += psav->displayYoffset;
    }

    /*
     * Process horizontal scaling
     *  upscaling and downscaling smaller than 2:1 controled by MM8198
     *  MM8190 controls downscaling mode larger than 2:1
     */
    scalratio = 0;
    ssControl = 0;

    if (src_w >= (drw_w * 2)) {
        if (src_w < (drw_w * 4)) {
            scalratio = HSCALING(2,1);
        } else if (src_w < (drw_w * 8)) {
            ssControl |= HDSCALE_4;
        } else if (src_w < (drw_w * 16)) {
            ssControl |= HDSCALE_8;
        } else if (src_w < (drw_w * 32)) {
            ssControl |= HDSCALE_16;
        }  else if (src_w < (drw_w * 64)) {
            ssControl |= HDSCALE_32;
        } else
            ssControl |= HDSCALE_64;
    } else 
        scalratio = HSCALING(src_w,drw_w);

    ssControl |= src_w;
    /*ssControl |= (1 << 24);*/
    ssControl |= (GetBlendForFourCC(psav->videoFourCC) << 24);
    /* Wait for VBLANK. */
    VerticalRetraceWait();
    OUTREG(SSTREAM_CONTROL_REG, ssControl);
    if (scalratio)
        OUTREG(SSTREAM_STRETCH_REG,scalratio);

    /* Calculate vertical scale factor. */
    OUTREG(SSTREAM_VINITIAL_REG, 0 );
    /*OUTREG(SSTREAM_VSCALE_REG, (src_h << 15) / drw_h );*/
    OUTREG(SSTREAM_VSCALE_REG, VSCALING(src_h,drw_h));

    /* Set surface location and stride. */
    OUTREG(SSTREAM_FBADDR0_REG, (offset + (x1>>15)) & (0x1ffffff & ~BASE_PAD) );
    OUTREG(SSTREAM_FBADDR1_REG, 0);
    OUTREG(SSTREAM_STRIDE_REG, pitch & 0xfff );
                                                                             
    OUTREG(SSTREAM_WINDOW_START_REG, OS_XY(dstBox->x1, dstBox->y1) );
    OUTREG(SSTREAM_WINDOW_SIZE_REG, OS_WH(dstBox->x2-dstBox->x1,
                                          dstBox->y2-dstBox->y1));

    /*
     * MM81E8:Secondary Stream Source Line Count
     *   bit_0~10: # of lines in the source image (before scaling)
     *   bit_15 = 1: Enable vertical interpolation
     *            0: Line duplicaion
     */
    /* 
     * Vertical Interpolation is very bandwidth intensive.  Some savages can't
     * seem to handle it.  Default is line doubling.  --AGD
     */
    if (pPriv->interpolation) {
        OUTREG(SSTREAM_LINES_REG, 0x8000 | src_h );
    } else {
        OUTREG(SSTREAM_LINES_REG, src_h );
    }

#if 0
    /* Set color key on primary. */

    SavageSetColorKey( pScrn );
#endif

    /* Set FIFO L2 on second stream. */

    if( pPriv->lastKnownPitch != pitch )
    {
	unsigned char cr92;

	pPriv->lastKnownPitch = pitch;

	pitch = (pitch + 7) / 8;
	VGAOUT8(vgaCRIndex, 0x92);
	cr92 = VGAIN8(vgaCRReg);
	VGAOUT8(vgaCRReg, (cr92 & 0x40) | (pitch >> 8) | 0x80);
	VGAOUT8(vgaCRIndex, 0x93);
	VGAOUT8(vgaCRReg, pitch);
    }
}

static void
SavageDisplayVideoNew(
    ScrnInfoPtr pScrn,
    int id,
    int offset,
    short width, short height,
    int pitch, 
    int x1, int y1, int x2, int y2,
    BoxPtr dstBox,
    short src_w, short src_h,
    short drw_w, short drw_h
){
    SavagePtr psav = SAVPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    /*DisplayModePtr mode = pScrn->currentMode;*/
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;
    int vgaCRIndex, vgaCRReg, vgaIOBase;


    vgaIOBase = hwp->IOBase;
    vgaCRIndex = vgaIOBase + 4;
    vgaCRReg = vgaIOBase + 5;
#if 0
    if ( psav->videoFourCC != id ) {
	SavageSetBlend(pScrn,id);
	SavageResetVideo(pScrn);
    }
#endif
    if( psav->videoFourCC != id )
      SavageStreamsOff(pScrn);

    if( !psav->videoFlags & VF_STREAMS_ON )
      {
	SavageSetBlend(pScrn,id);
	SavageStreamsOn(pScrn);
	SavageResetVideo(pScrn);
      }

    /* Calculate horizontal and vertical scale factors. */

    if ( S3_SAVAGE_MOBILE_SERIES(psav->Chipset) &&
	    (psav->DisplayType == MT_LCD) &&
	    !psav->CrtOnly &&
	    !psav->TvOn) 
    {
	drw_w = (drw_w * psav->XExp1)/psav->XExp2 + 1;
	drw_h = (drw_h * psav->YExp1)/psav->YExp2 + 1;
	dstBox->x1 = (dstBox->x1 * psav->XExp1)/psav->XExp2;
	dstBox->y1 = (dstBox->y1 * psav->YExp1)/psav->YExp2;
	dstBox->x1 += psav->displayXoffset;
	dstBox->y1 += psav->displayYoffset;
    }

	if (psav->IsSecondary) {
	    OUTREG(SEC_STREAM2_HSCALING, 
	        ((src_w&0xfff)<<20) | ((65536 * src_w / drw_w) & 0x1FFFF ));
	    /* BUGBUG need to add 00040000 if src stride > 2048 */
	    OUTREG(SEC_STREAM2_VSCALING, 
	        ((src_h&0xfff)<<20) | ((65536 * src_h / drw_h) & 0x1FFFF ));
	} else if (psav->IsPrimary) {
	    OUTREG(SEC_STREAM_HSCALING, 
	        ((src_w&0xfff)<<20) | ((65536 * src_w / drw_w) & 0x1FFFF ));
	    /* BUGBUG need to add 00040000 if src stride > 2048 */
	    OUTREG(SEC_STREAM_VSCALING, 
	        ((src_h&0xfff)<<20) | ((65536 * src_h / drw_h) & 0x1FFFF ));
	} else {
	    OUTREG(SEC_STREAM_HSCALING, 
	        ((src_w&0xfff)<<20) | ((65536 * src_w / drw_w) & 0x1FFFF ));
	    /* BUGBUG need to add 00040000 if src stride > 2048 */
	    OUTREG(SEC_STREAM_VSCALING, 
	        ((src_h&0xfff)<<20) | ((65536 * src_h / drw_h) & 0x1FFFF ));
#if 0
	    OUTREG(SEC_STREAM2_HSCALING, 
	        ((src_w&0xfff)<<20) | ((65536 * src_w / drw_w) & 0x1FFFF ));
	    /* BUGBUG need to add 00040000 if src stride > 2048 */
	    OUTREG(SEC_STREAM2_VSCALING, 
	        ((src_h&0xfff)<<20) | ((65536 * src_h / drw_h) & 0x1FFFF ));
#endif
	}

    /*
     * Set surface location and stride.  We use x1>>15 because all surfaces
     * are 2 bytes/pixel.
     */

    if (psav->IsSecondary) {
        OUTREG(SEC_STREAM2_FBUF_ADDR0, (offset + (x1>>15)) 
	   & (0x7ffffff & ~BASE_PAD));
        OUTREG(SEC_STREAM2_STRIDE_LPB, pitch & 0xfff );
        OUTREG(SEC_STREAM2_WINDOW_START, ((dstBox->x1+1) << 16) | (dstBox->y1+1) );
        OUTREG(SEC_STREAM2_WINDOW_SZ, ((dstBox->x2-dstBox->x1) << 16) 
	   | (dstBox->x2-dstBox->x1) );
    } else if (psav->IsPrimary) {
        OUTREG(SEC_STREAM_FBUF_ADDR0, (offset + (x1>>15)) 
	   & (0x7ffffff & ~BASE_PAD));
        OUTREG(SEC_STREAM_STRIDE, pitch & 0xfff );
        OUTREG(SEC_STREAM_WINDOW_START, ((dstBox->x1+1) << 16) | (dstBox->y1+1) );
        OUTREG(SEC_STREAM_WINDOW_SZ, ((dstBox->x2-dstBox->x1) << 16) 
	   | (dstBox->x2-dstBox->x1) );
    } else {
        OUTREG(SEC_STREAM_FBUF_ADDR0, (offset + (x1>>15)) 
	   & (0x7ffffff & ~BASE_PAD));
        OUTREG(SEC_STREAM_STRIDE, pitch & 0xfff );
        OUTREG(SEC_STREAM_WINDOW_START, ((dstBox->x1+1) << 16) | (dstBox->y1+1) );
        OUTREG(SEC_STREAM_WINDOW_SZ, ((dstBox->x2-dstBox->x1) << 16) 
	   | (dstBox->x2-dstBox->x1) );
#if 0
        OUTREG(SEC_STREAM2_FBUF_ADDR0, (offset + (x1>>15)) 
	   & (0x7ffffff & ~BASE_PAD));
        OUTREG(SEC_STREAM2_STRIDE_LPB, pitch & 0xfff );
        OUTREG(SEC_STREAM2_WINDOW_START, ((dstBox->x1+1) << 16) | (dstBox->y1+1) );
        OUTREG(SEC_STREAM2_WINDOW_SZ, ((dstBox->x2-dstBox->x1) << 16) 
	   | (dstBox->x2-dstBox->x1) );
#endif
    }

#if 0
    /* Set color key on primary. */

    SavageSetColorKey( pScrn );
#endif

    /* Set FIFO L2 on second stream. */
    /* Is CR92 shadowed for crtc2? -- AGD */
    if( pPriv->lastKnownPitch != pitch )
    {
	unsigned char cr92;

	pPriv->lastKnownPitch = pitch;
	pitch = (pitch + 7) / 8 - 4;
	VGAOUT8(vgaCRIndex, 0x92);
	cr92 = VGAIN8(vgaCRReg);
	VGAOUT8(vgaCRReg, (cr92 & 0x40) | (pitch >> 8) | 0x80);
	VGAOUT8(vgaCRIndex, 0x93);
	VGAOUT8(vgaCRReg, pitch);
    }
}

static void
SavageDisplayVideo2000(
    ScrnInfoPtr pScrn,
    int id,
    int offset,
    short width, short height,
    int pitch, 
    int x1, int y1, int x2, int y2,
    BoxPtr dstBox,
    short src_w, short src_h,
    short drw_w, short drw_h
){
    SavagePtr psav = SAVPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    /*DisplayModePtr mode = pScrn->currentMode;*/
    SavagePortPrivPtr pPriv = psav->adaptor->pPortPrivates[0].ptr;
    int vgaCRIndex, vgaCRReg, vgaIOBase;


    vgaIOBase = hwp->IOBase;
    vgaCRIndex = vgaIOBase + 4;
    vgaCRReg = vgaIOBase + 5;

    if ( psav->videoFourCC != id ) {
	SavageSetBlend(pScrn,id);
	SavageResetVideo(pScrn);
    }

    /* Calculate horizontal and vertical scale factors. */

    OUTREG(SEC_STREAM_HSCALING, 
	(65536 * src_w / drw_w) & 0x1FFFFF );

    if( src_w < drw_w )
	OUTREG(SEC_STREAM_HSCALE_NORMALIZE,
	    ((2048 * src_w / drw_w) & 0x7ff) << 16 );
    else
	OUTREG(SEC_STREAM_HSCALE_NORMALIZE, 2048 << 16 );

    OUTREG(SEC_STREAM_VSCALING, 
	    (65536 * src_h / drw_h) & 0x1FFFFF );

    /*
     * Set surface location and stride.  We use x1>>15 because all surfaces
     * are 2 bytes/pixel.
     */

    OUTREG(SEC_STREAM_FBUF_ADDR0, (offset + (x1>>15)) 
       & (0x7ffffff & ~BASE_PAD));
    OUTREG(SEC_STREAM_STRIDE, pitch & 0xfff );
    OUTREG(SEC_STREAM_WINDOW_START, ((dstBox->x1+1) << 16) | (dstBox->y1+1) );
    OUTREG(SEC_STREAM_WINDOW_SZ, ((dstBox->x2-dstBox->x1) << 16) 
       | (dstBox->x2-dstBox->x1) );

#if 0
    /* Set color key on primary. */

    SavageSetColor2000( pScrn );
#endif

    /* Set FIFO L2 on second stream. */
    /* Is CR92 shadowed for crtc2? -- AGD */
    if( pPriv->lastKnownPitch != pitch )
    {
	unsigned char cr92;

	pPriv->lastKnownPitch = pitch;
	pitch = (pitch + 7) / 8 - 4;
	VGAOUT8(vgaCRIndex, 0x92);
	cr92 = VGAIN8(vgaCRReg);
	VGAOUT8(vgaCRReg, (cr92 & 0x40) | (pitch >> 8) | 0x80);
	VGAOUT8(vgaCRIndex, 0x93);
	VGAOUT8(vgaCRReg, pitch);
    }
}

static int 
SavagePutImage( 
    ScrnInfoPtr pScrn, 
    short src_x, short src_y, 
    short drw_x, short drw_y,
    short src_w, short src_h, 
    short drw_w, short drw_h,
    int id, unsigned char* buf, 
    short width, short height, 
    Bool sync,
    RegionPtr clipBoxes, pointer data
){
    SavagePortPrivPtr pPriv = (SavagePortPrivPtr)data;
    SavagePtr psav = SAVPTR(pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    INT32 x1, x2, y1, y2;
    unsigned char *dst_start;
    int pitch, new_h, offset, offsetV=0, offsetU=0;
    int srcPitch, srcPitch2=0, dstPitch;
    int top, left, npixels, nlines;
    BoxRec dstBox;
    CARD32 tmp;
/*    xf86ErrorFVerb(XVTRACE,"SavagePutImage\n"); */
    if(drw_w > 16384) drw_w = 16384;

    /* Clip */
    x1 = src_x;
    x2 = src_x + src_w;
    y1 = src_y;
    y2 = src_y + src_h;

    dstBox.x1 = drw_x;
    dstBox.x2 = drw_x + drw_w;
    dstBox.y1 = drw_y;
    dstBox.y2 = drw_y + drw_h;

    SavageClipVideo(&dstBox, &x1, &x2, &y1, &y2, 
		REGION_EXTENTS(pScreen, clipBoxes), width, height);

    drw_w = dstBox.x2 - dstBox.x1;
    drw_h = dstBox.y2 - dstBox.y1;
    src_w = ( x2 - x1 ) >> 16;
    src_h = ( y2 - y1 ) >> 16;

    if((x1 >= x2) || (y1 >= y2))
	return Success;

    dstBox.x1 -= pScrn->frameX0;
    dstBox.x2 -= pScrn->frameX0;
    dstBox.y1 -= pScrn->frameY0;
    dstBox.y2 -= pScrn->frameY0;

    pitch = pScrn->bitsPerPixel * pScrn->displayWidth >> 3;

    dstPitch = ((width << 1) + 15) & ~15;
    new_h = ((dstPitch * height) + pitch - 1) / pitch;

    switch(id) {
    case FOURCC_Y211:		/* Y211 */
        srcPitch = width;
	break;
    case FOURCC_YV12:		/* YV12 */
	srcPitch = (width + 3) & ~3;
	offsetV = srcPitch * height;
	srcPitch2 = ((width >> 1) + 3) & ~3;
	offsetU = (srcPitch2 * (height >> 1)) + offsetV;
	break;
    case FOURCC_I420:
	srcPitch = (width + 3) & ~3;
	offsetU = srcPitch * height;
	srcPitch2 = ((width >> 1) + 3) & ~3;
	offsetV = (srcPitch2 * (height >> 1)) + offsetU;
	break;
    case FOURCC_RV15:		/* RGB15 */
    case FOURCC_RV16:		/* RGB16 */
    case FOURCC_YUY2:		/* YUY2 */
    default:
	srcPitch = (width << 1);
	break;
    }  

    if(!(pPriv->area = SavageAllocateMemory(pScrn, pPriv->area, new_h)))
	return BadAlloc;

    /* copy data */
    top = y1 >> 16;
    left = (x1 >> 16) & ~1;
    npixels = ((((x2 + 0xffff) >> 16) + 1) & ~1) - left;
    left <<= 1;

    offset = ((pPriv->area->box.y1 * pitch)) + (top * dstPitch);
    dst_start = (psav->FBBase + ((offset + left) & ~BASE_PAD));

    switch(id) {
    case FOURCC_YV12:		/* YV12 */
    case FOURCC_I420:
	top &= ~1;
	tmp = ((top >> 1) * srcPitch2) + (left >> 2);
	offsetU += tmp;
	offsetV += tmp;
	nlines = ((((y2 + 0xffff) >> 16) + 1) & ~1) - top;
        if (S3_SAVAGE4_SERIES(psav->Chipset) && psav->BCIforXv 
	    /*&& (!psav->disableCOB)*/) {
            SavageCopyPlanarDataBCI(
                pScrn,
	    	buf + (top * srcPitch) + (left >> 1), 
	    	buf + offsetV, 
	    	buf + offsetU, 
	    	dst_start, srcPitch, srcPitch2, dstPitch, nlines, npixels);
        } else {
	    SavageCopyPlanarData(
	    	buf + (top * srcPitch) + (left >> 1), 
	    	buf + offsetV, 
	    	buf + offsetU, 
	    	dst_start, srcPitch, srcPitch2, dstPitch, nlines, npixels);
        }
	break;
    case FOURCC_Y211:		/* Y211 */
    case FOURCC_RV15:		/* RGB15 */
    case FOURCC_RV16:		/* RGB16 */
    case FOURCC_YUY2:		/* YUY2 */
    default:
	buf += (top * srcPitch) + left;
	nlines = ((y2 + 0xffff) >> 16) - top;
	SavageCopyData(buf, dst_start, srcPitch, dstPitch, nlines, npixels);
	break;
    }  
   
    /* We need to enable the video before we draw the chroma color.
       Otherwise, we get blue flashes. */

    SavageDisplayVideo(pScrn, id, offset, width, height, dstPitch,
	     x1, y1, x2, y2, &dstBox, src_w, src_h, drw_w, drw_h);

    /* update cliplist */
    if(!REGION_EQUAL(pScreen, &pPriv->clip, clipBoxes)) {
	REGION_COPY(pScreen, &pPriv->clip, clipBoxes);
	/* draw these */
	xf86XVFillKeyHelper(pScrn->pScreen, pPriv->colorKey, clipBoxes);

    }

    pPriv->videoStatus = CLIENT_VIDEO_ON;

    return Success;
}

static int 
SavageQueryImageAttributes(
  ScrnInfoPtr pScrn, 
  int id, 
  unsigned short *w, unsigned short *h, 
  int *pitches, int *offsets
){
    int size, tmp;

    if(*w > 1024) *w = 1024;
    if(*h > 1024) *h = 1024;

    *w = (*w + 1) & ~1;
    if(offsets) offsets[0] = 0;

    switch(id) {
    case FOURCC_IA44:
        if (pitches) pitches[0]=*w;
        size=(*w)*(*h);
        break;
    case FOURCC_Y211:
	size = *w << 2;
	if(pitches) pitches[0] = size;
	size *= *h;
	break;
    case FOURCC_YV12:
    case FOURCC_I420:
	*h = (*h + 1) & ~1;
	size = (*w + 3) & ~3;
	if(pitches) pitches[0] = size;
	size *= *h;
	if(offsets) offsets[1] = size;
	tmp = ((*w >> 1) + 3) & ~3;
	if(pitches) pitches[1] = pitches[2] = tmp;
	tmp *= (*h >> 1);
	size += tmp;
	if(offsets) offsets[2] = size;
	size += tmp;
	break;
    case FOURCC_RV15:		/* RGB15 */
    case FOURCC_RV16:		/* RGB16 */
    case FOURCC_YUY2:
    default:
	size = *w << 1;
	if(pitches) pitches[0] = size;
	size *= *h;
	break;
    }

    return size;
}

/****************** Offscreen stuff ***************/

typedef struct {
  FBAreaPtr area;
  Bool isOn;
} OffscreenPrivRec, * OffscreenPrivPtr;

static int 
SavageAllocateSurface(
    ScrnInfoPtr pScrn,
    int id,
    unsigned short w, 	
    unsigned short h,
    XF86SurfacePtr surface
){
    FBAreaPtr area;
    int pitch, fbpitch, numlines;
    OffscreenPrivPtr pPriv;

    if((w > 1024) || (h > 1024))
	return BadAlloc;

    w = (w + 1) & ~1;
    pitch = ((w << 1) + 15) & ~15;
    fbpitch = pScrn->bitsPerPixel * pScrn->displayWidth >> 3;
    numlines = ((pitch * h) + fbpitch - 1) / fbpitch;

    if(!(area = SavageAllocateMemory(pScrn, NULL, numlines)))
	return BadAlloc;

    surface->width = w;
    surface->height = h;

    if(!(surface->pitches = xalloc(sizeof(int))))
	return BadAlloc;
    if(!(surface->offsets = xalloc(sizeof(int)))) {
	xfree(surface->pitches);
	return BadAlloc;
    }
    if(!(pPriv = xalloc(sizeof(OffscreenPrivRec)))) {
	xfree(surface->pitches);
	xfree(surface->offsets);
	return BadAlloc;
    }

    pPriv->area = area;
    pPriv->isOn = FALSE;

    surface->pScrn = pScrn;
    surface->id = id;   
    surface->pitches[0] = pitch;
    surface->offsets[0] = area->box.y1 * fbpitch;
    surface->devPrivate.ptr = (pointer)pPriv;

    return Success;
}

static int 
SavageStopSurface(
    XF86SurfacePtr surface
){
    OffscreenPrivPtr pPriv = (OffscreenPrivPtr)surface->devPrivate.ptr;
    xf86ErrorFVerb(XVTRACE,"SavageStopSurface\n");

    if(pPriv->isOn) {
	/*SavagePtr psav = SAVPTR(surface->pScrn);*/
	/*SavageClipVWindow(surface->pScrn);*/
	SavageStreamsOff( surface->pScrn );
	pPriv->isOn = FALSE;
    }

    return Success;
}


static int 
SavageFreeSurface(
    XF86SurfacePtr surface
){
    OffscreenPrivPtr pPriv = (OffscreenPrivPtr)surface->devPrivate.ptr;

    if(pPriv->isOn)
	SavageStopSurface(surface);
    xf86FreeOffscreenArea(pPriv->area);
    xfree(surface->pitches);
    xfree(surface->offsets);
    xfree(surface->devPrivate.ptr);

    return Success;
}

static int
SavageGetSurfaceAttribute(
    ScrnInfoPtr pScrn,
    Atom attribute,
    INT32 *value
){
    return SavageGetPortAttribute(pScrn, attribute, value, 
			(pointer)(GET_PORT_PRIVATE(pScrn)));
}

static int
SavageSetSurfaceAttribute(
    ScrnInfoPtr pScrn,
    Atom attribute,
    INT32 value
){
    return SavageSetPortAttribute(pScrn, attribute, value, 
			(pointer)(GET_PORT_PRIVATE(pScrn)));
}


static int 
SavageDisplaySurface(
    XF86SurfacePtr surface,
    short src_x, short src_y, 
    short drw_x, short drw_y,
    short src_w, short src_h, 
    short drw_w, short drw_h,
    RegionPtr clipBoxes
){
    OffscreenPrivPtr pPriv = (OffscreenPrivPtr)surface->devPrivate.ptr;
    ScrnInfoPtr pScrn = surface->pScrn;
    ScreenPtr pScreen = pScrn->pScreen;
    SavagePortPrivPtr portPriv = GET_PORT_PRIVATE(pScrn);
    INT32 x1, y1, x2, y2;
    BoxRec dstBox;
    xf86ErrorFVerb(XVTRACE,"SavageDisplaySurface\n");

    x1 = src_x;
    x2 = src_x + src_w;
    y1 = src_y;
    y2 = src_y + src_h;

    dstBox.x1 = drw_x;
    dstBox.x2 = drw_x + drw_w;
    dstBox.y1 = drw_y;
    dstBox.y2 = drw_y + drw_h;

    SavageClipVideo(&dstBox, &x1, &x2, &y1, &y2, 
                	REGION_EXTENTS(pScreen, clipBoxes), 
			surface->width, surface->height);

    if((x1 >= x2) || (y1 >= y2))
	return Success;

    dstBox.x1 -= pScrn->frameX0;
    dstBox.x2 -= pScrn->frameX0;
    dstBox.y1 -= pScrn->frameY0;
    dstBox.y2 -= pScrn->frameY0;

    SavageDisplayVideo(pScrn, surface->id, surface->offsets[0], 
	     surface->width, surface->height, surface->pitches[0],
	     x1, y1, x2, y2, &dstBox, src_w, src_h, drw_w, drw_h);

    xf86XVFillKeyHelper(pScrn->pScreen, portPriv->colorKey, clipBoxes);

    pPriv->isOn = TRUE;
#if 0
    if(portPriv->videoStatus & CLIENT_VIDEO_ON) {
	REGION_EMPTY(pScreen, &portPriv->clip);
	UpdateCurrentTime();
	portPriv->videoStatus = FREE_TIMER;
	portPriv->freeTime = currentTime.milliseconds + FREE_DELAY;
    }
#endif

    return Success;
}


static void 
SavageInitOffscreenImages(ScreenPtr pScreen)
{
    XF86OffscreenImagePtr offscreenImages;
    SavagePtr psav = SAVPTR(xf86Screens[pScreen->myNum]);

    /* need to free this someplace */
    if (!psav->offscreenImages) {
	if(!(offscreenImages = xalloc(sizeof(XF86OffscreenImageRec))))
	    return;
	psav->offscreenImages = offscreenImages;
    } else {
	offscreenImages = psav->offscreenImages;
    }

    offscreenImages[0].image = &Images[0];
    offscreenImages[0].flags = VIDEO_OVERLAID_IMAGES | 
			       VIDEO_CLIP_TO_VIEWPORT;
    offscreenImages[0].alloc_surface = SavageAllocateSurface;
    offscreenImages[0].free_surface = SavageFreeSurface;
    offscreenImages[0].display = SavageDisplaySurface;
    offscreenImages[0].stop = SavageStopSurface;
    offscreenImages[0].setAttribute = SavageSetSurfaceAttribute;
    offscreenImages[0].getAttribute = SavageGetSurfaceAttribute;
    offscreenImages[0].max_width = 1024;
    offscreenImages[0].max_height = 1024;
    offscreenImages[0].num_attributes = NUM_ATTRIBUTES;
    offscreenImages[0].attributes = Attributes;
    
    xf86XVRegisterOffscreenImages(pScreen, offscreenImages, 1);
}
