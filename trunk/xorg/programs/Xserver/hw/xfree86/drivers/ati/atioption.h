/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/atioption.h,v 1.12 2003/04/23 21:51:29 tsi Exp $ */
/*
 * Copyright 1999 through 2004 by Marc Aurele La France (TSI @ UQV), tsi@xfree86.org
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * DRI support by:
 *    Leif Delgass <ldelgass@retinalburn.net>
 */

#ifndef ___ATIOPTION_H___
#define ___ATIOPTION_H___ 1

#include "xf86str.h"

/*
 * Documented XF86Config options.
 */
typedef enum
{
    ATI_OPTION_ACCEL,
    ATI_OPTION_CRT_DISPLAY,
    ATI_OPTION_CSYNC,
    ATI_OPTION_HWCURSOR,

#ifndef AVOID_CPIO

    ATI_OPTION_LINEAR,

#endif /* AVOID_CPIO */

#ifdef XF86DRI_DEVEL

    ATI_OPTION_IS_PCI,
    ATI_OPTION_DMA_MODE,
    ATI_OPTION_AGP_MODE,
    ATI_OPTION_AGP_SIZE,
    ATI_OPTION_LOCAL_TEXTURES,
    ATI_OPTION_BUFFER_SIZE,

#endif /* XF86DRI_DEVEL */

    ATI_OPTION_MMIO_CACHE,
    ATI_OPTION_TEST_MMIO_CACHE,
    ATI_OPTION_PANEL_DISPLAY,
    ATI_OPTION_PROBE_CLOCKS,
    ATI_OPTION_REFERENCE_CLOCK,
    ATI_OPTION_SHADOW_FB,
    ATI_OPTION_SWCURSOR
} ATIPublicOptionType;

extern const OptionInfoRec   ATIPublicOptions[];
extern const unsigned long   ATIPublicOptionSize;

extern const OptionInfoRec * ATIAvailableOptions(int, int);

#endif /* ___ATIOPTION_H___ */
