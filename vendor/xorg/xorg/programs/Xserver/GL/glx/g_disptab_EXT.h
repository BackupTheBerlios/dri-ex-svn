/* $XFree86: xc/programs/Xserver/GL/glx/g_disptab_EXT.h,v 1.5 2004/01/28 18:11:50 alanh Exp $ */
/* DO NOT EDIT - THIS FILE IS AUTOMATICALLY GENERATED */
#ifndef _GLX_g_disptab_EXT_h_
#define _GLX_g_disptab_EXT_h_
/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: This software was created using the
** OpenGL(R) version 1.2.1 Sample Implementation published by SGI, but has
** not been independently verified as being compliant with the OpenGL(R)
** version 1.2.1 Specification.
*/

extern int __glXDisp_AreTexturesResidentEXT(__GLXclientState*, GLbyte*);
extern int __glXDisp_DeleteTexturesEXT(__GLXclientState*, GLbyte*);
extern int __glXDisp_GenTexturesEXT(__GLXclientState*, GLbyte*);
extern int __glXDisp_IsTextureEXT(__GLXclientState*, GLbyte*);

extern void __glXDisp_ColorTable(GLbyte*);
extern void __glXDisp_ColorTableParameterfv(GLbyte*);
extern void __glXDisp_ColorTableParameteriv(GLbyte*);
extern void __glXDisp_CopyColorTable(GLbyte*);
extern void __glXDisp_BlendColor(GLbyte*);
extern void __glXDisp_BlendEquation(GLbyte*);
extern void __glXDisp_TexSubImage1D(GLbyte*);
extern void __glXDisp_TexSubImage2D(GLbyte*);
extern void __glXDisp_ConvolutionFilter1D(GLbyte*);
extern void __glXDisp_ConvolutionFilter2D(GLbyte*);
extern void __glXDisp_ConvolutionParameterf(GLbyte*);
extern void __glXDisp_ConvolutionParameterfv(GLbyte*);
extern void __glXDisp_ConvolutionParameteri(GLbyte*);
extern void __glXDisp_ConvolutionParameteriv(GLbyte*);
extern void __glXDisp_CopyConvolutionFilter1D(GLbyte*);
extern void __glXDisp_CopyConvolutionFilter2D(GLbyte*);
extern void __glXDisp_SeparableFilter2D(GLbyte*);
extern void __glXDisp_Histogram(GLbyte*);
extern void __glXDisp_Minmax(GLbyte*);
extern void __glXDisp_ResetHistogram(GLbyte*);
extern void __glXDisp_ResetMinmax(GLbyte*);
extern void __glXDisp_TexImage3D(GLbyte*);
extern void __glXDisp_TexSubImage3D(GLbyte*);
extern void __glXDisp_DrawArraysEXT(GLbyte*);
extern void __glXDisp_BindTexture(GLbyte*);
extern void __glXDisp_PrioritizeTextures(GLbyte*);
extern void __glXDisp_CopyTexImage1D(GLbyte*);
extern void __glXDisp_CopyTexImage2D(GLbyte*);
extern void __glXDisp_CopyTexSubImage1D(GLbyte*);
extern void __glXDisp_CopyTexSubImage2D(GLbyte*);
extern void __glXDisp_CopyTexSubImage3D(GLbyte*);
extern void __glXDisp_PointParameterfARB(GLbyte*);
extern void __glXDisp_PointParameterfvARB(GLbyte*);

extern void __glXDisp_FogCoordfv(GLbyte *);
extern void __glXDisp_FogCoorddv(GLbyte *);
extern void __glXDispSwap_FogCoordfv(GLbyte *);
extern void __glXDispSwap_FogCoorddv(GLbyte *);

extern void __glXDisp_SecondaryColor3bv(GLbyte *);
extern void __glXDisp_SecondaryColor3sv(GLbyte *);
extern void __glXDisp_SecondaryColor3iv(GLbyte *);
extern void __glXDisp_SecondaryColor3ubv(GLbyte *);
extern void __glXDisp_SecondaryColor3usv(GLbyte *);
extern void __glXDisp_SecondaryColor3uiv(GLbyte *);
extern void __glXDisp_SecondaryColor3fv(GLbyte *);
extern void __glXDisp_SecondaryColor3dv(GLbyte *);
extern void __glXDispSwap_SecondaryColor3bv(GLbyte *);
extern void __glXDispSwap_SecondaryColor3sv(GLbyte *);
extern void __glXDispSwap_SecondaryColor3iv(GLbyte *);
extern void __glXDispSwap_SecondaryColor3ubv(GLbyte *);
extern void __glXDispSwap_SecondaryColor3usv(GLbyte *);
extern void __glXDispSwap_SecondaryColor3uiv(GLbyte *);
extern void __glXDispSwap_SecondaryColor3fv(GLbyte *);
extern void __glXDispSwap_SecondaryColor3dv(GLbyte *);

extern void __glXDisp_BlendFuncSeparate(GLbyte *);
extern void __glXDispSwap_BlendFuncSeparate(GLbyte *);

#ifdef __DARWIN__
extern void __glXDisp_PointParameteriNV(GLbyte *);
extern void __glXDisp_PointParameterivNV(GLbyte *);
extern void __glXDispSwap_PointParameteriNV(GLbyte *);
extern void __glXDispSwap_PointParameterivNV(GLbyte *);
#else
extern void __glXDisp_PointParameteri(GLbyte *);
extern void __glXDisp_PointParameteriv(GLbyte *);
extern void __glXDispSwap_PointParameteri(GLbyte *);
extern void __glXDispSwap_PointParameteriv(GLbyte *);
#endif

extern void __glXDisp_ActiveStencilFaceEXT(GLbyte*);

extern int __glXDispSwap_AreTexturesResidentEXT(__GLXclientState*, GLbyte*);
extern int __glXDispSwap_DeleteTexturesEXT(__GLXclientState*, GLbyte*);
extern int __glXDispSwap_GenTexturesEXT(__GLXclientState*, GLbyte*);
extern int __glXDispSwap_IsTextureEXT(__GLXclientState*, GLbyte*);

extern void __glXDispSwap_ColorTable(GLbyte*);
extern void __glXDispSwap_ColorTableParameterfv(GLbyte*);
extern void __glXDispSwap_ColorTableParameteriv(GLbyte*);
extern void __glXDispSwap_CopyColorTable(GLbyte*);
extern void __glXDispSwap_BlendColor(GLbyte*);
extern void __glXDispSwap_BlendEquation(GLbyte*);
extern void __glXDispSwap_TexSubImage1D(GLbyte*);
extern void __glXDispSwap_TexSubImage2D(GLbyte*);
extern void __glXDispSwap_ConvolutionFilter1D(GLbyte*);
extern void __glXDispSwap_ConvolutionFilter2D(GLbyte*);
extern void __glXDispSwap_ConvolutionParameterf(GLbyte*);
extern void __glXDispSwap_ConvolutionParameterfv(GLbyte*);
extern void __glXDispSwap_ConvolutionParameteri(GLbyte*);
extern void __glXDispSwap_ConvolutionParameteriv(GLbyte*);
extern void __glXDispSwap_CopyConvolutionFilter1D(GLbyte*);
extern void __glXDispSwap_CopyConvolutionFilter2D(GLbyte*);
extern void __glXDispSwap_SeparableFilter2D(GLbyte*);
extern void __glXDispSwap_Histogram(GLbyte*);
extern void __glXDispSwap_Minmax(GLbyte*);
extern void __glXDispSwap_ResetHistogram(GLbyte*);
extern void __glXDispSwap_ResetMinmax(GLbyte*);
extern void __glXDispSwap_TexImage3D(GLbyte*);
extern void __glXDispSwap_TexSubImage3D(GLbyte*);
extern void __glXDispSwap_DrawArraysEXT(GLbyte*);
extern void __glXDispSwap_BindTexture(GLbyte*);
extern void __glXDispSwap_PrioritizeTextures(GLbyte*);
extern void __glXDispSwap_CopyTexImage1D(GLbyte*);
extern void __glXDispSwap_CopyTexImage2D(GLbyte*);
extern void __glXDispSwap_CopyTexSubImage1D(GLbyte*);
extern void __glXDispSwap_CopyTexSubImage2D(GLbyte*);
extern void __glXDispSwap_CopyTexSubImage3D(GLbyte*);
extern void __glXDispSwap_PointParameterfARB(GLbyte*);
extern void __glXDispSwap_PointParameterfvARB(GLbyte*);
extern void __glXDispSwap_ActiveStencilFaceEXT(GLbyte*);

#define __GLX_MIN_RENDER_OPCODE_EXT 2053
#define __GLX_MAX_RENDER_OPCODE_EXT 4222
#define __GLX_MIN_VENDPRIV_OPCODE_EXT 11
#define __GLX_MAX_VENDPRIV_OPCODE_EXT 14
#define __GLX_VENDPRIV_TABLE_SIZE_EXT (__GLX_MAX_VENDPRIV_OPCODE_EXT - __GLX_MIN_VENDPRIV_OPCODE_EXT + 1)
#define __GLX_RENDER_TABLE_SIZE_EXT (__GLX_MAX_RENDER_OPCODE_EXT - __GLX_MIN_RENDER_OPCODE_EXT + 1)
extern __GLXdispatchRenderProcPtr __glXRenderTable_EXT[__GLX_RENDER_TABLE_SIZE_EXT];
extern __GLXdispatchVendorPrivProcPtr __glXVendorPrivTable_EXT[__GLX_VENDPRIV_TABLE_SIZE_EXT];
extern __GLXdispatchRenderProcPtr __glXSwapRenderTable_EXT[__GLX_RENDER_TABLE_SIZE_EXT];
extern __GLXdispatchVendorPrivProcPtr __glXSwapVendorPrivTable_EXT[__GLX_VENDPRIV_TABLE_SIZE_EXT];
#endif /* _GLX_g_disptab_EXT_h_ */