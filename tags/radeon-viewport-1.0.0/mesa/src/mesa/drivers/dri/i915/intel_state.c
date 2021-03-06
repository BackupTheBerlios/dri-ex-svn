/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "enums.h"
#include "dd.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "swrast/swrast.h"


static void intelDrawBuffer(GLcontext *ctx, GLenum mode )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   intelScreenPrivate *screen = intel->intelScreen;
   int front = 0;
 
   switch ( ctx->Color._DrawDestMask[0] ) {
   case DD_FRONT_LEFT_BIT:
      front = 1;
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   case DD_BACK_LEFT_BIT:
      front = 0;
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE );
      break;
   default:
      FALLBACK( intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE );
      return;
   }

   /* We want to update the s/w rast state too so that r200SetBuffer()
    * gets called.
    */
   _swrast_DrawBuffer(ctx, mode);

   if ( intel->sarea->pf_current_page == 1 ) 
      front ^= 1;
   
   intelSetFrontClipRects( intel );

   if (front) {
      intel->drawOffset = screen->frontOffset;
      intel->drawMap = (char *)intel->driScreen->pFB;
      intel->readMap = (char *)intel->driScreen->pFB;
   } else {
      intel->drawOffset = screen->backOffset;
      intel->drawMap = screen->back.map;
      intel->readMap = screen->back.map;
   }

   intel->vtbl.set_draw_offset( intel, intel->drawOffset );
}

static void intelReadBuffer( GLcontext *ctx, GLenum mode )
{
   /* nothing, until we implement h/w glRead/CopyPixels or CopyTexImage */
}


static void intelClearColor(GLcontext *ctx, const GLfloat color[4])
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   intelScreenPrivate *screen = intel->intelScreen;

   CLAMPED_FLOAT_TO_UBYTE(intel->clear_red, color[0]);
   CLAMPED_FLOAT_TO_UBYTE(intel->clear_green, color[1]);
   CLAMPED_FLOAT_TO_UBYTE(intel->clear_blue, color[2]);
   CLAMPED_FLOAT_TO_UBYTE(intel->clear_alpha, color[3]);

   intel->ClearColor = INTEL_PACKCOLOR(screen->fbFormat,
				       intel->clear_red, 
				       intel->clear_green, 
				       intel->clear_blue, 
				       intel->clear_alpha);
}


static void intelCalcViewport( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   const GLfloat *v = ctx->Viewport._WindowMap.m;
   GLfloat *m = intel->ViewportMatrix.m;

   /* See also intel_translate_vertex.  SUBPIXEL adjustments can be done
    * via state vars, too.
    */
   m[MAT_SX] =   v[MAT_SX];
   m[MAT_TX] =   v[MAT_TX] + SUBPIXEL_X;
   m[MAT_SY] = - v[MAT_SY];
   m[MAT_TY] = - v[MAT_TY] + intel->driDrawable->h + SUBPIXEL_Y;
   m[MAT_SZ] =   v[MAT_SZ] * intel->depth_scale;
   m[MAT_TZ] =   v[MAT_TZ] * intel->depth_scale;
}

static void intelViewport( GLcontext *ctx,
			  GLint x, GLint y,
			  GLsizei width, GLsizei height )
{
   intelCalcViewport( ctx );
}

static void intelDepthRange( GLcontext *ctx,
			    GLclampd nearval, GLclampd farval )
{
   intelCalcViewport( ctx );
}

/* Fallback to swrast for select and feedback.
 */
static void intelRenderMode( GLcontext *ctx, GLenum mode )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   FALLBACK( intel, INTEL_FALLBACK_RENDERMODE, (mode != GL_RENDER) );
}


void intelInitStateFuncs( struct dd_function_table *functions )
{
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
   functions->RenderMode = intelRenderMode;
   functions->Viewport = intelViewport;
   functions->DepthRange = intelDepthRange;
   functions->ClearColor = intelClearColor;
}

