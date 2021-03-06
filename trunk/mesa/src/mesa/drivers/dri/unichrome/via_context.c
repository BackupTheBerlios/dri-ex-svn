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

/**
 * \file via_context.c
 * 
 * \author John Sheng (presumably of either VIA Technologies or S3 Graphics)
 * \author Others at VIA Technologies?
 * \author Others at S3 Graphics?
 */

#include "glheader.h"
#include "context.h"
#include "matrix.h"
#include "simple_list.h"
#include "extensions.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "array_cache/acache.h"

#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "via_screen.h"
#include "via_dri.h"

#include "via_state.h"
#include "via_tex.h"
#include "via_span.h"
#include "via_tris.h"
#include "via_vb.h"
#include "via_ioctl.h"
#include "via_fb.h"
#include "via_regs.h"

#include <stdio.h>
#include "macros.h"

#define DRIVER_DATE	"20041215"

#include "vblank.h"
#include "utils.h"

#ifdef DEBUG
GLuint VIA_DEBUG = 0;
#endif


/**
 * Return various strings for \c glGetString.
 *
 * \sa glGetString
 */
static const GLubyte *viaGetString(GLcontext *ctx, GLenum name)
{
   static char buffer[128];
   unsigned   offset;


   switch (name) {
   case GL_VENDOR:
      return (GLubyte *)"VIA Technology";

   case GL_RENDERER: {
      static const char * const chipset_names[] = {
	 "UniChrome",
	 "CastleRock (CLE266)",
	 "UniChrome (KM400)",
	 "UniChrome (K8M800)",
	 "UniChrome (PM8x0/CN400)",
      };
      const viaContext * const via = VIA_CONTEXT(ctx);
      const unsigned id = via->viaScreen->deviceID;

      offset = driGetRendererString( buffer, 
				     chipset_names[(id > VIA_PM800) ? 0 : id],
				     DRIVER_DATE, 0 );
      return (GLubyte *)buffer;
   }

   default:
      return NULL;
   }
}


/**
 * Calculate a width that satisfies the hardware's alignment requirements.
 * On the Unichrome hardware, each scanline must be aligned to a multiple of
 * 16 pixels.
 *
 * \param width  Minimum buffer width, in pixels.
 * 
 * \returns A pixel width that meets the alignment requirements.
 */
static __inline__ unsigned
buffer_align( unsigned width )
{
    return (width + 0x0f) & ~0x0f;
}


/**
 * Calculate the framebuffer parameters for all buffers (front, back, depth,
 * and stencil) associated with the specified context.
 * 
 * \warning
 * This function also calls \c AllocateBuffer to actually allocate the
 * buffers.
 * 
 * \sa AllocateBuffer
 */
static GLboolean
calculate_buffer_parameters( viaContextPtr vmesa )
{
   const unsigned shift = vmesa->viaScreen->bitsPerPixel / 16;
   const unsigned extra = 32;
   unsigned w;
   unsigned h;

   /* Allocate front-buffer */
   if (vmesa->drawType == GLX_PBUFFER_BIT) {
      w = vmesa->driDrawable->w;
      h = vmesa->driDrawable->h;

      vmesa->front.bpp = vmesa->viaScreen->bitsPerPixel;
      vmesa->front.pitch = buffer_align( w ) << shift;
      vmesa->front.size = vmesa->front.pitch * h;

      if (vmesa->front.map)
	 via_free_draw_buffer(vmesa, &vmesa->front);
      if (!via_alloc_draw_buffer(vmesa, &vmesa->front))
	 return GL_FALSE;

   }
   else { 
      w = vmesa->viaScreen->width;
      h = vmesa->viaScreen->height;

      vmesa->front.bpp = vmesa->viaScreen->bitsPerPixel;
      vmesa->front.pitch = buffer_align( w ) << shift;
      vmesa->front.size = vmesa->front.pitch * h;
      vmesa->front.offset = 0;
      vmesa->front.map = (char *) vmesa->driScreen->pFB;
   }


   /* Allocate back-buffer */
   if (vmesa->hasBack) {
      vmesa->back.bpp = vmesa->viaScreen->bitsPerPixel;
      vmesa->back.pitch = (buffer_align( vmesa->driDrawable->w ) << shift) + extra;
      vmesa->back.size = vmesa->back.pitch * vmesa->driDrawable->h;
      if (vmesa->back.map)
	 via_free_draw_buffer(vmesa, &vmesa->back);
      if (!via_alloc_draw_buffer(vmesa, &vmesa->back))
	 return GL_FALSE;
   }
   else {
      /* KW: mem leak if vmesa->hasBack ever changes:
       */
      (void) memset( &vmesa->back, 0, sizeof( vmesa->back ) );
   }


   /* Allocate depth-buffer */
   if ( vmesa->hasStencil || vmesa->hasDepth ) {
      vmesa->depth.bpp = vmesa->depthBits;
      if (vmesa->depth.bpp == 24)
	 vmesa->depth.bpp = 32;

      vmesa->depth.pitch = (buffer_align( vmesa->driDrawable->w ) * (vmesa->depth.bpp/8)) + extra;
      vmesa->depth.size = vmesa->depth.pitch * vmesa->driDrawable->h;

      if (vmesa->depth.map)
	 via_free_draw_buffer(vmesa, &vmesa->depth);
      if (!via_alloc_draw_buffer(vmesa, &vmesa->depth)) {
	 return GL_FALSE;
      }
   }
   else {
      /* KW: mem leak if vmesa->hasStencil/hasDepth ever changes:
       */
      (void) memset( & vmesa->depth, 0, sizeof( vmesa->depth ) );
   }

   /*=* John Sheng [2003.5.31] flip *=*/
   if( vmesa->viaScreen->width == vmesa->driDrawable->w && 
       vmesa->viaScreen->height == vmesa->driDrawable->h ) {
#define ALLOW_EXPERIMENTAL_PAGEFLIP 0
#if ALLOW_EXPERIMENTAL_PAGEFLIP
      vmesa->doPageFlip = GL_TRUE;
#else
      vmesa->doPageFlip = GL_FALSE;
#endif
      /* vmesa->currentPage = 0; */
      assert(vmesa->back.pitch == vmesa->front.pitch);
   }
   else
      vmesa->doPageFlip = GL_FALSE;

   return GL_TRUE;
}


void viaReAllocateBuffers(GLframebuffer *drawbuffer)
{
    GET_CURRENT_CONTEXT(ctx);
    viaContextPtr vmesa = VIA_CONTEXT(ctx);

    _swrast_alloc_buffers( drawbuffer );
    calculate_buffer_parameters( vmesa );
}

static void viaBufferSize(GLframebuffer *buffer, GLuint *width, GLuint *height)
{
    GET_CURRENT_CONTEXT(ctx);
    viaContextPtr vmesa = VIA_CONTEXT(ctx);       
    *width = vmesa->driDrawable->w;
    *height = vmesa->driDrawable->h;
}

/* Extension strings exported by the Unichrome driver.
 */
static const char * const card_extensions[] = 
{
   "GL_ARB_multitexture",
   "GL_ARB_point_parameters",
   "GL_ARB_texture_env_add",
   "GL_ARB_texture_env_combine",
   "GL_ARB_texture_env_dot3",
   "GL_ARB_texture_mirrored_repeat",
   "GL_EXT_stencil_wrap",
   "GL_EXT_texture_env_combine",
   "GL_EXT_texture_env_dot3",
   "GL_EXT_texture_lod_bias",
   "GL_NV_blend_square",
   NULL
};

extern const struct tnl_pipeline_stage _via_fastrender_stage;
extern const struct tnl_pipeline_stage _via_render_stage;

static const struct tnl_pipeline_stage *via_pipeline[] = {
    &_tnl_vertex_transform_stage,
    &_tnl_normal_transform_stage,
    &_tnl_lighting_stage,
    &_tnl_fog_coordinate_stage,
    &_tnl_texgen_stage,
    &_tnl_texture_transform_stage,
    /* REMOVE: point attenuation stage */
    &_via_fastrender_stage,     /* ADD: unclipped rastersetup-to-dma */
    &_tnl_render_stage,
    0,
};


static GLboolean
AllocateDmaBuffer(const GLvisual *visual, viaContextPtr vmesa)
{
    if (vmesa->dma)
        via_free_dma_buffer(vmesa);
    
    if (!via_alloc_dma_buffer(vmesa))
        return GL_FALSE;

    vmesa->dmaLow = 0;
    vmesa->dmaCliprectAddr = 0;
    return GL_TRUE;
}

static void
FreeBuffer(viaContextPtr vmesa)
{
    if (vmesa->front.map)
	via_free_draw_buffer(vmesa, &vmesa->front);

    if (vmesa->back.map)
        via_free_draw_buffer(vmesa, &vmesa->back);

    if (vmesa->depth.map)
        via_free_draw_buffer(vmesa, &vmesa->depth);

    if (vmesa->dma)
        via_free_dma_buffer(vmesa);
}

static int
get_ust_nop( int64_t * ust )
{
   *ust = 1;
   return 0;
}

GLboolean
viaCreateContext(const __GLcontextModes *mesaVis,
                 __DRIcontextPrivate *driContextPriv,
                 void *sharedContextPrivate)
{
    GLcontext *ctx, *shareCtx;
    viaContextPtr vmesa;
    __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
    viaScreenPrivate *viaScreen = (viaScreenPrivate *)sPriv->private;
    drm_via_sarea_t *saPriv = (drm_via_sarea_t *)
        (((GLubyte *)sPriv->pSAREA) + viaScreen->sareaPrivOffset);
    struct dd_function_table functions;

    /* Allocate via context */
    vmesa = (viaContextPtr) CALLOC_STRUCT(via_context_t);
    if (!vmesa) {
        return GL_FALSE;
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    

    /* Parse configuration files.
     */
    driParseConfigFiles (&vmesa->optionCache, &viaScreen->optionCache,
			 sPriv->myNum, "via");

    /* pick back buffer */
    vmesa->hasBack = mesaVis->doubleBufferMode;

    switch(mesaVis->depthBits) {
    case 0:			
       vmesa->hasDepth = GL_FALSE;
       vmesa->depthBits = 0; 
       vmesa->depth_max = 1.0;
       break;
    case 16:
       vmesa->hasDepth = GL_TRUE;
       vmesa->depthBits = mesaVis->depthBits;
       vmesa->have_hw_stencil = GL_FALSE;
       vmesa->depth_max = (GLfloat)0xffff;
       vmesa->depth_clear_mask = 0xf << 28;
       vmesa->ClearDepth = 0xffff;
       vmesa->polygon_offset_scale = 1.0 / vmesa->depth_max;
       break;
    case 24:
       vmesa->hasDepth = GL_TRUE;
       vmesa->depthBits = mesaVis->depthBits;
       vmesa->depth_max = (GLfloat) 0xffffff;
       vmesa->depth_clear_mask = 0xe << 28;
       vmesa->ClearDepth = 0xffffff00;

       assert(mesaVis->haveStencilBuffer);
       assert(mesaVis->stencilBits == 8);

       vmesa->have_hw_stencil = GL_TRUE;
       vmesa->stencilBits = mesaVis->stencilBits;
       vmesa->stencil_clear_mask = 0x1 << 28;
       vmesa->polygon_offset_scale = 2.0 / vmesa->depth_max;
       break;
    case 32:
       vmesa->hasDepth = GL_TRUE;
       vmesa->depthBits = mesaVis->depthBits;
       assert(!mesaVis->haveStencilBuffer);
       vmesa->have_hw_stencil = GL_FALSE;
       vmesa->depth_max = (GLfloat)0xffffffff;
       vmesa->depth_clear_mask = 0;
       vmesa->ClearDepth = 0xffffffff;
       vmesa->depth_clear_mask = 0xf << 28;
       vmesa->polygon_offset_scale = 2.0 / vmesa->depth_max;
       break;
    default:
       assert(0); 
       break;
    }


    _mesa_init_driver_functions(&functions);
    viaInitTextureFuncs(&functions);

    /* Allocate the Mesa context */
    if (sharedContextPrivate)
        shareCtx = ((viaContextPtr) sharedContextPrivate)->glCtx;
    else
        shareCtx = NULL;

    vmesa->glCtx = _mesa_create_context(mesaVis, shareCtx, &functions, (void*) vmesa);
    
    vmesa->shareCtx = shareCtx;
    
    if (!vmesa->glCtx) {
        FREE(vmesa);
        return GL_FALSE;
    }
    driContextPriv->driverPrivate = vmesa;

    ctx = vmesa->glCtx;
    
    ctx->Const.MaxTextureLevels = 10;    
    ctx->Const.MaxTextureUnits = 2;
    ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
    ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

    ctx->Const.MinLineWidth = 1.0;
    ctx->Const.MinLineWidthAA = 1.0;
    ctx->Const.MaxLineWidth = 1.0;
    ctx->Const.MaxLineWidthAA = 1.0;
    ctx->Const.LineWidthGranularity = 1.0;

    ctx->Const.MinPointSize = 1.0;
    ctx->Const.MinPointSizeAA = 1.0;
    ctx->Const.MaxPointSize = 1.0;
    ctx->Const.MaxPointSizeAA = 1.0;
    ctx->Const.PointSizeGranularity = 1.0;

    ctx->Driver.GetBufferSize = viaBufferSize;
/*    ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;  *//* FIXME ?? */
    ctx->Driver.GetString = viaGetString;

    ctx->DriverCtx = (void *)vmesa;
    vmesa->glCtx = ctx;

    /* Initialize the software rasterizer and helper modules.
     */
    _swrast_CreateContext(ctx);
    _ac_CreateContext(ctx);
    _tnl_CreateContext(ctx);
    _swsetup_CreateContext(ctx);

    /* Install the customized pipeline:
     */
    _tnl_destroy_pipeline(ctx);
    _tnl_install_pipeline(ctx, via_pipeline);

    /* Configure swrast and T&L to match hardware characteristics:
     */
    _swrast_allow_pixel_fog(ctx, GL_FALSE);
    _swrast_allow_vertex_fog(ctx, GL_TRUE);
    _tnl_allow_pixel_fog(ctx, GL_FALSE);
    _tnl_allow_vertex_fog(ctx, GL_TRUE);

/*     vmesa->display = dpy; */
    vmesa->display = sPriv->display;
    
    vmesa->hHWContext = driContextPriv->hHWContext;
    vmesa->driFd = sPriv->fd;
    vmesa->driHwLock = &sPriv->pSAREA->lock;

    vmesa->viaScreen = viaScreen;
    vmesa->driScreen = sPriv;
    vmesa->sarea = saPriv;
    vmesa->glBuffer = NULL;

    vmesa->texHeap = mmInit(0, viaScreen->textureSize);
    vmesa->renderIndex = ~0;

    make_empty_list(&vmesa->TexObjList);
    make_empty_list(&vmesa->SwappedOut);

    vmesa->CurrentTexObj[0] = 0;
    vmesa->CurrentTexObj[1] = 0;
    
    _math_matrix_ctr(&vmesa->ViewportMatrix);

    /* Do this early, before VIA_FLUSH_DMA can be called:
     */
    if (!AllocateDmaBuffer(mesaVis, vmesa)) {
	fprintf(stderr ,"AllocateDmaBuffer fail\n");
	FreeBuffer(vmesa);
        FREE(vmesa);
        return GL_FALSE;
    }

    driInitExtensions( ctx, card_extensions, GL_TRUE );
    viaInitStateFuncs(ctx);
    viaInitTextures(ctx);
    viaInitTriFuncs(ctx);
    viaInitSpanFuncs(ctx);
    viaInitIoctlFuncs(ctx);
    viaInitVB(ctx);
    viaInitState(ctx);
        
#ifdef DEBUG
    if (getenv("VIA_DEBUG"))
	VIA_DEBUG = 1;
    else
	VIA_DEBUG = 0;	
#endif	

    if (getenv("VIA_NO_RAST"))
       FALLBACK(vmesa, VIA_FALLBACK_USER_DISABLE, 1);

	

    /* I don't understand why this isn't working:
     */
    vmesa->vblank_flags =
       vmesa->viaScreen->irqEnabled ?
        driGetDefaultVBlankFlags(&vmesa->optionCache) : VBLANK_FLAG_NO_IRQ;

    /* Hack this up in its place:
     */
    vmesa->vblank_flags = getenv("VIA_VSYNC") ? VBLANK_FLAG_SYNC : VBLANK_FLAG_NO_IRQ;

    
    vmesa->get_ust = (PFNGLXGETUSTPROC) glXGetProcAddress( (const GLubyte *) "__glXGetUST" );
    if ( vmesa->get_ust == NULL ) {
       vmesa->get_ust = get_ust_nop;
    }
    vmesa->get_ust( &vmesa->swap_ust );


    vmesa->regMMIOBase = (GLuint *)((GLuint)viaScreen->reg);
    vmesa->pnGEMode = (GLuint *)((GLuint)viaScreen->reg + 0x4);
    vmesa->regEngineStatus = (GLuint *)((GLuint)viaScreen->reg + 0x400);
    vmesa->regTranSet = (GLuint *)((GLuint)viaScreen->reg + 0x43C);
    vmesa->regTranSpace = (GLuint *)((GLuint)viaScreen->reg + 0x440);
    vmesa->agpBase = viaScreen->agpBase;
    if (VIA_DEBUG) {
	fprintf(stderr, "regEngineStatus = %x\n", *vmesa->regEngineStatus);
    }
    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    return GL_TRUE;
}

void
viaDestroyContext(__DRIcontextPrivate *driContextPriv)
{
    viaContextPtr vmesa = (viaContextPtr)driContextPriv->driverPrivate;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    assert(vmesa); /* should never be null */

    if (vmesa) {
	/*=* John Sheng [2003.5.31]  agp tex *=*/
        WAIT_IDLE(vmesa);
	if(VIA_DEBUG) fprintf(stderr, "agpFullCount = %d\n", vmesa->agpFullCount);    
	
	_swsetup_DestroyContext(vmesa->glCtx);
        _tnl_DestroyContext(vmesa->glCtx);
        _ac_DestroyContext(vmesa->glCtx);
        _swrast_DestroyContext(vmesa->glCtx);
        viaFreeVB(vmesa->glCtx);
	FreeBuffer(vmesa);
        /* free the Mesa context */
	_mesa_destroy_context(vmesa->glCtx);
	vmesa->glCtx->DriverCtx = NULL;
        FREE(vmesa);
    }
    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}


void viaXMesaWindowMoved(viaContextPtr vmesa)
{
   __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
   GLuint bytePerPixel = vmesa->viaScreen->bitsPerPixel >> 3;

   if (!dPriv)
      return;

   switch (vmesa->glCtx->Color._DrawDestMask[0]) {
   case DD_FRONT_LEFT_BIT: 
      if (dPriv->numBackClipRects == 0) {
	 vmesa->numClipRects = dPriv->numClipRects;
	 vmesa->pClipRects = dPriv->pClipRects;
      } 
      else {
	 vmesa->numClipRects = dPriv->numBackClipRects;
	 vmesa->pClipRects = dPriv->pBackClipRects;
      }
      break;
   case DD_BACK_LEFT_BIT:
      vmesa->numClipRects = dPriv->numClipRects;
      vmesa->pClipRects = dPriv->pClipRects;
      break;
   default:
      vmesa->numClipRects = 0;
      break;
   }

   if (vmesa->drawW != dPriv->w ||
       vmesa->drawH != dPriv->h) 
      calculate_buffer_parameters( vmesa );

   vmesa->drawXoff = (GLuint)(((dPriv->x * bytePerPixel) & 0x1f) / bytePerPixel);  
   vmesa->drawX = dPriv->x - vmesa->drawXoff;
   vmesa->drawY = dPriv->y;
   vmesa->drawW = dPriv->w;
   vmesa->drawH = dPriv->h;

   vmesa->front.orig = (vmesa->front.offset + 
			vmesa->drawY * vmesa->front.pitch + 
			vmesa->drawX * bytePerPixel);

   vmesa->front.origMap = (vmesa->front.map + 
			   vmesa->drawY * vmesa->front.pitch + 
			   vmesa->drawX * bytePerPixel);

   vmesa->back.orig = vmesa->back.offset;
   vmesa->depth.orig = vmesa->depth.offset;   
   vmesa->back.origMap = vmesa->back.map;
   vmesa->depth.origMap = vmesa->depth.map;

   viaCalcViewport(vmesa->glCtx);
}

GLboolean
viaUnbindContext(__DRIcontextPrivate *driContextPriv)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    return GL_TRUE;
}

GLboolean
viaMakeCurrent(__DRIcontextPrivate *driContextPriv,
               __DRIdrawablePrivate *driDrawPriv,
               __DRIdrawablePrivate *driReadPriv)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
  
    if (VIA_DEBUG) {
	fprintf(stderr, "driContextPriv = %08x\n", (GLuint)driContextPriv);
	fprintf(stderr, "driContextPriv = %08x\n", (GLuint)driDrawPriv);    
	fprintf(stderr, "driContextPriv = %08x\n", (GLuint)driReadPriv);
    }	

    if (driContextPriv) {
        viaContextPtr vmesa = (viaContextPtr)driContextPriv->driverPrivate;
	GLcontext *ctx = vmesa->glCtx;

	if (VIA_DEBUG) fprintf(stderr, "viaMakeCurrent: w = %d\n", vmesa->driDrawable->w);

	if ( vmesa->driDrawable != driDrawPriv ) {
	   driDrawableInitVBlank( driDrawPriv, vmesa->vblank_flags );
	   vmesa->driDrawable = driDrawPriv;
	   if ( ! calculate_buffer_parameters( vmesa ) ) {
	      return GL_FALSE;
	   }
	   ctx->Driver.DrawBuffer( ctx, ctx->Color.DrawBuffer[0] );
	}

        _mesa_make_current2(vmesa->glCtx,
                            (GLframebuffer *)driDrawPriv->driverPrivate,
                            (GLframebuffer *)driReadPriv->driverPrivate);
	if (VIA_DEBUG) fprintf(stderr, "Context %d MakeCurrent\n", vmesa->hHWContext);
	
	/* These are probably needed only the first time a context is
	 * made current:
	 */
	viaXMesaWindowMoved(vmesa);
	ctx->Driver.Scissor(ctx, 
			    ctx->Scissor.X,
			    ctx->Scissor.Y,
			    ctx->Scissor.Width,
			    ctx->Scissor.Height);

    }
    else {
        _mesa_make_current(0,0);
    }
        
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
    return GL_TRUE;
}

void viaGetLock(viaContextPtr vmesa, GLuint flags)
{
    __DRIdrawablePrivate *dPriv = vmesa->driDrawable;
    __DRIscreenPrivate *sPriv = vmesa->driScreen;

    drmGetLock(vmesa->driFd, vmesa->hHWContext, flags);

    DRI_VALIDATE_DRAWABLE_INFO( sPriv, dPriv );

    if (vmesa->sarea->ctxOwner != vmesa->hHWContext) {
       vmesa->sarea->ctxOwner = vmesa->hHWContext;
       vmesa->newEmitState = ~0;
    }

    if (vmesa->lastStamp != dPriv->lastStamp) {
       viaXMesaWindowMoved(vmesa);
       vmesa->lastStamp = dPriv->lastStamp;
    }
}


void
viaSwapBuffers(__DRIdrawablePrivate *drawablePrivate)
{
    __DRIdrawablePrivate *dPriv = (__DRIdrawablePrivate *)drawablePrivate;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);	
    if (dPriv && dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
        viaContextPtr vmesa;
        GLcontext *ctx;
	
        vmesa = (viaContextPtr)dPriv->driContextPriv->driverPrivate;
        ctx = vmesa->glCtx;
        if (ctx->Visual.doubleBufferMode) {
            _mesa_notifySwapBuffers(ctx);
            if (vmesa->doPageFlip) {
                viaPageFlip(dPriv);
            }
            else {
                viaCopyBuffer(dPriv);
            }
        }
	else
	    VIA_FLUSH_DMA(vmesa);
    }
    else {
        _mesa_problem(NULL, "viaSwapBuffers: drawable has no context!\n");
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);	
}
