/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/savage/savage_driver.h,v 1.16 2003/01/18 15:22:30 eich Exp $ */

#ifndef SAVAGE_VGAHWMMIO_H
#define SAVAGE_VGAHWMMIO_H

#define MODE_24 24

#include "xf86_ansic.h"
#include "compiler.h"
#include "vgaHW.h"
#include "xf86.h"
#include "xf86Resources.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86_OSproc.h"
#include "xf86Cursor.h"
#include "mipointer.h"
#include "micmap.h"
#include "fb.h"
#include "fboverlay.h"
#include "xf86cmap.h"
#include "vbe.h"
#include "xaa.h"
#include "xf86xv.h"

#include "savage_regs.h"
#include "savage_vbe.h"

#ifdef XF86DRI
#define _XF86DRI_SERVER_
#include "savage_dripriv.h"
#include "savage_dri.h"
#include "savage_drm.h"
#include "dri.h"
#include "GL/glxint.h"
#endif

typedef enum {
    MT_NONE,
    MT_CRT,
    MT_LCD,
    MT_DFP,
    MT_TV
} SavageMonitorType;

typedef struct
{
    Bool HasSecondary;
    Bool TvOn;
    ScrnInfoPtr pSecondaryScrn;
    ScrnInfoPtr pPrimaryScrn;
  
} SavageEntRec, *SavageEntPtr;

#define VGAIN8(addr) MMIO_IN8(psav->MapBase+0x8000, addr)
#define VGAIN16(addr) MMIO_IN16(psav->MapBase+0x8000, addr)
#define VGAIN(addr) MMIO_IN32(psav->MapBase+0x8000, addr)
 
#define VGAOUT8(addr,val) MMIO_OUT8(psav->MapBase+0x8000, addr, val)
#define VGAOUT16(addr,val) MMIO_OUT16(psav->MapBase+0x8000, addr, val)
#define VGAOUT(addr,val) MMIO_OUT32(psav->MapBase+0x8000, addr, val)

#define INREG8(addr) MMIO_IN8(psav->MapBase, addr)
#define INREG16(addr) MMIO_IN16(psav->MapBase, addr)
#define INREG32(addr) MMIO_IN32(psav->MapBase, addr)
#define OUTREG8(addr,val) MMIO_OUT8(psav->MapBase, addr, val)
#define OUTREG16(addr,val) MMIO_OUT16(psav->MapBase, addr, val)
#define OUTREG32(addr,val) MMIO_OUT32(psav->MapBase, addr, val)
#define INREG(addr) INREG32(addr) 
#define OUTREG(addr,val) OUTREG32(addr,val) 

#if X_BYTE_ORDER == X_LITTLE_ENDIAN
#define B_O16(x)  (x)
#define B_O32(x)  (x)
#else
#define B_O16(x)  ((((x) & 0xff) << 8) | (((x) & 0xff) >> 8))
#define B_O32(x)  ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) \
                  | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#endif
#define L_ADD(x)  (B_O32(x) & 0xffff) + ((B_O32(x) >> 12) & 0xffff00)

#define SAVAGEIOMAPSIZE	0x80000

#define SAVAGE_CRT_ON	1
#define SAVAGE_LCD_ON	2
#define SAVAGE_TV_ON	4

typedef struct _S3VMODEENTRY {
   unsigned short Width;
   unsigned short Height;
   unsigned short VesaMode;
   unsigned char RefreshCount;
   unsigned char * RefreshRate;
} SavageModeEntry, *SavageModeEntryPtr;


typedef struct _S3VMODETABLE {
   unsigned short NumModes;
   SavageModeEntry Modes[1];
} SavageModeTableRec, *SavageModeTablePtr;


typedef struct {
    unsigned int mode, refresh;
    unsigned char SR08, SR0E, SR0F;
    unsigned char SR10, SR11, SR12, SR13, SR15, SR18, SR1B, SR29, SR30;
    unsigned char SR54[8];
    unsigned char Clock;
    unsigned char CR31, CR32, CR33, CR34, CR36, CR3A, CR3B, CR3C;
    unsigned char CR40, CR41, CR42, CR43, CR45;
    unsigned char CR50, CR51, CR53, CR55, CR58, CR5B, CR5D, CR5E;
    unsigned char CR60, CR63, CR65, CR66, CR67, CR68, CR69, CR6D, CR6F;
    unsigned char CR86, CR88;
    unsigned char CR90, CR91, CRB0;
    unsigned int  STREAMS[22];	/* yuck, streams regs */
    unsigned int  MMPR0, MMPR1, MMPR2, MMPR3;
} SavageRegRec, *SavageRegPtr;

typedef  struct {
    CARD32 redMask, greenMask, blueMask;
    int redShift, greenShift, blueShift;
} savageOverlayRec;

/*  Tiling defines */
#define TILE_SIZE_BYTE          2048   /* 0x800, 2K */
#define TILE_SIZE_BYTE_2000     4096

#define TILEHEIGHT_16BPP        16
#define TILEHEIGHT_32BPP        16
#define TILEHEIGHT              16      /* all 16 and 32bpp tiles are 16 lines high */
#define TILEHEIGHT_2000         32      /* 32 lines on savage 2000 */

#define TILEWIDTH_BYTES         128     /* 2048/TILEHEIGHT (** not for use w/8bpp tiling) */
#define TILEWIDTH8BPP_BYTES     64      /* 2048/TILEHEIGHT_8BPP */
#define TILEWIDTH_16BPP         64      /* TILEWIDTH_BYTES/2-BYTES-PER-PIXEL */
#define TILEWIDTH_32BPP         32      /* TILEWIDTH_BYTES/4-BYTES-PER-PIXEL */

/* Bitmap descriptor structures for BCI */
typedef struct _HIGH {
    unsigned short Stride;
    unsigned char Bpp;
    unsigned char ResBWTile;
} HIGH;

typedef struct _BMPDESC1 {
    unsigned long Offset;
    HIGH  HighPart;
} BMPDESC1;

typedef struct _BMPDESC2 {
    unsigned long LoPart;
    unsigned long HiPart;
} BMPDESC2;

typedef union _BMPDESC {
    BMPDESC1 bd1;
    BMPDESC2 bd2;
} BMPDESC;

typedef struct _StatInfo {
    int     origMode;
    int     pageCnt;    
    pointer statBuf;
    int     realSeg;    
    int     realOff;
} StatInfoRec,*StatInfoPtr;

typedef struct _Savage {
    SavageRegRec	SavedReg;
    SavageRegRec	ModeReg;
    xf86CursorInfoPtr	CursorInfoRec;
    Bool		ModeStructInit;
    Bool		NeedSTREAMS;
    Bool		STREAMSRunning;
    int			Bpp, Bpl, ScissB;
    unsigned		PlaneMask;
    I2CBusPtr		I2C;
    I2CBusPtr		DVI;
    unsigned char       DDCPort;
    unsigned char       I2CPort;

    int			videoRambytes;
    int			videoRamKbytes;
    int			MemOffScreen;
    int			CursorKByte;
    int			endfb;

    /* These are physical addresses. */
    unsigned long	FrameBufferBase;
    unsigned long	MmioBase;
    unsigned long	ApertureBase;
    unsigned long	ShadowPhysical;

    /* These are linear addresses. */
    unsigned char*	MapBase;
    unsigned char*	BciMem;
    unsigned char*	MapBaseDense;
    unsigned char*	FBBase;
    unsigned char*	ApertureMap;
    unsigned char*	FBStart;
    CARD32 volatile *	ShadowVirtual;

    Bool		PrimaryVidMapped;
    int			dacSpeedBpp;
    int			minClock, maxClock;
    int			HorizScaleFactor;
    int			MCLK, REFCLK, LCDclk;
    double		refclk_fact;
    int			GEResetCnt;

    /* Here are all the Options */

    OptionInfoPtr	Options;
    Bool		ShowCache;
    Bool		pci_burst;
    Bool		NoPCIRetry;
    Bool		fifo_conservative;
    Bool		fifo_moderate;
    Bool		fifo_aggressive;
    Bool		hwcursor;
    Bool		hwc_on;
    Bool		NoAccel;
    Bool		shadowFB;
    Bool		UseBIOS;
    int			rotate;
    double		LCDClock;
    Bool		ShadowStatus;
    Bool		CrtOnly;
    Bool		TvOn;
    Bool		PAL;
    Bool		ForceInit;
    int			iDevInfo;
    int			iDevInfoPrim;

    Bool		FPExpansion;
    int			PanelX;		/* panel width */
    int			PanelY;		/* panel height */
    int			iResX;		/* crtc X display */
    int			iResY;		/* crtc Y display */
    int			XFactor;	/* overlay X factor */
    int			YFactor;	/* overlay Y factor */
    int			displayXoffset;	/* overlay X offset */
    int			displayYoffset;	/* overlay Y offset */
    int			XExp1;		/* expansion ratio in x */
    int			XExp2;
    int			YExp1;		/* expansion ratio in x */
    int			YExp2;
    int			cxScreen;
    int			TVSizeX;
    int			TVSizeY;

    CloseScreenProcPtr	CloseScreen;
    pciVideoPtr		PciInfo;
    PCITAG		PciTag;
    int			Chipset;
    int			ChipId;
    int			ChipRev;
    vbeInfoPtr		pVbe;
    int			EntityIndex;
    int			ShadowCounter;
    int			vgaIOBase;	/* 3b0 or 3d0 */

    /* The various Savage wait handlers. */
    int			(*WaitQueue)(struct _Savage *, int);
    int			(*WaitIdle)(struct _Savage *);
    int			(*WaitIdleEmpty)(struct _Savage *);

    /* Support for shadowFB and rotation */
    unsigned char *	ShadowPtr;
    int			ShadowPitch;
    void		(*PointerMoved)(int index, int x, int y);

    /* Support for XAA acceleration */
    XAAInfoRecPtr	AccelInfoRec;
    xRectangle		Rect;
    unsigned int	SavedBciCmd;
    unsigned int	SavedFgColor;
    unsigned int	SavedBgColor;
    unsigned int	SavedSbdOffset;
    unsigned int	SavedSbd;

    SavageModeTablePtr	ModeTable;

    /* Support for the Savage command overflow buffer. */
    unsigned long	cobIndex;	/* size index */
    unsigned long	cobSize;	/* size in bytes */
    unsigned long	cobOffset;	/* offset in frame buffer */

    /* Support for DGA */
    int			numDGAModes;
    DGAModePtr		DGAModes;
    Bool		DGAactive;
    int			DGAViewportStatus;

    /* Support for XVideo */

    unsigned int	videoFlags;
    unsigned int	blendBase;
    int			videoFourCC;
    XF86VideoAdaptorPtr	adaptor;
    int			VideoZoomMax;
    int			dwBCIWait2DIdle;
    XF86OffscreenImagePtr offscreenImages;

    /* Support for Overlays */
     unsigned char *	FBStart2nd;
     savageOverlayRec	overlay;
     int                 overlayDepth;
     int			primStreamBpp;

#ifdef XF86DRI
    int 		LockHeld;
    Bool 		directRenderingEnabled;
    DRIInfoPtr 		pDRIInfo;
    int 		drmFD;
    int 		numVisualConfigs;
    __GLXvisualConfig*	pVisualConfigs;
    SAVAGEConfigPrivPtr 	pVisualConfigsPriv;
    SAVAGEDRIServerPrivatePtr DRIServerInfo;


#if 0
    Bool		haveQuiescense;
    void		(*GetQuiescence)(ScrnInfoPtr pScrn);
#endif

    int 		agpMode;
    drmSize		agpSize;
    FBLinearPtr		reserved;
    
    unsigned int surfaceAllocation[7];
    unsigned int xvmcContext;
    unsigned int DRIrunning;
    unsigned int hwmcOffset;
    unsigned int hwmcSize;

    Bool bDisableXvMC;

#endif

    Bool disableCOB;
    Bool BCIforXv;

    /* Bitmap Descriptors for BCI */
    BMPDESC GlobalBD;
    BMPDESC PrimaryBD;
    BMPDESC SecondBD;
    /* do we disable tile mode by option? */
    Bool bDisableTile;
    /* if we enable tile,we only support tile under 16/32bpp */
    Bool bTiled;
    int  lDelta;
    int  ulAperturePitch; /* aperture pitch */

    /*
     * cxMemory is number of pixels across screen width
     * cyMemory is number of scanlines in available adapter memory.
     *
     * cxMemory * cyMemory is used to determine how much memory to
     * allocate to our heap manager.  So make sure that any space at the
     * end of video memory set aside at bInitializeHardware time is kept
     * out of the cyMemory calculation.
     */
    int cxMemory,cyMemory;
    
    StatInfoRec     StatInfo; /* save the SVGA state */

    /* for dvi option */
    Bool  dvi;

    SavageMonitorType   DisplayType;
    /* DuoView stuff */
    Bool		HasCRTC2;     /* MX, IX, Supersavage */
    Bool		IsSecondary;  /* second Screen */	
    Bool		IsPrimary;  /* first Screen */
    EntityInfoPtr       pEnt;

} SavageRec, *SavagePtr;

/* Video flags. */

#define VF_STREAMS_ON	0x0001

#define SAVPTR(p)	((SavagePtr)((p)->driverPrivate))

/* Make the names of these externals driver-unique */
#define gpScrn savagegpScrn
#define readdw savagereaddw
#define readfb savagereadfb
#define writedw savagewritedw
#define writefb savagewritefb
#define writescan savagewritescan

/* add for support DRI */
#ifdef XF86DRI

#define SAVAGE_FRONT	0x1
#define SAVAGE_BACK	0x2
#define SAVAGE_DEPTH	0x4
#define SAVAGE_STENCIL	0x8

Bool SAVAGEDRIScreenInit( ScreenPtr pScreen );
Bool SAVAGEInitMC(ScreenPtr pScreen);
void SAVAGEDRICloseScreen( ScreenPtr pScreen );
Bool SAVAGEDRIFinishScreenInit( ScreenPtr pScreen );

Bool SAVAGELockUpdate( ScrnInfoPtr pScrn, drmLockFlags flags );

#if 0
void SAVAGEGetQuiescence( ScrnInfoPtr pScrn );
void SAVAGEGetQuiescenceShared( ScrnInfoPtr pScrn );
#endif

void SAVAGESelectBuffer(ScrnInfoPtr pScrn, int which);

#if 0
Bool SAVAGECleanupDma(ScrnInfoPtr pScrn);
Bool SAVAGEInitDma(ScrnInfoPtr pScrn, int prim_size);
#endif

#define SAVAGE_AGP_1X_MODE		0x01
#define SAVAGE_AGP_2X_MODE		0x02
#define SAVAGE_AGP_4X_MODE		0x04
#define SAVAGE_AGP_MODE_MASK	0x07

#endif


/* Prototypes. */

extern void SavageCommonCalcClock(long freq, int min_m, int min_n1,
			int max_n1, int min_n2, int max_n2,
			long freq_min, long freq_max,
			unsigned char *mdiv, unsigned char *ndiv);
void SavageAdjustFrame(int scrnIndex, int y, int x, int flags);
void SavageDoAdjustFrame(ScrnInfoPtr pScrn, int y, int x, int crtc2);
Bool SavageSwitchMode(int scrnIndex, DisplayModePtr mode, int flags);

/* In savage_cursor.c. */

Bool SavageHWCursorInit(ScreenPtr pScreen);
void SavageShowCursor(ScrnInfoPtr);
void SavageHideCursor(ScrnInfoPtr);

/* In savage_accel.c. */

Bool SavageInitAccel(ScreenPtr);
void SavageInitialize2DEngine(ScrnInfoPtr);
void SavageSetGBD(ScrnInfoPtr);
void SavageAccelSync(ScrnInfoPtr);
/*int SavageHelpSolidROP(ScrnInfoPtr pScrn, int *fg, int pm, int *rop);*/

/* In savage_i2c.c. */

Bool SavageI2CInit(ScrnInfoPtr pScrn);

/* In savage_shadow.c */

void SavagePointerMoved(int index, int x, int y);
void SavageRefreshArea(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void SavageRefreshArea8(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void SavageRefreshArea16(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void SavageRefreshArea24(ScrnInfoPtr pScrn, int num, BoxPtr pbox);
void SavageRefreshArea32(ScrnInfoPtr pScrn, int num, BoxPtr pbox);

/* In savage_vbe.c */

void SavageSetTextMode( SavagePtr psav );
void SavageSetVESAMode( SavagePtr psav, int n, int Refresh );
void SavageSetPanelEnabled( SavagePtr psav, Bool active );
void SavageFreeBIOSModeTable( SavagePtr psav, SavageModeTablePtr* ppTable );
SavageModeTablePtr SavageGetBIOSModeTable( SavagePtr psav, int iDepth );
ModeStatus SavageMatchBiosMode(ScrnInfoPtr pScrn,int width,int height,int refresh,
                              unsigned int *vesaMode,unsigned int *newRefresh);

unsigned short SavageGetBIOSModes( 
    SavagePtr psav,
    int iDepth,
    SavageModeEntryPtr s3vModeTable );

/* In savage_video.c */

void SavageInitVideo( ScreenPtr pScreen );

/* In savage_streams.c */

void SavageStreamsOn(ScrnInfoPtr pScrn);
void SavageStreamsOff(ScrnInfoPtr pScrn);
void SavageInitSecondaryStream(ScrnInfoPtr pScrn);
void SavageInitStreamsOld(ScrnInfoPtr pScrn);
void SavageInitStreamsNew(ScrnInfoPtr pScrn);
void SavageInitStreams2000(ScrnInfoPtr pScrn);


#if (MODE_24 == 32)
# define  BYTES_PP24 4
#else
# define BYTES_PP24 3
#endif


#define DEPTH_BPP(depth) (depth == 24 ? (BYTES_PP24 << 3) : (depth + 7) & ~0x7)
#define DEPTH_2ND(pScrn) (pScrn->depth > 8 ? pScrn->depth\
                              : SAVPTR(pScrn)->overlayDepth)

#endif /* SAVAGE_VGAHWMMIO_H */

