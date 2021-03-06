/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x/2000/XP/XBox (Win32)
*
* Description:  GLDirect fastpath pipeline stage
*
****************************************************************************/

//---------------------------------------------------------------------------

//#include "../GLDirect.h"
//#include "../gld_log.h"
//#include "gld_dx8.h"

#include "dglcontext.h"
#include "ddlog.h"
#include "gld_dx9.h"

//---------------------------------------------------------------------------

#include "glheader.h"
#include "context.h"
#include "macros.h"
// #include "mem.h"
#include "mtypes.h"
#include "mmath.h"

#include "math/m_matrix.h"
#include "math/m_xform.h"

#include "tnl/t_pipeline.h"

//---------------------------------------------------------------------------

__inline void _gldSetVertexShaderConstants(
	GLcontext *ctx,
	GLD_driver_dx9 *gld)
{
	D3DXMATRIX mat, matView, matProj;
	GLfloat		*pM;

	// Mesa 5: Altered to a Stack
	//pM = ctx->ModelView.m;
	pM = ctx->ModelviewMatrixStack.Top->m;
	matView._11 = pM[0];
	matView._12 = pM[1];
	matView._13 = pM[2];
	matView._14 = pM[3];
	matView._21 = pM[4];
	matView._22 = pM[5];
	matView._23 = pM[6];
	matView._24 = pM[7];
	matView._31 = pM[8];
	matView._32 = pM[9];
	matView._33 = pM[10];
	matView._34 = pM[11];
	matView._41 = pM[12];
	matView._42 = pM[13];
	matView._43 = pM[14];
	matView._44 = pM[15];

	// Mesa 5: Altered to a Stack
	//pM = ctx->ProjectionMatrix.m;
	pM = ctx->ProjectionMatrixStack.Top->m;
	matProj._11 = pM[0];
	matProj._12 = pM[1];
	matProj._13 = pM[2];
	matProj._14 = pM[3];
	matProj._21 = pM[4];
	matProj._22 = pM[5];
	matProj._23 = pM[6];
	matProj._24 = pM[7];
	matProj._31 = pM[8];
	matProj._32 = pM[9];
	matProj._33 = pM[10];
	matProj._34 = pM[11];
	matProj._41 = pM[12];
	matProj._42 = pM[13];
	matProj._43 = pM[14];
	matProj._44 = pM[15];

	D3DXMatrixMultiply( &mat, &matView, &matProj );
	D3DXMatrixTranspose( &mat, &mat );

	_GLD_DX9_DEV(SetVertexShaderConstantF(gld->pDev, 0, (float*)&mat, 4));
}

//---------------------------------------------------------------------------

static GLboolean gld_d3d_render_stage_run(
	GLcontext *ctx,
	struct gl_pipeline_stage *stage)
{
	GLD_context				*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx9			*gld	= GLD_GET_DX9_DRIVER(gldCtx);

	TNLcontext				*tnl;
	struct vertex_buffer	*VB;
	GLuint					new_inputs;
	tnl_render_func				*tab;
	GLint					pass;
	GLD_pb_dx9				*gldPB = &gld->PB3d;
/*
	static int count = 0;
	count++;
	if (count != 2)
		return GL_FALSE;
*/
	// The "check" function should disable this stage,
	// but we'll test gld->bUseMesaTnL anyway.
	if (gld->bUseMesaTnL) {
		// Do nothing in this stage, but continue pipeline
		return GL_FALSE;
	}
	
	tnl = TNL_CONTEXT(ctx);
	VB = &tnl->vb;
	new_inputs = stage->changed_inputs;
	pass = 0;

   tnl->Driver.Render.Start( ctx );

#if 0
   // For debugging: Useful to see if an app passes colour data in
   // an unusual format.
   switch (VB->ColorPtr[0]->Type) {
   case GL_FLOAT:
	   ddlogMessage(GLDLOG_SYSTEM, "ColorPtr: GL_FLOAT\n");
	   break;
   case GL_UNSIGNED_BYTE:
	   ddlogMessage(GLDLOG_SYSTEM, "ColorPtr: GL_UNSIGNED_BYTE\n");
	   break;
   default:
	   ddlogMessage(GLDLOG_SYSTEM, "ColorPtr: *?*\n");
	   break;
   }
#endif

   tnl->Driver.Render.Points		= gld_Points3D_DX9;
   if (ctx->_TriangleCaps & DD_FLATSHADE) {
	   tnl->Driver.Render.Line		= gld_Line3DFlat_DX9;
	   tnl->Driver.Render.Triangle	= gld_Triangle3DFlat_DX9;
	   tnl->Driver.Render.Quad		= gld_Quad3DFlat_DX9;
   } else {
	   tnl->Driver.Render.Line		= gld_Line3DSmooth_DX9;
	   tnl->Driver.Render.Triangle	= gld_Triangle3DSmooth_DX9;
	   tnl->Driver.Render.Quad		= gld_Quad3DSmooth_DX9;
   }

	_GLD_DX9_VB(Lock(gldPB->pVB, 0, 0, &gldPB->pPoints, D3DLOCK_DISCARD));
	gldPB->nPoints = gldPB->nLines = gldPB->nTriangles = 0;
	// Allocate primitive pointers
	// gldPB->pPoints is always first
	gldPB->pLines		= gldPB->pPoints + (gldPB->dwStride * gldPB->iFirstLine);
	gldPB->pTriangles	= gldPB->pPoints + (gldPB->dwStride * gldPB->iFirstTriangle);
	
	ASSERT(tnl->Driver.Render.BuildVertices);
	ASSERT(tnl->Driver.Render.PrimitiveNotify);
	ASSERT(tnl->Driver.Render.Points);
	ASSERT(tnl->Driver.Render.Line);
	ASSERT(tnl->Driver.Render.Triangle);
	ASSERT(tnl->Driver.Render.Quad);
	ASSERT(tnl->Driver.Render.ResetLineStipple);
	ASSERT(tnl->Driver.Render.Interp);
	ASSERT(tnl->Driver.Render.CopyPV);
	ASSERT(tnl->Driver.Render.ClippedLine);
	ASSERT(tnl->Driver.Render.ClippedPolygon);
	ASSERT(tnl->Driver.Render.Finish);

	tab = (VB->Elts ? tnl->Driver.Render.PrimTabElts : tnl->Driver.Render.PrimTabVerts);
	
	do {
		GLuint i, length, flags = 0;
		for (i = 0 ; !(flags & PRIM_LAST) ; i += length)
		{
			flags = VB->Primitive[i];
			length= VB->PrimitiveLength[i];
			ASSERT(length || (flags & PRIM_LAST));
			ASSERT((flags & PRIM_MODE_MASK) <= GL_POLYGON+1);
			if (length)
				tab[flags & PRIM_MODE_MASK]( ctx, i, i + length, flags );
		}
	} while (tnl->Driver.Render.Multipass &&
		tnl->Driver.Render.Multipass( ctx, ++pass ));
	
	_GLD_DX9_VB(Unlock(gldPB->pVB));

	_GLD_DX9_DEV(SetStreamSource(gld->pDev, 0, gldPB->pVB, 0, gldPB->dwStride));

	_GLD_DX9_DEV(SetTransform(gld->pDev, D3DTS_PROJECTION, &gld->matProjection));
	_GLD_DX9_DEV(SetTransform(gld->pDev, D3DTS_WORLD, &gld->matModelView));

	if (gldPB->nPoints) {
		_GLD_DX9_DEV(DrawPrimitive(gld->pDev, D3DPT_POINTLIST, 0, gldPB->nPoints));
		gldPB->nPoints = 0;
	}

	if (gldPB->nLines) {
		_GLD_DX9_DEV(DrawPrimitive(gld->pDev, D3DPT_LINELIST, gldPB->iFirstLine, gldPB->nLines));
		gldPB->nLines = 0;
	}

	if (gldPB->nTriangles) {
		_GLD_DX9_DEV(DrawPrimitive(gld->pDev, D3DPT_TRIANGLELIST, gldPB->iFirstTriangle, gldPB->nTriangles));
		gldPB->nTriangles = 0;
	}

	return GL_FALSE;		/* finished the pipe */
}

//---------------------------------------------------------------------------

static void gld_d3d_render_stage_check(
	GLcontext *ctx,
	struct gl_pipeline_stage *stage)
{
	GLD_context				*gldCtx	= GLD_GET_CONTEXT(ctx);
	GLD_driver_dx9			*gld	= GLD_GET_DX9_DRIVER(gldCtx);
	// Is this thread safe?
	stage->active = (gld->bUseMesaTnL) ? GL_FALSE : GL_TRUE;
	return;
}

//---------------------------------------------------------------------------

static void gld_d3d_render_stage_dtr( struct gl_pipeline_stage *stage )
{
}

//---------------------------------------------------------------------------

const struct gl_pipeline_stage _gld_d3d_render_stage =
{
   "gld_d3d_render_stage",
   (_NEW_BUFFERS |
    _DD_NEW_SEPARATE_SPECULAR |
    _DD_NEW_FLATSHADE |
    _NEW_TEXTURE|
    _NEW_LIGHT|
    _NEW_POINT|
    _NEW_FOG|
    _DD_NEW_TRI_UNFILLED |
    _NEW_RENDERMODE),		/* re-check (new inputs, interp function) */
   0,				/* re-run (always runs) */
   GL_TRUE,			/* active */
   0, 0,			/* inputs (set in check_render), outputs */
   0, 0,			/* changed_inputs, private */
   gld_d3d_render_stage_dtr,		/* destructor */
   gld_d3d_render_stage_check,		/* check */
   gld_d3d_render_stage_run			/* run */
};

//---------------------------------------------------------------------------
