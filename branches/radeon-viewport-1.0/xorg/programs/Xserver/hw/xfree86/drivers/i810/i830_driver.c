/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/i810/i830_driver.c,v 1.50 2004/02/20 00:06:00 alanh Exp $ */
/**************************************************************************

Copyright 2001 VA Linux Systems Inc., Fremont, California.
Copyright © 2002 by David Dawes

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
THE COPYRIGHT HOLDERS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Reformatted with GNU indent (2.2.8), using the following options:
 *
 *    -bad -bap -c41 -cd0 -ncdb -ci6 -cli0 -cp0 -ncs -d0 -di3 -i3 -ip3 -l78
 *    -lp -npcs -psl -sob -ss -br -ce -sc -hnl
 *
 * This provides a good match with the original i810 code and preferred
 * XFree86 formatting conventions.
 *
 * When editing this driver, please follow the existing formatting, and edit
 * with <TAB> characters expanded at 8-column intervals.
 */

/*
 * Authors: Jeff Hartmann <jhartmann@valinux.com>
 *          Abraham van der Merwe <abraham@2d3d.co.za>
 *          David Dawes <dawes@xfree86.org>
 *          Alan Hourihane <alanh@tungstengraphics.com>
 */

/*
 * Mode handling is based on the VESA driver written by:
 * Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 */

/*
 * Changes:
 *
 *    23/08/2001 Abraham van der Merwe <abraham@2d3d.co.za>
 *        - Fixed display timing bug (mode information for some
 *          modes were not initialized correctly)
 *        - Added workarounds for GTT corruptions (I don't adjust
 *          the pitches for 1280x and 1600x modes so we don't
 *          need extra memory)
 *        - The code will now default to 60Hz if LFP is connected
 *        - Added different refresh rate setting code to work
 *          around 0x4f02 BIOS bug
 *        - BIOS workaround for some mode sets (I use legacy BIOS
 *          calls for setting those)
 *        - Removed 0x4f04, 0x01 (save state) BIOS call which causes
 *          LFP to malfunction (do some house keeping and restore
 *          modes ourselves instead - not perfect, but at least the
 *          LFP is working now)
 *        - Several other smaller bug fixes
 *
 *    06/09/2001 Abraham van der Merwe <abraham@2d3d.co.za>
 *        - Preliminary local memory support (works without agpgart)
 *        - DGA fixes (the code were still using i810 mode sets, etc.)
 *        - agpgart fixes
 *
 *    18/09/2001
 *        - Proper local memory support (should work correctly now
 *          with/without agpgart module)
 *        - more agpgart fixes
 *        - got rid of incorrect GTT adjustments
 *
 *    09/10/2001
 *        - Changed the DPRINTF() variadic macro to an ANSI C compatible
 *          version
 *
 *    10/10/2001
 *        - Fixed DPRINTF_stub(). I forgot the __...__ macros in there
 *          instead of the function arguments :P
 *        - Added a workaround for the 1600x1200 bug (Text mode corrupts
 *          when you exit from any 1600x1200 mode and 1280x1024@85Hz. I
 *          suspect this is a BIOS bug (hence the 1280x1024@85Hz case)).
 *          For now I'm switching to 800x600@60Hz then to 80x25 text mode
 *          and then restoring the registers - very ugly indeed.
 *
 *    15/10/2001
 *        - Improved 1600x1200 mode set workaround. The previous workaround
 *          was causing mode set problems later on.
 *
 *    18/10/2001
 *        - Fixed a bug in I830BIOSLeaveVT() which caused a bug when you
 *          switched VT's
 */
/*
 *    07/2002 David Dawes
 *        - Add Intel(R) 855GM/852GM support.
 */
/*
 *    07/2002 David Dawes
 *        - Cleanup code formatting.
 *        - Improve VESA mode selection, and fix refresh rate selection.
 *        - Don't duplicate functions provided in 4.2 vbe modules.
 *        - Don't duplicate functions provided in the vgahw module.
 *        - Rewrite memory allocation.
 *        - Rewrite initialisation and save/restore state handling.
 *        - Decouple the i810 support from i830 and later.
 *        - Remove various unnecessary hacks and workarounds.
 *        - Fix an 845G problem with the ring buffer not in pre-allocated
 *          memory.
 *        - Fix screen blanking.
 *        - Clear the screen at startup so you don't see the previous session.
 *        - Fix some HW cursor glitches, and turn HW cursor off at VT switch
 *          and exit.
 *
 *    08/2002 Keith Whitwell
 *        - Fix DRI initialisation.
 *
 *
 *    08/2002 Alan Hourihane and David Dawes
 *        - Add XVideo support.
 *
 *
 *    10/2002 David Dawes
 *        - Add Intel(R) 865G support.
 *
 *
 *    01/2004 Alan Hourihane
 *        - Add Intel(R) 915G support.
 *        - Add Dual Head and Clone capabilities.
 *        - Add lid status checking
 *        - Fix Xvideo with high-res LFP's
 *        - Add ARGB HW cursor support
 */

#ifndef PRINT_MODE_INFO
#define PRINT_MODE_INFO 0
#endif

#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xf86Resources.h"
#include "xf86RAC.h"
#include "xf86cmap.h"
#include "compiler.h"
#include "mibstore.h"
#include "vgaHW.h"
#include "mipointer.h"
#include "micmap.h"

#include "fb.h"
#include "miscstruct.h"
#include "xf86xv.h"
#include "Xv.h"
#include "vbe.h"
#include "vbeModes.h"

#include "i830.h"

#ifdef XF86DRI
#include "dri.h"
#endif

#define BIT(x) (1 << (x))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define NB_OF(x) (sizeof (x) / sizeof (*x))

/* *INDENT-OFF* */
static SymTabRec I830BIOSChipsets[] = {
   {PCI_CHIP_I830_M,		"i830"},
   {PCI_CHIP_845_G,		"845G"},
   {PCI_CHIP_I855_GM,		"852GM/855GM"},
   {PCI_CHIP_I865_G,		"865G"},
   {PCI_CHIP_I915_G,		"915G"},
   {-1,				NULL}
};

static PciChipsets I830BIOSPciChipsets[] = {
   {PCI_CHIP_I830_M,		PCI_CHIP_I830_M,	RES_SHARED_VGA},
   {PCI_CHIP_845_G,		PCI_CHIP_845_G,		RES_SHARED_VGA},
   {PCI_CHIP_I855_GM,		PCI_CHIP_I855_GM,	RES_SHARED_VGA},
   {PCI_CHIP_I865_G,		PCI_CHIP_I865_G,	RES_SHARED_VGA},
   {PCI_CHIP_I915_G,		PCI_CHIP_I915_G,	RES_SHARED_VGA},
   {-1,				-1,			RES_UNDEFINED}
};

/*
 * Note: "ColorKey" is provided for compatibility with the i810 driver.
 * However, the correct option name is "VideoKey".  "ColorKey" usually
 * refers to the tranparency key for 8+24 overlays, not for video overlays.
 */

typedef enum {
   OPTION_NOACCEL,
   OPTION_SW_CURSOR,
   OPTION_CACHE_LINES,
   OPTION_DRI,
   OPTION_PAGEFLIP,
   OPTION_XVIDEO,
   OPTION_VIDEO_KEY,
   OPTION_COLOR_KEY,
   OPTION_VBE_RESTORE,
   OPTION_DISPLAY_INFO,
   OPTION_DEVICE_PRESENCE,
   OPTION_MONITOR_LAYOUT,
   OPTION_CLONE,
   OPTION_CLONE_REFRESH,
   OPTION_CHECKLID,
   OPTION_FLIP_PRIMARY
} I830Opts;

static OptionInfoRec I830BIOSOptions[] = {
   {OPTION_NOACCEL,	"NoAccel",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_SW_CURSOR,	"SWcursor",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_CACHE_LINES,	"CacheLines",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_DRI,		"DRI",		OPTV_BOOLEAN,	{0},	TRUE},
   {OPTION_PAGEFLIP,	"PageFlip",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_XVIDEO,	"XVideo",	OPTV_BOOLEAN,	{0},	TRUE},
   {OPTION_COLOR_KEY,	"ColorKey",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_VIDEO_KEY,	"VideoKey",	OPTV_INTEGER,	{0},	FALSE},
   {OPTION_VBE_RESTORE,	"VBERestore",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_DISPLAY_INFO,"DisplayInfo",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_DEVICE_PRESENCE,"DevicePresence",OPTV_BOOLEAN,{0},	FALSE},
   {OPTION_MONITOR_LAYOUT, "MonitorLayout", OPTV_ANYSTR,{0},	FALSE},
   {OPTION_CLONE,	"Clone",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_CLONE_REFRESH,"CloneRefresh",OPTV_INTEGER,	{0},	FALSE},
   {OPTION_CHECKLID,    "CheckLid",	OPTV_BOOLEAN,	{0},	FALSE},
   {OPTION_FLIP_PRIMARY,"FlipPrimary",	OPTV_BOOLEAN,	{0},	FALSE},
   {-1,			NULL,		OPTV_NONE,	{0},	FALSE}
};
/* *INDENT-ON* */

static void I830DisplayPowerManagementSet(ScrnInfoPtr pScrn,
					  int PowerManagementMode, int flags);
static void I830BIOSAdjustFrame(int scrnIndex, int x, int y, int flags);
static Bool I830BIOSCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool I830BIOSSaveScreen(ScreenPtr pScreen, int unblack);
static Bool I830BIOSEnterVT(int scrnIndex, int flags);
static Bool I830VESASetVBEMode(ScrnInfoPtr pScrn, int mode,
			       VbeCRTCInfoBlock *block);
static CARD32 I830LidTimer(OsTimerPtr timer, CARD32 now, pointer arg);
static Bool SetPipeAccess(ScrnInfoPtr pScrn);
static Bool IsPrimary(ScrnInfoPtr pScrn);

extern int I830EntityIndex;


#ifdef I830DEBUG
void
I830DPRINTF_stub(const char *filename, int line, const char *function,
		 const char *fmt, ...)
{
   va_list ap;

   ErrorF("\n##############################################\n"
	  "*** In function %s, on line %d, in file %s ***\n",
	  function, line, filename);
   va_start(ap, fmt);
   VErrorF(fmt, ap);
   va_end(ap);
   ErrorF("##############################################\n\n");
}
#else /* #ifdef I830DEBUG */
void
I830DPRINTF_stub(const char *filename, int line, const char *function,
		 const char *fmt, ...)
{
   /* do nothing */
}
#endif /* #ifdef I830DEBUG */

/* XXX Check if this is still needed. */
const OptionInfoRec *
I830BIOSAvailableOptions(int chipid, int busid)
{
   int i;

   for (i = 0; I830BIOSPciChipsets[i].PCIid > 0; i++) {
      if (chipid == I830BIOSPciChipsets[i].PCIid)
	 return I830BIOSOptions;
   }
   return NULL;
}

static Bool
I830BIOSGetRec(ScrnInfoPtr pScrn)
{
   I830Ptr pI830;

   if (pScrn->driverPrivate)
      return TRUE;
   pI830 = pScrn->driverPrivate = xnfcalloc(sizeof(I830Rec), 1);
   pI830->vesa = xnfcalloc(sizeof(VESARec), 1);
   return TRUE;
}

static void
I830BIOSFreeRec(ScrnInfoPtr pScrn)
{
   I830Ptr pI830;
   VESAPtr pVesa;
   DisplayModePtr mode;

   if (!pScrn)
      return;
   if (!pScrn->driverPrivate)
      return;

   pI830 = I830PTR(pScrn);
   mode = pScrn->modes;

   if (mode) {
      do {
	 if (mode->Private) {
	    VbeModeInfoData *data = (VbeModeInfoData *) mode->Private;

	    if (data->block)
	       xfree(data->block);
	    xfree(data);
	    mode->Private = NULL;
	 }
	 mode = mode->next;
      } while (mode && mode != pScrn->modes);
   }

   if (pI830->vbeInfo)
      VBEFreeVBEInfo(pI830->vbeInfo);
   if (pI830->pVbe)
      vbeFree(pI830->pVbe);

   pVesa = pI830->vesa;
   if (pVesa->monitor)
      xfree(pVesa->monitor);
   if (pVesa->savedPal)
      xfree(pVesa->savedPal);
   xfree(pVesa);

   xfree(pScrn->driverPrivate);
   pScrn->driverPrivate = NULL;
}

static void
I830BIOSProbeDDC(ScrnInfoPtr pScrn, int index)
{
   vbeInfoPtr pVbe;

   /* The vbe module gets loaded in PreInit(), so no need to load it here. */

   pVbe = VBEInit(NULL, index);
   ConfiguredMonitor = vbeDoEDID(pVbe, NULL);
}

/* Various extended video BIOS functions. */
static const int refreshes[] = {
   43, 56, 60, 70, 72, 75, 85, 100, 120
};
static const int nrefreshes = sizeof(refreshes) / sizeof(refreshes[0]);

static Bool
Check5fStatus(ScrnInfoPtr pScrn, int func, int ax)
{
   if (ax == 0x005f)
      return TRUE;
   else if (ax == 0x015f) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Extended BIOS function 0x%04x failed.\n", func);
      return FALSE;
   } else if ((ax & 0xff) != 0x5f) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Extended BIOS function 0x%04x not supported.\n", func);
      return FALSE;
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Extended BIOS function 0x%04x returns 0x%04x.\n",
		 func, ax & 0xffff);
      return FALSE;
   }
}

#if 0
static int
BitToRefresh(int bits)
{
   int i;

   for (i = 0; i < nrefreshes; i++)
      if (bits & (1 << i))
	 return refreshes[i];
   return 0;
}

static int
GetRefreshRate(ScrnInfoPtr pScrn, int mode, int refresh, int *availRefresh)
{
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "GetRefreshRate\n");

   /* Only 8-bit mode numbers are supported. */
   if (mode & 0x100)
      return 0;

   SetPipeAccess(pScrn);

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f05;
   pVbe->pInt10->bx = (mode & 0xff) | 0x100;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f05, pVbe->pInt10->ax)) {
      if (availRefresh)
         *availRefresh = pVbe->pInt10->bx;
      return BitToRefresh(pVbe->pInt10->cx);
   } else
      return 0;
}
#endif

static int
SetRefreshRate(ScrnInfoPtr pScrn, int mode, int refresh)
{
   int i;
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "SetRefreshRate: mode 0x%x, refresh: %d\n", mode, refresh);

   /* Only 8-bit mode numbers are supported. */
   if (mode & 0x100)
      return 0;

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f05;
   pVbe->pInt10->bx = mode & 0xff;

   for (i = nrefreshes - 1; i >= 0; i--) {
      /*
       * Look for the highest value that the requested (refresh + 2) is
       * greater than or equal to.
       */
      if (refreshes[i] <= (refresh + 2))
	 break;
   }
   /* i can be 0 if the requested refresh was higher than the max. */
   if (i == 0) {
      if (refresh >= refreshes[nrefreshes - 1])
         i = nrefreshes - 1;
   }
   DPRINTF(PFX, "Setting refresh rate to %dHz for mode 0x%02x\n",
	   refreshes[i], mode & 0xff);
   pVbe->pInt10->cx = 1 << i;
   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f05, pVbe->pInt10->ax))
      return refreshes[i];
   else
      return 0;
}

static Bool
GetModeSupport(ScrnInfoPtr pScrn, int modePipeA, int modePipeB,
	       int devicesPipeA, int devicesPipeB, int *maxBandwidth,
	       int *bandwidthPipeA, int *bandwidthPipeB)
{
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "GetModeSupport: modes 0x%x, 0x%x, devices: 0x%x, 0x%x\n",
	   modePipeA, modePipeB, devicesPipeA, devicesPipeB);

   /* Only 8-bit mode numbers are supported. */
   if ((modePipeA & 0x100) || (modePipeB & 0x100))
      return 0;

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f28;
   pVbe->pInt10->bx = (modePipeA & 0xff) | ((modePipeB & 0xff) << 8);
   if ((devicesPipeA & 0x80) || (devicesPipeB & 0x80))
      pVbe->pInt10->cx = 0x8000;
   else
      pVbe->pInt10->cx = (devicesPipeA & 0xff) | ((devicesPipeB & 0xff) << 8);

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f28, pVbe->pInt10->ax)) {
      if (maxBandwidth)
	 *maxBandwidth = pVbe->pInt10->cx;
      if (bandwidthPipeA)
	 *bandwidthPipeA = pVbe->pInt10->dx & 0xffff;
      /* XXX For XFree86 4.2.0 and earlier, ->dx is truncated to 16 bits. */
      if (bandwidthPipeB)
	 *bandwidthPipeB = (pVbe->pInt10->dx >> 16) & 0xffff;
      return TRUE;
   } else
      return FALSE;
}

static int
GetLFPCompMode(ScrnInfoPtr pScrn)
{
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "GetLFPCompMode\n");

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f61;
   pVbe->pInt10->bx = 0x100;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f61, pVbe->pInt10->ax))
      return pVbe->pInt10->cx & 0xffff;
   else
      return -1;
}

#if 0
static Bool
SetLFPCompMode(ScrnInfoPtr pScrn, int compMode)
{
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "SetLFPCompMode: compMode %d\n", compMode);

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f61;
   pVbe->pInt10->bx = 0;
   pVbe->pInt10->cx = compMode;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   return Check5fStatus(pScrn, 0x5f61, pVbe->pInt10->ax);
}
#endif

static int
GetDisplayDevices(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->pVbe;

   DPRINTF(PFX, "GetDisplayDevices\n");

#if 0
   {
      CARD32 temp;
      ErrorF("ADPA is 0x%08x\n", INREG(ADPA));
      ErrorF("DVOA is 0x%08x\n", INREG(DVOA));
      ErrorF("DVOB is 0x%08x\n", INREG(DVOB));
      ErrorF("DVOC is 0x%08x\n", INREG(DVOC));
      ErrorF("LVDS is 0x%08x\n", INREG(LVDS));
      temp = INREG(DVOA_SRCDIM);
      ErrorF("DVOA_SRCDIM is 0x%08x (%d x %d)\n", temp,
	     (temp >> 12) & 0xfff, temp & 0xfff);
      temp = INREG(DVOB_SRCDIM);
      ErrorF("DVOB_SRCDIM is 0x%08x (%d x %d)\n", temp,
	     (temp >> 12) & 0xfff, temp & 0xfff);
      temp = INREG(DVOC_SRCDIM);
      ErrorF("DVOC_SRCDIM is 0x%08x (%d x %d)\n", temp,
	     (temp >> 12) & 0xfff, temp & 0xfff);
      ErrorF("SWF0 is 0x%08x\n", INREG(SWF0));
      ErrorF("SWF4 is 0x%08x\n", INREG(SWF4));
   }
#endif

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f64;
   pVbe->pInt10->bx = 0x100;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f64, pVbe->pInt10->ax))
      return pVbe->pInt10->cx & 0xffff;
   else
      return -1;
}

/* This is needed for SetDisplayDevices to work correctly on I915G
 * and possibly later Video BIOS builds around 3272 (but not sure here).
 * So enable for all chipsets now as it has no bad side effects, apart
 * from slightly longer startup time.
 */
#define I915G_WORKAROUND

static Bool
SetDisplayDevices(ScrnInfoPtr pScrn, int devices)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->pVbe;
   CARD32 temp;
#ifdef I915G_WORKAROUND
   int getmode;
   int mode;
   switch (pScrn->depth) {
   case 8:
      mode = 0x30;
      break;
   case 15:
      mode = 0x40;
      break;
   case 16:
      mode = 0x41;
      break;
   case 24:
      mode = 0x50;
      break;
   default: 
      mode = 0x30;
      break;
   }
   mode |= (1 << 15) | (1 << 14);
#endif

   DPRINTF(PFX, "SetDisplayDevices: devices 0x%x\n", devices);

#ifdef I915G_WORKAROUND
   if (pI830->bios_version >= 3272) {
      VBEGetVBEMode(pVbe, &getmode);
      I830VESASetVBEMode(pScrn, mode, NULL);
   }
#endif

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f64;
   pVbe->pInt10->bx = 0x1;
   pVbe->pInt10->cx = devices;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f64, pVbe->pInt10->ax)) {
#ifdef I915G_WORKAROUND
      if (pI830->bios_version >= 3272)
         I830VESASetVBEMode(pScrn, getmode, NULL);
#endif
      return TRUE;
   }

#ifdef I915G_WORKAROUND
   if (pI830->bios_version >= 3272)
      I830VESASetVBEMode(pScrn, getmode, NULL);
#endif

   xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	 "SetDisplayDevices call failed, writing config directly to SWF0.\n");
   temp = INREG(SWF0);
   OUTREG(SWF0, (temp & ~(0xffff)) | (devices & 0xffff));

   if (GetDisplayDevices(pScrn) != devices)
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "SetDisplayDevices failed with devices 0x%x instead of 0x%x\n",
	         GetDisplayDevices(pScrn), devices);

   return FALSE;
}

static Bool
GetBIOSVersion(ScrnInfoPtr pScrn, unsigned int *version)
{
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "GetBIOSVersion\n");

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f01;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f01, pVbe->pInt10->ax)) {
      *version = pVbe->pInt10->bx;
      return TRUE;
   }

   *version = 0;
   return FALSE;
}

static Bool
GetDevicePresence(ScrnInfoPtr pScrn, Bool *required, int *attached,
		  int *encoderPresent)
{
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "GetDevicePresence\n");

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f64;
   pVbe->pInt10->bx = 0x200;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f64, pVbe->pInt10->ax)) {
      if (required)
	 *required = ((pVbe->pInt10->bx & 0x1) == 0);
      if (attached)
	 *attached = (pVbe->pInt10->cx >> 8) & 0xff;
      if (encoderPresent)
	 *encoderPresent = pVbe->pInt10->cx & 0xff;
      return TRUE;
   } else
      return FALSE;
}

static Bool
GetDisplayInfo(ScrnInfoPtr pScrn, int device, Bool *attached, Bool *present,
	       short *x, short *y)
{
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "GetDisplayInfo: device: 0x%x\n", device);

   switch (device & 0xff) {
   case 0x01:
   case 0x02:
   case 0x04:
   case 0x08:
   case 0x10:
   case 0x20:
      break;
   default:
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "GetDisplayInfo: invalid device: 0x%x\n", device & 0xff);
      return FALSE;
   }

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f64;
   pVbe->pInt10->bx = 0x300;
   pVbe->pInt10->cx = device & 0xff;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f64, pVbe->pInt10->ax)) {
      if (attached)
	 *attached = ((pVbe->pInt10->bx & 0x2) != 0);
      if (present)
	 *present = ((pVbe->pInt10->bx & 0x1) != 0);
      if (pVbe->pInt10->cx != (device & 0xff)) {
	 if (y) {
	    *y = pVbe->pInt10->cx & 0xffff;
	 }
	 if (x) {
	    *x = (pVbe->pInt10->cx >> 16) & 0xffff;
	 }
      }
      return TRUE;
   } else
      return FALSE;
}

/*
 * Returns a string matching the device corresponding to the first bit set
 * in "device".  savedDevice is then set to device with that bit cleared.
 * Subsequent calls with device == -1 will use savedDevice.
 */

static const char *displayDevices[] = {
   "CRT",
   "TV",
   "DFP (digital flat panel)",
   "LFP (local flat panel)",
   "CRT2 (second CRT)",
   "TV2 (second TV)",
   "DFP2 (second digital flat panel)",
   "LFP2 (second local flat panel)",
   NULL
};

static const char *
DeviceToString(int device)
{
   static int savedDevice = -1;
   int bit = 0;
   const char *name;

   if (device == -1) {
      device = savedDevice;
      bit = 0;
   }

   if (device == -1)
      return NULL;

   while (displayDevices[bit]) {
      if (device & (1 << bit)) {
	 name = displayDevices[bit];
	 savedDevice = device & ~(1 << bit);
	 bit++;
	 return name;
      }
      bit++;
   }
   return NULL;
}

static void
PrintDisplayDeviceInfo(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int pipe, n;
   int displays;

   DPRINTF(PFX, "PrintDisplayDeviceInfo\n");

   displays = pI830->operatingDevices;
   if (displays == -1) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "No active display devices.\n");
      return;
   }

   /* Check for active devices connected to each display pipe. */
   for (n = 0; n < pI830->availablePipes; n++) {
      pipe = ((displays >> PIPE_SHIFT(n)) & PIPE_ACTIVE_MASK);
      if (pipe) {
	 const char *name;

	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Currently active displays on Pipe %c:\n", PIPE_NAME(n));
	 name = DeviceToString(pipe);
	 do {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "\t%s\n", name);
	    name = DeviceToString(-1);
	 } while (name);

	 if (pipe & PIPE_UNKNOWN_ACTIVE)
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "\tSome unknown display devices may also be present\n");

      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "No active displays on Pipe %c.\n", PIPE_NAME(n));
      }

      if (pI830->pipeDisplaySize[n].x2 != 0) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Lowest common panel size for pipe %c is %d x %d\n",
		    PIPE_NAME(n), pI830->pipeDisplaySize[n].x2,
		    pI830->pipeDisplaySize[n].y2);
      } else if (pI830->pipeEnabled[n] && pipe & ~PIPE_CRT_ACTIVE) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "No display size information available for pipe %c.\n",
		    PIPE_NAME(n));
      }
   }
}

static void
GetPipeSizes(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int pipe, n;
   DisplayType i;

   DPRINTF(PFX, "GetPipeSizes\n");


   for (n = 0; n < pI830->availablePipes; n++) {
      pipe = (pI830->operatingDevices >> PIPE_SHIFT(n)) & PIPE_ACTIVE_MASK;
      pI830->pipeDisplaySize[n].x1 = pI830->pipeDisplaySize[n].y1 = 0;
      pI830->pipeDisplaySize[n].x2 = pI830->pipeDisplaySize[n].y2 = 4096;
      for (i = 0; i < NumKnownDisplayTypes; i++) {
         if (pipe & (1 << i) & PIPE_SIZED_DISP_MASK) {
	    if (pI830->displaySize[i].x2 != 0) {
	       xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		          "Size of device %s is %d x %d\n",
		          displayDevices[i],
		          pI830->displaySize[i].x2,
		          pI830->displaySize[i].y2);
	       if (pI830->displaySize[i].x2 < pI830->pipeDisplaySize[n].x2)
	          pI830->pipeDisplaySize[n].x2 = pI830->displaySize[i].x2;
	       if (pI830->displaySize[i].y2 < pI830->pipeDisplaySize[n].y2)
	          pI830->pipeDisplaySize[n].y2 = pI830->displaySize[i].y2;
	    }
         }
      }

      if (pI830->pipeDisplaySize[n].x2 == 4096)
         pI830->pipeDisplaySize[n].x2 = 0;
      if (pI830->pipeDisplaySize[n].y2 == 4096)
         pI830->pipeDisplaySize[n].y2 = 0;
   }
}

static Bool
I830DetectDisplayDevice(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int pipe, n;
   DisplayType i;
   
   /* This seems to lockup some Dell BIOS'. So it's on option to turn on */
   if (pI830->displayInfo) {
       xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		  "Broken BIOSes cause the system to hang here.\n"
		  "\t      If you encounter this problem please add \n"
		  "\t\t Option \"DisplayInfo\" \"FALSE\"\n"
		  "\t      to the Device section of your XF86Config file.\n");
      for (i = 0; i < NumKnownDisplayTypes; i++) {
         if (GetDisplayInfo(pScrn, 1 << i, &pI830->displayAttached[i],
			 &pI830->displayPresent[i],
			 &pI830->displaySize[i].x2,
			 &pI830->displaySize[i].y2)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Display Info: %s: attached: %s, present: %s, size: "
		    "(%d,%d)\n", displayDevices[i],
		    BOOLTOSTRING(pI830->displayAttached[i]),
		    BOOLTOSTRING(pI830->displayPresent[i]),
		    pI830->displaySize[i].x2, pI830->displaySize[i].y2);
         }
      }
   }

   /* Check for active devices connected to each display pipe. */
   for (n = 0; n < pI830->availablePipes; n++) {
      pipe = ((pI830->operatingDevices >> PIPE_SHIFT(n)) & PIPE_ACTIVE_MASK);
      if (pipe) {
	 pI830->pipeEnabled[n] = TRUE;
      }
   }

   GetPipeSizes(pScrn);

   return TRUE;
}

static int
I830DetectMemory(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   PCITAG bridge;
   CARD16 gmch_ctrl;
   int memsize = 0;
   int range;

   bridge = pciTag(0, 0, 0);		/* This is always the host bridge */
   gmch_ctrl = pciReadWord(bridge, I830_GMCH_CTRL);

   /* We need to reduce the stolen size, by the GTT and the popup.
    * The GTT varying according the the FbMapSize and the popup is 4KB */
   range = (pI830->FbMapSize / (1024*1024)) + 4;

   if (IS_I85X(pI830) || IS_I865G(pI830) || IS_I915G(pI830)) {
      switch (gmch_ctrl & I830_GMCH_GMS_MASK) {
      case I855_GMCH_GMS_STOLEN_1M:
	 memsize = MB(1) - KB(range);
	 break;
      case I855_GMCH_GMS_STOLEN_4M:
	 memsize = MB(4) - KB(range);
	 break;
      case I855_GMCH_GMS_STOLEN_8M:
	 memsize = MB(8) - KB(range);
	 break;
      case I855_GMCH_GMS_STOLEN_16M:
	 memsize = MB(16) - KB(range);
	 break;
      case I855_GMCH_GMS_STOLEN_32M:
	 memsize = MB(32) - KB(range);
	 break;
      case I915G_GMCH_GMS_STOLEN_48M:
	 if (IS_I915G(pI830))
	    memsize = MB(48) - KB(range);
	 break;
      case I915G_GMCH_GMS_STOLEN_64M:
	 if (IS_I915G(pI830))
	    memsize = MB(64) - KB(range);
	 break;
      }
   } else {
      switch (gmch_ctrl & I830_GMCH_GMS_MASK) {
      case I830_GMCH_GMS_STOLEN_512:
	 memsize = KB(512) - KB(range);
	 break;
      case I830_GMCH_GMS_STOLEN_1024:
	 memsize = MB(1) - KB(range);
	 break;
      case I830_GMCH_GMS_STOLEN_8192:
	 memsize = MB(8) - KB(range);
	 break;
      case I830_GMCH_GMS_LOCAL:
	 memsize = 0;
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Local memory found, but won't be used.\n");
	 break;
      }
   }
   if (memsize > 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "detected %d kB stolen memory.\n", memsize / 1024);
   } else {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "no video memory detected.\n");
   }
   return memsize;
}

static Bool
I830MapMMIO(ScrnInfoPtr pScrn)
{
   int mmioFlags;
   I830Ptr pI830 = I830PTR(pScrn);

#if !defined(__alpha__)
   mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT;
#else
   mmioFlags = VIDMEM_MMIO | VIDMEM_READSIDEEFFECT | VIDMEM_SPARSE;
#endif

   pI830->MMIOBase = xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
				   pI830->PciTag,
				   pI830->MMIOAddr, I810_REG_SIZE);
   if (!pI830->MMIOBase)
      return FALSE;
   return TRUE;
}

static Bool
I830MapMem(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   long i;

   for (i = 2; i < pI830->FbMapSize; i <<= 1) ;
   pI830->FbMapSize = i;

   if (!I830MapMMIO(pScrn))
      return FALSE;

   pI830->FbBase = xf86MapPciMem(pScrn->scrnIndex, VIDMEM_FRAMEBUFFER,
				 pI830->PciTag,
				 pI830->LinearAddr, pI830->FbMapSize);
   if (!pI830->FbBase)
      return FALSE;

   if (IsPrimary(pScrn))
   pI830->LpRing->virtual_start = pI830->FbBase + pI830->LpRing->mem.Start;

   return TRUE;
}

static void
I830UnmapMMIO(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   xf86UnMapVidMem(pScrn->scrnIndex, (pointer) pI830->MMIOBase,
		   I810_REG_SIZE);
   pI830->MMIOBase = 0;
}

static Bool
I830UnmapMem(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   xf86UnMapVidMem(pScrn->scrnIndex, (pointer) pI830->FbBase,
		   pI830->FbMapSize);
   pI830->FbBase = 0;
   I830UnmapMMIO(pScrn);
   return TRUE;
}

#ifndef HAVE_GET_PUT_BIOSMEMSIZE
#define HAVE_GET_PUT_BIOSMEMSIZE 1
#endif

#if HAVE_GET_PUT_BIOSMEMSIZE
/*
 * Tell the BIOS how much video memory is available.  The BIOS call used
 * here won't always be available.
 */
static Bool
PutBIOSMemSize(ScrnInfoPtr pScrn, int memSize)
{
   vbeInfoPtr pVbe = I830PTR(pScrn)->pVbe;

   DPRINTF(PFX, "PutBIOSMemSize: %d kB\n", memSize / 1024);

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f11;
   pVbe->pInt10->bx = 0;
   pVbe->pInt10->cx = memSize / GTT_PAGE_SIZE;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   return Check5fStatus(pScrn, 0x5f11, pVbe->pInt10->ax);
}

/*
 * This reports what the previous VBEGetVBEInfo() found.  Be sure to call
 * VBEGetVBEInfo() after changing the BIOS memory size view.  If
 * a separate BIOS call is added for this, it can be put here.  Only
 * return a valid value if the funtionality for PutBIOSMemSize()
 * is available.
 */
static int
GetBIOSMemSize(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int memSize = KB(pI830->vbeInfo->TotalMemory * 64);

   DPRINTF(PFX, "GetBIOSMemSize\n");

   if (PutBIOSMemSize(pScrn, memSize))
      return memSize;
   else
      return -1;
}
#endif

/*
 * These three functions allow the video BIOS's view of the available video
 * memory to be changed.  This is currently implemented only for the 830
 * and 845G, which can do this via a BIOS scratch register that holds the
 * BIOS's view of the (pre-reserved) memory size.  If another mechanism
 * is available in the future, it can be plugged in here.  
 *
 * The mapping used for the 830/845G scratch register's low 4 bits is:
 *
 *             320k => 0
 *             832k => 1
 *            8000k => 8
 *
 * The "unusual" values are the 512k, 1M, 8M pre-reserved memory, less
 * overhead, rounded down to the BIOS-reported 64k granularity.
 */

static Bool
SaveBIOSMemSize(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   DPRINTF(PFX, "SaveBIOSMemSize\n");

   pI830->useSWF1 = FALSE;

#if HAVE_GET_PUT_BIOSMEMSIZE
   if ((pI830->saveBIOSMemSize = GetBIOSMemSize(pScrn)) != -1)
      return TRUE;
#endif

   if (IS_I830(pI830) || IS_845G(pI830)) {
      pI830->useSWF1 = TRUE;
      pI830->saveSWF1 = INREG(SWF1) & 0x0f;

      /*
       * This is for sample purposes only.  pI830->saveBIOSMemSize isn't used
       * when pI830->useSWF1 is TRUE.
       */
      switch (pI830->saveSWF1) {
      case 0:
	 pI830->saveBIOSMemSize = KB(320);
	 break;
      case 1:
	 pI830->saveBIOSMemSize = KB(832);
	 break;
      case 8:
	 pI830->saveBIOSMemSize = KB(8000);
	 break;
      default:
	 pI830->saveBIOSMemSize = 0;
	 break;
      }
      return TRUE;
   }
   return FALSE;
}

/*
 * TweakMemorySize() tweaks the BIOS image to set the correct size.
 * Original implementation by Christian Zietz in a stand-alone tool.
 */
static CARD32
TweakMemorySize(ScrnInfoPtr pScrn, CARD32 newsize, Bool preinit)
{
#define SIZE 0x10000
#define _855_IDOFFSET (-23)
#define _845_IDOFFSET (-19)
    
    const char *MAGICstring = "Total time for VGA POST:";
    const int len = strlen(MAGICstring);
    I830Ptr pI830 = I830PTR(pScrn);
    volatile char *position;
    char *biosAddr;
    CARD32 oldsize;
    CARD32 oldpermission;
    CARD32 ret = 0;
    int i,j = 0;
    int reg = (IS_845G(pI830) || IS_I865G(pI830)) ? _845_DRAM_RW_CONTROL
	: _855_DRAM_RW_CONTROL;
    
    PCITAG tag =pciTag(0,0,0);

    if(!pI830->PciInfo 
       || !(IS_845G(pI830) || IS_I85X(pI830) || IS_I865G(pI830)))
	return 0;

    if (!pI830->pVbe)
	return 0;

    biosAddr = xf86int10Addr(pI830->pVbe->pInt10, 
				    pI830->pVbe->pInt10->BIOSseg << 4);

    if (!pI830->BIOSMemSizeLoc) {
	if (!preinit)
	    return 0;

	/* Search for MAGIC string */
	for (i = 0; i < SIZE; i++) {
	    if (biosAddr[i] == MAGICstring[j]) {
		if (++j == len)
		    break;
	    } else {
		i -= j;
		j = 0;
	    }
	}
	if (j < len) return 0;

	pI830->BIOSMemSizeLoc =  (i - j + 1 + (IS_845G(pI830)
					    ? _845_IDOFFSET : _855_IDOFFSET));
    }
    
    position = biosAddr + pI830->BIOSMemSizeLoc;
    oldsize = *(CARD32 *)position;

    ret = oldsize - 0x21000;
    
    /* verify that register really contains current size */
    if (preinit && ((ret >> 16) !=  pI830->vbeInfo->TotalMemory))
	return 0;

    oldpermission = pciReadLong(tag, reg);
    pciWriteLong(tag, reg, DRAM_WRITE | (oldpermission & 0xffff)); 
    
    *(CARD32 *)position = newsize + 0x21000;

    if (preinit) {
	/* reinitialize VBE for new size */
	VBEFreeVBEInfo(pI830->vbeInfo);
	vbeFree(pI830->pVbe);
	pI830->pVbe = VBEInit(NULL, pI830->pEnt->index);
	pI830->vbeInfo = VBEGetVBEInfo(pI830->pVbe);
	
	/* verify that change was successful */
	if (pI830->vbeInfo->TotalMemory != (newsize >> 16)){
	    ret = 0;
	    *(CARD32 *)position = oldsize;
	} else {
	    pI830->BIOSMemorySize = KB(pI830->vbeInfo->TotalMemory * 64);
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO, 
		       "Tweak BIOS image to %d kB VideoRAM\n",
		       (int)(pI830->BIOSMemorySize / 1024));
	}
    }

    pciWriteLong(tag, reg, oldpermission);

     return ret;
}

static void
RestoreBIOSMemSize(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   CARD32 swf1;

   DPRINTF(PFX, "RestoreBIOSMemSize\n");

   if (TweakMemorySize(pScrn, pI830->saveBIOSMemSize,FALSE))
       return;

   if (!pI830->overrideBIOSMemSize)
      return;

#if HAVE_GET_PUT_BIOSMEMSIZE
   if (!pI830->useSWF1) {
      PutBIOSMemSize(pScrn, pI830->saveBIOSMemSize);
      return;
   }
#endif

   if ((IS_I830(pI830) || IS_845G(pI830)) && pI830->useSWF1) {
      swf1 = INREG(SWF1);
      swf1 &= ~0x0f;
      swf1 |= (pI830->saveSWF1 & 0x0f);
      OUTREG(SWF1, swf1);
   }
}

static void
SetBIOSMemSize(ScrnInfoPtr pScrn, int newSize)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned long swf1;
   Bool mapped;

   DPRINTF(PFX, "SetBIOSMemSize: %d kB\n", newSize / 1024);

   if (!pI830->overrideBIOSMemSize)
      return;

#if HAVE_GET_PUT_BIOSMEMSIZE
   if (!pI830->useSWF1) {
      PutBIOSMemSize(pScrn, newSize);
      return;
   }
#endif

   if ((IS_I830(pI830) || IS_845G(pI830)) && pI830->useSWF1) {
      unsigned long newSWF1;

      /* Need MMIO access here. */
      mapped = (pI830->MMIOBase != NULL);
      if (!mapped)
	 I830MapMMIO(pScrn);

      if (newSize <= KB(832))
	 newSWF1 = 1;
      else
	 newSWF1 = 8;

      swf1 = INREG(SWF1);
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Before: SWF1 is 0x%08lx\n", swf1);
      swf1 &= ~0x0f;
      swf1 |= (newSWF1 & 0x0f);
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "After: SWF1 is 0x%08lx\n", swf1);
      OUTREG(SWF1, swf1);
      if (!mapped)
	 I830UnmapMMIO(pScrn);
   }
}

/*
 * Use the native method instead of the vgahw method.  So far this is
 * only used for 8-bit mode.
 *
 * XXX Look into using the 10-bit gamma correction mode for 15/16/24 bit,
 * and see if a DirectColor visual can be offered.
 */
static void
I830LoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		LOCO * colors, VisualPtr pVisual)
{
   I830Ptr pI830;
   int i, index;
   unsigned char r, g, b;
   CARD32 val, temp;

   DPRINTF(PFX, "I830LoadPalette: numColors: %d\n", numColors);
   pI830 = I830PTR(pScrn);

   if (pI830->pipe == 0) {
      /* It seems that an initial read is needed. */
      temp = INREG(PALETTE_A);
      for (i = 0; i < numColors; i++) {
	 index = indices[i];
	 r = colors[index].red;
	 g = colors[index].green;
	 b = colors[index].blue;
	 val = (r << 16) | (g << 8) | b;
	 OUTREG(PALETTE_A + index * 4, val);
      }
   }
   if (pI830->pipe == 1) {
      /* It seems that an initial read is needed. */
      temp = INREG(PALETTE_B);
      for (i = 0; i < numColors; i++) {
	 index = indices[i];
	 r = colors[index].red;
	 g = colors[index].green;
	 b = colors[index].blue;
	 val = (r << 16) | (g << 8) | b;
	 OUTREG(PALETTE_B + index * 4, val);
      }
   }
}

static void
PreInitCleanup(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   if (IsPrimary(pScrn) && pI830->LpRing) {
      xfree(pI830->LpRing);
      pI830->LpRing = NULL;
   }
   if (IsPrimary(pScrn) && pI830->CursorMem) {
      xfree(pI830->CursorMem);
      pI830->CursorMem = NULL;
   }
   if (IsPrimary(pScrn) && pI830->CursorMemARGB) {
      xfree(pI830->CursorMemARGB);
      pI830->CursorMemARGB = NULL;
   }
   if (IsPrimary(pScrn) && pI830->OverlayMem) {
      xfree(pI830->OverlayMem);
      pI830->OverlayMem = NULL;
   }
   if (IsPrimary(pScrn) && pI830->overlayOn) {
      xfree(pI830->overlayOn);
      pI830->overlayOn = NULL;
   }
   if (!IsPrimary(pScrn) && pI830->entityPrivate)
      pI830->entityPrivate->pScrn_2 = NULL;
   RestoreBIOSMemSize(pScrn);
   if (pI830->swfSaved) {
      OUTREG(SWF0, pI830->saveSWF0);
      OUTREG(SWF4, pI830->saveSWF4);
   }
   if (pI830->MMIOBase)
      I830UnmapMMIO(pScrn);
   I830BIOSFreeRec(pScrn);
}

static int
GetBIOSPipe(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->pVbe;
   int pipe;

   DPRINTF(PFX, "GetBIOSPipe:\n");

   /* single pipe machines should always return Pipe A */
   if (pI830->availablePipes == 1) return 0;

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f1c;
   pVbe->pInt10->bx = 0x100;

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f1c, pVbe->pInt10->ax)) {
      if (pI830->newPipeSwitch) {
         pipe = ((pVbe->pInt10->bx & 0x0001));
      } else {
         pipe = ((pVbe->pInt10->cx & 0x0100) >> 8);
      }
      return pipe;
   }

   return -1;
}

static Bool
SetBIOSPipe(ScrnInfoPtr pScrn, int pipe)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->pVbe;

   DPRINTF(PFX, "SetBIOSPipe: pipe 0x%x\n", pipe);

   /* single pipe machines should always return TRUE */
   if (pI830->availablePipes == 1) return TRUE;

   pVbe->pInt10->num = 0x10;
   pVbe->pInt10->ax = 0x5f1c;
   if (pI830->newPipeSwitch) {
      pVbe->pInt10->bx = pipe;
      pVbe->pInt10->cx = 0;
   } else {
      pVbe->pInt10->bx = 0x0;
      pVbe->pInt10->cx = pipe << 8;
   }

   xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   if (Check5fStatus(pScrn, 0x5f1c, pVbe->pInt10->ax))
      return TRUE;
	
   return FALSE;
}

static Bool
SetPipeAccess(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   /* Don't try messing with the pipe, unless we're dual head */
   if (xf86IsEntityShared(pScrn->entityList[0]) || pI830->Clone) {
      if (!SetBIOSPipe(pScrn, pI830->pipe))
         return FALSE;
   }
   
   return TRUE;
}

static Bool
IsPrimary(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   if (xf86IsEntityShared(pScrn->entityList[0])) {
	if (pI830->init == 0) return TRUE;
	else return FALSE;
   }

   return TRUE;
}

static Bool
I830BIOSPreInit(ScrnInfoPtr pScrn, int flags)
{
   vgaHWPtr hwp;
   I830Ptr pI830;
   MessageType from;
   rgb defaultWeight = { 0, 0, 0 };
   EntityInfoPtr pEnt;
   I830EntPtr pI830Ent = NULL;					
   int mem, memsize;
   int flags24;
   int i, n;
   char *s;
   pointer pDDCModule, pVBEModule;
   Bool enable;
   const char *chipname;
   unsigned int ver;
   char v[5];

   if (pScrn->numEntities != 1)
      return FALSE;

   /* Load int10 module */
   if (!xf86LoadSubModule(pScrn, "int10"))
      return FALSE;
   xf86LoaderReqSymLists(I810int10Symbols, NULL);

   /* Load vbe module */
   if (!(pVBEModule = xf86LoadSubModule(pScrn, "vbe")))
      return FALSE;
   xf86LoaderReqSymLists(I810vbeSymbols, NULL);

   pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

   if (flags & PROBE_DETECT) {
      I830BIOSProbeDDC(pScrn, pEnt->index);
      return TRUE;
   }

   /* The vgahw module should be loaded here when needed */
   if (!xf86LoadSubModule(pScrn, "vgahw"))
      return FALSE;
   xf86LoaderReqSymLists(I810vgahwSymbols, NULL);

   /* Allocate a vgaHWRec */
   if (!vgaHWGetHWRec(pScrn))
      return FALSE;

   /* Allocate driverPrivate */
   if (!I830BIOSGetRec(pScrn))
      return FALSE;

   pI830 = I830PTR(pScrn);
   pI830->SaveGeneration = -1;
   pI830->pEnt = pEnt;

   if (pI830->pEnt->location.type != BUS_PCI)
      return FALSE;

   pI830->PciInfo = xf86GetPciInfoForEntity(pI830->pEnt->index);
   pI830->PciTag = pciTag(pI830->PciInfo->bus, pI830->PciInfo->device,
			  pI830->PciInfo->func);

    /* Allocate an entity private if necessary */
    if (xf86IsEntityShared(pScrn->entityList[0])) {
	pI830Ent = xf86GetEntityPrivate(pScrn->entityList[0],
					I830EntityIndex)->ptr;
        pI830->entityPrivate = pI830Ent;
    } else 
        pI830->entityPrivate = NULL;

   if (xf86RegisterResources(pI830->pEnt->index, 0, ResNone)) {
      PreInitCleanup(pScrn);
      return FALSE;
   }

   pScrn->racMemFlags = RAC_FB | RAC_COLORMAP;
   pScrn->monitor = pScrn->confScreen->monitor;
   pScrn->progClock = TRUE;
   pScrn->rgbBits = 8;

   flags24 = Support32bppFb | PreferConvert24to32 | SupportConvert24to32;

   if (!xf86SetDepthBpp(pScrn, 0, 0, 0, flags24))
      return FALSE;

   switch (pScrn->depth) {
   case 8:
   case 15:
   case 16:
   case 24:
      break;
   default:
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Given depth (%d) is not supported by I830 driver\n",
		 pScrn->depth);
      return FALSE;
   }
   xf86PrintDepthBpp(pScrn);

   if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
      return FALSE;
   if (!xf86SetDefaultVisual(pScrn, -1))
      return FALSE;

   hwp = VGAHWPTR(pScrn);
   pI830->cpp = pScrn->bitsPerPixel / 8;

   pI830->preinit = TRUE;

   /* Process the options */
   xf86CollectOptions(pScrn, NULL);
   if (!(pI830->Options = xalloc(sizeof(I830BIOSOptions))))
      return FALSE;
   memcpy(pI830->Options, I830BIOSOptions, sizeof(I830BIOSOptions));
   xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pI830->Options);

   /* We have to use PIO to probe, because we haven't mapped yet. */
   I830SetPIOAccess(pI830);

   /* Initialize VBE record */
   if ((pI830->pVbe = VBEInit(NULL, pI830->pEnt->index)) == NULL) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "VBE initialization failed.\n");
      return FALSE;
   }

   switch (pI830->PciInfo->chipType) {
   case PCI_CHIP_I830_M:
      chipname = "830M";
      break;
   case PCI_CHIP_845_G:
      chipname = "845G";
      break;
   case PCI_CHIP_I855_GM:
      /* Check capid register to find the chipset variant */
      pI830->variant = (pciReadLong(pI830->PciTag, I85X_CAPID)
				>> I85X_VARIANT_SHIFT) & I85X_VARIANT_MASK;
      switch (pI830->variant) {
      case I855_GM:
	 chipname = "855GM";
	 break;
      case I855_GME:
	 chipname = "855GME";
	 break;
      case I852_GM:
	 chipname = "852GM";
	 break;
      case I852_GME:
	 chipname = "852GME";
	 break;
      default:
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Unknown 852GM/855GM variant: 0x%x)\n", pI830->variant);
	 chipname = "852GM/855GM (unknown variant)";
	 break;
      }
      break;
   case PCI_CHIP_I865_G:
      chipname = "865G";
      break;
   case PCI_CHIP_I915_G:
      chipname = "915G";
      break;
   default:
      chipname = "unknown chipset";
      break;
   }
   xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	      "Integrated Graphics Chipset: Intel(R) %s\n", chipname);

   pI830->vbeInfo = VBEGetVBEInfo(pI830->pVbe);

   /* Set the Chipset and ChipRev, allowing config file entries to override. */
   if (pI830->pEnt->device->chipset && *pI830->pEnt->device->chipset) {
      pScrn->chipset = pI830->pEnt->device->chipset;
      from = X_CONFIG;
   } else if (pI830->pEnt->device->chipID >= 0) {
      pScrn->chipset = (char *)xf86TokenToString(I830BIOSChipsets,
						 pI830->pEnt->device->chipID);
      from = X_CONFIG;
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipID override: 0x%04X\n",
		 pI830->pEnt->device->chipID);
   } else {
      from = X_PROBED;
      pScrn->chipset = (char *)xf86TokenToString(I830BIOSChipsets,
						 pI830->PciInfo->chipType);
   }

   if (pI830->pEnt->device->chipRev >= 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "ChipRev override: %d\n",
		 pI830->pEnt->device->chipRev);
   }

   xf86DrvMsg(pScrn->scrnIndex, from, "Chipset: \"%s\"\n",
	      (pScrn->chipset != NULL) ? pScrn->chipset : "Unknown i8xx");

   if (pI830->pEnt->device->MemBase != 0) {
      pI830->LinearAddr = pI830->pEnt->device->MemBase;
      from = X_CONFIG;
   } else {
      if (IS_I915G(pI830)) {
	 pI830->LinearAddr = pI830->PciInfo->memBase[2] & 0xF0000000;
	 from = X_PROBED;
      } else if (pI830->PciInfo->memBase[1] != 0) {
	 /* XXX Check mask. */
	 pI830->LinearAddr = pI830->PciInfo->memBase[0] & 0xFF000000;
	 from = X_PROBED;
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "No valid FB address in PCI config space\n");
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
   }

   xf86DrvMsg(pScrn->scrnIndex, from, "Linear framebuffer at 0x%lX\n",
	      (unsigned long)pI830->LinearAddr);

   if (pI830->pEnt->device->IOBase != 0) {
      pI830->MMIOAddr = pI830->pEnt->device->IOBase;
      from = X_CONFIG;
   } else {
      if (IS_I915G(pI830)) {
	 pI830->MMIOAddr = pI830->PciInfo->memBase[0] & 0xFFF80000;
	 from = X_PROBED;
      } else if (pI830->PciInfo->memBase[1]) {
	 pI830->MMIOAddr = pI830->PciInfo->memBase[1] & 0xFFF80000;
	 from = X_PROBED;
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "No valid MMIO address in PCI config space\n");
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
   }

   xf86DrvMsg(pScrn->scrnIndex, from, "IO registers at addr 0x%lX\n",
	      (unsigned long)pI830->MMIOAddr);

   /* Some of the probing needs MMIO access, so map it here. */
   I830MapMMIO(pScrn);

#if 1
   pI830->saveSWF0 = INREG(SWF0);
   pI830->saveSWF4 = INREG(SWF4);
   pI830->swfSaved = TRUE;

   /* Set "extended desktop" */
   OUTREG(SWF0, pI830->saveSWF0 | (1 << 21));

   /* Set "driver loaded",  "OS unknown", "APM 1.2" */
   OUTREG(SWF4, (pI830->saveSWF4 & ~((3 << 19) | (7 << 16))) |
		(1 << 23) | (2 << 16));
#endif

   if (IS_I830(pI830) || IS_845G(pI830)) {
      PCITAG bridge;
      CARD16 gmch_ctrl;

      bridge = pciTag(0, 0, 0);		/* This is always the host bridge */
      gmch_ctrl = pciReadWord(bridge, I830_GMCH_CTRL);
      if ((gmch_ctrl & I830_GMCH_MEM_MASK) == I830_GMCH_MEM_128M) {
	 pI830->FbMapSize = 0x8000000;
      } else {
	 pI830->FbMapSize = 0x4000000; /* 64MB - has this been tested ?? */
      }
   } else {
      if (IS_I915G(pI830)) {
	 if (pI830->PciInfo->memBase[2] & 0x08000000)
	    pI830->FbMapSize = 0x8000000;	/* 128MB aperture */
	 else
	    pI830->FbMapSize = 0x10000000;	/* 256MB aperture */
      } else
	 /* 128MB aperture for later chips */
	 pI830->FbMapSize = 0x8000000;
   }


   if (xf86IsEntityShared(pScrn->entityList[0])) {
      if (xf86IsPrimInitDone(pScrn->entityList[0])) {
	 pI830->init = 1;

         if (!pI830Ent->pScrn_1) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
 		 "Failed to setup second head due to primary head failure.\n");
	    return FALSE;
         }

      } else {
         xf86SetPrimInitDone(pScrn->entityList[0]);
	 pI830->init = 0;
      }
   }

   if (IS_MOBILE(pI830) || IS_I915G(pI830))
      pI830->availablePipes = 2;
   else
      pI830->availablePipes = 1;
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "%d display pipe%s available.\n",
	      pI830->availablePipes, pI830->availablePipes > 1 ? "s" : "");

   /*
    * Get the pre-allocated (stolen) memory size.
    */
   pI830->StolenMemory.Size = I830DetectMemory(pScrn);
   pI830->StolenMemory.Start = 0;
   pI830->StolenMemory.End = pI830->StolenMemory.Size;

   /* Sanity check: compare with what the BIOS thinks. */
   if (pI830->vbeInfo->TotalMemory != pI830->StolenMemory.Size / 1024 / 64) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Detected stolen memory (%ld kB) doesn't match what the BIOS"
		 " reports (%d kB)\n",
		 ROUND_DOWN_TO(pI830->StolenMemory.Size / 1024, 64),
		 pI830->vbeInfo->TotalMemory * 64);
   }

   /* Find the maximum amount of agpgart memory available. */
   if (IsPrimary(pScrn)) {
      mem = I830CheckAvailableMemory(pScrn);
      pI830->StolenOnly = FALSE;
   } else {
      /* videoRam isn't used on the second head, but faked */
      mem = pI830->entityPrivate->pScrn_1->videoRam;
      pI830->StolenOnly = TRUE;
   }

   if (mem <= 0) {
      if (pI830->StolenMemory.Size <= 0) {
	 /* Shouldn't happen. */
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "/dev/agpgart is either not available, or no memory "
		 "is available\nfor allocation, "
		 "and no pre-allocated memory is available.\n");
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "/dev/agpgart is either not available, or no memory "
		 "is available\nfor allocation.  "
		 "Using pre-allocated memory only.\n");
      mem = 0;
      pI830->StolenOnly = TRUE;
   }

   if (xf86ReturnOptValBool(pI830->Options, OPTION_NOACCEL, FALSE)) {
      pI830->noAccel = TRUE;
   }
   if (xf86ReturnOptValBool(pI830->Options, OPTION_SW_CURSOR, FALSE)) {
      pI830->SWCursor = TRUE;
   }

   pI830->directRenderingDisabled =
	!xf86ReturnOptValBool(pI830->Options, OPTION_DRI, TRUE);

#ifdef XF86DRI
   if (!pI830->directRenderingDisabled) {
      if (pI830->noAccel || pI830->SWCursor) {
	 xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "DRI is disabled because it "
		    "needs HW cursor and 2D acceleration.\n");
	 pI830->directRenderingDisabled = TRUE;
      } else if (pScrn->depth != 16 && pScrn->depth != 24) {
	 xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "DRI is disabled because it "
		    "runs only at depths 16 and 24.\n");
	 pI830->directRenderingDisabled = TRUE;
      }
   }
#endif

   pI830->MonType1 = PIPE_NONE;
   pI830->MonType2 = PIPE_NONE;

   if ((s = xf86GetOptValString(pI830->Options, OPTION_MONITOR_LAYOUT)) &&
      IsPrimary(pScrn)) {
      char *Mon1;
      char *Mon2;
      char *sub;
        
      Mon1 = strtok(s, ",");
      Mon2 = strtok(NULL, ",");

      if (Mon1) {
         sub = strtok(Mon1, "+");
         do {
            if (strcmp(sub, "NONE") == 0)
               pI830->MonType1 |= PIPE_NONE;
            else if (strcmp(sub, "CRT") == 0)
               pI830->MonType1 |= PIPE_CRT;
            else if (strcmp(sub, "TV") == 0)
               pI830->MonType1 |= PIPE_TV;
            else if (strcmp(sub, "DFP") == 0)
               pI830->MonType1 |= PIPE_DFP;
            else if (strcmp(sub, "LFP") == 0)
               pI830->MonType1 |= PIPE_LFP;
            else if (strcmp(sub, "CRT2") == 0)
               pI830->MonType1 |= PIPE_CRT2;
            else if (strcmp(sub, "TV2") == 0)
               pI830->MonType1 |= PIPE_TV2;
            else if (strcmp(sub, "DFP2") == 0)
               pI830->MonType1 |= PIPE_DFP2;
            else if (strcmp(sub, "LFP2") == 0)
               pI830->MonType1 |= PIPE_LFP2;
            else 
               xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
			       "Invalid Monitor type specified for Pipe A\n"); 

            sub = strtok(NULL, "+");
         } while (sub);
      }

      if (Mon2) {
         sub = strtok(Mon2, "+");
         do {
            if (strcmp(sub, "NONE") == 0)
               pI830->MonType2 |= PIPE_NONE;
            else if (strcmp(sub, "CRT") == 0)
               pI830->MonType2 |= PIPE_CRT;
            else if (strcmp(sub, "TV") == 0)
               pI830->MonType2 |= PIPE_TV;
            else if (strcmp(sub, "DFP") == 0)
               pI830->MonType2 |= PIPE_DFP;
            else if (strcmp(sub, "LFP") == 0)
               pI830->MonType2 |= PIPE_LFP;
            else if (strcmp(sub, "CRT2") == 0)
               pI830->MonType2 |= PIPE_CRT2;
            else if (strcmp(sub, "TV2") == 0)
               pI830->MonType2 |= PIPE_TV2;
            else if (strcmp(sub, "DFP2") == 0)
               pI830->MonType2 |= PIPE_DFP2;
            else if (strcmp(sub, "LFP2") == 0)
               pI830->MonType2 |= PIPE_LFP2;
            else 
               xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
			       "Invalid Monitor type specified for Pipe B\n"); 

               sub = strtok(NULL, "+");
            } while (sub);
         }
    
         if (pI830->availablePipes == 1 && pI830->MonType2 != PIPE_NONE) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Monitor 2 cannot be specified on single pipe devices\n");
            return FALSE;
         }

         if (pI830->MonType1 == PIPE_NONE && pI830->MonType2 == PIPE_NONE) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Monitor 1 and 2 cannot be type NONE\n");
            return FALSE;
      }
   }

   if (xf86ReturnOptValBool(pI830->Options, OPTION_CLONE, FALSE)) {
      if (pI830->availablePipes == 1) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
 		 "Can't enable Clone Mode because this is a single pipe device\n");
         PreInitCleanup(pScrn);
         return FALSE;
      }
      if (pI830->entityPrivate) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR, 
 		 "Can't enable Clone Mode because second head is configured\n");
         PreInitCleanup(pScrn);
         return FALSE;
      }
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Enabling Clone Mode\n");
      pI830->CloneRefresh = 60; /* default to 60Hz */
      if (xf86GetOptValInteger(pI830->Options, OPTION_CLONE_REFRESH,
			    &(pI830->CloneRefresh))) {
         xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Clone Monitor Refresh Rate %d\n",
		 pI830->CloneRefresh);
      }
      if (pI830->CloneRefresh < 60 || pI830->CloneRefresh > 120) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Bad Clone Refresh Rate\n");
         PreInitCleanup(pScrn);
         return FALSE;
      }
      pI830->Clone = TRUE;
   }

    if ((pI830->entityPrivate && IsPrimary(pScrn)) || pI830->Clone) {
      if ((!xf86GetOptValString(pI830->Options, OPTION_MONITOR_LAYOUT))) {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "You must have a MonitorLayout "
	 		"defined for use in a DualHead or Clone setup.\n");
         PreInitCleanup(pScrn);
         return FALSE;
      }
         
      if (pI830->MonType1 == PIPE_NONE || pI830->MonType2 == PIPE_NONE) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Monitor 1 or Monitor 2 "
	 		"cannot be type NONE in Dual or Clone setup.\n");
         PreInitCleanup(pScrn);
         return FALSE;
      }
    }

   /*
    * Let's setup the mobile systems to check the lid status
    */
   if (IS_MOBILE(pI830)) {
      pI830->checkLid = TRUE;

      if (!xf86ReturnOptValBool(pI830->Options, OPTION_CHECKLID, TRUE)) {
         pI830->checkLid = FALSE;
         xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Checking Lid status disabled\n");
      } else
      if (pI830->entityPrivate && !IsPrimary(pScrn) &&
          !I830PTR(pI830->entityPrivate->pScrn_1)->checkLid) {
         /* If checklid is off, on the primary head, then 
          * turn it off on the secondary*/
         xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Checking Lid status disabled\n");
         pI830->checkLid = FALSE;
      } else
         xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Checking Lid status enabled\n");
   } else
      pI830->checkLid = FALSE;

   /*
    * The "VideoRam" config file parameter specifies the total amount of
    * memory that will be used/allocated.  When agpgart support isn't
    * available (StolenOnly == TRUE), this is limited to the amount of
    * pre-allocated ("stolen") memory.
    */

   /*
    * Default to I830_DEFAULT_VIDEOMEM_2D (8192KB) for 2D-only,
    * or I830_DEFAULT_VIDEOMEM_3D (32768KB) for 3D.  If the stolen memory
    * amount is higher, default to it rounded up to the nearest MB.  This
    * guarantees that by default there will be at least some run-time
    * space for things that need a physical address.
    * But, we double the amounts when dual head is enabled, and therefore
    * for 2D-only we use 16384KB, and 3D we use 65536KB. The VideoRAM 
    * for the second head is never used, as the primary head does the 
    * allocation.
    */
   if (!pI830->pEnt->device->videoRam) {
      from = X_DEFAULT;
#ifdef XF86DRI
      if (!pI830->directRenderingDisabled)
	 pScrn->videoRam = I830_DEFAULT_VIDEOMEM_3D;
      else
#endif
	 pScrn->videoRam = I830_DEFAULT_VIDEOMEM_2D;

      if (xf86IsEntityShared(pScrn->entityList[0])) {
         if (IsPrimary(pScrn))
            pScrn->videoRam *= 2;
      else
            pScrn->videoRam = I830_MAXIMUM_VBIOS_MEM;
      } 

      if (pI830->StolenMemory.Size / 1024 > pScrn->videoRam)
	 pScrn->videoRam = ROUND_TO(pI830->StolenMemory.Size / 1024, 1024);
   } else {
      from = X_CONFIG;
      pScrn->videoRam = pI830->pEnt->device->videoRam;
   }

   DPRINTF(PFX,
	   "Available memory: %dk\n"
	   "Requested memory: %dk\n", mem, pScrn->videoRam);


   if (mem + (pI830->StolenMemory.Size / 1024) < pScrn->videoRam) {
      pScrn->videoRam = mem + (pI830->StolenMemory.Size / 1024);
      from = X_PROBED;
      if (mem + (pI830->StolenMemory.Size / 1024) <
	  pI830->pEnt->device->videoRam) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "VideoRAM reduced to %d kByte "
		    "(limited to available sysmem)\n", pScrn->videoRam);
      }
   }

   if (pScrn->videoRam > pI830->FbMapSize / 1024) {
      pScrn->videoRam = pI830->FbMapSize / 1024;
      if (pI830->FbMapSize / 1024 < pI830->pEnt->device->videoRam)
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "VideoRam reduced to %d kByte (limited to aperture size)\n",
		    pScrn->videoRam);
   }

   if (mem > 0) {
      /*
       * If the reserved (BIOS accessible) memory is less than the desired
       * amount, try to increase it.  So far this is only implemented for
       * the 845G and 830, but those details are handled in SetBIOSMemSize().
       * 
       * The BIOS-accessible amount is only important for setting video
       * modes.  The maximum amount we try to set is limited to what would
       * be enough for 1920x1440 with a 2048 pitch.
       *
       * If ALLOCATE_ALL_BIOSMEM is enabled in i830_memory.c, all of the
       * BIOS-aware memory will get allocated.  If it isn't then it may
       * not be, and in that case there is an assumption that the video
       * BIOS won't attempt to access memory beyond what is needed for
       * modes that are actually used.  ALLOCATE_ALL_BIOSMEM is enabled by
       * default.
       */

      /* Try to keep HW cursor and Overlay amounts separate from this. */
      int reserve = (HWCURSOR_SIZE + HWCURSOR_SIZE_ARGB + OVERLAY_SIZE) / 1024;

      if (pScrn->videoRam - reserve >= I830_MAXIMUM_VBIOS_MEM)
	 pI830->newBIOSMemSize = KB(I830_MAXIMUM_VBIOS_MEM);
      else 
	 pI830->newBIOSMemSize =
			KB(ROUND_DOWN_TO(pScrn->videoRam - reserve, 64));
      if (pI830->vbeInfo->TotalMemory * 64 < pI830->newBIOSMemSize / 1024) {

	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Will attempt to tell the BIOS that there is "
		    "%d kB VideoRAM\n", pI830->newBIOSMemSize / 1024);
	 if (SaveBIOSMemSize(pScrn)) {
	    pI830->overrideBIOSMemSize = TRUE;
	    SetBIOSMemSize(pScrn, pI830->newBIOSMemSize);

	    VBEFreeVBEInfo(pI830->vbeInfo);
	    vbeFree(pI830->pVbe);
	    pI830->pVbe = VBEInit(NULL, pI830->pEnt->index);
	    pI830->vbeInfo = VBEGetVBEInfo(pI830->pVbe);

	    pI830->BIOSMemorySize = KB(pI830->vbeInfo->TotalMemory * 64);
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		       "BIOS now sees %ld kB VideoRAM\n",
		       pI830->BIOSMemorySize / 1024);
 	 } else if ((pI830->saveBIOSMemSize
		 = TweakMemorySize(pScrn, pI830->newBIOSMemSize,TRUE)) != 0) 
	     pI830->overrideBIOSMemSize = TRUE;
	 else {
	     xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			"BIOS view of memory size can't be changed "
			"(this is not an error).\n");
	 }
      }
   }

   xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	      "Pre-allocated VideoRAM: %ld kByte\n",
	      pI830->StolenMemory.Size / 1024);
   xf86DrvMsg(pScrn->scrnIndex, from, "VideoRAM: %d kByte\n",
	      pScrn->videoRam);

   pI830->TotalVideoRam = KB(pScrn->videoRam);

   /*
    * If the requested videoRam amount is less than the stolen memory size,
    * reduce the stolen memory size accordingly.
    */
   if (pI830->StolenMemory.Size > pI830->TotalVideoRam) {
      pI830->StolenMemory.Size = pI830->TotalVideoRam;
      pI830->StolenMemory.End = pI830->TotalVideoRam;
   }

   if (xf86GetOptValInteger(pI830->Options, OPTION_CACHE_LINES,
			    &(pI830->CacheLines))) {
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Requested %d cache lines\n",
		 pI830->CacheLines);
   } else {
      pI830->CacheLines = -1;
   }

   pI830->XvDisabled =
	!xf86ReturnOptValBool(pI830->Options, OPTION_XVIDEO, TRUE);

#ifdef I830_XV
   if (xf86GetOptValInteger(pI830->Options, OPTION_VIDEO_KEY,
			    &(pI830->colorKey))) {
      from = X_CONFIG;
   } else if (xf86GetOptValInteger(pI830->Options, OPTION_COLOR_KEY,
			    &(pI830->colorKey))) {
      from = X_CONFIG;
   } else {
      pI830->colorKey = (1 << pScrn->offset.red) |
			(1 << pScrn->offset.green) |
			(((pScrn->mask.blue >> pScrn->offset.blue) - 1) <<
			 pScrn->offset.blue);
      from = X_DEFAULT;
   }
   xf86DrvMsg(pScrn->scrnIndex, from, "video overlay key set to 0x%x\n",
	      pI830->colorKey);
#endif

   pI830->allowPageFlip = FALSE;
   enable = xf86ReturnOptValBool(pI830->Options, OPTION_PAGEFLIP, FALSE);
#ifdef XF86DRI
   if (!pI830->directRenderingDisabled) {
      pI830->allowPageFlip = enable;
      xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "page flipping %s\n",
		 enable ? "enabled" : "disabled");
   }
#endif

   /*
    * If the driver can do gamma correction, it should call xf86SetGamma() here.
    */

   {
      Gamma zeros = { 0.0, 0.0, 0.0 };

      if (!xf86SetGamma(pScrn, zeros)) {
         PreInitCleanup(pScrn);
	 return FALSE;
      }
   }

   GetBIOSVersion(pScrn, &ver);

   v[0] = (ver & 0xff000000) >> 24;
   v[1] = (ver & 0x00ff0000) >> 16;
   v[2] = (ver & 0x0000ff00) >> 8;
   v[3] = (ver & 0x000000ff) >> 0;
   v[4] = 0;
   
   pI830->bios_version = atoi(v);

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS Build: %d\n",pI830->bios_version);

   /* BIOS build 3062 changed the pipe switching functionality */
   if (pI830->availablePipes == 2 && pI830->bios_version >= 3062) {
      pI830->newPipeSwitch = TRUE;
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Using new Pipe switch code\n");
   } else
      pI830->newPipeSwitch = FALSE;

   pI830->devicePresence = FALSE;
   from = X_DEFAULT;
   if (xf86ReturnOptValBool(pI830->Options, OPTION_DEVICE_PRESENCE, FALSE)) {
      pI830->devicePresence = TRUE;
      from = X_CONFIG;
   }
   xf86DrvMsg(pScrn->scrnIndex, from, "Device Presence: %s.\n",
	      pI830->devicePresence ? "enabled" : "disabled");

   /* This performs an active detect of the currently attached monitors
    * or, at least it's meant to..... alas it doesn't seem to always work.
    */
   if (pI830->devicePresence) {
      int req, att, enc;
      GetDevicePresence(pScrn, &req, &att, &enc);
      for (i = 0; i < NumDisplayTypes; i++) {
         xf86DrvMsg(pScrn->scrnIndex, X_INFO,
	    "Display Presence: %s: attached: %s, encoder: %s\n",
	    displayDevices[i],
	    BOOLTOSTRING(((1<<i) & att)>>i),
	    BOOLTOSTRING(((1<<i) & enc)>>i));
      }
   }

   /* Save old configuration of detected devices */
   pI830->savedDevices = GetDisplayDevices(pScrn);

   if (IsPrimary(pScrn)) {
      pI830->pipe = GetBIOSPipe(pScrn);
      
      if (xf86ReturnOptValBool(pI830->Options, OPTION_FLIP_PRIMARY, FALSE)) {
         xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Primary flipping enabled\n");
         pI830->pipe = !pI830->pipe;
      }

      /* If the monitors aren't setup, read from the current config */
      if (pI830->MonType1 == PIPE_NONE)
         pI830->MonType1 = pI830->savedDevices & 0xff;
      if (pI830->MonType2 == PIPE_NONE)
         pI830->MonType2 = (pI830->savedDevices & 0xff00) >> 8;
   
      pI830->operatingDevices = (pI830->MonType2 << 8) | pI830->MonType1;

      if (!xf86IsEntityShared(pScrn->entityList[0]) && !pI830->Clone) {
	  /* If we're not dual head or clone, turn off the second head,
          * if monitorlayout is also specified. */

         if (pI830->pipe == 0)
            pI830->operatingDevices = pI830->MonType1;
         else
            pI830->operatingDevices = pI830->MonType2 << 8;

         if (pI830->operatingDevices & 0xFF00)
            xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Primary Pipe is %s, switching off second monitor (0x%x)\n",
			pI830->pipe ? "B" : "A", pI830->operatingDevices);
      }
   } else {
      I830Ptr pI8301 = I830PTR(pI830Ent->pScrn_1);
      pI830->operatingDevices = pI8301->operatingDevices;
   }

   /* Buggy BIOS 3066 is known to cause this, so turn this off */
   if (pI830->bios_version == 3066) {
      pI830->displayInfo = FALSE;
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Detected Broken Video BIOS, turning off displayInfo.\n");
   } else
      pI830->displayInfo = TRUE;
   from = X_DEFAULT;
   if (!xf86ReturnOptValBool(pI830->Options, OPTION_DISPLAY_INFO, TRUE)) {
      pI830->displayInfo = FALSE;
      from = X_CONFIG;
   }
   if (xf86ReturnOptValBool(pI830->Options, OPTION_DISPLAY_INFO, FALSE)) {
      pI830->displayInfo = TRUE;
      from = X_CONFIG;
   }
   xf86DrvMsg(pScrn->scrnIndex, from, "Display Info: %s.\n",
	      pI830->displayInfo ? "enabled" : "disabled");

   if (!I830DetectDisplayDevice(pScrn)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Couldn't detect display devices.\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

   if (!SetDisplayDevices(pScrn, pI830->operatingDevices)) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
 		 "Failed to switch to monitor configuration (0x%x)\n",
                 pI830->operatingDevices);
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Please check the devices specified in your MonitorLayout\n");
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "is configured correctly.\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

   PrintDisplayDeviceInfo(pScrn);

   if (xf86IsEntityShared(pScrn->entityList[0])) {
      if (!IsPrimary(pScrn)) {
         I830Ptr pI8301 = I830PTR(pI830Ent->pScrn_1);

         pI830Ent->pScrn_2 = pScrn;
         pI830->pipe = !pI8301->pipe;

	 /* This could be made to work with a little more fiddling */
	 pI830->directRenderingDisabled = TRUE;

         xf86DrvMsg(pScrn->scrnIndex, from, "Secondary head is using Pipe %s\n",
		pI830->pipe ? "B" : "A");
      } else {
         pI830Ent->pScrn_1 = pScrn;
         pI830Ent->pScrn_2 = NULL;

         xf86DrvMsg(pScrn->scrnIndex, from, "Primary head is using Pipe %s\n",
		pI830->pipe ? "B" : "A");
      }
   } else {
      xf86DrvMsg(pScrn->scrnIndex, from, "Display is using Pipe %s\n",
		pI830->pipe ? "B" : "A");
   }

   /* Alloc our pointers for the primary head */
   if (IsPrimary(pScrn)) {
      pI830->LpRing = xalloc(sizeof(I830RingBuffer));
      pI830->CursorMem = xalloc(sizeof(I830MemRange));
      pI830->CursorMemARGB = xalloc(sizeof(I830MemRange));
      pI830->OverlayMem = xalloc(sizeof(I830MemRange));
      pI830->overlayOn = xalloc(sizeof(Bool));
      if (!pI830->LpRing || !pI830->CursorMem || !pI830->CursorMemARGB ||
          !pI830->OverlayMem || !pI830->overlayOn) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Could not allocate primary data structures.\n");
         PreInitCleanup(pScrn);
         return FALSE;
      }
      *pI830->overlayOn = FALSE;
      if (pI830->entityPrivate)
         pI830->entityPrivate->XvInUse = -1;
   }

   /* Check if the HW cursor needs physical address. */
   if (IS_MOBILE(pI830) || IS_I915G(pI830))
      pI830->CursorNeedsPhysical = TRUE;
   else
      pI830->CursorNeedsPhysical = FALSE;

   /* Force ring buffer to be in low memory for the 845G and later. */
   if (IS_845G(pI830) || IS_I85X(pI830) || IS_I865G(pI830) || IS_I915G(pI830))
      pI830->NeedRingBufferLow = TRUE;

   /*
    * XXX If we knew the pre-initialised GTT format for certain, we could
    * probably figure out the physical address even in the StolenOnly case.
    */
   if (!IsPrimary(pScrn)) {
   	I830Ptr pI8301 = I830PTR(pI830Ent->pScrn_1);
	if (!pI8301->SWCursor) {
          xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "Using HW Cursor because it's enabled on primary head.\n");
          pI830->SWCursor = FALSE;
        }
   } else 
   if (pI830->StolenOnly && pI830->CursorNeedsPhysical && !pI830->SWCursor) {
      xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		 "HW Cursor disabled because it needs agpgart memory.\n");
      pI830->SWCursor = TRUE;
   }

   /*
    * Reduce the maximum videoram available for video modes by the ring buffer,
    * minimum scratch space and HW cursor amounts.
    */
   if (!pI830->SWCursor) {
      pScrn->videoRam -= (HWCURSOR_SIZE / 1024);
      pScrn->videoRam -= (HWCURSOR_SIZE_ARGB / 1024);
   }
   if (!pI830->XvDisabled)
      pScrn->videoRam -= (OVERLAY_SIZE / 1024);
   if (!pI830->noAccel) {
      pScrn->videoRam -= (PRIMARY_RINGBUFFER_SIZE / 1024);
      pScrn->videoRam -= (MIN_SCRATCH_BUFFER_SIZE / 1024);
   }

   xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	      "Maximum frambuffer space: %d kByte\n", pScrn->videoRam);

   SetPipeAccess(pScrn);
   if ((pDDCModule = xf86LoadSubModule(pScrn, "ddc")) == NULL) {
      PreInitCleanup(pScrn);
      return FALSE;
   }

   if ((pI830->vesa->monitor = vbeDoEDID(pI830->pVbe, pDDCModule)) != NULL) {
      xf86PrintEDID(pI830->vesa->monitor);
   }
   if ((pScrn->monitor->DDC = pI830->vesa->monitor) != NULL)
      xf86SetDDCproperties(pScrn, pI830->vesa->monitor);
   xf86UnloadSubModule(pDDCModule);

   /* XXX Move this to a header. */
#define VIDEO_BIOS_SCRATCH 0x18

#if 1
   /*
    * XXX This should be in ScreenInit/EnterVT.  PreInit should not leave the
    * state changed.
    */
   /* Enable hot keys by writing the proper value to GR18 */
   {
      CARD8 gr18;

      gr18 = pI830->readControl(pI830, GRX, VIDEO_BIOS_SCRATCH);
      gr18 &= ~0x80;			/*
					 * Clear Hot key bit so that Video
					 * BIOS performs the hot key
					 * servicing
					 */
      pI830->writeControl(pI830, GRX, VIDEO_BIOS_SCRATCH, gr18);
   }
#endif

   pI830->useExtendedRefresh = FALSE;

   if (xf86IsEntityShared(pScrn->entityList[0]) || pI830->Clone) {
      int pipe =
	  (pI830->operatingDevices >> PIPE_SHIFT(pI830->pipe)) & PIPE_ACTIVE_MASK;
      if (pipe & ~PIPE_CRT_ACTIVE) {
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		    "A non-CRT device is attached to pipe %c.\n"
		    "\tNo refresh rate overrides will be attempted.\n",
		    PIPE_NAME(pI830->pipe));
	 pI830->vesa->useDefaultRefresh = TRUE;
      }
      /*
       * Some desktop platforms might not have 0x5f05, so useExtendedRefresh
       * would need to be set to FALSE for those cases.
       */
      if (!pI830->vesa->useDefaultRefresh) 
	 pI830->useExtendedRefresh = TRUE;
   } else {
      for (i = 0; i < pI830->availablePipes; i++) {
         int pipe =
	  (pI830->operatingDevices >> PIPE_SHIFT(i)) & PIPE_ACTIVE_MASK;
         if (pipe & ~PIPE_CRT_ACTIVE) {
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
		    "A non-CRT device is attached to pipe %c.\n"
		    "\tNo refresh rate overrides will be attempted.\n",
		    PIPE_NAME(i));
	    pI830->vesa->useDefaultRefresh = TRUE;
         }
         /*
          * Some desktop platforms might not have 0x5f05, so useExtendedRefresh
          * would need to be set to FALSE for those cases.
          */
         if (!pI830->vesa->useDefaultRefresh) 
	    pI830->useExtendedRefresh = TRUE;
      }
   }

   if (pI830->useExtendedRefresh) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		 "Will use BIOS call 0x5f05 to set refresh rates for CRTs.\n");
   }

   /*
    * Limit videoram available for mode selection to what the video
    * BIOS can see.
    */
   if (pScrn->videoRam > (pI830->vbeInfo->TotalMemory * 64))
      memsize = pI830->vbeInfo->TotalMemory * 64;
   else
      memsize = pScrn->videoRam;
   xf86DrvMsg(pScrn->scrnIndex, X_PROBED,
	      "Maximum space available for video modes: %d kByte\n", memsize);

   /*
    * Note: VBE modes (> 0x7f) won't work with Intel's extended BIOS
    * functions.  For that reason it's important to set only
    * V_MODETYPE_VGA in the flags for VBEGetModePool().
    */
   pScrn->modePool = VBEGetModePool(pScrn, pI830->pVbe, pI830->vbeInfo,
				    V_MODETYPE_VGA);

   if (!pScrn->modePool) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "No Video BIOS modes for chosen depth.\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

   SetPipeAccess(pScrn);
   VBESetModeNames(pScrn->modePool);

   /*
    * XXX DDC information: There's code in xf86ValidateModes
    * (VBEValidateModes) to set monitor defaults based on DDC information
    * where available.  If we need something that does better than this,
    * there's code in vesa/vesa.c.
    */

   /* XXX Need to get relevant modes and virtual parameters. */
   /* Do the mode validation without regard to special scanline pitches. */
   SetPipeAccess(pScrn);
   n = VBEValidateModes(pScrn, NULL, pScrn->display->modes, NULL,
			NULL, 0, MAX_DISPLAY_PITCH, 1,
			0, MAX_DISPLAY_HEIGHT,
			pScrn->display->virtualX,
			pScrn->display->virtualY,
			memsize, LOOKUP_BEST_REFRESH);
   if (n <= 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No valid modes.\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }

   xf86PruneDriverModes(pScrn);

   pScrn->currentMode = pScrn->modes;

   if (pScrn->modes == NULL) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes.\n");
      PreInitCleanup(pScrn);
      return FALSE;
   }
#ifndef USE_PITCHES
#define USE_PITCHES 1
#endif
   pI830->disableTiling = FALSE;

   /*
    * If DRI is potentially usable, check if there is enough memory available
    * for it, and if there's also enough to allow tiling to be enabled.
    */
#if defined(XF86DRI)
   if (IsPrimary(pScrn) && !pI830->directRenderingDisabled) {
      int savedDisplayWidth = pScrn->displayWidth;
      int memNeeded = 0;
      /* Good pitches to allow tiling.  Don't care about pitches < 256. */
      static const int pitches[] = {
	 128 * 2,
	 128 * 4,
	 128 * 8,
	 128 * 16,
	 128 * 32,
	 128 * 64,
	 0
      };

#ifdef I830_XV
      /*
       * Set this so that the overlay allocation is factored in when
       * appropriate.
       */
      pI830->XvEnabled = !pI830->XvDisabled;
#endif

      for (i = 0; pitches[i] != 0; i++) {
#if USE_PITCHES
	 if (pitches[i] >= pScrn->displayWidth) {
	    pScrn->displayWidth = pitches[i];
	    break;
	 }
#else
	 if (pitches[i] == pScrn->displayWidth)
	    break;
#endif
      }

      /*
       * If the displayWidth is a tilable pitch, test if there's enough
       * memory available to enable tiling.
       */
      if (pScrn->displayWidth == pitches[i]) {
	 I830ResetAllocations(pScrn, 0);
	 if (I830Allocate2DMemory(pScrn, ALLOCATE_DRY_RUN | ALLOC_INITIAL) &&
	     I830Allocate3DMemory(pScrn, ALLOCATE_DRY_RUN)) {
	    memNeeded = I830GetExcessMemoryAllocations(pScrn);
	    if (memNeeded > 0 || pI830->MemoryAperture.Size < 0) {
	       if (memNeeded > 0) {
		  xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			     "%d kBytes additional video memory is "
			     "required to\n\tenable tiling mode for DRI.\n",
			     (memNeeded + 1023) / 1024);
	       }
	       if (pI830->MemoryAperture.Size < 0) {
		  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			     "Allocation with DRI tiling enabled would "
			     "exceed the\n"
			     "\tmemory aperture size (%ld kB) by %ld kB.\n"
			     "\tReduce VideoRam amount to avoid this!\n",
			     pI830->FbMapSize / 1024,
			     -pI830->MemoryAperture.Size / 1024);
	       }
	       pScrn->displayWidth = savedDisplayWidth;
	       pI830->allowPageFlip = FALSE;
	    } else if (pScrn->displayWidth != savedDisplayWidth) {
	       xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			  "Increasing the scanline pitch to allow tiling mode "
			  "(%d -> %d).\n",
			  savedDisplayWidth, pScrn->displayWidth);
	    }
	 } else {
	    memNeeded = 0;
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Unexpected dry run allocation failure (1).\n");
	 }
      }
      if (memNeeded > 0 || pI830->MemoryAperture.Size < 0) {
	 /*
	  * Tiling can't be enabled.  Check if there's enough memory for DRI
	  * without tiling.
	  */
	 pI830->disableTiling = TRUE;
	 I830ResetAllocations(pScrn, 0);
	 if (I830Allocate2DMemory(pScrn, ALLOCATE_DRY_RUN | ALLOC_INITIAL) &&
	     I830Allocate3DMemory(pScrn, ALLOCATE_DRY_RUN | ALLOC_NO_TILING)) {
	    memNeeded = I830GetExcessMemoryAllocations(pScrn);
	    if (memNeeded > 0 || pI830->MemoryAperture.Size < 0) {
	       if (memNeeded > 0) {
		  xf86DrvMsg(pScrn->scrnIndex, X_INFO,
			     "%d kBytes additional video memory is required "
			     "to enable DRI.\n",
			     (memNeeded + 1023) / 1024);
	       }
	       if (pI830->MemoryAperture.Size < 0) {
		  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
			     "Allocation with DRI enabled would "
			     "exceed the\n"
			     "\tmemory aperture size (%ld kB) by %ld kB.\n"
			     "\tReduce VideoRam amount to avoid this!\n",
			     pI830->FbMapSize / 1024,
			     -pI830->MemoryAperture.Size / 1024);
	       }
	       pI830->directRenderingDisabled = TRUE;
	       xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Disabling DRI.\n");
	    }
	 } else {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		       "Unexpected dry run allocation failure (2).\n");
	 }
      }
   }
#endif

   SetPipeAccess(pScrn);
   VBEPrintModes(pScrn);

   if (!pI830->vesa->useDefaultRefresh) {
      /*
       * This sets the parameters for the VBE modes according to the best
       * usable parameters from the Monitor sections modes (usually the
       * default VESA modes), allowing for better than default refresh rates.
       * This only works for VBE 3.0 and later.  Also, we only do this
       * if there are no non-CRT devices attached.
       */
      SetPipeAccess(pScrn);
      VBESetModeParameters(pScrn, pI830->pVbe);
   }

   /* PreInit shouldn't leave any state changes, so restore this. */
   RestoreBIOSMemSize(pScrn);

   /* Don't need MMIO access anymore. */
   if (pI830->swfSaved) {
      OUTREG(SWF0, pI830->saveSWF0);
      OUTREG(SWF4, pI830->saveSWF4);
   }

   /* Set display resolution */
   xf86SetDpi(pScrn, 0, 0);

   /* Load the required sub modules */
   if (!xf86LoadSubModule(pScrn, "fb")) {
      PreInitCleanup(pScrn);
      return FALSE;
   }

   xf86LoaderReqSymLists(I810fbSymbols, NULL);

   if (!pI830->noAccel) {
      if (!xf86LoadSubModule(pScrn, "xaa")) {
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
      xf86LoaderReqSymLists(I810xaaSymbols, NULL);
   }

   if (!pI830->SWCursor) {
      if (!xf86LoadSubModule(pScrn, "ramdac")) {
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
      xf86LoaderReqSymLists(I810ramdacSymbols, NULL);
   }

   if (!SetDisplayDevices(pScrn, pI830->savedDevices)) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Failed to switch to saved display devices, continuing.\n");
   }

   I830UnmapMMIO(pScrn);

   /*  We won't be using the VGA access after the probe. */
   I830SetMMIOAccess(pI830);
   xf86SetOperatingState(resVgaIo, pI830->pEnt->index, ResUnusedOpr);
   xf86SetOperatingState(resVgaMem, pI830->pEnt->index, ResDisableOpr);

   VBEFreeVBEInfo(pI830->vbeInfo);
   vbeFree(pI830->pVbe);

   /* Use the VBE mode restore workaround by default. */
   pI830->vbeRestoreWorkaround = TRUE;
   from = X_DEFAULT;
   if (xf86ReturnOptValBool(pI830->Options, OPTION_VBE_RESTORE, FALSE)) {
      pI830->vbeRestoreWorkaround = FALSE;
      from = X_CONFIG;
   }
   xf86DrvMsg(pScrn->scrnIndex, from, "VBE Restore workaround: %s.\n",
	      pI830->vbeRestoreWorkaround ? "enabled" : "disabled");
      
#if defined(XF86DRI)
   /* Load the dri module if requested. */
   if (xf86ReturnOptValBool(pI830->Options, OPTION_DRI, FALSE) &&
       !pI830->directRenderingDisabled) {
      if (xf86LoadSubModule(pScrn, "dri")) {
	 xf86LoaderReqSymLists(I810driSymbols, I810drmSymbols, NULL);
      }
   }

   if (!pI830->directRenderingDisabled) {
      if (!xf86LoadSubModule(pScrn, "shadow")) {
	 PreInitCleanup(pScrn);
	 return FALSE;
      }
      xf86LoaderReqSymLists(I810shadowSymbols, NULL);
   }
#endif

   pI830->preinit = FALSE;

   return TRUE;
}

/*
 * As the name says.  Check that the initial state is reasonable.
 * If any unrecoverable problems are found, bail out here.
 */
static Bool
CheckInheritedState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int errors = 0, fatal = 0;
   unsigned long temp, head, tail;

   if (!IsPrimary(pScrn)) return TRUE;

   /* Check first for page table errors */
   temp = INREG(PGE_ERR);
   if (temp != 0) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "PGTBL_ER is 0x%08lx\n", temp);
      errors++;
   }
   temp = INREG(PGETBL_CTL);
   if (!(temp & 1)) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "PGTBL_CTL (0x%08lx) indicates GTT is disabled\n", temp);
      errors++;
   }
   temp = INREG(LP_RING + RING_LEN);
   if (temp & 1) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "PRB0_CTL (0x%08lx) indicates ring buffer enabled\n", temp);
      errors++;
   }
   head = INREG(LP_RING + RING_HEAD);
   tail = INREG(LP_RING + RING_TAIL);
   if ((tail & I830_TAIL_MASK) != (head & I830_HEAD_MASK)) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "PRB0_HEAD (0x%08lx) and PRB0_TAIL (0x%08lx) indicate "
		 "ring buffer not flushed\n", head, tail);
      errors++;
   }

#if 0
   if (errors)
      I830PrintErrorState(pScrn);
#endif

   if (fatal)
      FatalError("CheckInheritedState: can't recover from the above\n");

   return (errors != 0);
}

/*
 * Reset registers that it doesn't make sense to save/restore to a sane state.
 * This is basically the ring buffer and fence registers.  Restoring these
 * doesn't make sense without restoring GTT mappings.  This is something that
 * whoever gets control next should do.
 */
static void
ResetState(ScrnInfoPtr pScrn, Bool flush)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int i;
   unsigned long temp;

   DPRINTF(PFX, "ResetState: flush is %s\n", BOOLTOSTRING(flush));

   if (!IsPrimary(pScrn)) return;

   if (pI830->entityPrivate)
      pI830->entityPrivate->RingRunning = 0;

   /* Reset the fence registers to 0 */
   for (i = 0; i < 8; i++)
      OUTREG(FENCE + i * 4, 0);

   /* Flush the ring buffer (if enabled), then disable it. */
   if (pI830->AccelInfoRec != NULL && flush) {
      temp = INREG(LP_RING + RING_LEN);
      if (temp & 1) {
	 I830RefreshRing(pScrn);
	 I830Sync(pScrn);
	 DO_RING_IDLE();
      }
   }
   OUTREG(LP_RING + RING_LEN, 0);
   OUTREG(LP_RING + RING_HEAD, 0);
   OUTREG(LP_RING + RING_TAIL, 0);
   OUTREG(LP_RING + RING_START, 0);

   if (pI830->CursorInfoRec && pI830->CursorInfoRec->HideCursor)
       pI830->CursorInfoRec->HideCursor(pScrn);
}

static void
SetFenceRegs(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   int i;

   DPRINTF(PFX, "SetFenceRegs\n");

   if (!IsPrimary(pScrn)) return;

   for (i = 0; i < 8; i++) {
      OUTREG(FENCE + i * 4, pI830->ModeReg.Fence[i]);
      if (I810_DEBUG & DEBUG_VERBOSE_VGA)
	 ErrorF("Fence Register : %x\n", pI830->ModeReg.Fence[i]);
   }
}

static void
SetRingRegs(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned int itemp;

   DPRINTF(PFX, "SetRingRegs\n");

   if (pI830->noAccel)
      return;

   if (!IsPrimary(pScrn)) return;

   if (pI830->entityPrivate)
      pI830->entityPrivate->RingRunning = 1;

   OUTREG(LP_RING + RING_LEN, 0);
   OUTREG(LP_RING + RING_TAIL, 0);
   OUTREG(LP_RING + RING_HEAD, 0);

   if ((long)(pI830->LpRing->mem.Start & I830_RING_START_MASK) !=
       pI830->LpRing->mem.Start) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "I830SetRingRegs: Ring buffer start (%lx) violates its "
		 "mask (%x)\n", pI830->LpRing->mem.Start, I830_RING_START_MASK);
   }
   /* Don't care about the old value.  Reserved bits must be zero anyway. */
   itemp = pI830->LpRing->mem.Start & I830_RING_START_MASK;
   OUTREG(LP_RING + RING_START, itemp);

   if (((pI830->LpRing->mem.Size - 4096) & I830_RING_NR_PAGES) !=
       pI830->LpRing->mem.Size - 4096) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "I830SetRingRegs: Ring buffer size - 4096 (%lx) violates its "
		 "mask (%x)\n", pI830->LpRing->mem.Size - 4096,
		 I830_RING_NR_PAGES);
   }
   /* Don't care about the old value.  Reserved bits must be zero anyway. */
   itemp = (pI830->LpRing->mem.Size - 4096) & I830_RING_NR_PAGES;
   itemp |= (RING_NO_REPORT | RING_VALID);
   OUTREG(LP_RING + RING_LEN, itemp);
   I830RefreshRing(pScrn);
}

/*
 * This should be called everytime the X server gains control of the screen,
 * before any video modes are programmed (ScreenInit, EnterVT).
 */
static void
SetHWOperatingState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   DPRINTF(PFX, "SetHWOperatingState\n");

   if (!pI830->noAccel)
      SetRingRegs(pScrn);
   SetFenceRegs(pScrn);
   if (!pI830->SWCursor)
      I830InitHWCursor(pScrn);
}

static Bool
SaveHWState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->pVbe;
   vgaHWPtr hwp = VGAHWPTR(pScrn);
   vgaRegPtr vgaReg = &hwp->SavedReg;
   VbeModeInfoBlock *modeInfo;
   VESAPtr pVesa;

   DPRINTF(PFX, "SaveHWState\n");

   SetPipeAccess(pScrn);

   pVesa = pI830->vesa;

   /* Make sure we save at least this information in case of failure. */
   VBEGetVBEMode(pVbe, &pVesa->stateMode);
   modeInfo = VBEGetModeInfo(pVbe, pVesa->stateMode);
   pVesa->savedScanlinePitch = 0;
   if (modeInfo) {
      if (VBE_MODE_GRAPHICS(modeInfo)) {
         VBEGetLogicalScanline(pVbe, &pVesa->savedScanlinePitch, NULL, NULL);
      }
      VBEFreeModeInfo(modeInfo);
   }

   vgaHWUnlock(hwp);
   vgaHWSave(pScrn, vgaReg, VGA_SR_FONTS);

   pVesa = pI830->vesa;
   /*
    * This save/restore method doesn't work for 845G BIOS, or for some
    * other platforms.  Enable it in all cases.
    */
   /*
    * KW: This may have been because of the behaviour I've found on my
    * board: The 'save' command actually modifies the interrupt
    * registers, turning off the irq & breaking the kernel module
    * behaviour.
    */
   if (!pI830->vbeRestoreWorkaround) {
      CARD16 imr = INREG16(IMR);
      CARD16 ier = INREG16(IER);
      CARD16 hwstam = INREG16(HWSTAM);

      if (!VBESaveRestore(pVbe, MODE_SAVE, &pVesa->state, &pVesa->stateSize,
			  &pVesa->statePage)) {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "SaveHWState: VBESaveRestore(MODE_SAVE) failed.\n");
	 return FALSE;
      }

      OUTREG16(IMR, imr);
      OUTREG16(IER, ier);
      OUTREG16(HWSTAM, hwstam);
   }

   pVesa->savedPal = VBESetGetPaletteData(pVbe, FALSE, 0, 256,
					     NULL, FALSE, FALSE);
   if (!pVesa->savedPal) {
      xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "SaveHWState: VBESetGetPaletteData(GET) failed.\n");
      return FALSE;
   }

   VBEGetDisplayStart(pVbe, &pVesa->x, &pVesa->y);

   return TRUE;
}

static Bool
RestoreHWState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->pVbe;
   vgaHWPtr hwp = VGAHWPTR(pScrn);
   vgaRegPtr vgaReg = &hwp->SavedReg;
   VESAPtr pVesa;
   Bool restored = FALSE;

   DPRINTF(PFX, "RestoreHWState\n");

   pVesa = pI830->vesa;

   /*
    * Workaround for text mode restoration with some flat panels.
    * Temporarily program a 640x480 mode before switching back to
    * text mode.
    */
   if (pVesa->useDefaultRefresh) {
      int mode = 0;

      switch (pScrn->depth) {
      case 8:
	 mode = 0x30;
	 break;
      case 15:
	 mode = 0x40;
	 break;
      case 16:
	 mode = 0x41;
	 break;
      case 24:
	 mode = 0x50;
	 break;
      }
      mode |= (1 << 15) | (1 << 14);
      I830VESASetVBEMode(pScrn, mode, NULL);
   }

   if (pVesa->state && pVesa->stateSize) {
      CARD16 imr = INREG16(IMR);
      CARD16 ier = INREG16(IER);
      CARD16 hwstam = INREG16(HWSTAM);

      /* Make a copy of the state.  Don't rely on it not being touched. */
      if (!pVesa->pstate) {
	 pVesa->pstate = xalloc(pVesa->stateSize);
	 if (pVesa->pstate)
	    memcpy(pVesa->pstate, pVesa->state, pVesa->stateSize);
      }
      restored = VBESaveRestore(pVbe, MODE_RESTORE, &pVesa->state,
				   &pVesa->stateSize, &pVesa->statePage);
      if (!restored) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "RestoreHWState: VBESaveRestore failed.\n");
      }
      /* Copy back */
      if (pVesa->pstate)
	 memcpy(pVesa->state, pVesa->pstate, pVesa->stateSize);

      OUTREG16(IMR, imr);
      OUTREG16(IER, ier);
      OUTREG16(HWSTAM, hwstam);
   }
   /* If that failed, restore the original mode. */
   if (!restored) {
      xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Setting the original video mode instead of restoring\n\t"
		 "the saved state\n");
      I830VESASetVBEMode(pScrn, pVesa->stateMode, NULL);
   }
   if (pVesa->savedScanlinePitch)
       VBESetLogicalScanline(pVbe, pVesa->savedScanlinePitch);

   if (pVesa->savedPal)
      VBESetGetPaletteData(pVbe, TRUE, 0, 256, pVesa->savedPal, FALSE, TRUE);

   VBESetDisplayStart(pVbe, pVesa->x, pVesa->y, TRUE);

   vgaHWRestore(pScrn, vgaReg, VGA_SR_FONTS);
   vgaHWLock(hwp);
   return TRUE;
}

#ifndef USE_VBE
#define USE_VBE 1
#endif

static void I830SetCloneVBERefresh(ScrnInfoPtr pScrn, int mode, VbeCRTCInfoBlock * block, int refresh)
{
   I830Ptr pI830 = I830PTR(pScrn);
   DisplayModePtr p = NULL;
   int RefreshRate;
   int clock;

   /* Search for our mode and get a refresh to match */
   for (p = pScrn->monitor->Modes; p != NULL; p = p->next) {
      if ((p->HDisplay != pI830->CloneHDisplay) ||
          (p->VDisplay != pI830->CloneVDisplay) ||
          (p->Flags & (V_INTERLACE | V_DBLSCAN | V_CLKDIV2)))
         continue;
      RefreshRate = ((double)(p->Clock * 1000) /
                     (double)(p->HTotal * p->VTotal)) * 100;
      /* we could probably do better here that 2Hz boundaries */
      if (RefreshRate > (refresh - 200) && RefreshRate < (refresh + 200)) {
         block->HorizontalTotal = p->HTotal;
         block->HorizontalSyncStart = p->HSyncStart;
         block->HorizontalSyncEnd = p->HSyncEnd;
         block->VerticalTotal = p->VTotal;
         block->VerticalSyncStart = p->VSyncStart;
         block->VerticalSyncEnd = p->VSyncEnd;
         block->Flags = ((p->Flags & V_NHSYNC) ? CRTC_NHSYNC : 0) |
                        ((p->Flags & V_NVSYNC) ? CRTC_NVSYNC : 0);
         block->PixelClock = p->Clock * 1000;
         /* XXX May not have this. */
         clock = VBEGetPixelClock(pI830->pVbe, mode, block->PixelClock);
#ifdef DEBUG
         ErrorF("Setting clock %.2fMHz, closest is %.2fMHz\n",
                    (double)data->block->PixelClock / 1000000.0, 
                    (double)clock / 1000000.0);
#endif
         if (clock)
            block->PixelClock = clock;
         block->RefreshRate = ((double)(block->PixelClock) /
                               (double)(p->HTotal * p->VTotal)) * 100;
         return;
      }
   }
}

static Bool
I830VESASetVBEMode(ScrnInfoPtr pScrn, int mode, VbeCRTCInfoBlock * block)
{
   I830Ptr pI830 = I830PTR(pScrn);

   DPRINTF(PFX, "Setting mode 0x%.8x\n", mode);

   if (pI830->Clone && !pI830->preinit) {
      VbeCRTCInfoBlock newblock;
      int Mon;

      if (!pI830->pipe)
         Mon = pI830->MonType2;
      else
         Mon = pI830->MonType1;

      SetBIOSPipe(pScrn, !pI830->pipe);

      /* The reason for this code is if we've not got a CRT on this pipe, then
       * make sure we're using a 60Hz refresh */
      if (pI830->useExtendedRefresh && !pI830->vesa->useDefaultRefresh &&
         (mode & (1 << 11)) && block) {
         /* we'll call 5F05 to set the refresh later.... */
         if (Mon != PIPE_CRT)
            VBESetVBEMode(pI830->pVbe, mode, NULL);
         else
            VBESetVBEMode(pI830->pVbe, mode, block);
      } else { 
         if (Mon != PIPE_CRT)
            /* Set clone head to 60Hz because we ain't got a CRT on it */
            I830SetCloneVBERefresh(pScrn, mode, &newblock, 6000);
         else
            /* Set clone head to specified clone refresh rate */
            I830SetCloneVBERefresh(pScrn, mode, &newblock, pI830->CloneRefresh * 100);
         if (newblock.RefreshRate == 0)
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	       "Failed to setup clone head mode resolution and refresh.\n");
         else
            VBESetVBEMode(pI830->pVbe, mode, &newblock);
      }
   }

   SetPipeAccess(pScrn);

#if USE_VBE
   return VBESetVBEMode(pI830->pVbe, mode, block);
#else
   {
      vbeInfoPtr pVbe = pI830->pVbe;
      pVbe->pInt10->num = 0x10;
      pVbe->pInt10->ax = 0x80 | (mode & 0x7f);
      xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
      pVbe->pInt10->ax = 0x0f00;
      xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
      if ((pVbe->pInt10->ax & 0x7f) == (mode & 0x7f))
	 return TRUE;
      else
	 return FALSE;
   }
#endif
}

static Bool
I830VESASetMode(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->pVbe;
   VbeModeInfoData *data = (VbeModeInfoData *) pMode->Private;
   int mode, i;
   CARD32 planeA, planeB, temp;
   int refresh = 60;
#ifdef XF86DRI
   Bool didLock = FALSE;
#endif

   DPRINTF(PFX, "I830VESASetMode\n");

   /* Always Enable Linear Addressing */
   mode = data->mode | (1 << 15) | (1 << 14);

#ifdef XF86DRI
   if (pI830->directRenderingEnabled && !pI830->LockHeld) {
      DRILock(screenInfo.screens[pScrn->scrnIndex], 0);
      pI830->LockHeld = 1;
      didLock = TRUE;
   }
#endif

   /*
    * Do this early to find out if we can support it or not....
    * Test if the extendedRefresh BIOS function is supported.
    */
   if (pI830->useExtendedRefresh && !pI830->vesa->useDefaultRefresh &&
       (mode & (1 << 11)) && data && data->data && data->block) {
      SetPipeAccess(pScrn);
      if (!SetRefreshRate(pScrn, mode, 60)) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "BIOS call 0x5f05 not supported, "
		    "setting refresh with VBE 3 method.\n");
	 pI830->useExtendedRefresh = FALSE;
      }
   }

   if (pI830->Clone) {
      pI830->CloneHDisplay = pMode->HDisplay;
      pI830->CloneVDisplay = pMode->VDisplay;
   }

#ifndef MODESWITCH_RESET_STATE
#define MODESWITCH_RESET_STATE 0
#endif
#if MODESWITCH_RESET_STATE
   ResetState(pScrn, TRUE);
#endif

   /* XXX Add macros for the various mode parameter bits. */

   if (pI830->vesa->useDefaultRefresh)
      mode &= ~(1 << 11);

   if (I830VESASetVBEMode(pScrn, mode, data->block) == FALSE) {
      if ((data->block && (mode & (1 << 11))) &&
	  I830VESASetVBEMode(pScrn, (mode & ~(1 << 11)), NULL) == TRUE) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Set VBE Mode rejected this modeline.\n\t"
		    "Trying standard mode instead!\n");
	 DPRINTF(PFX, "OOPS!\n");
	 xfree(data->block);
	 data->block = NULL;
	 data->mode &= ~(1 << 11);
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Set VBE Mode failed!\n");
	 return FALSE;
      }
   }

   /*
    * The BIOS may not set a scanline pitch that would require more video
    * memory than it's aware of.  We check for this later, and set it
    * explicitly if necessary.
    */
   if (data->data->XResolution != pScrn->displayWidth) {
      if (pI830->Clone) {
         SetBIOSPipe(pScrn, !pI830->pipe);
         VBESetLogicalScanline(pVbe, pScrn->displayWidth);
      }
      SetPipeAccess(pScrn);
      VBESetLogicalScanline(pVbe, pScrn->displayWidth);
   }

   if (pScrn->bitsPerPixel >= 8 && pI830->vbeInfo->Capabilities[0] & 0x01) {
      if (pI830->Clone) {
         SetBIOSPipe(pScrn, !pI830->pipe);
         VBESetGetDACPaletteFormat(pVbe, 8);
      }
      SetPipeAccess(pScrn);
      VBESetGetDACPaletteFormat(pVbe, 8);
   }

   /*
    * When it's OK to set better than default refresh rates, set them here.
    */
   if (pI830->Clone) {
      int Mon;
      if (!pI830->pipe)
         Mon = pI830->MonType2;
      else
         Mon = pI830->MonType1;
      SetBIOSPipe(pScrn, !pI830->pipe);
      if (pI830->CloneRefresh && (Mon == PIPE_CRT)) {
         if (pI830->useExtendedRefresh && !pI830->vesa->useDefaultRefresh &&
             (mode & (1 << 11)) && data && data->data && data->block) {
            refresh = SetRefreshRate(pScrn, mode, pI830->CloneRefresh);
            if (!refresh)
	       xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Failed to set refresh rate to %dHz on Clone head.\n",
		    pI830->CloneRefresh);
            else
	       xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Set refresh rate to %dHz on Clone head.\n",
		    pI830->CloneRefresh);
         } else
	    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "Will use VBE3 method to set refresh on Clone head.\n");
      } else {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
	  "Not attempting to override default refresh on non-CRT clone head\n");
      }
   }

   if (pI830->useExtendedRefresh && !pI830->vesa->useDefaultRefresh &&
       (mode & (1 << 11)) && data && data->data && data->block) {
      SetPipeAccess(pScrn);
      refresh = SetRefreshRate(pScrn, mode, data->block->RefreshRate / 100);
      if (!refresh) {
	 refresh = 60;
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Failed to set refresh rate to %dHz.\n",
		    data->block->RefreshRate / 100);
      }
   }

   /* XXX Fix plane A with pipe A, and plane B with pipe B. */
   planeA = INREG(DSPACNTR);
   planeB = INREG(DSPBCNTR);

   pI830->planeEnabled[0] = ((planeA & DISPLAY_PLANE_ENABLE) != 0);
   pI830->planeEnabled[1] = ((planeB & DISPLAY_PLANE_ENABLE) != 0);

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Display plane A is %s and connected to %s.\n",
	      pI830->planeEnabled[0] ? "enabled" : "disabled",
	      planeA & DISPPLANE_SEL_PIPE_MASK ? "Pipe B" : "Pipe A");
   if (pI830->availablePipes == 2)
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Display plane B is %s and connected to %s.\n",
	      pI830->planeEnabled[1] ? "enabled" : "disabled",
	      planeB & DISPPLANE_SEL_PIPE_MASK ? "Pipe B" : "Pipe A");
   
   if (xf86IsEntityShared(pScrn->entityList[0]) || pI830->Clone) {
      pI830->planeEnabled[0] = 1;
      pI830->pipeEnabled[0] = 1;
      pI830->planeEnabled[1] = 1;
      pI830->pipeEnabled[1] = 1;
   }

   /*
    * Sometimes it seems that no display planes are enabled at this point.
    * For mobile platforms pick the plane(s) connected to enabled pipes.
    * For others choose plane A.
    */
   if (!pI830->planeEnabled[0] && !pI830->planeEnabled[1]) {
      if (pI830->availablePipes == 2) {
	 if ((pI830->pipeEnabled[0] &&
	      ((planeA & DISPPLANE_SEL_PIPE_MASK) == DISPPLANE_SEL_PIPE_A)) ||
	     (pI830->pipeEnabled[1] &&
	      ((planeA & DISPPLANE_SEL_PIPE_MASK) == DISPPLANE_SEL_PIPE_B))) {
	    pI830->planeEnabled[0] = TRUE;
	 }
	 if ((pI830->pipeEnabled[0] &&
	      ((planeB & DISPPLANE_SEL_PIPE_MASK) == DISPPLANE_SEL_PIPE_A)) ||
	     (pI830->pipeEnabled[1] &&
	      ((planeB & DISPPLANE_SEL_PIPE_MASK) == DISPPLANE_SEL_PIPE_B))) {
	    pI830->planeEnabled[1] = TRUE;
	 }
      } else {
	 pI830->planeEnabled[0] = TRUE;
      }
   }
   if (pI830->planeEnabled[0]) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Enabling plane A.\n");
      planeA |= DISPLAY_PLANE_ENABLE;
      planeA &= ~DISPPLANE_SEL_PIPE_MASK;
      planeA |= DISPPLANE_SEL_PIPE_A;
      OUTREG(DSPACNTR, planeA);
      /* flush the change. */
      temp = INREG(DSPABASE);
      OUTREG(DSPABASE, temp);
   }
   if (pI830->planeEnabled[1]) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Enabling plane B.\n");
      planeB |= DISPLAY_PLANE_ENABLE;
      planeB &= ~DISPPLANE_SEL_PIPE_MASK;
      planeB |= DISPPLANE_SEL_PIPE_B;
      OUTREG(DSPBCNTR, planeB);
      /* flush the change. */
      temp = INREG(DSPBADDR);
      OUTREG(DSPBADDR, temp);
   }

   planeA = INREG(DSPACNTR);
   planeB = INREG(DSPBCNTR);
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Display plane A is now %s and connected to %s.\n",
	      pI830->planeEnabled[0] ? "enabled" : "disabled",
	      planeA & DISPPLANE_SEL_PIPE_MASK ? "Pipe B" : "Pipe A");
   if (pI830->availablePipes == 2)
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Display plane B is now %s and connected to %s.\n",
	      pI830->planeEnabled[1] ? "enabled" : "disabled",
	      planeB & DISPPLANE_SEL_PIPE_MASK ? "Pipe B" : "Pipe A");

   /* XXX Plane C is ignored for now (overlay). */

   /*
    * Print out the PIPEACONF and PIPEBCONF registers.
    */
   temp = INREG(PIPEACONF);
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "PIPEACONF is 0x%08lx\n", temp);
   if (pI830->availablePipes == 2) {
      temp = INREG(PIPEBCONF);
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "PIPEBCONF is 0x%08lx\n", temp);
   }

   if (xf86IsEntityShared(pScrn->entityList[0])) {
      /* Clean this up !! */
      if (IsPrimary(pScrn)) {
         CARD32 stridereg = !pI830->pipe ? DSPASTRIDE : DSPBSTRIDE;
         CARD32 basereg = !pI830->pipe ? DSPABASE : DSPBBASE;
         CARD32 sizereg = !pI830->pipe ? DSPASIZE : DSPBSIZE;
         I830Ptr pI8301 = I830PTR(pI830->entityPrivate->pScrn_1);

         temp = INREG(stridereg);
         if (temp / pI8301->cpp != (CARD32)(pI830->entityPrivate->pScrn_1->displayWidth)) {
            xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Correcting plane %c stride (%d -> %d)\n", PIPE_NAME(pI830->pipe),
		    (int)(temp / pI8301->cpp), pI830->entityPrivate->pScrn_1->displayWidth);
	    OUTREG(stridereg, pI830->entityPrivate->pScrn_1->displayWidth * pI8301->cpp);
         }
         OUTREG(sizereg, (pMode->HDisplay - 1) | ((pMode->VDisplay - 1) << 16));
         /* Trigger update */
         temp = INREG(basereg);
         OUTREG(basereg, temp);

         if (pI830->entityPrivate && pI830->entityPrivate->pScrn_2) {
            I830Ptr pI8302 = I830PTR(pI830->entityPrivate->pScrn_2);
            stridereg = pI830->pipe ? DSPASTRIDE : DSPBSTRIDE;
            basereg = pI830->pipe ? DSPABASE : DSPBBASE;
            sizereg = pI830->pipe ? DSPASIZE : DSPBSIZE;

            temp = INREG(stridereg);
            if (temp / pI8302->cpp != (CARD32)(pI830->entityPrivate->pScrn_2->displayWidth)) {
	       xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Correcting plane %c stride (%d -> %d)\n", PIPE_NAME(!pI830->pipe),
		    (int)(temp / pI8302->cpp), pI830->entityPrivate->pScrn_2->displayWidth);
	       OUTREG(stridereg, pI830->entityPrivate->pScrn_2->displayWidth * pI8302->cpp);
            }
            OUTREG(sizereg, (pI830->entityPrivate->pScrn_2->currentMode->HDisplay - 1) | ((pI830->entityPrivate->pScrn_2->currentMode->VDisplay - 1) << 16));
            /* Trigger update */
            temp = INREG(basereg);
            OUTREG(basereg, temp);
         }
      } else {
         CARD32 stridereg = pI830->pipe ? DSPASTRIDE : DSPBSTRIDE;
         CARD32 basereg = pI830->pipe ? DSPABASE : DSPBBASE;
         CARD32 sizereg = pI830->pipe ? DSPASIZE : DSPBSIZE;
         I830Ptr pI8301 = I830PTR(pI830->entityPrivate->pScrn_1);
         I830Ptr pI8302 = I830PTR(pI830->entityPrivate->pScrn_2);

         temp = INREG(stridereg);
         if (temp / pI8301->cpp != (CARD32)(pI830->entityPrivate->pScrn_1->displayWidth)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Correcting plane %c stride (%d -> %d)\n", PIPE_NAME(pI830->pipe),
		    (int)(temp / pI8301->cpp), pI830->entityPrivate->pScrn_1->displayWidth);
	    OUTREG(stridereg, pI830->entityPrivate->pScrn_1->displayWidth * pI8301->cpp);
         }
         OUTREG(sizereg, (pI830->entityPrivate->pScrn_1->currentMode->HDisplay - 1) | ((pI830->entityPrivate->pScrn_1->currentMode->VDisplay - 1) << 16));
         /* Trigger update */
         temp = INREG(basereg);
         OUTREG(basereg, temp);

         stridereg = !pI830->pipe ? DSPASTRIDE : DSPBSTRIDE;
         basereg = !pI830->pipe ? DSPABASE : DSPBBASE;
         sizereg = !pI830->pipe ? DSPASIZE : DSPBSIZE;

         temp = INREG(stridereg);
         if (temp / pI8302->cpp != ((CARD32)pI830->entityPrivate->pScrn_2->displayWidth)) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Correcting plane %c stride (%d -> %d)\n", PIPE_NAME(!pI830->pipe),
		    (int)(temp / pI8302->cpp), pI830->entityPrivate->pScrn_2->displayWidth);
	    OUTREG(stridereg, pI830->entityPrivate->pScrn_2->displayWidth * pI8302->cpp);
         }
         OUTREG(sizereg, (pMode->HDisplay - 1) | ((pMode->VDisplay - 1) << 16));
         /* Trigger update */
         temp = INREG(basereg);
         OUTREG(basereg, temp);
      }
   } else {
      for (i = 0; i < pI830->availablePipes; i++) {
         CARD32 stridereg = i ? DSPBSTRIDE : DSPASTRIDE;
         CARD32 basereg = i ? DSPBBASE : DSPABASE;
         CARD32 sizereg = i ? DSPBSIZE : DSPASIZE;

         if (!pI830->planeEnabled[i])
	    continue;

         temp = INREG(stridereg);
         if (temp / pI830->cpp != (CARD32)pScrn->displayWidth) {
	    xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		    "Correcting plane %c stride (%d -> %d)\n", PIPE_NAME(i),
		    (int)(temp / pI830->cpp), pScrn->displayWidth);
	    OUTREG(stridereg, pScrn->displayWidth * pI830->cpp);
         }
         OUTREG(sizereg, (pMode->HDisplay - 1) | ((pMode->VDisplay - 1) << 16));
	 /* Trigger update */
	 temp = INREG(basereg);
	 OUTREG(basereg, temp);
      }
   }

#if 0
   /* Print out some CRTC/display information. */
   temp = INREG(HTOTAL_A);
   ErrorF("Horiz active: %d, Horiz total: %d\n", temp & 0x7ff,
	  (temp >> 16) & 0xfff);
   temp = INREG(HBLANK_A);
   ErrorF("Horiz blank start: %d, Horiz blank end: %d\n", temp & 0xfff,
	  (temp >> 16) & 0xfff);
   temp = INREG(HSYNC_A);
   ErrorF("Horiz sync start: %d, Horiz sync end: %d\n", temp & 0xfff,
	  (temp >> 16) & 0xfff);
   temp = INREG(VTOTAL_A);
   ErrorF("Vert active: %d, Vert total: %d\n", temp & 0x7ff,
	  (temp >> 16) & 0xfff);
   temp = INREG(VBLANK_A);
   ErrorF("Vert blank start: %d, Vert blank end: %d\n", temp & 0xfff,
	  (temp >> 16) & 0xfff);
   temp = INREG(VSYNC_A);
   ErrorF("Vert sync start: %d, Vert sync end: %d\n", temp & 0xfff,
	  (temp >> 16) & 0xfff);
   temp = INREG(PIPEASRC);
   ErrorF("Image size: %dx%d (%dx%d)\n",
          (temp >> 16) & 0x7ff, temp & 0x7ff,
	  (((temp >> 16) & 0x7ff) + 1), ((temp & 0x7ff) + 1));
   ErrorF("Pixel multiply is %d\n", (planeA >> 20) & 0x3);
   temp = INREG(DSPABASE);
   ErrorF("Plane A start offset is %d\n", temp);
   temp = INREG(DSPASTRIDE);
   ErrorF("Plane A stride is %d bytes (%d pixels)\n", temp, temp / pI830->cpp);
   temp = INREG(DSPAPOS);
   ErrorF("Plane A position %d %d\n", temp & 0xffff, (temp & 0xffff0000) >> 16);
   temp = INREG(DSPASIZE);
   ErrorF("Plane A size %d %d\n", temp & 0xffff, (temp & 0xffff0000) >> 16);

   /* Print out some CRTC/display information. */
   temp = INREG(HTOTAL_B);
   ErrorF("Horiz active: %d, Horiz total: %d\n", temp & 0x7ff,
	  (temp >> 16) & 0xfff);
   temp = INREG(HBLANK_B);
   ErrorF("Horiz blank start: %d, Horiz blank end: %d\n", temp & 0xfff,
	  (temp >> 16) & 0xfff);
   temp = INREG(HSYNC_B);
   ErrorF("Horiz sync start: %d, Horiz sync end: %d\n", temp & 0xfff,
	  (temp >> 16) & 0xfff);
   temp = INREG(VTOTAL_B);
   ErrorF("Vert active: %d, Vert total: %d\n", temp & 0x7ff,
	  (temp >> 16) & 0xfff);
   temp = INREG(VBLANK_B);
   ErrorF("Vert blank start: %d, Vert blank end: %d\n", temp & 0xfff,
	  (temp >> 16) & 0xfff);
   temp = INREG(VSYNC_B);
   ErrorF("Vert sync start: %d, Vert sync end: %d\n", temp & 0xfff,
	  (temp >> 16) & 0xfff);
   temp = INREG(PIPEBSRC);
   ErrorF("Image size: %dx%d (%dx%d)\n",
          (temp >> 16) & 0x7ff, temp & 0x7ff,
	  (((temp >> 16) & 0x7ff) + 1), ((temp & 0x7ff) + 1));
   ErrorF("Pixel multiply is %d\n", (planeA >> 20) & 0x3);
   temp = INREG(DSPBBASE);
   ErrorF("Plane B start offset is %d\n", temp);
   temp = INREG(DSPBSTRIDE);
   ErrorF("Plane B stride is %d bytes (%d pixels)\n", temp, temp / pI830->cpp);
   temp = INREG(DSPBPOS);
   ErrorF("Plane B position %d %d\n", temp & 0xffff, (temp & 0xffff0000) >> 16);
   temp = INREG(DSPBSIZE);
   ErrorF("Plane B size %d %d\n", temp & 0xffff, (temp & 0xffff0000) >> 16);
#endif

   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Mode bandwidth is %d Mpixel/s\n",
	      pMode->HDisplay * pMode->VDisplay * refresh / 1000000);

   {
      int maxBandwidth, bandwidthA, bandwidthB;

      if (GetModeSupport(pScrn, 0x80, 0x80, 0x80, 0x80,
			&maxBandwidth, &bandwidthA, &bandwidthB)) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "maxBandwidth is %d Mbyte/s, "
		    "pipe bandwidths are %d Mbyte/s, %d Mbyte/s\n",
		    maxBandwidth, bandwidthA, bandwidthB);
      }
   }

   {
      int ret;

      ret = GetLFPCompMode(pScrn);
      if (ret != -1) {
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO,
		    "LFP compensation mode: 0x%x\n", ret);
      }
   }

#if MODESWITCH_RESET_STATE
   ResetState(pScrn, TRUE);
   SetHWOperatingState(pScrn);
#endif

#ifdef XF86DRI
   if (pI830->directRenderingEnabled && didLock) {
      DRIUnlock(screenInfo.screens[pScrn->scrnIndex]);
      pI830->LockHeld = 0;
   }
#endif

   pScrn->vtSema = TRUE;
   return TRUE;
}

static void
InitRegisterRec(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   I830RegPtr i830Reg = &pI830->ModeReg;
   int i;

   if (!IsPrimary(pScrn)) return;

   for (i = 0; i < 8; i++)
      i830Reg->Fence[i] = 0;
}

/* Famous last words
 */
void
I830PrintErrorState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);

   ErrorF("pgetbl_ctl: 0x%lx pgetbl_err: 0x%lx\n",
	  INREG(PGETBL_CTL), INREG(PGE_ERR));

   ErrorF("ipeir: %lx iphdr: %lx\n", INREG(IPEIR), INREG(IPEHR));

   ErrorF("LP ring tail: %lx head: %lx len: %lx start %lx\n",
	  INREG(LP_RING + RING_TAIL),
	  INREG(LP_RING + RING_HEAD) & HEAD_ADDR,
	  INREG(LP_RING + RING_LEN), INREG(LP_RING + RING_START));

   ErrorF("eir: %x esr: %x emr: %x\n",
	  INREG16(EIR), INREG16(ESR), INREG16(EMR));

   ErrorF("instdone: %x instpm: %x\n", INREG16(INST_DONE), INREG8(INST_PM));

   ErrorF("memmode: %lx instps: %lx\n", INREG(MEMMODE), INREG(INST_PS));

   ErrorF("hwstam: %x ier: %x imr: %x iir: %x\n",
	  INREG16(HWSTAM), INREG16(IER), INREG16(IMR), INREG16(IIR));
}

#ifdef I830DEBUG
static void
dump_DSPACNTR(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned int tmp;

   /* Display A Control */
   tmp = INREG(0x70180);
   ErrorF("Display A Plane Control Register (0x%.8x)\n", tmp);

   if (tmp & BIT(31))
      ErrorF("   Display Plane A (Primary) Enable\n");
   else
      ErrorF("   Display Plane A (Primary) Disabled\n");

   if (tmp & BIT(30))
      ErrorF("   Display A pixel data is gamma corrected\n");
   else
      ErrorF("   Display A pixel data bypasses gamma correction logic (default)\n");

   switch ((tmp & 0x3c000000) >> 26) {	/* bit 29:26 */
   case 0x00:
   case 0x01:
   case 0x03:
      ErrorF("   Reserved\n");
      break;
   case 0x02:
      ErrorF("   8-bpp Indexed\n");
      break;
   case 0x04:
      ErrorF("   15-bit (5-5-5) pixel format (Targa compatible)\n");
      break;
   case 0x05:
      ErrorF("   16-bit (5-6-5) pixel format (XGA compatible)\n");
      break;
   case 0x06:
      ErrorF("   32-bit format (X:8:8:8)\n");
      break;
   case 0x07:
      ErrorF("   32-bit format (8:8:8:8)\n");
      break;
   default:
      ErrorF("   Unknown - Invalid register value maybe?\n");
   }

   if (tmp & BIT(25))
      ErrorF("   Stereo Enable\n");
   else
      ErrorF("   Stereo Disable\n");

   if (tmp & BIT(24))
      ErrorF("   Display A, Pipe B Select\n");
   else
      ErrorF("   Display A, Pipe A Select\n");

   if (tmp & BIT(22))
      ErrorF("   Source key is enabled\n");
   else
      ErrorF("   Source key is disabled\n");

   switch ((tmp & 0x00300000) >> 20) {	/* bit 21:20 */
   case 0x00:
      ErrorF("   No line duplication\n");
      break;
   case 0x01:
      ErrorF("   Line/pixel Doubling\n");
      break;
   case 0x02:
   case 0x03:
      ErrorF("   Reserved\n");
      break;
   }

   if (tmp & BIT(18))
      ErrorF("   Stereo output is high during second image\n");
   else
      ErrorF("   Stereo output is high during first image\n");
}

static void
dump_DSPBCNTR(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned int tmp;

   /* Display B/Sprite Control */
   tmp = INREG(0x71180);
   ErrorF("Display B/Sprite Plane Control Register (0x%.8x)\n", tmp);

   if (tmp & BIT(31))
      ErrorF("   Display B/Sprite Enable\n");
   else
      ErrorF("   Display B/Sprite Disable\n");

   if (tmp & BIT(30))
      ErrorF("   Display B pixel data is gamma corrected\n");
   else
      ErrorF("   Display B pixel data bypasses gamma correction logic (default)\n");

   switch ((tmp & 0x3c000000) >> 26) {	/* bit 29:26 */
   case 0x00:
   case 0x01:
   case 0x03:
      ErrorF("   Reserved\n");
      break;
   case 0x02:
      ErrorF("   8-bpp Indexed\n");
      break;
   case 0x04:
      ErrorF("   15-bit (5-5-5) pixel format (Targa compatible)\n");
      break;
   case 0x05:
      ErrorF("   16-bit (5-6-5) pixel format (XGA compatible)\n");
      break;
   case 0x06:
      ErrorF("   32-bit format (X:8:8:8)\n");
      break;
   case 0x07:
      ErrorF("   32-bit format (8:8:8:8)\n");
      break;
   default:
      ErrorF("   Unknown - Invalid register value maybe?\n");
   }

   if (tmp & BIT(25))
      ErrorF("   Stereo is enabled and both start addresses are used in a two frame sequence\n");
   else
      ErrorF("   Stereo disable and only a single start address is used\n");

   if (tmp & BIT(24))
      ErrorF("   Display B/Sprite, Pipe B Select\n");
   else
      ErrorF("   Display B/Sprite, Pipe A Select\n");

   if (tmp & BIT(22))
      ErrorF("   Sprite source key is enabled\n");
   else
      ErrorF("   Sprite source key is disabled (default)\n");

   switch ((tmp & 0x00300000) >> 20) {	/* bit 21:20 */
   case 0x00:
      ErrorF("   No line duplication\n");
      break;
   case 0x01:
      ErrorF("   Line/pixel Doubling\n");
      break;
   case 0x02:
   case 0x03:
      ErrorF("   Reserved\n");
      break;
   }

   if (tmp & BIT(18))
      ErrorF("   Stereo output is high during second image\n");
   else
      ErrorF("   Stereo output is high during first image\n");

   if (tmp & BIT(15))
      ErrorF("   Alpha transfer mode enabled\n");
   else
      ErrorF("   Alpha transfer mode disabled\n");

   if (tmp & BIT(0))
      ErrorF("   Sprite is above overlay\n");
   else
      ErrorF("   Sprite is above display A (default)\n");
}

void
I830_dump_registers(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   unsigned int i;

   ErrorF("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");

   dump_DSPACNTR(pScrn);
   dump_DSPBCNTR(pScrn);

   ErrorF("0x71400 == 0x%.8x\n", INREG(0x71400));
   ErrorF("0x70008 == 0x%.8x\n", INREG(0x70008));
   for (i = 0x71410; i <= 0x71428; i += 4)
      ErrorF("0x%x == 0x%.8x\n", i, INREG(i));

   ErrorF("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n");
}
#endif

static Bool
I830BIOSScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
   ScrnInfoPtr pScrn;
   vgaHWPtr hwp;
   I830Ptr pI830;
   VisualPtr visual;
   I830EntPtr pI830Ent = NULL;
   I830Ptr pI8301 = NULL;
#ifdef XF86DRI
   Bool driDisabled;
#endif

   pScrn = xf86Screens[pScreen->myNum];
   pI830 = I830PTR(pScrn);
   hwp = VGAHWPTR(pScrn);

   if (xf86IsEntityShared(pScrn->entityList[0])) {
      pI830Ent = pI830->entityPrivate;
      pI8301 = I830PTR(pI830Ent->pScrn_1);

      /* PreInit failed on the second head, so make sure we turn it off */
      if (IsPrimary(pScrn) && !pI830->entityPrivate->pScrn_2) {
         if (pI830->pipe == 0) {
            pI830->operatingDevices &= 0xFF;
         } else {
            pI830->operatingDevices &= 0xFF00;
         }
      }
   }

   pI830->starting = TRUE;

   /* Alloc our pointers for the primary head */
   if (IsPrimary(pScrn)) {
      if (!pI830->LpRing)
         pI830->LpRing = xalloc(sizeof(I830RingBuffer));
      if (!pI830->CursorMem)
         pI830->CursorMem = xalloc(sizeof(I830MemRange));
      if (!pI830->CursorMemARGB)
         pI830->CursorMemARGB = xalloc(sizeof(I830MemRange));
      if (!pI830->OverlayMem)
         pI830->OverlayMem = xalloc(sizeof(I830MemRange));
      if (!pI830->overlayOn)
         pI830->overlayOn = xalloc(sizeof(Bool));
      if (!pI830->LpRing || !pI830->CursorMem || !pI830->CursorMemARGB ||
          !pI830->OverlayMem || !pI830->overlayOn) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Could not allocate primary data structures.\n");
         return FALSE;
      }
      *pI830->overlayOn = FALSE;
      if (pI830->entityPrivate)
         pI830->entityPrivate->XvInUse = -1;
   }

   if (xf86IsEntityShared(pScrn->entityList[0])) {
	/* Make our second head point to the first heads structures */
	if (!IsPrimary(pScrn)) {
	   pI830->LpRing = pI8301->LpRing;
	   pI830->CursorMem = pI8301->CursorMem;
	   pI830->CursorMemARGB = pI8301->CursorMemARGB;
	   pI830->OverlayMem = pI8301->OverlayMem;
           pI830->overlayOn = pI8301->overlayOn;
	}
   }

   /*
    * If we're changing the BIOS's view of the video memory size, do that
    * first, then re-initialise the VBE information.
    */
   pI830->pVbe = VBEInit(NULL, pI830->pEnt->index);
   if (!TweakMemorySize(pScrn, pI830->newBIOSMemSize,FALSE))
       SetBIOSMemSize(pScrn, pI830->newBIOSMemSize);
   if (!pI830->pVbe)
      return FALSE;
   pI830->vbeInfo = VBEGetVBEInfo(pI830->pVbe);

   miClearVisualTypes();
   if (!xf86SetDefaultVisual(pScrn, -1))
      return FALSE;
   if (pScrn->bitsPerPixel > 8) {
      if (!miSetVisualTypes(pScrn->depth, TrueColorMask,
			    pScrn->rgbBits, TrueColor))
	 return FALSE;
   } else {
      if (!miSetVisualTypes(pScrn->depth,
			    miGetDefaultVisualMask(pScrn->depth),
			    pScrn->rgbBits, pScrn->defaultVisual))
	 return FALSE;
   }
   if (!miSetPixmapDepths())
      return FALSE;

#ifdef I830_XV
   pI830->XvEnabled = !pI830->XvDisabled;
   if (pI830->XvEnabled) {
      if (!IsPrimary(pScrn)) {
         if (!pI8301->XvEnabled || pI830->noAccel) {
            pI830->XvEnabled = FALSE;
	    xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Xv is disabled.\n");
         }
      } else
      if (pI830->noAccel || pI830->StolenOnly) {
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "Xv is disabled because it "
		    "needs 2D accel and AGPGART.\n");
	 pI830->XvEnabled = FALSE;
      }
   }
#else
   pI830->XvEnabled = FALSE;
#endif

   if (IsPrimary(pScrn)) {
      I830ResetAllocations(pScrn, 0);

      if (!I830Allocate2DMemory(pScrn, ALLOC_INITIAL))
	return FALSE;
   }

   if (!pI830->noAccel) {
      if (pI830->LpRing->mem.Size == 0) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		     "Disabling acceleration because the ring buffer "
		      "allocation failed.\n");
	   pI830->noAccel = TRUE;
      }
   }

   if (!pI830->SWCursor) {
      if (pI830->CursorMem->Size == 0) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		     "Disabling HW cursor because the cursor memory "
		      "allocation failed.\n");
	   pI830->SWCursor = TRUE;
      }
   }

#ifdef I830_XV
   if (pI830->XvEnabled) {
      if (pI830->noAccel) {
	 xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Disabling Xv because it "
		    "needs 2D acceleration.\n");
	 pI830->XvEnabled = FALSE;
      }
      if (pI830->OverlayMem->Physical == 0) {
	  xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		     "Disabling Xv because the overlay register buffer "
		      "allocation failed.\n");
	 pI830->XvEnabled = FALSE;
      }
   }
#endif

   InitRegisterRec(pScrn);

#ifdef XF86DRI
   /*
    * pI830->directRenderingDisabled is set once in PreInit.  Reinitialise
    * pI830->directRenderingEnabled based on it each generation.
    */
   pI830->directRenderingEnabled = !pI830->directRenderingDisabled;
   /*
    * Setup DRI after visuals have been established, but before fbScreenInit
    * is called.   fbScreenInit will eventually call into the drivers
    * InitGLXVisuals call back.
    */

   if (pI830->directRenderingEnabled) {
      if (pI830->noAccel || pI830->SWCursor || pI830->StolenOnly) {
	 xf86DrvMsg(pScrn->scrnIndex, X_PROBED, "DRI is disabled because it "
		    "needs HW cursor, 2D accel and AGPGART.\n");
	 pI830->directRenderingEnabled = FALSE;
      }
   }

   driDisabled = !pI830->directRenderingEnabled;

   if (pI830->directRenderingEnabled)
      pI830->directRenderingEnabled = I830DRIScreenInit(pScreen);

   if (pI830->directRenderingEnabled) {
      pI830->directRenderingEnabled =
	 I830Allocate3DMemory(pScrn,
			      pI830->disableTiling ? ALLOC_NO_TILING : 0);
      if (!pI830->directRenderingEnabled)
	  I830DRICloseScreen(pScreen);
   }

#else
   pI830->directRenderingEnabled = FALSE;
#endif

   /*
    * After the 3D allocations have been done, see if there's any free space
    * that can be added to the framebuffer allocation.
    */
   if (IsPrimary(pScrn)) {
      I830Allocate2DMemory(pScrn, 0);

      DPRINTF(PFX, "assert(if(!I830DoPoolAllocation(pScrn, pI830->StolenPool)))\n");
      if (!I830DoPoolAllocation(pScrn, &(pI830->StolenPool)))
         return FALSE;

      DPRINTF(PFX, "assert( if(!I830FixupOffsets(pScrn)) )\n");
      if (!I830FixupOffsets(pScrn))
         return FALSE;
   }

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      I830SetupMemoryTiling(pScrn);
      pI830->directRenderingEnabled = I830DRIDoMappings(pScreen);
   }
#endif

   DPRINTF(PFX, "assert( if(!I830MapMem(pScrn)) )\n");
   if (!I830MapMem(pScrn))
      return FALSE;

   pScrn->memPhysBase = (unsigned long)pI830->FbBase;

   if (IsPrimary(pScrn)) {
      pScrn->fbOffset = pI830->FrontBuffer.Start;
   } else {
      I830Ptr pI8301 = I830PTR(pI830Ent->pScrn_1);
      pScrn->fbOffset = pI8301->FrontBuffer2.Start;
   }

   pI830->xoffset = (pScrn->fbOffset / pI830->cpp) % pScrn->displayWidth;
   pI830->yoffset = (pScrn->fbOffset / pI830->cpp) / pScrn->displayWidth;

   vgaHWSetMmioFuncs(hwp, pI830->MMIOBase, 0);
   vgaHWGetIOBase(hwp);
   DPRINTF(PFX, "assert( if(!vgaHWMapMem(pScrn)) )\n");
   if (!vgaHWMapMem(pScrn))
      return FALSE;

   /* Clear SavedReg */
   memset(&pI830->SavedReg, 0, sizeof(pI830->SavedReg));

   DPRINTF(PFX, "assert( if(!I830BIOSEnterVT(scrnIndex, 0)) )\n");

   if (!I830BIOSEnterVT(scrnIndex, 0))
      return FALSE;

   DPRINTF(PFX, "assert( if(!fbScreenInit(pScreen, ...) )\n");
   if (!fbScreenInit(pScreen, pI830->FbBase + pScrn->fbOffset,
		     pScrn->virtualX, pScrn->virtualY,
		     pScrn->xDpi, pScrn->yDpi,
		     pScrn->displayWidth, pScrn->bitsPerPixel))
      return FALSE;

   if (pScrn->bitsPerPixel > 8) {
      /* Fixup RGB ordering */
      visual = pScreen->visuals + pScreen->numVisuals;
      while (--visual >= pScreen->visuals) {
	 if ((visual->class | DynamicClass) == DirectColor) {
	    visual->offsetRed = pScrn->offset.red;
	    visual->offsetGreen = pScrn->offset.green;
	    visual->offsetBlue = pScrn->offset.blue;
	    visual->redMask = pScrn->mask.red;
	    visual->greenMask = pScrn->mask.green;
	    visual->blueMask = pScrn->mask.blue;
	 }
      }
   }

   fbPictureInit(pScreen, 0, 0);

   xf86SetBlackWhitePixels(pScreen);

   I830DGAInit(pScreen);

   DPRINTF(PFX,
	   "assert( if(!xf86InitFBManager(pScreen, &(pI830->FbMemBox))) )\n");
   if (IsPrimary(pScrn)) {
      if (!xf86InitFBManager(pScreen, &(pI830->FbMemBox))) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Failed to init memory manager\n");
         return FALSE;
      }
   } else {
      if (!xf86InitFBManager(pScreen, &(pI8301->FbMemBox2))) {
         xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		 "Failed to init memory manager\n");
         return FALSE;
      }
   }

   if (!pI830->noAccel) {
      if (!I830AccelInit(pScreen)) {
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Hardware acceleration initialization failed\n");
      }
   }

   miInitializeBackingStore(pScreen);
   xf86SetBackingStore(pScreen);
   xf86SetSilkenMouse(pScreen);
   miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

   if (!pI830->SWCursor) {
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Initializing HW Cursor\n");
      if (!I830CursorInit(pScreen))
	 xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
		    "Hardware cursor initialization failed\n");
   } else
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Initializing SW Cursor!\n");

   DPRINTF(PFX, "assert( if(!miCreateDefColormap(pScreen)) )\n");
   if (!miCreateDefColormap(pScreen))
      return FALSE;

   DPRINTF(PFX, "assert( if(!xf86HandleColormaps(pScreen, ...)) )\n");
   if (!xf86HandleColormaps(pScreen, 256, 8, I830LoadPalette, 0,
			    CMAP_RELOAD_ON_MODE_SWITCH)) {
      return FALSE;
   }

   xf86DPMSInit(pScreen, I830DisplayPowerManagementSet, 0);

#ifdef I830_XV
   /* Init video */
   if (pI830->XvEnabled)
      I830InitVideo(pScreen);
#endif

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      pI830->directRenderingEnabled = I830DRIFinishScreenInit(pScreen);
   }
#endif

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      pI830->directRenderingOpen = TRUE;
      xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering: Enabled\n");
      /* Setup 3D engine */
      I830EmitInvarientState(pScrn);
   } else {
      if (driDisabled)
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering: Disabled\n");
      else
	 xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering: Failed\n");
   }
#else
   xf86DrvMsg(pScrn->scrnIndex, X_INFO, "direct rendering: Not available\n");
#endif

   pScreen->SaveScreen = I830BIOSSaveScreen;
   pI830->CloseScreen = pScreen->CloseScreen;
   pScreen->CloseScreen = I830BIOSCloseScreen;
 
   if (pI830->checkLid)
      pI830->lidTimer = TimerSet(NULL, 0, 1000, I830LidTimer, pScrn);

   if (serverGeneration == 1)
      xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

#ifdef I830DEBUG
   I830_dump_registers(pScrn);
#endif

   pI830->starting = FALSE;
   pI830->closing = FALSE;
   pI830->suspended = FALSE;
   return TRUE;
}

static void
I830BIOSAdjustFrame(int scrnIndex, int x, int y, int flags)
{
   ScrnInfoPtr pScrn;
   I830Ptr pI830;
   vbeInfoPtr pVbe;

   pScrn = xf86Screens[scrnIndex];
   pI830 = I830PTR(pScrn);
   pVbe = pI830->pVbe;

   DPRINTF(PFX, "I830BIOSAdjustFrame: y = %d (+ %d), x = %d (+ %d)\n",
	   x, pI830->xoffset, y, pI830->yoffset);

   /* The i830M just happens to have some problems programming offsets via
    * this VESA BIOS call. Especially in dual head configurations which
    * have high resolutions which cause the DSP{A,B}BASE registers to be
    * programmed incorrectly. Thus, it warrants bypassing the BIOS for i830M
    * and hitting the DSP{A,B}BASE registers directly. 
    *
    * We could probably do this for other platforms too, but we don't
    * know what else the Video BIOS may do when calling it. It seems safe
    * though for i830M during testing......
    *
    * Also note, calling the Video BIOS version first and then fixing the
    * registers fail on i830M and eventually cause a lockup of the hardware
    * in my testing.
    */

   if (pI830->Clone) {
      if (!IS_I830(pI830)) {
         SetBIOSPipe(pScrn, !pI830->pipe);
         VBESetDisplayStart(pVbe, x + pI830->xoffset, y + pI830->yoffset, TRUE);
      } else {
         if (!pI830->pipe == 0) {
            OUTREG(DSPABASE, pScrn->fbOffset + ((y * pScrn->displayWidth + x) * pI830->cpp));
         } else {
            OUTREG(DSPBBASE, pScrn->fbOffset + ((y * pScrn->displayWidth + x) * pI830->cpp));
         }
      }
   }

   if (!IS_I830(pI830)) {
      SetPipeAccess(pScrn);
      VBESetDisplayStart(pVbe, x + pI830->xoffset, y + pI830->yoffset, TRUE);
   } else {
      if (pI830->pipe == 0) {
         OUTREG(DSPABASE, pScrn->fbOffset + ((y * pScrn->displayWidth + x) * pI830->cpp));
      } else {
         OUTREG(DSPBBASE, pScrn->fbOffset + ((y * pScrn->displayWidth + x) * pI830->cpp));
      }
   }
}

static void
I830BIOSFreeScreen(int scrnIndex, int flags)
{
   I830BIOSFreeRec(xf86Screens[scrnIndex]);
   if (xf86LoaderCheckSymbol("vgaHWFreeHWRec"))
      vgaHWFreeHWRec(xf86Screens[scrnIndex]);
}

#ifndef SAVERESTORE_HWSTATE
#define SAVERESTORE_HWSTATE 0
#endif

#if SAVERESTORE_HWSTATE
static void
SaveHWOperatingState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   I830RegPtr save = &pI830->SavedReg;

   DPRINTF(PFX, "SaveHWOperatingState\n");

   return;
}

static void
RestoreHWOperatingState(ScrnInfoPtr pScrn)
{
   I830Ptr pI830 = I830PTR(pScrn);
   I830RegPtr save = &pI830->SavedReg;

   DPRINTF(PFX, "RestoreHWOperatingState\n");

   return;
}
#endif

static void
I830BIOSLeaveVT(int scrnIndex, int flags)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   I830Ptr pI830 = I830PTR(pScrn);

   DPRINTF(PFX, "Leave VT\n");

   if (!IsPrimary(pScrn)) {
   	I830Ptr pI8301 = I830PTR(pI830->entityPrivate->pScrn_1);
	if (!pI8301->GttBound) {
		return;
	}
    }

#ifdef XF86DRI
   if (pI830->directRenderingOpen) {
      DPRINTF(PFX, "calling dri lock\n");
      DRILock(screenInfo.screens[scrnIndex], 0);
      pI830->LockHeld = 1;
   }
#endif

#if SAVERESTORE_HWSTATE
   if (!pI830->closing)
      SaveHWOperatingState(pScrn);
#endif

   ResetState(pScrn, TRUE);
   RestoreHWState(pScrn);
   RestoreBIOSMemSize(pScrn);
   if (IsPrimary(pScrn))
      I830UnbindGARTMemory(pScrn);
   if (pI830->AccelInfoRec)
      pI830->AccelInfoRec->NeedToSync = FALSE;
   if (IsPrimary(pScrn)) {
      if (!SetDisplayDevices(pScrn, pI830->savedDevices)) {
         xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Failed to switch back to original display devices (0x%x)\n",
		 pI830->savedDevices);
      } else {
         xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
		 "Successfully set original devices\n");
      }
   }
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
I830BIOSEnterVT(int scrnIndex, int flags)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   I830Ptr pI830 = I830PTR(pScrn);

   DPRINTF(PFX, "Enter VT\n");

   if (IsPrimary(pScrn)) {
      if (!SetDisplayDevices(pScrn, pI830->operatingDevices)) {
         xf86DrvMsg(pScrn->scrnIndex, X_WARNING,
 		 "Failed to switch to configured display devices\n");
	 return FALSE;
      }
   }

   /* Setup for checking lid status */
   pI830->monitorSwitch = INREG(SWF0) & 0x0000FFFF;

   if (IsPrimary(pScrn))
      if (!I830BindGARTMemory(pScrn))
         return FALSE;

   CheckInheritedState(pScrn);
   if (!TweakMemorySize(pScrn, pI830->newBIOSMemSize,FALSE))
       SetBIOSMemSize(pScrn, pI830->newBIOSMemSize);

   /*
    * Only save state once per server generation since that's what most
    * drivers do.  Could change this to save state at each VT enter.
    */
   if (pI830->SaveGeneration != serverGeneration) {
      pI830->SaveGeneration = serverGeneration;
      SaveHWState(pScrn);
   }
   ResetState(pScrn, FALSE);
   SetHWOperatingState(pScrn);

#if 1
   /* Clear the framebuffer */
   memset(pI830->FbBase + pScrn->fbOffset, 0,
	  pScrn->virtualY * pScrn->displayWidth * pI830->cpp);
#endif

   if (!I830VESASetMode(pScrn, pScrn->currentMode))
      return FALSE;
#ifdef I830_XV
   I830VideoSwitchModeAfter(pScrn, pScrn->currentMode);
#endif

   ResetState(pScrn, TRUE);
   SetHWOperatingState(pScrn);

   pScrn->AdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

#if SAVERESTORE_HWSTATE
   RestoreHWOperatingState(pScrn);
#endif

#ifdef XF86DRI
   if (pI830->directRenderingEnabled) {
      if (!pI830->starting) {
	 I830EmitInvarientState(pScrn);
	 I830RefreshRing(pScrn);
	 I830Sync(pScrn);
	 DO_RING_IDLE();

	 DPRINTF(PFX, "calling dri unlock\n");
	 DRIUnlock(screenInfo.screens[scrnIndex]);
      }
      pI830->LockHeld = 0;
   }
#endif

   return TRUE;
}

static Bool
I830BIOSSwitchMode(int scrnIndex, DisplayModePtr mode, int flags)
{

   int _head;
   int _tail;
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   I830Ptr pI830 = I830PTR(pScrn);
   int ret = TRUE;

   DPRINTF(PFX, "I830BIOSSwitchMode: mode == %p\n", mode);

   /* Stops head pointer freezes for 845G */
   if (!pI830->noAccel && (1 || IS_845G(pI830))) {
      do {
	 _head = INREG(LP_RING + RING_HEAD) & I830_HEAD_MASK;
	 _tail = INREG(LP_RING + RING_TAIL) & I830_TAIL_MASK;
	 DELAY(1000);
      } while (_head != _tail);
   }

#ifndef BINDUNBIND
#define BINDUNBIND 0
#endif
#if BINDUNBIND
   if (IsPrimary(pScrn))
      I830UnbindGARTMemory(pScrn);
#endif
#ifdef I830_XV
   /* Give the video overlay code a chance to see the new mode. */
   I830VideoSwitchModeBefore(pScrn, mode);
#endif
   if (!I830VESASetMode(pScrn, mode))
      ret = FALSE;
#ifdef I830_XV
   /* Give the video overlay code a chance to see the new mode. */
   I830VideoSwitchModeAfter(pScrn, mode);
#endif
#if BINDUNBIND
   if (IsPrimary(pScrn))
      I830BindGARTMemory(pScrn);
#endif

   return ret;
}

static Bool
I830BIOSSaveScreen(ScreenPtr pScreen, int mode)
{
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   I830Ptr pI830 = I830PTR(pScrn);
   Bool on = xf86IsUnblank(mode);
   CARD32 temp, ctrl, base;
   int i;

   DPRINTF(PFX, "I830BIOSSaveScreen: %d, on is %s\n", mode, BOOLTOSTRING(on));

   if (pScrn->vtSema) {
      for (i = 0; i < pI830->availablePipes; i++) {
	 if (i == 0) {
	    ctrl = DSPACNTR;
	    base = DSPABASE;
	 } else {
	    ctrl = DSPBCNTR;
	    base = DSPBADDR;
	 }
	 if (pI830->planeEnabled[i]) {
	    temp = INREG(ctrl);
	    if (on)
	       temp |= DISPLAY_PLANE_ENABLE;
	    else
	       temp &= ~DISPLAY_PLANE_ENABLE;
	    OUTREG(ctrl, temp);
	    /* Flush changes */
	    temp = INREG(base);
	    OUTREG(base, temp);
	 }
      }

      if (pI830->CursorInfoRec && !pI830->SWCursor && pI830->cursorOn) {
	 if (on)
	    pI830->CursorInfoRec->ShowCursor(pScrn);
	 else
	    pI830->CursorInfoRec->HideCursor(pScrn);
	 pI830->cursorOn = TRUE;
      }
   }
   return TRUE;
}

/* Use the VBE version when available. */
static void
I830DisplayPowerManagementSet(ScrnInfoPtr pScrn, int PowerManagementMode,
			      int flags)
{
   I830Ptr pI830 = I830PTR(pScrn);
   vbeInfoPtr pVbe = pI830->pVbe;

   if (pI830->Clone) {
      SetBIOSPipe(pScrn, !pI830->pipe);
      if (xf86LoaderCheckSymbol("VBEDPMSSet")) {
         VBEDPMSSet(pVbe, PowerManagementMode);
      } else {
         pVbe->pInt10->num = 0x10;
         pVbe->pInt10->ax = 0x4f10;
         pVbe->pInt10->bx = 0x01;

         switch (PowerManagementMode) {
         case DPMSModeOn:
	    break;
         case DPMSModeStandby:
	    pVbe->pInt10->bx |= 0x0100;
	    break;
         case DPMSModeSuspend:
	    pVbe->pInt10->bx |= 0x0200;
	    break;
         case DPMSModeOff:
	    pVbe->pInt10->bx |= 0x0400;
	    break;
         }
         xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
      }
   }

   SetPipeAccess(pScrn);

   if (xf86LoaderCheckSymbol("VBEDPMSSet")) {
      VBEDPMSSet(pVbe, PowerManagementMode);
   } else {
      pVbe->pInt10->num = 0x10;
      pVbe->pInt10->ax = 0x4f10;
      pVbe->pInt10->bx = 0x01;

      switch (PowerManagementMode) {
      case DPMSModeOn:
	 break;
      case DPMSModeStandby:
	 pVbe->pInt10->bx |= 0x0100;
	 break;
      case DPMSModeSuspend:
	 pVbe->pInt10->bx |= 0x0200;
	 break;
      case DPMSModeOff:
	 pVbe->pInt10->bx |= 0x0400;
	 break;
      }
      xf86ExecX86int10_wrapper(pVbe->pInt10, pScrn);
   }
}

static Bool
I830BIOSCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   I830Ptr pI830 = I830PTR(pScrn);
   XAAInfoRecPtr infoPtr = pI830->AccelInfoRec;

   pI830->closing = TRUE;
#ifdef XF86DRI
   if (pI830->directRenderingOpen) {
      pI830->directRenderingOpen = FALSE;
      I830DRICloseScreen(pScreen);
   }
#endif

   if (pScrn->vtSema == TRUE) {
      I830BIOSLeaveVT(scrnIndex, 0);
   }

   DPRINTF(PFX, "\nUnmapping memory\n");
   I830UnmapMem(pScrn);
   vgaHWUnmapMem(pScrn);

   if (pI830->ScanlineColorExpandBuffers) {
      xfree(pI830->ScanlineColorExpandBuffers);
      pI830->ScanlineColorExpandBuffers = 0;
   }

   if (infoPtr) {
      if (infoPtr->ScanlineColorExpandBuffers)
	 xfree(infoPtr->ScanlineColorExpandBuffers);
      XAADestroyInfoRec(infoPtr);
      pI830->AccelInfoRec = NULL;
   }

   if (pI830->CursorInfoRec) {
      xf86DestroyCursorInfoRec(pI830->CursorInfoRec);
      pI830->CursorInfoRec = 0;
   }

   if (IsPrimary(pScrn)) {
      xf86GARTCloseScreen(scrnIndex);

      xfree(pI830->LpRing);
      pI830->LpRing = NULL;
      xfree(pI830->CursorMem);
      pI830->CursorMem = NULL;
      xfree(pI830->CursorMemARGB);
      pI830->CursorMemARGB = NULL;
      xfree(pI830->OverlayMem);
      pI830->OverlayMem = NULL;
      xfree(pI830->overlayOn);
      pI830->overlayOn = NULL;
   }

   if (pI830->lidTimer)
      TimerCancel(pI830->lidTimer);

   pScrn->vtSema = FALSE;
   pI830->closing = FALSE;
   pScreen->CloseScreen = pI830->CloseScreen;
   return (*pScreen->CloseScreen) (scrnIndex, pScreen);
}

static ModeStatus
I830ValidMode(int scrnIndex, DisplayModePtr mode, Bool verbose, int flags)
{
   if (mode->Flags & V_INTERLACE) {
      if (verbose) {
	 xf86DrvMsg(scrnIndex, X_PROBED,
		    "Removing interlaced mode \"%s\"\n", mode->name);
      }
      return MODE_BAD;
   }
   return MODE_OK;
}

#ifndef SUSPEND_SLEEP
#define SUSPEND_SLEEP 0
#endif
#ifndef RESUME_SLEEP
#define RESUME_SLEEP 0
#endif

/*
 * This function is only required if we need to do anything differently from
 * DoApmEvent() in common/xf86PM.c, including if we want to see events other
 * than suspend/resume.
 */
static Bool
I830PMEvent(int scrnIndex, pmEvent event, Bool undo)
{
   ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
   I830Ptr pI830 = I830PTR(pScrn);

   DPRINTF(PFX, "Enter VT, event %d, undo: %s\n", event, BOOLTOSTRING(undo));

   switch(event) {
   case XF86_APM_SYS_SUSPEND:
   case XF86_APM_CRITICAL_SUSPEND: /*do we want to delay a critical suspend?*/
   case XF86_APM_USER_SUSPEND:
   case XF86_APM_SYS_STANDBY:
   case XF86_APM_USER_STANDBY:
      if (!undo && !pI830->suspended) {
	 pScrn->LeaveVT(scrnIndex, 0);
	 pI830->suspended = TRUE;
	 sleep(SUSPEND_SLEEP);
      } else if (undo && pI830->suspended) {
	 sleep(RESUME_SLEEP);
	 pScrn->EnterVT(scrnIndex, 0);
	 pI830->suspended = FALSE;
      }
      break;
   case XF86_APM_STANDBY_RESUME:
   case XF86_APM_NORMAL_RESUME:
   case XF86_APM_CRITICAL_RESUME:
      if (pI830->suspended) {
	 sleep(RESUME_SLEEP);
	 pScrn->EnterVT(scrnIndex, 0);
	 pI830->suspended = FALSE;
	 /*
	  * Turn the screen saver off when resuming.  This seems to be
	  * needed to stop xscreensaver kicking in (when used).
	  *
	  * XXX DoApmEvent() should probably call this just like
	  * xf86VTSwitch() does.  Maybe do it here only in 4.2
	  * compatibility mode.
	  */
	 SaveScreens(SCREEN_SAVER_FORCER, ScreenSaverReset);
      }
      break;
   default:
      ErrorF("I830PMEvent: received APM event %d\n", event);
   }
   return TRUE;
}

static CARD32
I830LidTimer(OsTimerPtr timer, CARD32 now, pointer arg)
{
   ScrnInfoPtr pScrn = (ScrnInfoPtr) arg;
   I830Ptr pI830 = I830PTR(pScrn);

   if (pScrn->vtSema) {
      /* Check for monitor lid being closed/opened and act accordingly */
      int temp = INREG(SWF0) & 0x0000FFFF;

      if (pI830->monitorSwitch != temp) {
	 int conf = pI830->operatingDevices;
         xf86DrvMsg(pScrn->scrnIndex, X_WARNING, 
			"Detected possible lid operation, fixing up.\n");
         if ((temp & 0x0808) == 0x0000) {
	    /* LFP (PIPE A or B) GOING OFF - PROBABLE LID CLOSURE */
	    conf = pI830->operatingDevices & 0xF7F7;
#if 0
            xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Lid is being closed.\n");
#endif
         } else {
#if 0
            xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Lid is being opened.\n");
#endif
         }

         /* If we've defined our own monitors, then get them and set them
          * up when switching in single head mode, no effect in dual heads
          * NOTE: This assumes that the LCD is always on Pipe B..... */
         conf |= pI830->MonType1;
	 if (IsPrimary(pScrn)) {
            if (!SetDisplayDevices(pScrn, conf)) 
               xf86DrvMsg(pScrn->scrnIndex, X_WARNING, "Failed to switch "
 		    "to configured display devices during lid operation.\n");
         }
         pI830->monitorSwitch = conf;
   
         /* Now, when we're single head, make sure we switch pipes */
         if (!(xf86IsEntityShared(pScrn->entityList[0]) || pI830->Clone)) {
            if ((temp & 0xFF00) == 0x0000)
               pI830->pipe = 0;
            if ((temp & 0x00FF) == 0x0000)
               pI830->pipe = 1;
         }
         I830BIOSSwitchMode(pScrn->pScreen->myNum, pScrn->currentMode, 0);
         I830BIOSAdjustFrame(pScrn->pScreen->myNum, pScrn->frameX0, pScrn->frameY0, 0);
 
         /* Everything should be o.k. now, so make sure the HW cursor is
          * on the correct pipe */
         if (pI830->CursorInfoRec && !pI830->SWCursor && pI830->cursorOn) {
            pI830->CursorInfoRec->ShowCursor(pScrn);
            pI830->cursorOn = TRUE;
         }
      }
   }
  
   return 1000;
}

void
I830InitpScrn(ScrnInfoPtr pScrn)
{
   pScrn->PreInit = I830BIOSPreInit;
   pScrn->ScreenInit = I830BIOSScreenInit;
   pScrn->SwitchMode = I830BIOSSwitchMode;
   pScrn->AdjustFrame = I830BIOSAdjustFrame;
   pScrn->EnterVT = I830BIOSEnterVT;
   pScrn->LeaveVT = I830BIOSLeaveVT;
   pScrn->FreeScreen = I830BIOSFreeScreen;
   pScrn->ValidMode = I830ValidMode;
   pScrn->PMEvent = I830PMEvent;
}