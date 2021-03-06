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


#ifdef GLX_DIRECT_RENDERING

#include <X11/Xlibint.h>
#include <stdio.h>

#include "savagecontext.h"
#include "context.h"
#include "matrix.h"

#include "simple_list.h"

#include "utils.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "array_cache/acache.h"

#include "tnl/t_pipeline.h"

#include "drivers/common/driverfuncs.h"

#include "savagedd.h"
#include "savagestate.h"
#include "savagetex.h"
#include "savagespan.h"
#include "savagetris.h"
#include "savageioctl.h"
#include "savage_bci.h"

#include "savage_dri.h"

#include "savagedma.h"

#include "xmlpool.h"

/* Configuration
 */
PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
    DRI_CONF_SECTION_QUALITY
        DRI_CONF_TEXTURE_DEPTH(DRI_CONF_TEXTURE_DEPTH_FB)
        DRI_CONF_COLOR_REDUCTION(DRI_CONF_COLOR_REDUCTION_DITHER)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_PERFORMANCE
        DRI_CONF_MAX_TEXTURE_UNITS(2,1,2)
    DRI_CONF_SECTION_END
    DRI_CONF_SECTION_DEBUG
        DRI_CONF_NO_RAST(false)
    DRI_CONF_SECTION_END
DRI_CONF_END;
static const GLuint __driNConfigOptions = 4;

#ifdef USE_NEW_INTERFACE
static PFNGLXCREATECONTEXTMODES create_context_modes = NULL;
#endif /* USE_NEW_INTERFACE */

static const struct dri_debug_control debug_control[] =
{
    { "fall",  DEBUG_FALLBACKS },
    { "api",   DEBUG_VERBOSE_API },
    { "lru",   DEBUG_VERBOSE_LRU },
    { NULL,    0 }
};
#ifndef SAVAGE_DEBUG
int SAVAGE_DEBUG = 0;
#endif


/*For time caculating test*/
#if defined(DEBUG_TIME) && DEBUG_TIME
struct timeval tv_s,tv_f;
unsigned long time_sum=0;
struct timeval tv_s1,tv_f1;
#endif

static const char *const card_extensions[] =
{
    "GL_ARB_multitexture",
    "GL_EXT_texture_lod_bias",
    "GL_EXT_texture_env_add",
    NULL
};

extern const struct tnl_pipeline_stage _savage_texnorm_stage;

static const struct tnl_pipeline_stage *savage_pipeline[] = {

   &_tnl_vertex_transform_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_savage_texnorm_stage,
   &_tnl_render_stage,
   0,
};


/* this is first function called in dirver*/

static GLboolean
savageInitDriver(__DRIscreenPrivate *sPriv)
{
  savageScreenPrivate *savageScreen;
  SAVAGEDRIPtr         gDRIPriv = (SAVAGEDRIPtr)sPriv->pDevPriv;


   /* Allocate the private area */
   savageScreen = (savageScreenPrivate *)Xmalloc(sizeof(savageScreenPrivate));
   if (!savageScreen)
      return GL_FALSE;

   savageScreen->driScrnPriv = sPriv;
   sPriv->private = (void *)savageScreen;

   savageScreen->chipset=gDRIPriv->chipset; 
   savageScreen->width=gDRIPriv->width;
   savageScreen->height=gDRIPriv->height;
   savageScreen->mem=gDRIPriv->mem;
   savageScreen->cpp=gDRIPriv->cpp;
   savageScreen->zpp=gDRIPriv->zpp;
   savageScreen->frontPitch=gDRIPriv->frontPitch;
   savageScreen->frontOffset=gDRIPriv->frontOffset;
   savageScreen->frontBitmapDesc = gDRIPriv->frontBitmapDesc;
   
   if (gDRIPriv->cpp == 4) 
       savageScreen->frontFormat = DV_PF_8888;
   else
       savageScreen->frontFormat = DV_PF_565;

   savageScreen->backOffset = gDRIPriv->backOffset; 
   savageScreen->backBitmapDesc = gDRIPriv->backBitmapDesc; 
   savageScreen->depthOffset=gDRIPriv->depthOffset;
   savageScreen->depthBitmapDesc = gDRIPriv->depthBitmapDesc; 
#if 0   
   savageScreen->backPitch = gDRIPriv->auxPitch;
   savageScreen->backPitchBits = gDRIPriv->auxPitchBits;
#endif   
   savageScreen->textureOffset[SAVAGE_CARD_HEAP] = 
                                   gDRIPriv->textureOffset;
   savageScreen->textureSize[SAVAGE_CARD_HEAP] = 
                                   gDRIPriv->textureSize;
   savageScreen->logTextureGranularity[SAVAGE_CARD_HEAP] = 
                                   gDRIPriv->logTextureGranularity;

   savageScreen->textureOffset[SAVAGE_AGP_HEAP] = 
                                   gDRIPriv->agpTextures.handle;
   savageScreen->textureSize[SAVAGE_AGP_HEAP] = 
                                   gDRIPriv->agpTextures.size;
   savageScreen->logTextureGranularity[SAVAGE_AGP_HEAP] =
                                   gDRIPriv->logAgpTextureGranularity;
   
   savageScreen->back.handle = gDRIPriv->backbuffer;
   savageScreen->back.size = gDRIPriv->backbufferSize;
   savageScreen->back.map = 
       (drmAddress)(((unsigned int)sPriv->pFB)+gDRIPriv->backOffset);
   
   savageScreen->depth.handle = gDRIPriv->depthbuffer;
   savageScreen->depth.size = gDRIPriv->depthbufferSize;

   savageScreen->depth.map = 
              (drmAddress)(((unsigned int)sPriv->pFB)+gDRIPriv->depthOffset);
   
   savageScreen->sarea_priv_offset = gDRIPriv->sarea_priv_offset;

   savageScreen->texVirtual[SAVAGE_CARD_HEAP] = 
             (drmAddress)(((unsigned int)sPriv->pFB)+gDRIPriv->textureOffset);

   if (drmMap(sPriv->fd, 
	      gDRIPriv->registers.handle, 
	      gDRIPriv->registers.size, 
	      (drmAddress *)&(gDRIPriv->registers.map)) != 0) 
   {
      Xfree(savageScreen);
      sPriv->private = NULL;
      return GL_FALSE;
   }
   
   if (drmMap(sPriv->fd, 
	      gDRIPriv->agpTextures.handle, 
	      gDRIPriv->agpTextures.size, 
	      (drmAddress *)&(gDRIPriv->agpTextures.map)) != 0) 
   {
      Xfree(savageScreen);
      sPriv->private = NULL;
      return GL_FALSE;
   }

/* agp texture*/
   savageScreen->texVirtual[SAVAGE_AGP_HEAP] = 
                        (drmAddress)(gDRIPriv->agpTextures.map);

   gDRIPriv->BCIcmdBuf.map = (drmAddress *)
                           ((unsigned int)gDRIPriv->registers.map+0x00010000);

   savageScreen->aperture.handle = gDRIPriv->aperture.handle;
   savageScreen->aperture.size   = gDRIPriv->aperture.size;
   if (drmMap(sPriv->fd, 
	      savageScreen->aperture.handle, 
	      savageScreen->aperture.size, 
	      (drmAddress *)&savageScreen->aperture.map) != 0) 
   {
      Xfree(savageScreen);
      sPriv->private = NULL;
      return GL_FALSE;
   } 
      
   /* parse information in __driConfigOptions */
   driParseOptionInfo (&savageScreen->optionCache,
		       __driConfigOptions, __driNConfigOptions);

#if 0
   savageDDFastPathInit();
   savageDDTrifuncInit();
   savageDDSetupInit();
#endif
   return GL_TRUE;
}

/* Accessed by dlsym from dri_mesa_init.c
 */
static void
savageDestroyScreen(__DRIscreenPrivate *sPriv)
{
   savageScreenPrivate *savageScreen = (savageScreenPrivate *)sPriv->private;

   /* free all option information */
   driDestroyOptionInfo (&savageScreen->optionCache);

   Xfree(savageScreen);
   sPriv->private = NULL;
}

#if 0
GLvisual *XMesaCreateVisual(Display *dpy,
                            __DRIscreenPrivate *driScrnPriv,
                            const XVisualInfo *visinfo,
                            const __GLXvisualConfig *config)
{
   /* Drivers may change the args to _mesa_create_visual() in order to
    * setup special visuals.
    */
   return _mesa_create_visual( config->rgba,
                               config->doubleBuffer,
                               config->stereo,
                               _mesa_bitcount(visinfo->red_mask),
                               _mesa_bitcount(visinfo->green_mask),
                               _mesa_bitcount(visinfo->blue_mask),
                               config->alphaSize,
                               0, /* index bits */
                               config->depthSize,
                               config->stencilSize,
                               config->accumRedSize,
                               config->accumGreenSize,
                               config->accumBlueSize,
                               config->accumAlphaSize,
                               0 /* num samples */ );
}
#endif


static GLboolean
savageCreateContext( const __GLcontextModes *mesaVis,
		     __DRIcontextPrivate *driContextPriv,
		     void *sharedContextPrivate )
{
   GLcontext *ctx, *shareCtx;
   savageContextPtr imesa;
   __DRIscreenPrivate *sPriv = driContextPriv->driScreenPriv;
   struct dd_function_table functions;
   SAVAGEDRIPtr         gDRIPriv = (SAVAGEDRIPtr)sPriv->pDevPriv;
   savageScreenPrivate *savageScreen = (savageScreenPrivate *)sPriv->private;
   drm_savage_sarea_t *saPriv=(drm_savage_sarea_t *)(((char*)sPriv->pSAREA)+
						 savageScreen->sarea_priv_offset);
   GLuint maxTextureSize, minTextureSize, maxTextureLevels;
   int i;
   imesa = (savageContextPtr)Xcalloc(sizeof(savageContext), 1);
   if (!imesa) {
      return GL_FALSE;
   }

   /* Init default driver functions then plug in savage-specific texture
    * functions that are needed as early as during context creation. */
   _mesa_init_driver_functions( &functions );
   savageDDInitTextureFuncs( &functions );

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((savageContextPtr) sharedContextPrivate)->glCtx;
   else 
      shareCtx = NULL;
   ctx = _mesa_create_context(mesaVis, shareCtx, &functions, imesa);
   if (!ctx) {
      Xfree(imesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = imesa;

   /* Parse configuration files */
   driParseConfigFiles (&imesa->optionCache, &savageScreen->optionCache,
                        sPriv->myNum, "savage");

   imesa->texture_depth = driQueryOptioni (&imesa->optionCache,
					   "texture_depth");
   if (imesa->texture_depth == DRI_CONF_TEXTURE_DEPTH_FB)
       imesa->texture_depth = ( savageScreen->cpp == 4 ) ?
	   DRI_CONF_TEXTURE_DEPTH_32 : DRI_CONF_TEXTURE_DEPTH_16;

   if (savageScreen->chipset >= S3_SAVAGE4)
       ctx->Const.MaxTextureUnits = 2;
   else
       ctx->Const.MaxTextureUnits = 1;
   if (driQueryOptioni(&imesa->optionCache, "texture_units") <
       ctx->Const.MaxTextureUnits)
       ctx->Const.MaxTextureUnits =
	   driQueryOptioni(&imesa->optionCache, "texture_units");
   ctx->Const.MaxTextureImageUnits = ctx->Const.MaxTextureUnits;
   ctx->Const.MaxTextureCoordUnits = ctx->Const.MaxTextureUnits;

   /* Set the maximum texture size small enough that we can guarentee
    * that all texture units can bind a maximal texture and have them
    * in memory at once.
    */
   if (savageScreen->textureSize[SAVAGE_CARD_HEAP] >
       savageScreen->textureSize[SAVAGE_AGP_HEAP]) {
       maxTextureSize = savageScreen->textureSize[SAVAGE_CARD_HEAP];
       minTextureSize = savageScreen->textureSize[SAVAGE_AGP_HEAP];
   } else {
       maxTextureSize = savageScreen->textureSize[SAVAGE_AGP_HEAP];
       minTextureSize = savageScreen->textureSize[SAVAGE_CARD_HEAP];
   }
   if (ctx->Const.MaxTextureUnits == 2) {
       /* How to distribute two maximum sized textures to two texture heaps?
	* If the smaller heap is less then half the size of the bigger one
	* then the maximum size is half the size of the bigger heap.
	* Otherwise it's the size of the smaller heap. */
       if (minTextureSize < maxTextureSize / 2)
	   maxTextureSize = maxTextureSize / 2;
       else
	   maxTextureSize = minTextureSize;
   }
   for (maxTextureLevels = 1; maxTextureLevels <= 11; ++maxTextureLevels) {
       GLuint size = 1 << maxTextureLevels;
       size *= size * 4;  /* 4 bytes per texel */
       size = size * 4/3; /* all mipmap levels together take roughly
		             4/3 the size of the biggest level */
       if (size > maxTextureSize)
	   break;
   }
   ctx->Const.MaxTextureLevels = maxTextureLevels;
   assert (ctx->Const.MaxTextureLevels > 6); /*spec requires at least 64x64*/

#if 0
   ctx->Const.MinLineWidth = 1.0;
   ctx->Const.MinLineWidthAA = 1.0;
   ctx->Const.MaxLineWidth = 3.0;
   ctx->Const.MaxLineWidthAA = 3.0;
   ctx->Const.LineWidthGranularity = 1.0;
#endif
   
   /* Dri stuff
    */
   imesa->hHWContext = driContextPriv->hHWContext;
   imesa->driFd = sPriv->fd;
   imesa->driHwLock = &sPriv->pSAREA->lock;
   
   imesa->savageScreen = savageScreen;
   imesa->driScreen = sPriv;
   imesa->sarea = saPriv;
   imesa->glBuffer = NULL;
   
   /* DMA buffer */

   /*The shadow pointer*/
   imesa->shadowPointer = 
     (volatile GLuint *)((((GLuint)(&saPriv->shadow_status)) + 31) & 0xffffffe0L) ;
   /* here we use eventTag1 because eventTag0 is used by HWXvMC*/
   imesa->eventTag1 = (volatile GLuint *)(imesa->shadowPointer + 6);
   /*   imesa->eventTag1=(volatile GLuint *)(imesa->MMIO_BASE+0x48c04);*/
   imesa->shadowCounter = MAX_SHADOWCOUNTER;
   imesa->shadowStatus = GL_TRUE;/*Will judge by 2d message */

   imesa->MMIO_BASE = (GLuint)gDRIPriv->registers.map;
   imesa->BCIBase= (GLuint)gDRIPriv->BCIcmdBuf.map;
   for(i=0;i<5;i++)
   {
       imesa->apertureBase[i] = ((GLuint)savageScreen->aperture.map + 
                                 0x01000000 * i );
   }
   
   imesa->aperturePitch = gDRIPriv->aperturePitch;
   
   
   /* change texHeap initialize to support two kind of texture heap*/
   /* here is some parts of initialization, others in InitDriver() */
    
   imesa->lastTexHeap = savageScreen->texVirtual[SAVAGE_AGP_HEAP] ? 2 : 1;
   
   /*allocate texHeap for multi-tex*/ 
   {
     int i;
     
     for(i=0;i<SAVAGE_NR_TEX_HEAPS;i++)
     {
       imesa->texHeap[i] = mmInit( 0, savageScreen->textureSize[i] );
       make_empty_list(&imesa->TexObjList[i]);
     }
     
     make_empty_list(&imesa->SwappedOut);
   }

   imesa->hw_stencil = mesaVis->stencilBits && mesaVis->depthBits == 24;
   imesa->depth_scale = (imesa->savageScreen->zpp == 2) ?
       (1.0F/0x10000):(1.0F/0x1000000);

   imesa->vertex_dma_buffer = NULL;

   /* Uninitialized vertex format. Force setting the vertex state in
    * savageRenderStart.
    */
   imesa->vertex_size = 0;

   /* Utah stuff
    */
   imesa->new_state = ~0;
   imesa->RenderIndex = ~0;
   imesa->dirty = ~0;
   imesa->lostContext = GL_TRUE;
   imesa->TextureMode = ctx->Texture.Unit[0].EnvMode;
   imesa->CurrentTexObj[0] = 0;
   imesa->CurrentTexObj[1] = 0;
   imesa->texAge[SAVAGE_CARD_HEAP]=0;
   imesa->texAge[SAVAGE_AGP_HEAP]=0;

   /* Initialize the software rasterizer and helper modules.
    */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   
   _swsetup_CreateContext( ctx );

   /* Install the customized pipeline:
    */
#if 1
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, savage_pipeline );
#endif

   /* Configure swrast to match hardware characteristics:
    */
   _tnl_allow_pixel_fog( ctx, GL_FALSE );
   _tnl_allow_vertex_fog( ctx, GL_TRUE );
   _swrast_allow_pixel_fog( ctx, GL_FALSE );
   _swrast_allow_vertex_fog( ctx, GL_TRUE );

   ctx->DriverCtx = (void *) imesa;
   imesa->glCtx = ctx;
   if (savageDMAInit(imesa) == GL_FALSE)
       return GL_FALSE;  

#ifndef SAVAGE_DEBUG
   SAVAGE_DEBUG = driParseDebugString( getenv( "SAVAGE_DEBUG" ),
				       debug_control );
#endif

   driInitExtensions( ctx, card_extensions, GL_TRUE );

   savageDDInitStateFuncs( ctx );
   savageDDInitSpanFuncs( ctx );
   savageDDInitDriverFuncs( ctx );
   savageDDInitIoctlFuncs( ctx );
   savageInitTriFuncs( ctx );

   savageDDInitState( imesa );

   if (driQueryOptionb(&imesa->optionCache, "no_rast"))
       FALLBACK(ctx, SAVAGE_FALLBACK_NORAST, GL_TRUE);

   driContextPriv->driverPrivate = (void *) imesa;

   return GL_TRUE;
}

static void
savageDestroyContext(__DRIcontextPrivate *driContextPriv)
{
   savageContextPtr imesa = (savageContextPtr) driContextPriv->driverPrivate;

   assert (imesa); /* should never be NULL */
   if (imesa) {
      savageTextureObjectPtr next_t, t;

      FLUSH_BATCH(imesa);

      /* update for multi-tex*/ 
      {
       int i;
       for(i=0;i<SAVAGE_NR_TEX_HEAPS;i++)
          foreach_s (t, next_t, &(imesa->TexObjList[i]))
	 savageDestroyTexObj(imesa, t);
      }
      foreach_s (t, next_t, &(imesa->SwappedOut))
	 savageDestroyTexObj(imesa, t);
      /*free the dma buffer*/
      savageDMAClose(imesa);
      _swsetup_DestroyContext(imesa->glCtx );
      _tnl_DestroyContext( imesa->glCtx );
      _ac_DestroyContext( imesa->glCtx );
      _swrast_DestroyContext( imesa->glCtx );

      /* free the Mesa context */
      imesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context(imesa->glCtx);

      /* no longer use vertex_dma_buf*/
      Xfree(imesa);
   }
}

static GLboolean
savageCreateBuffer( __DRIscreenPrivate *driScrnPriv,
		    __DRIdrawablePrivate *driDrawPriv,
		    const __GLcontextModes *mesaVis,
		    GLboolean isPixmap)
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
       GLboolean swStencil = mesaVis->stencilBits > 0 && mesaVis->depthBits != 24;
      driDrawPriv->driverPrivate = (void *) 
         _mesa_create_framebuffer(mesaVis,
                                  GL_FALSE,  /* software depth buffer? */
                                  swStencil,
                                  mesaVis->accumRedBits > 0,
                                  mesaVis->alphaBits > 0 );

      return (driDrawPriv->driverPrivate != NULL);
   }
}

static void
savageDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
}

#if 0
void XMesaSwapBuffers(__DRIdrawablePrivate *driDrawPriv)
{
   /* XXX should do swap according to the buffer, not the context! */
   savageContextPtr imesa = savageCtx; 

   FLUSH_VB( imesa->glCtx, "swap buffers" );
   savageSwapBuffers(imesa);
}
#endif

void savageXMesaSetFrontClipRects( savageContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;

   imesa->numClipRects = dPriv->numClipRects;
   imesa->pClipRects = dPriv->pClipRects;
   imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS;
   imesa->drawX = dPriv->x;
   imesa->drawY = dPriv->y;

   savageEmitDrawingRectangle( imesa );
}


void savageXMesaSetBackClipRects( savageContextPtr imesa )
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;

   if (dPriv->numBackClipRects == 0) 
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

   savageEmitDrawingRectangle( imesa );
   imesa->dirty |= SAVAGE_UPLOAD_CLIPRECTS;


}


static void savageXMesaWindowMoved( savageContextPtr imesa ) 
{
   if (0)
      fprintf(stderr, "savageXMesaWindowMoved\n\n");

   switch (imesa->glCtx->Color._DrawDestMask[0]) {
   case DD_FRONT_LEFT_BIT:
      savageXMesaSetFrontClipRects( imesa );
      break;
   case DD_BACK_LEFT_BIT:
      savageXMesaSetBackClipRects( imesa );
      break;
   default:
       break;
   }
}


static GLboolean
savageUnbindContext(__DRIcontextPrivate *driContextPriv)
{
   savageContextPtr savage = (savageContextPtr) driContextPriv->driverPrivate;
   if (savage)
      savage->dirty = ~0;

   return GL_TRUE;
}

#if 0
static GLboolean
savageOpenFullScreen(__DRIcontextPrivate *driContextPriv)
{
    
  
    
    if (driContextPriv) {
      savageContextPtr imesa = (savageContextPtr) driContextPriv->driverPrivate;
      imesa->IsFullScreen = GL_TRUE;
      imesa->backup_frontOffset = imesa->savageScreen->frontOffset;
      imesa->backup_backOffset = imesa->savageScreen->backOffset;
      imesa->backup_frontBitmapDesc = imesa->savageScreen->frontBitmapDesc;
      imesa->savageScreen->frontBitmapDesc = imesa->savageScreen->backBitmapDesc;      
      imesa->toggle = TARGET_BACK;
   }

    return GL_TRUE;
}

static GLboolean
savageCloseFullScreen(__DRIcontextPrivate *driContextPriv)
{
    
    if (driContextPriv) {
      savageContextPtr imesa = (savageContextPtr) driContextPriv->driverPrivate;
      WAIT_IDLE_EMPTY;
      imesa->IsFullScreen = GL_FALSE;   
      imesa->savageScreen->frontOffset = imesa->backup_frontOffset;
      imesa->savageScreen->backOffset = imesa->backup_backOffset;
      imesa->savageScreen->frontBitmapDesc = imesa->backup_frontBitmapDesc;
   }
    return GL_TRUE;
}
#endif

static GLboolean
savageMakeCurrent(__DRIcontextPrivate *driContextPriv,
		  __DRIdrawablePrivate *driDrawPriv,
		  __DRIdrawablePrivate *driReadPriv)
{
   if (driContextPriv) {
      savageContextPtr imesa = (savageContextPtr) driContextPriv->driverPrivate;
      
      imesa->driReadable = driReadPriv;
      imesa->driDrawable = driDrawPriv;
      imesa->mesa_drawable = driDrawPriv;
      imesa->dirty = ~0;
      
      _mesa_make_current2(imesa->glCtx,
                          (GLframebuffer *) driDrawPriv->driverPrivate,
                          (GLframebuffer *) driReadPriv->driverPrivate);
      
      savageXMesaWindowMoved( imesa );
   }
   else 
   {
      _mesa_make_current(NULL, NULL);
   }
   return GL_TRUE;
}


void savageGetLock( savageContextPtr imesa, GLuint flags ) 
{
   __DRIdrawablePrivate *dPriv = imesa->driDrawable;
   __DRIscreenPrivate *sPriv = imesa->driScreen;
   drm_savage_sarea_t *sarea = imesa->sarea;
   int me = imesa->hHWContext;
   int stamp = dPriv->lastStamp; 
   int heap;

  

   /* We know there has been contention.
    */
   drmGetLock(imesa->driFd, imesa->hHWContext, flags);	


   /* Note contention for throttling hint
    */
   imesa->any_contend = 1;

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
      imesa->dirty |= (SAVAGE_UPLOAD_CTX |
		       SAVAGE_UPLOAD_CLIPRECTS |
		       SAVAGE_UPLOAD_TEX0 |
		       SAVAGE_UPLOAD_TEX1);
      imesa->lostContext = GL_TRUE;
      sarea->ctxOwner = me;
   }

   /* Shared texture managment - if another client has played with
    * texture space, figure out which if any of our textures have been
    * ejected, and update our global LRU.
    */
   /*frank just for compiling,texAge,texList,AGP*/ 
   
   for(heap= 0 ;heap < imesa->lastTexHeap ; heap++)
   {
       if (sarea->texAge[heap] != imesa->texAge[heap]) {
           int sz = 1 << (imesa->savageScreen->logTextureGranularity[heap]);
           int idx, nr = 0;

      /* Have to go right round from the back to ensure stuff ends up
       * LRU in our local list...
       */
           for (idx = sarea->texList[heap][SAVAGE_NR_TEX_REGIONS].prev ; 
	       idx != SAVAGE_NR_TEX_REGIONS && nr < SAVAGE_NR_TEX_REGIONS ; 
	       idx = sarea->texList[heap][idx].prev, nr++)
           {
	       if (sarea->texList[heap][idx].age > imesa->texAge[heap])
	       {
	           savageTexturesGone(imesa, heap ,idx * sz, sz, 
	                              sarea->texList[heap][idx].in_use);
	       }
           }

           if (nr == SAVAGE_NR_TEX_REGIONS) 
           {
	      savageTexturesGone(imesa, heap, 0, 
	                         imesa->savageScreen->textureSize[heap], 0);
	      savageResetGlobalLRU( imesa , heap );
           }

           imesa->dirty |= SAVAGE_UPLOAD_TEX0IMAGE;
           imesa->dirty |= SAVAGE_UPLOAD_TEX1IMAGE;
           imesa->texAge[heap] = sarea->texAge[heap];
       }
   } /* end of for loop */ 
   
   if (dPriv->lastStamp != stamp)
      savageXMesaWindowMoved( imesa );

  
   
}



static const struct __DriverAPIRec savageAPI = {
   savageInitDriver,
   savageDestroyScreen,
   savageCreateContext,
   savageDestroyContext,
   savageCreateBuffer,
   savageDestroyBuffer,
   savageSwapBuffers,
   savageMakeCurrent,
   savageUnbindContext
};



#ifndef DRI_NEW_INTERFACE_ONLY
/*
 * This is the (old) bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
void *__driCreateScreen(Display *dpy, int scrn, __DRIscreen *psc,
                        int numConfigs, __GLXvisualConfig *config)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreen(dpy, scrn, psc, numConfigs, config, &savageAPI);
   return (void *) psp;
}
#endif /* DRI_NEW_INTERFACE_ONLY */



#ifdef USE_NEW_INTERFACE
static __GLcontextModes *
savageFillInModes( unsigned pixel_bits, unsigned depth_bits,
		   unsigned stencil_bits, GLboolean have_back_buffer )
{
    __GLcontextModes * modes;
    __GLcontextModes * m;
    unsigned num_modes;
    unsigned depth_buffer_factor;
    unsigned back_buffer_factor;
    GLenum fb_format;
    GLenum fb_type;

    /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
     * enough to add support.  Basically, if a context is created with an
     * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
     * will never be used.
     *
     * FK: What about drivers that don't use page flipping? Could they
     * just expose GLX_SWAP_COPY_OML?
     */
    static const GLenum back_buffer_modes[] = {
	GLX_NONE, GLX_SWAP_UNDEFINED_OML /*, GLX_SWAP_COPY_OML */
    };

    u_int8_t depth_bits_array[2];
    u_int8_t stencil_bits_array[2];


    depth_bits_array[0] = depth_bits;
    depth_bits_array[1] = depth_bits;
    
    /* Just like with the accumulation buffer, always provide some modes
     * with a stencil buffer.  It will be a sw fallback, but some apps won't
     * care about that.
     */
    stencil_bits_array[0] = 0;
    stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

    depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 2 : 1;
    back_buffer_factor  = (have_back_buffer) ? 2 : 1;

    num_modes = depth_buffer_factor * back_buffer_factor * 4;

    if ( pixel_bits == 16 ) {
        fb_format = GL_RGB;
        fb_type = GL_UNSIGNED_SHORT_5_6_5;
    }
    else {
        fb_format = GL_BGR;
        fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    }

    modes = (*create_context_modes)( num_modes, sizeof( __GLcontextModes ) );
    m = modes;
    if ( ! driFillInModes( & m, fb_format, fb_type,
			   depth_bits_array, stencil_bits_array, depth_buffer_factor,
			   back_buffer_modes, back_buffer_factor,
			   GLX_TRUE_COLOR ) ) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    if ( ! driFillInModes( & m, fb_format, fb_type,
			   depth_bits_array, stencil_bits_array, depth_buffer_factor,
			   back_buffer_modes, back_buffer_factor,
			   GLX_DIRECT_COLOR ) ) {
	fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
		 __func__, __LINE__ );
	return NULL;
    }

    /* Mark the visual as slow if there are "fake" stencil bits.
     */
    for ( m = modes ; m != NULL ; m = m->next ) {
	if ( (m->stencilBits != 0) && (m->stencilBits != stencil_bits) ) {
	    m->visualRating = GLX_SLOW_CONFIG;
	}
    }

    return modes;
}


/**
 * This is the bootstrap function for the driver.  libGL supplies all of the
 * requisite information about the system, and the driver initializes itself.
 * This routine also fills in the linked list pointed to by \c driver_modes
 * with the \c __GLcontextModes that the driver can support for windows or
 * pbuffers.
 * 
 * \return A pointer to a \c __DRIscreenPrivate on success, or \c NULL on 
 *         failure.
 */
PUBLIC
void * __driCreateNewScreen( __DRInativeDisplay *dpy, int scrn, __DRIscreen *psc,
			     const __GLcontextModes * modes,
			     const __DRIversion * ddx_version,
			     const __DRIversion * dri_version,
			     const __DRIversion * drm_version,
			     const __DRIframebuffer * frame_buffer,
			     drmAddress pSAREA, int fd, 
			     int internal_api_version,
			     __GLcontextModes ** driver_modes )
			     
{
   __DRIscreenPrivate *psp;
   static const __DRIversion ddx_expected = { 1, 0, 0 };
   static const __DRIversion dri_expected = { 4, 0, 0 };
   static const __DRIversion drm_expected = { 1, 0, 0 };


   if ( ! driCheckDriDdxDrmVersions2( "Savage",
				      dri_version, & dri_expected,
				      ddx_version, & ddx_expected,
				      drm_version, & drm_expected ) ) {
      return NULL;
   }
      
   psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				  ddx_version, dri_version, drm_version,
				  frame_buffer, pSAREA, fd,
				  internal_api_version, &savageAPI);
   if ( psp != NULL ) {
      create_context_modes = (PFNGLXCREATECONTEXTMODES)
	  glXGetProcAddress( (const GLubyte *) "__glXCreateContextModes" );
      if ( create_context_modes != NULL ) {
	 SAVAGEDRIPtr dri_priv = (SAVAGEDRIPtr)psp->pDevPriv;
	 *driver_modes = savageFillInModes( dri_priv->cpp*8,
					    (dri_priv->cpp == 2) ? 16 : 24,
					    (dri_priv->cpp == 2) ? 0  : 8,
					    (dri_priv->backOffset != dri_priv->depthOffset) );
      }
   }

   return (void *) psp;
}
#endif /* USE_NEW_INTERFACE */

#endif
