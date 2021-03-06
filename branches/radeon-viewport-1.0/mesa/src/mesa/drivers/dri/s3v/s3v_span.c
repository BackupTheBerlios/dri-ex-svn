/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "s3v_context.h"
#include "s3v_lock.h"

#include "swrast/swrast.h"

#define _SPANLOCK 1
#define DBG 0

#define LOCAL_VARS \
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);	\
	s3vScreenPtr s3vscrn = vmesa->s3vScreen; \
	__DRIscreenPrivate *sPriv = vmesa->driScreen; \
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable; \
	GLuint pitch = ( (vmesa->Flags & S3V_BACK_BUFFER) ? \
			((dPriv->w+31)&~31) * s3vscrn->cpp \
			: sPriv->fbWidth * s3vscrn->cpp); \
	GLuint height = dPriv->h; \
	char *buf = ( (vmesa->Flags & S3V_BACK_BUFFER) ? \
			(char *)(sPriv->pFB + vmesa->drawOffset) \
			: (char *)(sPriv->pFB + vmesa->drawOffset \
			  + dPriv->x * s3vscrn->cpp + dPriv->y * pitch) ); \
	char *read_buf = ( (vmesa->Flags & S3V_BACK_BUFFER) ? \
			(char *)(sPriv->pFB + vmesa->drawOffset) \
			: (char *)(sPriv->pFB + vmesa->drawOffset \
		          + dPriv->x * s3vscrn->cpp + dPriv->y * pitch) ); \
	GLuint p; \
	(void) read_buf; (void) buf; (void) p; (void) pitch

/* FIXME! Depth/Stencil read/writes don't work ! */
#define LOCAL_DEPTH_VARS \
	s3vScreenPtr s3vscrn = vmesa->s3vScreen; \
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable; \
	__DRIscreenPrivate *sPriv = vmesa->driScreen; \
	GLuint pitch = s3vscrn->depthPitch; \
	GLuint height = dPriv->h; \
	char *buf = (char *)(sPriv->pFB + \
			s3vscrn->depthOffset); /* + \	
			dPriv->x * s3vscrn->cpp + \
			dPriv->y * pitch)*/ \
	(void) pitch

#define LOCAL_STENCIL_VARS	LOCAL_DEPTH_VARS


#define CLIPPIXEL( _x, _y ) \
	((_x >= minx) && (_x < maxx) && (_y >= miny) && (_y < maxy))


#define CLIPSPAN( _x, _y, _n, _x1, _n1, _i ) \
	if ( _y < miny || _y >= maxy ) { \
		_n1 = 0, _x1 = x; \
	} else { \
		_n1 = _n; \
		_x1 = _x; \
		if ( _x1 < minx ) \
			_i += (minx-_x1), n1 -= (minx-_x1), _x1 = minx; \
      		if ( _x1 + _n1 >= maxx ) \
			n1 -= (_x1 + n1 - maxx); \
	}

#define Y_FLIP( _y )	(height - _y - 1)

#if _SPANLOCK	/* OK, we lock */

#define HW_LOCK() \
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);	\
	(void) vmesa; \
	DMAFLUSH(); \
	S3V_SIMPLE_FLUSH_LOCK(vmesa);
#define HW_UNLOCK() S3V_SIMPLE_UNLOCK(vmesa);

#else			/* plz, don't lock */

#define HW_LOCK() \
	s3vContextPtr vmesa = S3V_CONTEXT(ctx); \
    (void) vmesa; \
	DMAFLUSH(); 
#define HW_UNLOCK()

#endif

#define HW_CLIPLOOP()							\
   do {									\
      __DRIdrawablePrivate *dPriv = vmesa->driDrawable;			\
      int _nc = dPriv->numClipRects;					\
									\
      while ( _nc-- ) {							\
	 int minx = dPriv->pClipRects[_nc].x1 - dPriv->x;		\
	 int miny = dPriv->pClipRects[_nc].y1 - dPriv->y;		\
	 int maxx = dPriv->pClipRects[_nc].x2 - dPriv->x;		\
	 int maxy = dPriv->pClipRects[_nc].y2 - dPriv->y;

#define HW_ENDCLIPLOOP()						\
      }									\
   } while (0)


/* ================================================================
 * Color buffer
 */

/* 16 bit, RGB565 color spanline and pixel functions
 */
#define INIT_MONO_PIXEL(p, color) \
  p = S3VIRGEPACKCOLOR555( color[0], color[1], color[2], color[3] )

#define WRITE_RGBA( _x, _y, r, g, b, a ) \
do { \
   *(GLushort *)(buf + _x*2 + _y*pitch) = ((((int)r & 0xf8) << 7) | \
					   (((int)g & 0xf8) << 2) | \
					   (((int)b & 0xf8) >> 3)); \
   DEBUG(("buf=0x%x drawOffset=0x%x dPriv->x=%i s3vscrn->cpp=%i dPriv->y=%i pitch=%i\n", \
   	sPriv->pFB, vmesa->drawOffset, dPriv->x, s3vscrn->cpp, dPriv->y, pitch)); \
   DEBUG(("dPriv->w = %i\n", dPriv->w)); \
} while(0)

#define WRITE_PIXEL( _x, _y, p ) \
   *(GLushort *)(buf + _x*2 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y ) \
   do { \
      GLushort p = *(GLushort *)(read_buf + _x*2 + _y*pitch); \
      rgba[0] = (p >> 7) & 0xf8; \
      rgba[1] = (p >> 2) & 0xf8; \
      rgba[2] = (p << 3) & 0xf8; \
      rgba[3] = 0xff; /*
      if ( rgba[0] & 0x08 ) rgba[0] |= 0x07; \ 
      if ( rgba[1] & 0x04 ) rgba[1] |= 0x03; \
      if ( rgba[2] & 0x08 ) rgba[2] |= 0x07; */ \
   } while (0)

#define TAG(x) s3v##x##_RGB555
#include "spantmp.h"


/* 32 bit, ARGB8888 color spanline and pixel functions
 */

#undef INIT_MONO_PIXEL
#define INIT_MONO_PIXEL(p, color) \
  p = PACK_COLOR_8888( color[3], color[0], color[1], color[2] )

#define WRITE_RGBA( _x, _y, r, g, b, a ) \
   *(GLuint *)(buf + _x*4 + _y*pitch) = ((b <<  0) | \
					 (g <<  8) | \
					 (r << 16) | \
					 (a << 24) )

#define WRITE_PIXEL( _x, _y, p ) \
   *(GLuint *)(buf + _x*4 + _y*pitch) = p

#define READ_RGBA( rgba, _x, _y ) \
do { \
   GLuint p = *(GLuint *)(read_buf + _x*4 + _y*pitch); \
   rgba[0] = (p >> 16) & 0xff; \
   rgba[1] = (p >>  8) & 0xff; \
   rgba[2] = (p >>  0) & 0xff; \
   rgba[3] = (p >> 24) & 0xff; \
} while (0)

#define TAG(x) s3v##x##_ARGB8888
#include "spantmp.h"


/* 16 bit depthbuffer functions.
 */
#define WRITE_DEPTH( _x, _y, d ) \
   *(GLushort *)(buf + _x*2 + _y*dPriv->w*2) = d

#define READ_DEPTH( d, _x, _y ) \
   d = *(GLushort *)(buf + _x*2 + _y*dPriv->w*2);

#define TAG(x) s3v##x##_16
#include "depthtmp.h"




/* 32 bit depthbuffer functions.
 */
#if 0
#define WRITE_DEPTH( _x, _y, d )	\
   *(GLuint *)(buf + _x*4 + _y*pitch) = d;

#define READ_DEPTH( d, _x, _y )		\
   d = *(GLuint *)(buf + _x*4 + _y*pitch);	

#define TAG(x) s3v##x##_32
#include "depthtmp.h"
#endif


/* 24/8 bit interleaved depth/stencil functions
 */
#if 0
#define WRITE_DEPTH( _x, _y, d ) { \
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xff; \
   tmp |= (d) & 0xffffff00; \
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp; \
}

#define READ_DEPTH( d, _x, _y ) \
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & ~0xff	


#define TAG(x) s3v##x##_24_8
#include "depthtmp.h"

#define WRITE_STENCIL( _x, _y, d ) { \
   GLuint tmp = *(GLuint *)(buf + _x*4 + _y*pitch);	\
   tmp &= 0xffffff00; \
   tmp |= d & 0xff; \
   *(GLuint *)(buf + _x*4 + _y*pitch) = tmp; \
}

#define READ_STENCIL( d, _x, _y ) \
   d = *(GLuint *)(buf + _x*4 + _y*pitch) & 0xff	

#define TAG(x) s3v##x##_24_8
#include "stenciltmp.h"

#endif

static void s3vSetBuffer( GLcontext *ctx, GLframebuffer *colorBuffer,
			  GLuint bufferBit )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);

   switch ( bufferBit ) {
   case DD_FRONT_LEFT_BIT:
      vmesa->drawOffset = vmesa->readOffset = 0;
      break;
   case DD_BACK_LEFT_BIT:
      vmesa->drawOffset = vmesa->readOffset = vmesa->driScreen->fbHeight *
                                              vmesa->driScreen->fbWidth *
                                              vmesa->s3vScreen->cpp; 
      break;
   }
}


void s3vInitSpanFuncs( GLcontext *ctx )
{
   s3vContextPtr vmesa = S3V_CONTEXT(ctx);
   struct swrast_device_driver *swdd = _swrast_GetDeviceDriverReference(ctx);

   swdd->SetBuffer = s3vSetBuffer;

   switch ( vmesa->s3vScreen->cpp ) {
   case 2:
      swdd->WriteRGBASpan	= s3vWriteRGBASpan_RGB555;
      swdd->WriteRGBSpan	= s3vWriteRGBSpan_RGB555;
      swdd->WriteMonoRGBASpan	= s3vWriteMonoRGBASpan_RGB555;
      swdd->WriteRGBAPixels	= s3vWriteRGBAPixels_RGB555;
      swdd->WriteMonoRGBAPixels	= s3vWriteMonoRGBAPixels_RGB555;
      swdd->ReadRGBASpan	= s3vReadRGBASpan_RGB555;
      swdd->ReadRGBAPixels      = s3vReadRGBAPixels_RGB555;
      break;

   case 4:
      swdd->WriteRGBASpan	= s3vWriteRGBASpan_ARGB8888;
      swdd->WriteRGBSpan	= s3vWriteRGBSpan_ARGB8888;
      swdd->WriteMonoRGBASpan   = s3vWriteMonoRGBASpan_ARGB8888;
      swdd->WriteRGBAPixels     = s3vWriteRGBAPixels_ARGB8888;
      swdd->WriteMonoRGBAPixels = s3vWriteMonoRGBAPixels_ARGB8888;
#if 1
      swdd->ReadRGBASpan    = s3vReadRGBASpan_ARGB8888;
#else
      swdd->ReadRGBASpan    = s3vReadRGBASpan8888;
#endif
      swdd->ReadRGBAPixels  = s3vReadRGBAPixels_ARGB8888;
      break;

   default:
      break;
   }

   switch ( vmesa->glCtx->Visual.depthBits ) {
   case 15:
   case 16:
      swdd->ReadDepthSpan	= s3vReadDepthSpan_16;
      swdd->WriteDepthSpan	= s3vWriteDepthSpan_16;
      swdd->ReadDepthPixels	= s3vReadDepthPixels_16;
      swdd->WriteDepthPixels	= s3vWriteDepthPixels_16;
      break;

#if 0
   case 24:
      swdd->ReadDepthSpan	= s3vReadDepthSpan_24_8;
      swdd->WriteDepthSpan	= s3vWriteDepthSpan_24_8;
      swdd->ReadDepthPixels	= s3vReadDepthPixels_24_8;
      swdd->WriteDepthPixels	= s3vWriteDepthPixels_24_8;

      swdd->ReadStencilSpan	= s3vReadStencilSpan_24_8;
      swdd->WriteStencilSpan	= s3vWriteStencilSpan_24_8;
      swdd->ReadStencilPixels	= s3vReadStencilPixels_24_8;
      swdd->WriteStencilPixels	= s3vWriteStencilPixels_24_8;
      break;
#endif

   default:
      break;
   }
}
