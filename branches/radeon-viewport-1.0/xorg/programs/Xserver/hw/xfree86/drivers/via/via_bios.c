/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_bios.c,v 1.11 2004/02/20 21:50:06 dawes Exp $ */
/*
 * Copyright 2004 The Unichrome Project  [unichrome.sf.net]
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
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

#include "via_driver.h"

/*
 *
 * Dumping the VGA BIOS to file.
 *
 */

#ifdef HAVE_DEBUG

/* The position of some BIOS information from start of BIOS */
#define     VIA_BIOS_SIZE_POS               0x2
#define     VIA_BIOS_OFFSET_POS             0x1B

/* The offset of table from table start */
#define     VIA_BIOS_BIOSVER_POS            18
#define     VIA_BIOS_BCPPOST_POS            20

#define VIA_VGABIOS_OFFSET  0xC0000
#define VIA_VGABIOS_SIZE    0x10000
/*
 *
 */
void
ViaDumpVGAROM(ScrnInfoPtr pScrn)
{
    VIAPtr pVia = VIAPTR(pScrn);
    CARD8 *Image, *p, *pTable, Date[9];
    int i, size, sum;
    CARD16 Version;
    char Name[70];
    FILE *Dump;

    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ViaDumpVGAROM\n"));

    if (!(Image = xcalloc(1, VIA_VGABIOS_SIZE)))  {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to allocate"
		   " memory for VGA ROM Image.\n");
        return;
    }

    if (xf86ReadBIOS(VIA_VGABIOS_OFFSET, 0, Image, VIA_VGABIOS_SIZE) != VIA_VGABIOS_SIZE)  {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Failed to read VGA ROM Image.\n");
        xfree(Image);
	return;
    }


    if (*((CARD16 *) Image) != 0xAA55) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ROM Image header mismatch.\n");
	xfree(Image);
	return;
    }

    size = *((CARD8 *) (Image + VIA_BIOS_SIZE_POS)) * 512;
    p = Image;
    sum = 0;

    for (i = 0; i < size; i++, p++)
	sum += *p;
	
    if (((CARD8) sum) != 0) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ROM Image Checksum mismatch."
		   " (%x)\n", sum);
	xfree(Image);
	return;
    }

    /* Get the start of Table */
    p = Image + VIA_BIOS_OFFSET_POS;
    pTable = Image + *((CARD16 *)p);

    /* Get the start of biosver structure */
    p = pTable + VIA_BIOS_BIOSVER_POS;
    DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "bcpPost: %X\n", *((CARD16 *)p)));
    p = Image + *((CARD16 *)p);

    /* The offset should be 44, but the actual image is less three char. */
    /* pRom += 44; */
    p += 41;
    Version = (*p << 8) | *(p + 1);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "VGA BIOS Version: %d.%d\n",
            (Version >> 8), (Version & 0xFF));

    /* Get the start of bcpPost structure */
    p = pTable + VIA_BIOS_BCPPOST_POS;
    p = Image + *((CARD16 *)p);

    p += 10;
    for (i = 0; i < 8; i++) {
	if (*p == '/')
	    Date[i] = '-';
	else
	    Date[i] = *p;
        p++;
    }
    Date[8] = 0;
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "VGA BIOS Release Date: %s\n", Date);

    /* dump image here */

    snprintf(Name, sizeof(Name), "/tmp/Unichrome_VGAROM-%04X_%04X_r%04X-%04X_%04X-v%02d.%02d_[%s].img",
	     pVia->PciInfo->vendor, pVia->PciInfo->chipType, pVia->ChipRev, pVia->PciInfo->subsysVendor,
	     pVia->PciInfo->subsysCard, (Version >> 8), (Version & 0xFF), Date);

    /* If anyone has more X specific code for this: i'm listening -- Luc */
    if ((Dump = fopen(Name, "w+")) == NULL) {
	xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "Unable to open %s\n", Name);
    } else {
	fwrite(Image, VIA_VGABIOS_SIZE, sizeof(char), Dump);
	fclose(Dump);
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "VGA ROM image written to %s\n", Name);
    }
    
    xfree(Image);
}
#endif /* HAVE_DEBUG */

/*
 *
 * VBE OEM extensions.
 *
 */

/*
 * CARD8 ViaVBEGetActiveDevice(ScrnInfoPtr pScrn);
 *
 *     - Determine which devices (CRT1, LCD, TV, DFP) are active
 *
 * Need more information: return does not match my biossetting -- luc
 *
 *
 * VBE OEM subfunction 0x0103 (from via code)
 *     cx = 0x00
 * returns:
 *     cx = active device
 *
 */
CARD8 
ViaVBEGetActiveDevice(ScrnInfoPtr pScrn)
{
    if (VIAPTR(pScrn)->pVbe) {

	xf86Int10InfoPtr pInt10 = VIAPTR(pScrn)->pVbe->pInt10;
	CARD8 device = 0;
	
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ViaVBEGetActiveDevice\n"));
	
	pInt10->ax = 0x4F14;
	pInt10->bx = 0x0103;

	pInt10->cx = 0x00;

	pInt10->num = 0x10;
	xf86ExecX86int10(pInt10);
	
	if ((pInt10->ax & 0xFF) != 0x4F) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ViaVBEGetActiveDevice: VBE call failed.\n");
	    return 0xFF;
	}
	
	if (pInt10->cx & 0x01)
	    device = VIA_DEVICE_CRT1;
	if (pInt10->cx & 0x02)
	    device |= VIA_DEVICE_LCD;
	if (pInt10->cx & 0x04)
	    device |= VIA_DEVICE_TV;
	if (pInt10->cx & 0x20)
	    device |= VIA_DEVICE_DFP;
	
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Active Device(s): %u\n", device));
	return device;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ViaVBEGetActiveDevice: VBE not initialised.\n");
    return 0xFF;
}

/*
 * CARD16 ViaVBEGetDisplayDeviceInfo(pScrn, *numDevice);
 *
 *     - Returns the maximal vertical resolution of the Display Device
 *       provided in numDevice (CRT (0), DVI (1), LCD/Panel (2))
 *
 *
 * VBE OEM subfunction 0x0806 (from via code)
 *     cx = *numDevice
 *     di = 0x00
 * returns:
 *     cx = *numDevice
 *     di = max. vertical resolution
 *
 */
CARD16
ViaVBEGetDisplayDeviceInfo(ScrnInfoPtr pScrn, CARD8 *numDevice)
{
    if (VIAPTR(pScrn)->pVbe) {

	xf86Int10InfoPtr pInt10 = VIAPTR(pScrn)->pVbe->pInt10;
	
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ViaVBEGetDisplayDeviceInfo\n"));
	
	pInt10->ax = 0x4F14;
	pInt10->bx = 0x0806;

	pInt10->cx = *numDevice;
	pInt10->di = 0x00;

	pInt10->num = 0x10;
	xf86ExecX86int10(pInt10);
	
	if ((pInt10->ax & 0xFF) != 0x4F) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ViaVBEGetDisplayDeviceInfo: VBE call failed.\n");
	    return 0xFFFF;
	}
	
	*numDevice = pInt10->cx;

	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Vertical Resolution: %u\n", pInt10->di & 0xFFFF));
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Panel ID: %u\n", *numDevice));
	
	return (pInt10->di & 0xFFFF);
    }
    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ViaVBEGetDisplayDeviceInfo: VBE not initialised.\n");
    return 0xFFFF;
}

/*
 * VBE OEM subfunction 0x0100 (from via code)
 *     cx = 0x00
 *     dx = 0x00
 *     si = 0x00
 * returns:
 *     bx = year
 *     cx = month
 *     dx = day
 *
 */
void
ViaVBEPrintBIOSDate(ScrnInfoPtr pScrn)
{
    if (VIAPTR(pScrn)->pVbe) {

	xf86Int10InfoPtr pInt10 = VIAPTR(pScrn)->pVbe->pInt10;
	CARD8 Year, Month, Day;
	
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ViaVBEGetBIOSDate\n"));
	
	pInt10->ax = 0x4F14;
	pInt10->bx = 0x0100;

	pInt10->cx = 0x00;
	pInt10->dx = 0x00;
	pInt10->si = 0x00;

	pInt10->num = 0x10;
	xf86ExecX86int10(pInt10);
	
	if ((pInt10->ax & 0xff) != 0x4f) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ViaVBEGetBIOSDate: VBE call failed.\n");
	    return;
	}
	
	Year = ((pInt10->bx >> 8) - 48) + ((pInt10->bx & 0xFF) - 48) * 10;
	Month = ((pInt10->cx >> 8) - 48) + ((pInt10->cx & 0xFF) - 48) * 10;
	Day = ((pInt10->dx >> 8) - 48) + ((pInt10->dx & 0xFF) - 48) * 10;
	
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS Release Date: %d/%d/%d\n",
			 Year + 2000, Month, Day);
	return;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ViaVBEGetBIOSDate: VBE not initialised.\n");
}

/*
 * Calls VBE subfunction 00h (VBE supplemental specification information.) 
 * Not functional.
 * 
 * VBE OEM subfunction 0x0000 (??? - from via code)
 *     cx = 0x00
 * returns:
 *     bx = version.
 *
 */
void
ViaVBEPrintBIOSVersion(ScrnInfoPtr pScrn)
{
    if (VIAPTR(pScrn)->pVbe) {

	xf86Int10InfoPtr pInt10 = VIAPTR(pScrn)->pVbe->pInt10;
	
	DEBUG(xf86DrvMsg(pScrn->scrnIndex, X_INFO, "ViaVBEGetBIOSVersion\n"));
	
	pInt10->ax = 0x4F14;
	pInt10->bx = 0x00; /* ??? */
	pInt10->cx = 0x00;
	pInt10->num = 0x10;
	xf86ExecX86int10(pInt10);
	
	if ((pInt10->ax & 0xff) != 0x4f) {
	    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ViaVBEGetBIOSVersion: VBE call failed.\n");
	    return;
	}
	
	xf86DrvMsg(pScrn->scrnIndex, X_INFO, "BIOS Version: %d.%d\n", (pInt10->bx >> 8) & 0xFF,
		   pInt10->bx & 0xFF);
	return;
    }
    xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "ViaVBEGetBIOSVersion: VBE not initialised.\n");
}
