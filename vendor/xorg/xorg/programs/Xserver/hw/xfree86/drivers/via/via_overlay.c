/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/via/via_overlay.c,v 1.2 2003/08/27 15:16:11 tsi Exp $ */
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
 
/* I N C L U D E S ---------------------------------------------------------*/

#include "xf86.h"
#include "via.h"
#include "via_driver.h"
#include "via_overlay.h"
#include "via_id.h"
#include <math.h>

/* F U N C T I O N ----------------------------------------------------------*/

void viaOverlayGetV1Format(VIAPtr pVia, unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF, unsigned long * lpdwVidCtl,unsigned long * lpdwHQVCtl )
{

   if (lpDPF->dwFlags & DDPF_FOURCC)
   {
       *lpdwVidCtl |= V1_COLORSPACE_SIGN;
       switch (lpDPF->dwFourCC) {
       case FOURCC_YV12:
       case FOURCC_VIA:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpdwVidCtl |= (V1_YUV422 | V1_SWAP_HW_HQV );
                *lpdwHQVCtl |= HQV_SRC_SW|HQV_YUV420|HQV_ENABLE|HQV_SW_FLIP;
            }
            else
            {
                *lpdwVidCtl |= V1_YCbCr420;
            }
            break;
       case FOURCC_YUY2:
            DBG_DD(ErrorF("DDOver_GetV1Format : FOURCC_YUY2\n"));
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpdwVidCtl |= (V1_YUV422 | V1_SWAP_HW_HQV );
                *lpdwHQVCtl |= HQV_SRC_SW|HQV_YUV422|HQV_ENABLE|HQV_SW_FLIP;
            }
            else
            {
                *lpdwVidCtl |= V1_YUV422;
            }
            break;

       default :
            DBG_DD(ErrorF("DDOver_GetV1Format : Invalid FOURCC format :(0x%lx)in V1!\n", lpDPF->dwFourCC));
            *lpdwVidCtl |= V1_YUV422;
            break;
       }
   }
   else if (lpDPF->dwFlags & DDPF_RGB)
   {
       switch (lpDPF->dwRGBBitCount) {
       case 16:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpdwVidCtl |= (V1_RGB16 | V1_SWAP_HW_HQV );
                *lpdwHQVCtl |= HQV_SRC_SW|HQV_ENABLE|HQV_SW_FLIP;
                *lpdwHQVCtl |= (lpDPF->dwGBitMask==0x07E0 ?
                            HQV_RGB16 : HQV_RGB15);
            }
            else
            {
                *lpdwVidCtl |= (lpDPF->dwGBitMask==0x07E0 ?
                            V1_RGB16 : V1_RGB15);
            }
           break;
       case 32:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpdwVidCtl |= (V1_RGB32 | V1_SWAP_HW_HQV );
                *lpdwHQVCtl |= HQV_SRC_SW|HQV_RGB32|HQV_ENABLE|HQV_SW_FLIP;
            }
            else
            {
                *lpdwVidCtl |= V1_RGB32;
            }
           break;

       default :
            DBG_DD(ErrorF("DDOver_GetV1Format : invalid RGB format %ld bits\n",lpDPF->dwRGBBitCount));
            break;
       }
   }
}

void viaOverlayGetV3Format(VIAPtr pVia, unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF, unsigned long * lpdwVidCtl,unsigned long * lpdwHQVCtl )
{

   if (lpDPF->dwFlags & DDPF_FOURCC)
   {
       *lpdwVidCtl |= V3_COLORSPACE_SIGN;
       switch (lpDPF->dwFourCC) {
       case FOURCC_YV12:
       case FOURCC_VIA:

            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpdwVidCtl |= (V3_YUV422 | V3_SWAP_HW_HQV );
                *lpdwHQVCtl |= HQV_SRC_SW|HQV_YUV420|HQV_ENABLE|HQV_SW_FLIP;
            }
            else
            {
                /* *lpdwVidCtl |= V3_YCbCr420;*/
                DBG_DD(ErrorF("DDOver_GetV3Format : Invalid FOURCC format :(0x%lx)in V3!\n", lpDPF->dwFourCC));
            }
            break;
       case FOURCC_YUY2:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpdwVidCtl |= (V3_YUV422 | V3_SWAP_HW_HQV );
                *lpdwHQVCtl |= HQV_SRC_SW|HQV_YUV422|HQV_ENABLE|HQV_SW_FLIP;
            }
            else
            {
                *lpdwVidCtl |= V3_YUV422;
            }
            break;

       default :
            DBG_DD(ErrorF("DDOver_GetV3Format : Invalid FOURCC format :(0x%lx)in V3!\n", lpDPF->dwFourCC));
            *lpdwVidCtl |= V3_YUV422;
            break;
       }
   }
   else if (lpDPF->dwFlags & DDPF_RGB) {
       switch (lpDPF->dwRGBBitCount) {
       case 16:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpdwVidCtl |= (V3_RGB16 | V3_SWAP_HW_HQV );
                *lpdwHQVCtl |= HQV_SRC_SW|HQV_ENABLE|HQV_SW_FLIP;
                *lpdwHQVCtl |= (lpDPF->dwGBitMask==0x07E0 ?
                            HQV_RGB16 : HQV_RGB15);
            }
            else
            {
                *lpdwVidCtl |= (lpDPF->dwGBitMask==0x07E0 ?
                            V3_RGB16 : V3_RGB15);
            }
           break;
       case 32:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpdwVidCtl |= (V3_RGB32 | V3_SWAP_HW_HQV );
                *lpdwHQVCtl |= HQV_SRC_SW|HQV_RGB32|HQV_ENABLE|HQV_SW_FLIP;
            }
            else
            {
                *lpdwVidCtl |= V3_RGB32;
            }
           break;

       default :
            DBG_DD(ErrorF("DDOver_GetV3Format : invalid RGB format %ld bits\n",lpDPF->dwRGBBitCount));
            break;
       }
   }
}

unsigned long viaOverlayGetSrcStartAddress(VIAPtr pVia, unsigned long dwVideoFlag,RECTL rSrc,RECTL rDest, unsigned long dwSrcPitch,LPDDPIXELFORMAT lpDPF,unsigned long * lpHQVoffset )
{
   unsigned long dwOffset=0;
   unsigned long dwHQVsrcWidth=0,dwHQVdstWidth=0;
   unsigned long dwHQVsrcHeight=0,dwHQVdstHeight=0;
   unsigned long dwHQVSrcTopOffset=0,dwHQVSrcLeftOffset=0;
   
   dwHQVsrcWidth = (unsigned long)(rSrc.right - rSrc.left);
   dwHQVdstWidth = (unsigned long)(rDest.right - rDest.left);
   dwHQVsrcHeight = (unsigned long)(rSrc.bottom - rSrc.top);
   dwHQVdstHeight = (unsigned long)(rDest.bottom - rDest.top);

   if ( (rSrc.left != 0) || (rSrc.top != 0) )
   {

       if (lpDPF->dwFlags & DDPF_FOURCC)
       {
           switch (lpDPF->dwFourCC)
           {
            case FOURCC_YUY2:
            case FOURCC_UYVY:
                DBG_DD(ErrorF("GetSrcStartAddress : FOURCC format :(0x%lx)\n", lpDPF->dwFourCC));
                if (dwVideoFlag&VIDEO_HQV_INUSE)
                {
                    dwOffset = (((rSrc.top&~3) * (dwSrcPitch)) +
                               ((rSrc.left << 1)&~31));
                    if (dwHQVsrcHeight>dwHQVdstHeight)
                    {
                        dwHQVSrcTopOffset = ((rSrc.top&~3) * dwHQVdstHeight / dwHQVsrcHeight)* dwSrcPitch;
                    }
                    else
                    {
                        dwHQVSrcTopOffset = (rSrc.top&~3) * (dwSrcPitch);
                    }
                    
                    if (dwHQVsrcWidth>dwHQVdstWidth)
                    {
                        dwHQVSrcLeftOffset = ((rSrc.left << 1)&~31) * dwHQVdstWidth / dwHQVsrcWidth;
                    }
                    else
                    {
                        dwHQVSrcLeftOffset = (rSrc.left << 1)&~31 ;
                    }
                    *lpHQVoffset = dwHQVSrcTopOffset+dwHQVSrcLeftOffset;
                }
                else
                {   
                    dwOffset = ((rSrc.top * dwSrcPitch) +
                               ((rSrc.left << 1)&~15));
                }
                break;

            case FOURCC_YV12:
	   case FOURCC_VIA:

                if (dwVideoFlag&VIDEO_HQV_INUSE)
                {
                    dwOffset = (((rSrc.top&~3) * (dwSrcPitch<<1)) +
                                ((rSrc.left << 1)&~31));
                }
                else
                {
                    dwOffset = ((((rSrc.top&~3) * dwSrcPitch) +
                                rSrc.left)&~31) ;
                    if (rSrc.top >0)
                    {
                        pVia->swov.overlayRecordV1.dwUVoffset = (((((rSrc.top&~3)>>1) * dwSrcPitch) +
                                       rSrc.left)&~31) >>1;
                    }
                    else
                    {
                        pVia->swov.overlayRecordV1.dwUVoffset = dwOffset >>1 ;
                    }
                }
                break;

            default:
                DBG_DD(ErrorF("DDOver_GetSrcStartAddress : Invalid FOURCC format :(0x%lx)in V3!\n", lpDPF->dwFourCC));
                break;
            }
       }
       else if (lpDPF->dwFlags & DDPF_RGB)
       {
                if (dwVideoFlag&VIDEO_HQV_INUSE)
                {
                    dwOffset = (((rSrc.top&~3) * (dwSrcPitch<<1)) +
                                ((rSrc.left << 1)&~31));

                    if (dwHQVsrcHeight>dwHQVdstHeight)
                    {
                        dwHQVSrcTopOffset = ((rSrc.top&~3) * dwHQVdstHeight / dwHQVsrcHeight)* dwSrcPitch;
                    }
                    else
                    {
                        dwHQVSrcTopOffset = (rSrc.top&~3) * (dwSrcPitch);
                    }
                    
                    if (dwHQVsrcWidth>dwHQVdstWidth)
                    {
                        dwHQVSrcLeftOffset = ((rSrc.left << 1)&~31) * dwHQVdstWidth / dwHQVsrcWidth;
                    }
                    else
                    {
                        dwHQVSrcLeftOffset = (rSrc.left << 1)&~31 ;
                    }
                    *lpHQVoffset = dwHQVSrcTopOffset+dwHQVSrcLeftOffset;

                }
                else
                {
                    dwOffset = (rSrc.top * dwSrcPitch) +
                           ((rSrc.left * lpDPF->dwRGBBitCount) >> 3);
                }
       }
   }
   else 
   {
        pVia->swov.overlayRecordV1.dwUVoffset = dwOffset = 0;
   }

   return dwOffset;
}

YCBCRREC viaOverlayGetYCbCrStartAddress(unsigned long dwVideoFlag,unsigned long dwStartAddr, unsigned long dwOffset,unsigned long dwUVoffset,unsigned long dwSrcPitch/*lpGbl->lPitch*/,unsigned long dwSrcHeight/*lpGbl->wHeight*/)
{
   YCBCRREC YCbCr;

   /*dwStartAddr =  (unsigned long)lpGbl->fpVidMem - VideoBase;*/
   if (dwVideoFlag&VIDEO_HQV_INUSE)
   {
       YCbCr.dwY   =  dwStartAddr;
       YCbCr.dwCB  =  dwStartAddr + dwSrcPitch * dwSrcHeight ;
       YCbCr.dwCR  =  dwStartAddr + dwSrcPitch * dwSrcHeight
                         + dwSrcPitch * (dwSrcHeight >>2);
   }
   else
   {
       YCbCr.dwY   =  dwStartAddr+dwOffset;
       YCbCr.dwCB  =  dwStartAddr + dwSrcPitch * dwSrcHeight 
                         + dwUVoffset;
       YCbCr.dwCR  =  dwStartAddr + dwSrcPitch * dwSrcHeight
                         + dwSrcPitch * (dwSrcHeight >>2) 
                         + dwUVoffset;
   }
   return YCbCr;
}


unsigned long viaOverlayHQVCalcZoomWidth(VIAPtr pVia, unsigned long dwVideoFlag, unsigned long srcWidth , unsigned long dstWidth,
                           unsigned long * lpzoomCtl, unsigned long * lpminiCtl, unsigned long * lpHQVfilterCtl, unsigned long * lpHQVminiCtl,unsigned long * lpHQVzoomflag)
{
    unsigned long dwTmp;

    if (srcWidth == dstWidth)
    {       
        *lpHQVfilterCtl |= HQV_H_FILTER_DEFAULT;
    }
    else
    {
    
        if (srcWidth < dstWidth) {
            /* zoom in*/
            *lpzoomCtl = srcWidth*0x0800 / dstWidth;
            *lpzoomCtl = (((*lpzoomCtl) & 0x7FF) << 16) | V1_X_ZOOM_ENABLE;
            *lpminiCtl |= ( V1_X_INTERPOLY );  /* set up interpolation*/
            *lpHQVzoomflag = 1;
            *lpHQVfilterCtl |= HQV_H_FILTER_DEFAULT ;
        } else if (srcWidth > dstWidth) {
            /* zoom out*/
            unsigned long srcWidth1;
    
            /*HQV rounding patch
            //dwTmp = dstWidth*0x0800 / srcWidth;*/
            dwTmp = dstWidth*0x0800*0x400 / srcWidth;
            dwTmp = dwTmp / 0x400 + ((dwTmp & 0x3ff)?1:0);

            *lpHQVminiCtl = (dwTmp & 0x7FF)| HQV_H_MINIFY_ENABLE;
    
    
            srcWidth1 = srcWidth >> 1;
            if (srcWidth1 <= dstWidth) {
                *lpminiCtl |= V1_X_DIV_2+V1_X_INTERPOLY;
                if (dwVideoFlag&VIDEO_1_INUSE)
                {
                    pVia->swov.overlayRecordV1.dwFetchAlignment = 3;
                    pVia->swov.overlayRecordV1.dwminifyH = 2;
                }
                else
                {
                    pVia->swov.overlayRecordV3.dwFetchAlignment = 3;
                    pVia->swov.overlayRecordV3.dwminifyH = 2;
                }
                *lpHQVfilterCtl |= HQV_H_TAP4_121;
                /* *lpHQVminiCtl = 0x00000c00;*/
            }
            else {
                srcWidth1 >>= 1;
    
                if (srcWidth1 <= dstWidth) {
                    *lpminiCtl |= V1_X_DIV_4+V1_X_INTERPOLY;
                    if (dwVideoFlag&VIDEO_1_INUSE)
                    {
                        pVia->swov.overlayRecordV1.dwFetchAlignment = 7;
                        pVia->swov.overlayRecordV1.dwminifyH = 4;
                    }
                    else
                    {
                        pVia->swov.overlayRecordV3.dwFetchAlignment = 7;
                        pVia->swov.overlayRecordV3.dwminifyH = 4;
                    }
                    *lpHQVfilterCtl |= HQV_H_TAP4_121;
                    /* *lpHQVminiCtl = 0x00000a00;*/
                }
                else {
                    srcWidth1 >>= 1;
    
                    if (srcWidth1 <= dstWidth) {
                        *lpminiCtl |= V1_X_DIV_8+V1_X_INTERPOLY;
                        if (dwVideoFlag&VIDEO_1_INUSE)
                        {
                            pVia->swov.overlayRecordV1.dwFetchAlignment = 15;
                            pVia->swov.overlayRecordV1.dwminifyH = 8;
                        }
                        else
                        {
                            pVia->swov.overlayRecordV3.dwFetchAlignment = 15;
                            pVia->swov.overlayRecordV3.dwminifyH = 8;
                        }
                        *lpHQVfilterCtl |= HQV_H_TAP8_12221;
                        /* *lpHQVminiCtl = 0x00000900;*/
                    }
                    else {
                        srcWidth1 >>= 1;
    
                        if (srcWidth1 <= dstWidth) {
                            *lpminiCtl |= V1_X_DIV_16+V1_X_INTERPOLY;
                            if (dwVideoFlag&VIDEO_1_INUSE)
                            {
                                pVia->swov.overlayRecordV1.dwFetchAlignment = 15;
                                pVia->swov.overlayRecordV1.dwminifyH = 16;
                            }
                            else
                            {
                                pVia->swov.overlayRecordV3.dwFetchAlignment = 15;
                                pVia->swov.overlayRecordV3.dwminifyH = 16;
                            }
                            *lpHQVfilterCtl |= HQV_H_TAP8_12221;
                            /* *lpHQVminiCtl = 0x00000880;*/
                        }
                        else {
                            /* too small to handle
                            //VIDOutD(V_COMPOSE_MODE, dwCompose);
                            //lpUO->ddRVal = PI_OK;
                            //return DDHAL_DRIVER_NOTHANDLED;*/
                            *lpminiCtl |= V1_X_DIV_16+V1_X_INTERPOLY;
                            if (dwVideoFlag&VIDEO_1_INUSE)
                            {
                                pVia->swov.overlayRecordV1.dwFetchAlignment = 15;
                                pVia->swov.overlayRecordV1.dwminifyH = 16;
                            }
                            else
                            {
                                pVia->swov.overlayRecordV3.dwFetchAlignment = 15;
                                pVia->swov.overlayRecordV3.dwminifyH = 16;
                            }
                            *lpHQVfilterCtl |= HQV_H_TAP8_12221;
                        }
                    }
                }
            }
    
            *lpHQVminiCtl |= HQV_HDEBLOCK_FILTER;

            if (srcWidth1 < dstWidth) {
                /* CLE bug
                   *lpzoomCtl = srcWidth1*0x0800 / dstWidth;*/
                *lpzoomCtl = (srcWidth1-2)*0x0800 / dstWidth;                
                *lpzoomCtl = ((*lpzoomCtl & 0x7FF) << 16) | V1_X_ZOOM_ENABLE;
            }
        }
    }

    return ~PI_ERR;
}

unsigned long viaOverlayHQVCalcZoomHeight (VIAPtr pVia, unsigned long srcHeight,unsigned long dstHeight,
                             unsigned long * lpzoomCtl, unsigned long * lpminiCtl, unsigned long * lpHQVfilterCtl, unsigned long * lpHQVminiCtl,unsigned long * lpHQVzoomflag)
{
    unsigned long dwTmp;
/*    if (pVia->pBIOSInfo->scaleY)
    {
        dstHeight = dstHeight + 1;
    }*/
    
    if (srcHeight < dstHeight) 
    {
        /* zoom in*/
        dwTmp = srcHeight * 0x0400 / dstHeight;
        *lpzoomCtl |= ((dwTmp & 0x3FF) | V1_Y_ZOOM_ENABLE);
        *lpminiCtl |= (V1_Y_INTERPOLY | V1_YCBCR_INTERPOLY);
        *lpHQVzoomflag = 1;
        *lpHQVfilterCtl |= HQV_V_TAP4_121;
    } 
    else if (srcHeight == dstHeight)
    {       
        *lpHQVfilterCtl |= HQV_V_TAP4_121;
    }
    else if (srcHeight > dstHeight) 
    {
        /* zoom out*/
        unsigned long srcHeight1;
      
        /*HQV rounding patch
        //dwTmp = dstHeight*0x0800 / srcHeight;*/
        dwTmp = dstHeight*0x0800*0x400 / srcHeight;
        dwTmp = dwTmp / 0x400 + ((dwTmp & 0x3ff)?1:0);
        
        *lpHQVminiCtl |= ((dwTmp& 0x7FF)<<16)|HQV_V_MINIFY_ENABLE;
      
        srcHeight1 = srcHeight >> 1;
        if (srcHeight1 <= dstHeight) 
        {
            *lpminiCtl |= V1_Y_DIV_2;
            *lpHQVfilterCtl |= HQV_V_TAP4_121 ;
            /* *lpHQVminiCtl |= 0x0c000000;*/
        }
        else 
        {
            srcHeight1 >>= 1;
            if (srcHeight1 <= dstHeight) 
            {
                *lpminiCtl |= V1_Y_DIV_4;
                *lpHQVfilterCtl |= HQV_V_TAP4_121 ;
                /* *lpHQVminiCtl |= 0x0a000000;*/
            }
            else 
            {
                srcHeight1 >>= 1;
      
                if (srcHeight1 <= dstHeight) 
                {
                    *lpminiCtl |= V1_Y_DIV_8;
                    *lpHQVfilterCtl |= HQV_V_TAP8_12221;
                    /* *lpHQVminiCtl |= 0x09000000;*/
                }
                else 
                {
                    srcHeight1 >>= 1;
      
                    if (srcHeight1 <= dstHeight) 
                    {
                        *lpminiCtl |= V1_Y_DIV_16;
                        *lpHQVfilterCtl |= HQV_V_TAP8_12221;
                        /* *lpHQVminiCtl |= 0x08800000;*/
                    }
                    else 
                    {
                        /* too small to handle
                        //VIDOutD(V_COMPOSE_MODE, dwCompose);
                        //lpUO->ddRVal = PI_OK;
                        //Fixed QAW91013
                        //return DDHAL_DRIVER_NOTHANDLED;*/
                        *lpminiCtl |= V1_Y_DIV_16;
                        *lpHQVfilterCtl |= HQV_V_TAP8_12221;
                    }
                }
            }
        }
      
        *lpHQVminiCtl |= HQV_VDEBLOCK_FILTER;

        if (srcHeight1 < dstHeight) 
        {
            dwTmp = srcHeight1 * 0x0400 / dstHeight;
            *lpzoomCtl |= ((dwTmp & 0x3FF) | V1_Y_ZOOM_ENABLE);
            *lpminiCtl |= ( V1_Y_INTERPOLY|V1_YCBCR_INTERPOLY);
        }
    }
    
    return ~PI_ERR;
}


unsigned long viaOverlayGetFetch(unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF,unsigned long dwSrcWidth,unsigned long dwDstWidth,unsigned long dwOriSrcWidth,unsigned long * lpHQVsrcFetch)
{
   unsigned long dwFetch=0;
   
   if (lpDPF->dwFlags & DDPF_FOURCC)
   {
       DBG_DD(ErrorF("DDOver_GetFetch : FourCC= 0x%lx\n", lpDPF->dwFourCC));
       switch (lpDPF->dwFourCC) {
       case FOURCC_YV12:
       case FOURCC_VIA:

            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpHQVsrcFetch = dwOriSrcWidth;
                if (dwDstWidth >= dwSrcWidth)
                    dwFetch = ((((dwSrcWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
                else
                    dwFetch = ((((dwDstWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;            
            }
            else
            {
                /* we fetch one more quadword to avoid get less video data
                //dwFetch = (((dwSrcWidth +V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT)>> V1_FETCHCOUNT_UNIT) +1;*/
                dwFetch = (((dwSrcWidth + 0x1F)&~0x1f)>> V1_FETCHCOUNT_UNIT);
            }
            break;
       case FOURCC_UYVY:
       case FOURCC_YUY2:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpHQVsrcFetch = dwOriSrcWidth<<1;
                if (dwDstWidth >= dwSrcWidth)
                    dwFetch = ((((dwSrcWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
                else
                    dwFetch = ((((dwDstWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
            }
            else
            {
                /*Comment by Vinecnt ,we fetch one more quadword to avoid get less video data*/
                dwFetch = ((((dwSrcWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
            }
            break;
       default :
            DBG_DD(ErrorF("DDOver_GetFetch : Invalid FOURCC format :(0x%lx)in V1!\n", lpDPF->dwFourCC));
            break;
       }
   }
   else if (lpDPF->dwFlags & DDPF_RGB) {
       switch (lpDPF->dwRGBBitCount) {
       case 16:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpHQVsrcFetch = dwOriSrcWidth<<1;
                if (dwDstWidth >= dwSrcWidth)
                    dwFetch = ((((dwSrcWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
                else
                    dwFetch = ((((dwDstWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
            }
            else
            {
                /*Comment by Vinecnt ,we fetch one more quadword to avoid get less video data*/
                dwFetch = ((((dwSrcWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
            }
           break;
       case 32:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpHQVsrcFetch = dwOriSrcWidth<<2;
                if (dwDstWidth >= dwSrcWidth)
                    dwFetch = ((((dwSrcWidth<<2)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
                else
                    dwFetch = ((((dwDstWidth<<2)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
            }
            else
            {
                /*Comment by Vinecnt ,we fetch one more quadword to avoid get less video data*/
                dwFetch = ((((dwSrcWidth<<2)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
            }
           break;

       default :
            DBG_DD(ErrorF("DDOver_GetFetch : invalid RGB format %ld bits\n",lpDPF->dwRGBBitCount));
            break;
       }
   }

   /*Fix plannar mode problem*/
   if (dwFetch <4)
   {
        dwFetch = 4;
   }
   return dwFetch;
}

void viaOverlayGetDisplayCount(VIAPtr pVia, unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF,unsigned long dwSrcWidth,unsigned long * lpDisplayCountW)
{
    
   /*unsigned long dwFetch=0;*/
   
   if (lpDPF->dwFlags & DDPF_FOURCC)
   {
       switch (lpDPF->dwFourCC) {
       case FOURCC_YV12:
       case FOURCC_UYVY:
       case FOURCC_YUY2:
       case FOURCC_VIA:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpDisplayCountW = dwSrcWidth - 1;
            }
            else
            {
                /* *lpDisplayCountW = dwSrcWidth - 2*pVia->swov.overlayRecordV1.dwminifyH;*/
                *lpDisplayCountW = dwSrcWidth - pVia->swov.overlayRecordV1.dwminifyH;
            }
            break;
       default :
            DBG_DD(ErrorF("DDOver_GetDisplayCount : Invalid FOURCC format :(0x%lx)in V1!\n", lpDPF->dwFourCC));
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpDisplayCountW = dwSrcWidth - 1;
            }
            else
            {
                /* *lpDisplayCountW = dwSrcWidth - 2*pVia->swov.overlayRecordV1.dwminifyH;*/
                *lpDisplayCountW = dwSrcWidth - pVia->swov.overlayRecordV1.dwminifyH;
            }
            break;
       }
   }
   else if (lpDPF->dwFlags & DDPF_RGB) {
       switch (lpDPF->dwRGBBitCount) {
       case 16:
       case 32:
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpDisplayCountW = dwSrcWidth - 1;
            }
            else
            {
                *lpDisplayCountW = dwSrcWidth - pVia->swov.overlayRecordV1.dwminifyH;
            }
            break;

       default :
            DBG_DD(ErrorF("DDOver_GetDisplayCount : invalid RGB format %ld bits\n",lpDPF->dwRGBBitCount));
            if (dwVideoFlag&VIDEO_HQV_INUSE)
            {
                *lpDisplayCountW = dwSrcWidth - 1;
            }
            else
            {
                *lpDisplayCountW = dwSrcWidth - pVia->swov.overlayRecordV1.dwminifyH;
            }
            break;
       }
   }   
}

/*
 * This function uses quadratic mapping to adjust the midpoint of the scaling.
 */


static float rangeEqualize(float inLow,float inHigh,float outLow,float outHigh,float outMid,float inValue)
{
    float
        inRange = inHigh - inLow,
	outRange = outHigh - outLow,
	normIn = ((inValue - inLow) / inRange)*2.-1.,
	delta = outMid - outRange*0.5 - outLow;
    return (inValue - inLow) * outRange / inRange + outLow + (1. - normIn*normIn)*delta;
}

static unsigned vPackFloat(float val, float hiLimit, float loLimit, float mult, int shift, 
			   Bool doSign) 
{
    unsigned packed,mask,sign;
    val = (val > hiLimit) ? hiLimit : val;
    val = (val < loLimit) ? loLimit : val;
    sign = (val < 0) ? 1:0;
    val = (sign) ? -val : val;
    packed = ((unsigned)(val*mult + 1.)) >> 1;
    mask = (1 << shift) - 1;
    return (((packed >= mask) ? mask : packed) | ((doSign) ? (sign << shift) : 0));
    
}


typedef float colorCoeff[5];
static colorCoeff colorCTable[] = {{1.1875,1.625,0.875,0.375,2.0}, 
				   {1.164,1.596,0.54,0.45,2.2}}; 

/*
 * This function is a partial rewrite of the overlay.c file of the original VIA drivers,
 * which was extremely nasty and difficult to follow. Coefficient for new chipset models should
 * be added in the table above and, if needed, implemented in the model switch below.
 */

void viaCalculateVideoColor(VIAPtr pVia, int hue, int saturation, int brightness, 
			    int contrast,Bool reset,CARD32 *col1,CARD32 *col2)
{
    float fA,fB1,fC1,fD,fB2,fC2,fB3,fC3;
    float fPI,fContrast,fSaturation,fHue,fBrightness;
    const float *mCoeff; 
    unsigned long dwA,dwB1,dwC1,dwD,dwB2,dwC2,dwB3,dwC3,dwS;
    unsigned long dwD_Int,dwD_Dec;
    int intD;
    int model;
    fPI = (float)(M_PI/180.);    

    if ( reset ) {
	saturation = 10000;
	brightness = 5000;
	contrast = 10000;
    }

    switch ( pVia->ChipId ) {
    case PCI_CHIP_VT3205:
	model = 0;
	break;
    case PCI_CHIP_CLE3122:
	model = (CLE266_REV_IS_CX(pVia->ChipRev) ? 0 : 1); 
	break;
    default:
	ErrorF("Unknown Chip ID\n");
	model = 0;
    }

    switch( model ) {
    case 0:
	fBrightness = rangeEqualize(0.,10000.,-128.,128.,-16.,(float) brightness);
	fContrast = rangeEqualize(0.,20000.,0.,1.6645,1.0,(float) contrast);
	fSaturation = rangeEqualize(0.,20000,0.,2.,1.,(float) saturation);
	break;
    default:
	fBrightness = rangeEqualize(0.,10000.,-128.,128.,-12.,(float) brightness);
	fContrast = rangeEqualize(0.,20000.,0.,1.6645,1.1,(float) contrast);
	fSaturation = rangeEqualize(0.,20000,0.,2.,1.15,(float) saturation);
	break;
    }
    fHue = (float)hue;
	  
    mCoeff = colorCTable[model];

    fA  = (float)(mCoeff[0]*fContrast);
    fB1 = (float)(-mCoeff[1]*fContrast*fSaturation*sin(fHue*fPI));
    fC1 = (float)(mCoeff[1]*fContrast*fSaturation*cos(fHue*fPI));
    fD  = (float)(mCoeff[0]*(fBrightness));
    fB2 = (float)((mCoeff[2]*sin(fHue*fPI)-
		   mCoeff[3]*cos(fHue*fPI))*fContrast*fSaturation);
    fC2 = (float)(-(mCoeff[2]*cos(fHue*fPI)+
		    mCoeff[3]*sin(fHue*fPI))*fContrast*fSaturation);
    fB3 = (float)(mCoeff[4]*fContrast*fSaturation*cos(fHue*fPI));
    fC3 = (float)(mCoeff[4]*fContrast*fSaturation*sin(fHue*fPI));
      
    switch(model) {
    case 0:
	dwA = vPackFloat(fA,1.9375,0.,32.,5,0);
	dwB1 = vPackFloat(fB1,2.125,-2.125,16.,5,1);
	dwC1 = vPackFloat(fC1,2.125,-2.125,16.,5,1);
	
	if (fD>=0) {
	    intD=(int)fD;
	    if (intD>127)
		intD = 127;
	    dwD_Int = ((unsigned long)intD)&0xff;
	    dwD = ((unsigned long)(fD*16+1))>>1;        
	    dwD_Dec= dwD&0x7;
	} else {        
	    intD=(int)fD;
	    if (intD< -128)
		intD = -128;        
	    intD = intD+256; 
	    dwD_Int = ((unsigned long)intD)&0xff;
	    fD = -fD;
	    dwD = ((unsigned long)(fD*16+1))>>1;                
	    dwD_Dec= dwD&0x7;
	}

	dwB2 = vPackFloat(fB2,1.875,-1.875,16,4,1);
	dwC2 = vPackFloat(fC2,1.875,-1.875,16,4,1);
	dwB3 = vPackFloat(fB3,3.875,-3.875,16,5,1);
	dwC3 = vPackFloat(fC3,3.875,-3.875,16,5,1);
	*col1 = (dwA<<24)|(dwB1<<16)|(dwC1<<8)|dwD_Int;
	*col2 = (dwD_Dec<<29|dwB2<<24)|(dwC2<<16)|(dwB3<<8)|(dwC3);
	break;

    default:
	dwA = vPackFloat(fA,1.9375,-0.,32,5,0);
	dwB1 = vPackFloat(fB1,0.75,-0.75,8.,2,1);
	dwC1 = vPackFloat(fC1,2.875,1.,16.,5,0);
	
	if (fD>=127)
	    fD=127;
	
	if (fD<=-128)
	    fD=-128;    
	
	if (fD>=0) {
	    dwS = 0;
	}
	else {
	    dwS = 1;
	    fD = fD+128;
	}
	
	dwD = ((unsigned long)(fD*2+1))>>1;
	if (dwD>=0x7f) {
	    dwD = 0x7f|(dwS<<7);
	} else {
	    dwD = (dwD&0x7f)|(dwS<<7);
	}
	
	dwB2 = vPackFloat(fB2,0.,-0.875,16.,3,0); 
	dwC2 = vPackFloat(fC2,0.,-1.875,16.,4,0); 
	dwB3 = vPackFloat(fB3,3.75,0.,8.,4,0);
	dwC3 = vPackFloat(fC3,1.25,-1.25,8.,3,1);
	*col1 = (dwA<<24)|(dwB1<<18)|(dwC1<<9)|dwD;
	*col2 = (dwB2<<25)|(dwC2<<17)|(dwB3<<10)|(dwC3<<2);
	break;
    }
}


