/* (c) Itai Nahshon
 *
 * This code is derived from and inspired by the I2C driver
 * from the Linux kernel.
 *      (c) 1998 Gerd Knorr <kraxel@cs.tu-berlin.de>
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/i2c/xf86i2cmodule.c,v 1.6 1999/01/26 05:54:10 dawes Exp $ */

#include "xf86Module.h"

static MODULESETUPPROTO(i2cSetup);

static XF86ModuleVersionInfo i2cVersRec =
{
        "i2c",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XORG_VERSION_CURRENT,
        1, 2, 0,
        ABI_CLASS_VIDEODRV,		/* This needs the video driver ABI */
        ABI_VIDEODRV_VERSION,
        MOD_CLASS_NONE,
        {0,0,0,0}
};

XF86ModuleData i2cModuleData = { &i2cVersRec, i2cSetup, NULL };

static pointer
i2cSetup(pointer module, pointer opts, int *errmaj, int *errmin) {
/* ErrorF("i2cSetup\n"); */
   return (pointer)1;
}
