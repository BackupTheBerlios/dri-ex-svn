/*
 * Copyright 2004 The Unichrome Project  [unichrome.sf.net]
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
 * Wrap around xf86 vgaHW
 * Provide general IO calls too as they are not part of the vgaHW implementation
 * It's a bit daft to provide this short stuff in a seperate file, 
 * but then again, we'd only complicate matters in already complicated files.
 */
#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86.h"
#include "via_driver.h" /* for HAVE_DEBUG */
#include "via_vgahw.h"

/*
 * The current vgaHW implementation lacks generic IO
 *
 */

/*
 *
 */
static
CARD8
ViaVgahwIn(vgaHWPtr hwp, int address)
{
    if (hwp->MMIOBase)
	return MMIO_IN8(hwp->MMIOBase, hwp->MMIOOffset + address);
    else
	return inb(hwp->PIOOffset + address);
}

/*
 *
 */
static
void
ViaVgahwOut(vgaHWPtr hwp, int address, CARD8 value)
{
    if (hwp->MMIOBase)
	MMIO_OUT8(hwp->MMIOBase, hwp->MMIOOffset + address, value);
    else
	outb(hwp->PIOOffset + address, value);
}

/*
 * indexed.
 */
static
CARD8
ViaVgahwRead(vgaHWPtr hwp, int indexaddress, CARD8 index,
	     int valueaddress)
{
    ViaVgahwOut(hwp, indexaddress, index);
    return ViaVgahwIn(hwp, valueaddress);
}

/*
 * indexed.
 */
void
ViaVgahwWrite(vgaHWPtr hwp, int indexaddress, CARD8 index,
	     int valueaddress, CARD8 value)
{
    ViaVgahwOut(hwp, indexaddress, index);
    ViaVgahwOut(hwp, valueaddress, value);
}

/*
 * This code makes a big change in readability, allows one to clearly
 * formulate what is changed. 
 *
 */

/*
 *
 */
void 
ViaVgahwMask(vgaHWPtr hwp, int indexaddress, CARD8 index,
	       int valueaddress, CARD8 value, CARD8 mask)
{
    CARD8 tmp;

    tmp = ViaVgahwRead(hwp, indexaddress, index, valueaddress);
    
    tmp &= ~mask;
    tmp |= (value & mask);
    
    ViaVgahwWrite(hwp, indexaddress, index, valueaddress, tmp);
}

/*
 * 
 */
void 
ViaCrtcMask(vgaHWPtr hwp, CARD8 index, CARD8 value, CARD8 mask)
{
    CARD8 tmp;

    tmp = hwp->readCrtc(hwp, index);
    tmp &= ~mask;
    tmp |= (value & mask);

    hwp->writeCrtc(hwp, index, tmp);
}

/*
 *
 */
void 
ViaSeqMask(vgaHWPtr hwp, CARD8 index, CARD8 value, CARD8 mask)
{
    CARD8 tmp;

    tmp = hwp->readSeq(hwp, index);
    tmp &= ~mask;
    tmp |= (value & mask);

    hwp->writeSeq(hwp, index, tmp);
}

/*
 *
 */
#ifdef HAVE_DEBUG
void
ViaVgahwPrint(vgaHWPtr hwp)
{
    int i;
    xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "Printing VGA Sequence registers:\n");
    for (i = 0x00; i < 0x93; i++)
	xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "SR%02X: 0x%02X\n", i, hwp->readSeq(hwp, i));

    xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "Printing VGA CRTM/C registers:\n");
    for (i = 0x00; i < 0x19; i++)
        xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "CR%02X: 0x%02X\n", i, hwp->readCrtc(hwp, i));
    for (i = 0x33; i < 0xA3; i++)
	xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "CR%02X: 0x%02X\n", i, hwp->readCrtc(hwp, i));

    xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "Printing VGA Graphics registers:\n");
    for (i = 0x00; i < 0x08; i++)
	xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "GR%02X: 0x%02X\n", i, hwp->readGr(hwp, i));

    xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "Printing VGA Attribute registers:\n");
    for (i = 0x00; i < 0x14; i++)
	xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "AR%02X: 0x%02X\n", i, hwp->readAttr(hwp, i));
    
    xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "Printing VGA Miscellaneous register:\n");
    xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "Misc: 0x%02X\n", hwp->readMiscOut(hwp));

    xf86DrvMsg(hwp->pScrn->scrnIndex, X_INFO, "End of VGA Registers.\n");
}
#endif /* HAVE_DEBUG */
