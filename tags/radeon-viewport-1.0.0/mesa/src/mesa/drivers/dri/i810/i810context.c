/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/i810/i810context.c,v 1.3 2002/10/30 12:51:33 alanh Exp $ */

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */


#include "glheader.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"
#include "extensions.h"
#include "imports.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "array_cache/acache.h"

#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810state.h"
#include "i810tex.h"
#include "i810span.h"
#include "i810tris.h"
#include "i810vb.h"
#include "i810ioctl.h"

#include "utils.h"
#ifndef I810_DEBUG
int I810_DEBUG = (0);
#endif

PUBLIC const char __driConfigOptions[] = { 0 };
const GLuint __driNConfigOptions = 0;

static const GLubyte *i810GetString( GLcontext *ctx, GLenum name )
{
   switch (name) {
   case GL_VENDOR:
      return (GLubyte *)"Keith Whitwell";
   case GL_RENDERER:
      return (GLubyte *)"Mesa DRI I810 20021125";
   default:
      return 0;
   }
}

static void i810BufferSize(GLframebuffer *buffer, GLuint *width, GLuint *height)
{
   GET_CURRENT_CONTEXT(ctx);
   i810ContextPtr imesa = I810_CONTEXT(ctx);

   /* Need to lock to make sure the driDrawable is uptodate.  This
    * information is used to resize Mesa's software buffers, so it has
    * to be correct.
    */
   LOCK_HARDWARE(imesa);
   *width = imesa->driDrawable->w;
   *height = imesa->driDrawable->h;
   UNLOCK_HARDWARE(imesa);
}

/* Extension strings exported by the i810 driver.
 */
static const char * const card_extensions[] =
{
   "GL_ARB_multitexture",
   "GL_ARB_texture_env_add",
   "GL_ARB_texture_mirrored_repeat",
   "GL_EXT_stencil_wrap",
   "GL_EXT_texture_edge_clamp",
   "GL_EXT_texture_lod_bias",
   "GL_MESA_ycbcr_texture",
   "GL_SGIS_generate_mipmap",
   NULL
};

extern const struct tnl_pipeline_stage _i810_render_stage;

static const struct tnl_pipeline_stage *i810_pipeline[] = {
   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
				/* REMOVE: point attenuation stage */
#if 1
   &_i810_render_stage,		/* ADD: unclipped rastersetup-to-dma */
#endif
   &_tnl_render_stage,
   0,
};

static const struct dri_debug_control debug_control[] =
{
    { "fall",  DEBUG_FALLBACKS },
    { "tex",   DEBUG_TEXTURE },
    { "ioctl", DEBUG_IOCTL },
    { "prim",  DEBUG_PRIMS },
    { "vert",  DEBUG_VERTS },
    { "state", DEBUG_STATE },
    { "verb",  DEBUG_VERBOSE },
    { "dri",   DEBUG_DRI },
    { "dma",   DEBUG_DMA },
    { "san",   DEBUG_SANITY },
    { "sync",  DEBUG_SYNC },
    { "sleep", DEBUG_SLEEP },
    { NULL,    0 }
};

GLboolean
i810CreateContext( const __GLcontextModes *mesaVis,
                   __DRIcontextPrivate *driContextPriv,
                   void *sharedContextPrivate )
{
   GLcontext *ctx, *shareCtx;
   i810ContextPtr imesa;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   i810ScreenPrivate *i810Screen = (i810ScreenPrivate *)sPriv->private;
   I810SAREAPtr saPriv = (I810SAREAPtr)
      (((GLubyte *)sPriv->pSAREA) + i810Screen->sarea_priv_offset);
   struct dd_function_table functions;

   /* Allocate i810 context */
   imesa = (i810ContextPtr) CALLOC_STRUCT(i810_context_t);
   if (!imesa) {
      return GL_FALSE;
   }

   driContextPriv->driverPrivate = imesa;

   imesa->i810Screen = i810Screen;
   imesa->driScreen = sPriv;
   imesa->sarea = saPriv;
   imesa->glBuffer = NULL;

   /* Init default driver functions then plug in our I810-specific functions
    * (the texture functions are especially important)
    */
   _mesa_init_driver_functions( &functions );
   i810InitIoctlFuncs( &functions );
   i810InitTextureFuncs( &functions );


   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((i810ContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;
   imesa->glCtx = _mesa_create_context(mesaVis, shareCtx,
                                       &functions, (void*) imesa);
   if (!imesa->glCtx) {
      FREE(imesa);
      return GL_FALSE;
   }

   (void) memset( imesa->texture_heaps, 0, sizeof( imesa->texture_heaps ) );
   make_empty_list( & imesa->swapped );
   
   imesa->nr_heaps = 1;
   imesa->texture_heaps[0] = driCreateTextureHeap( 0, imesa,
	    i810Screen->textureSize,
	    12,
	    I810_NR_TEX_REGIONS,
	    imesa->sarea->texList,
	    (unsigned *) & imesa->sarea->texAge, /* XXX we shouldn't cast! */
	    & imesa->swapped,
	    sizeof( struct i810_texture_object_t ),
	    (destroy_texture_object_t *) i810DestroyTexObj );



   /* Set the maximum texture size small enough that we can guarentee
    * that both texture units can bind a maximal texture and have them
    * in memory at once.
    */



   ctx = imesa->glCtx;
   ctx->Const.MaxTextureUnits = 2;
   ctx->Const.MaxTextureImageUnits = 2;
   ctx->Const.MaxTextureCoordUnits = 2;


   /* FIXME: driCalcualteMaxTextureLevels assumes that mipmaps are tightly
    * FIXME: packed, but they're not in Intel graphics hardware.
    */
   driCalculateMaxTextureLevels( imesa->texture_heaps,
				 imesa->nr_heaps,
				 & ctx->Const,
				 4,
				 11, /* max 2D texture size is 2048x2048 */
				 0,  /* 3D textures unsupported */
				 0,  /* cube textures unsupported. */
				 0,  /* texture rectangles unsupported. */
				 12,
				 GL_FALSE );

   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 3.0;
   ctx->Const.MaxLineWidthAA = 3.0;
   ctx->Const.LineWidthGranularity = 1.0;

   ctx->Const.MinPointSize = 1.0;
   ctx->Const.MinPointSizeAA = 1.0;
   ctx->Const.MaxPointSize = 3.0;
   ctx->Const.MaxPointSizeAA = 3.0;
   ctx->Const.PointSizeGranularity = 1.0;

   ctx->Driver.GetBufferSize = i810BufferSize;
   ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;
   ctx->Driver.GetString = i810GetString;

   /* Who owns who?
    */
   ctx->DriverCtx = (void *) imesa;
   imesa->glCtx = ctx;

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, i810_pipeline );

   /* Configure swrast and T&L to match hardware characteristics:
    */
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );

   /* Dri stuff
    */
   imesa->hHWContext = driContextPriv->hHWContext;
   imesa->driFd = sPriv->fd;
   imesa->driHwLock = &sPriv->pSAREA->lock;

   imesa->stipple_in_hw = 1;
   imesa->RenderIndex = ~0;
   imesa->dirty = I810_UPLOAD_CTX|I810_UPLOAD_BUFFERS;
   imesa->upload_cliprects = GL_TRUE;

   imesa->CurrentTexObj[0] = 0;
   imesa->CurrentTexObj[1] = 0;

   _math_matrix_ctr( &imesa->ViewportMatrix );

   driInitExtensions( ctx, card_extensions, GL_TRUE );
   /* XXX these should really go right after _mesa_init_driver_functions() */
   i810InitStateFuncs( ctx );
   i810InitTriFuncs( ctx );
   i810InitSpanFuncs( ctx );
   i810InitVB( ctx );
   i810InitState( ctx );

#if DO_DEBUG
   I810_DEBUG  = driParseDebugString( getenv( "I810_DEBUG" ),
				      debug_control );
   I810_DEBUG |= driParseDebugString( getenv( "INTEL_DEBUG" ),
				      debug_control );
#endif

   return GL_TRUE;
}

void
i810DestroyContext(__DRIcontextPrivate *driContextPriv)
{
   i810ContextPtr imesa = (i810ContextPtr) driContextPriv->driverPrivate;

   assert(imesa); /* should never be null */
   if (imesa) {
      GLboolean   release_texture_heaps;


      release_texture_heaps = (imesa->glCtx->Shared->RefCount == 1);
      _swsetup_DestroyContext( imesa->glCtx );
      _tnl_DestroyContext( imesa->glCtx );
      _ac_DestroyContext( imesa->glCtx );
      _swrast_DestroyContext( imesa->glCtx );

      i810FreeVB( imesa->glCtx );

      /* free the Mesa context */
      imesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(imesa->glCtx);
      if ( release_texture_heaps ) {
 	 /* This share group is about to go away, free our private
          * texture object data.
          */
         int i;

         for ( i = 0 ; i < imesa->nr_heaps ; i++ ) {
	    driDestroyTextureHeap( imesa->texture_heaps[ i ] );
	    imesa->texture_heaps[ i ] = NULL;
         }

	 assert( is_empty_list( & imesa->swapped ) );
      }

      FREE(imesa);
   }
}


void i810XMesaSetFrontClipRects( i810ContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;

   imesa->numClipRects = dPriv->numClipRects;
   imesa->pClipRects = dPriv->pClipRects;
   imesa->drawX = dPriv->x;
   imesa->drawY = dPriv->y;

   i810EmitDrawingRectangle( imesa );
   imesa->upload_cliprects = GL_TRUE;
}


void i810XMesaSetBackClipRects( i810ContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;

   if (imesa->sarea->pf_enabled == 0 && dPriv->numBackClipRects == 0)
   {
      imesa->numClipRects = dPriv->numClipRects;
      imesa->pClipRects = dPriv->pClipRects;
      imesa->drawX = dPriv->x;
      imesa->drawY = dPriv->y;
   } else {
      imesa->numClipRects = dPriv->numBackClipRects;
      imesa->pClipRects = dPriv->pBackClipRects;
      imesa->drawX = dPriv->backX;
      imesa->drawY = dPriv->backY;
   }

   i810EmitDrawingRectangle( imesa );
   imesa->upload_cliprects = GL_TRUE;
}


static void i810XMesaWindowMoved( i810ContextPtr imesa )
{
   switch (imesa->glCtx->Color._DrawDestMask[0]) {
   case DD_FRONT_LEFT_BIT:
      i810XMesaSetFrontClipRects( imesa );
      break;
   case DD_BACK_LEFT_BIT:
      i810XMesaSetBackClipRects( imesa );
      break;
   default:
      /* glDrawBuffer(GL_NONE or GL_FRONT_AND_BACK): software fallback */
      i810XMesaSetFrontClipRects( imesa );
   }
}


GLboolean
i810UnbindContext(__DRIcontextPrivate *driContextPriv)
{
   i810ContextPtr imesa = (i810ContextPtr) driContextPriv->driverPrivate;
   if (imesa) {
      imesa->dirty = I810_UPLOAD_CTX|I810_UPLOAD_BUFFERS;
      if (imesa->CurrentTexObj[0]) imesa->dirty |= I810_UPLOAD_TEX0;
      if (imesa->CurrentTexObj[1]) imesa->dirty |= I810_UPLOAD_TEX1;
   }

   return GL_TRUE;
}


GLboolean
i810MakeCurrent(__DRIcontextPrivate *driContextPriv,
                __DRIdrawablePrivate *driDrawPriv,
                __DRIdrawablePrivate *driReadPriv)
{
   if (driContextPriv) {
      i810ContextPtr imesa = (i810ContextPtr) driContextPriv->driverPrivate;

      /* Shouldn't the readbuffer be stored also?
       */
      imesa->driDrawable = driDrawPriv;

      _mesa_make_current2(imesa->glCtx,
                          (GLframebuffer *) driDrawPriv->driverPrivate,
                          (GLframebuffer *) driReadPriv->driverPrivate);

      /* Are these necessary?
       */
      i810XMesaWindowMoved( imesa );
   }
   else {
      _mesa_make_current(0,0);
   }

   return GL_TRUE;
}

static void
i810UpdatePageFlipping( i810ContextPtr imesa )
{
   GLcontext *ctx = imesa->glCtx;
   int front = 0;

   switch (ctx->Color._DrawDestMask[0]) {
   case DD_FRONT_LEFT_BIT:
      front = 1;
      break;
   case DD_BACK_LEFT_BIT:
      front = 0;
      break;
   default:
      return;
   }

   if ( imesa->sarea->pf_current_page == 1 ) 
     front ^= 1;
   
   if (front) {
      imesa->BufferSetup[I810_DESTREG_DI1] = imesa->i810Screen->fbOffset | imesa->i810Screen->backPitchBits;
      imesa->drawMap = (char *)imesa->driScreen->pFB;
      imesa->readMap = (char *)imesa->driScreen->pFB;
   } else {
      imesa->BufferSetup[I810_DESTREG_DI1] = imesa->i810Screen->backOffset | imesa->i810Screen->backPitchBits;
      imesa->drawMap = imesa->i810Screen->back.map;
      imesa->readMap = imesa->i810Screen->back.map;
   }

   imesa->dirty |= I810_UPLOAD_BUFFERS;
}

void i810GetLock( i810ContextPtr imesa, GLuint flags )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   __DRIscreenPrivate *sPriv = imesa->driScreen;
   I810SAREAPtr sarea = imesa->sarea;
   int me = imesa->hHWContext;
   unsigned i;

   drmGetLock(imesa->driFd, imesa->hHWContext, flags);

   /* If the window moved, may need to set a new cliprect now.
    *
    * NOTE: This releases and regains the hw lock, so all state
    * checking must be done *after* this call:
    */
   DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);


   /* If we lost context, need to dump all registers to hardware.
    * Note that we don't care about 2d contexts, even if they perform
    * accelerated commands, so the DRI locking in the X server is even
    * more broken than usual.
    */
   if (sarea->ctxOwner != me) {
      imesa->upload_cliprects = GL_TRUE;
      imesa->dirty = I810_UPLOAD_CTX|I810_UPLOAD_BUFFERS;
      if (imesa->CurrentTexObj[0]) imesa->dirty |= I810_UPLOAD_TEX0;
      if (imesa->CurrentTexObj[1]) imesa->dirty |= I810_UPLOAD_TEX1;
      sarea->ctxOwner = me;
   }

   /* Shared texture managment - if another client has played with
    * texture space, figure out which if any of our textures have been
    * ejected, and update our global LRU.
    */ 
   for ( i = 0 ; i < imesa->nr_heaps ; i++ ) {
      DRI_AGE_TEXTURES( imesa->texture_heaps[ i ] );
   }

   if (imesa->lastStamp != dPriv->lastStamp) {
      i810UpdatePageFlipping( imesa );
      i810XMesaWindowMoved( imesa );
      imesa->lastStamp = dPriv->lastStamp;
   }
}


void
i810SwapBuffers( __DRIdrawablePrivate *dPriv )
{
   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      i810ContextPtr imesa;
      GLcontext *ctx;
      imesa = (i810ContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = imesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */
         if ( imesa->sarea->pf_active ) {
            i810PageFlip( dPriv );
         } else {
            i810CopyBuffer( dPriv );
         }
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "i810SwapBuffers: drawable has no context!\n");
   }
}

