/*
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
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_i2c.h,v 1.1 2003/04/15 15:35:47 alanh Exp $ */
#ifndef __VIAI2C_H
#define __VIAI2C_H

typedef struct
{
    int         scrnIndex;
    vgaHWPtr    hwp;

    CARD8       Address;
    int         RiseFallTime;
    int         StartTimeout;
    int         BitTimeout;
    int         ByteTimeout;
    int         HoldTimeout;

    void (*Delay)(I2CBusPtr pBus, int usec); /* we are free to pass NULL as first arg */
} GpioI2cRec, *GpioI2cPtr;

void VIAGPIOI2C_Initial(GpioI2cPtr pDev, CARD8 Address);
Bool VIAGPIOI2C_Write(GpioI2cPtr pDev, CARD8 SubAddress, CARD8 Data);
Bool VIAGPIOI2C_Read(GpioI2cPtr pDev, CARD8 SubAddress, CARD8 *Buffer, int BufferLen);
Bool VIAGPIOI2C_ReadByte(GpioI2cPtr pDev, CARD8 SubAddress, CARD8 * value);
Bool ViaGpioI2c_Probe(GpioI2cPtr pDev, CARD8 Address);

void VIAI2CInit(ScrnInfoPtr pScrn);

#endif	   /*__VIAI2C_H */
