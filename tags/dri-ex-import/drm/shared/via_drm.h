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
#ifndef _VIA_DRM_H_
#define _VIA_DRM_H_

/* WARNING: These defines must be the same as what the Xserver uses.
 * if you change them, you must change the defines in the Xserver.
 */

#ifndef _VIA_DEFINES_
#define _VIA_DEFINES_

#define VIA_DMA_BUF_ORDER		12
#define VIA_DMA_BUF_SZ 		        (1 << VIA_DMA_BUF_ORDER)
#define VIA_DMA_BUF_NR 			256
#define VIA_NR_SAREA_CLIPRECTS 		8
#define VIA_NR_XVMC_PORTS               10
#define VIA_NR_XVMC_LOCKS               5
#define VIA_MAX_CACHELINE_SIZE          64
#define XVMCLOCKPTR(saPriv,lockNo)					\
        ((volatile int *)(((((unsigned long) (saPriv)->XvMCLockArea) +	\
                            (VIA_MAX_CACHELINE_SIZE - 1)) &             \
                           ~(VIA_MAX_CACHELINE_SIZE - 1)) +             \
                          VIA_MAX_CACHELINE_SIZE*(lockNo)))

/* Each region is a minimum of 64k, and there are at most 64 of them.
 */
#define VIA_NR_TEX_REGIONS 64
#define VIA_LOG_MIN_TEX_REGION_SIZE 16
#endif

#define VIA_UPLOAD_TEX0IMAGE  0x1	/* handled clientside */
#define VIA_UPLOAD_TEX1IMAGE  0x2	/* handled clientside */
#define VIA_UPLOAD_CTX        0x4
#define VIA_UPLOAD_BUFFERS    0x8
#define VIA_UPLOAD_TEX0       0x10
#define VIA_UPLOAD_TEX1       0x20
#define VIA_UPLOAD_CLIPRECTS  0x40
#define VIA_UPLOAD_ALL        0xff

/* VIA specific ioctls */
#define DRM_IOCTL_VIA_ALLOCMEM	DRM_IOWR(0x40, drm_via_mem_t)
#define DRM_IOCTL_VIA_FREEMEM	DRM_IOW(0x41, drm_via_mem_t)
#define DRM_IOCTL_VIA_AGP_INIT	DRM_IOWR(0x42, drm_via_agp_t)
#define DRM_IOCTL_VIA_FB_INIT	DRM_IOWR(0x43, drm_via_fb_t)
#define DRM_IOCTL_VIA_MAP_INIT	DRM_IOWR(0x44, drm_via_init_t)
#define DRM_IOCTL_VIA_DEC_FUTEX DRM_IOW(0x45, drm_via_futex_t)
#define DRM_IOCTL_VIA_DMA_INIT	DRM_IOWR(0x47, drm_via_dma_init_t)
#define DRM_IOCTL_VIA_CMDBUFFER	DRM_IOW(0x48, drm_via_cmdbuffer_t)
#define DRM_IOCTL_VIA_FLUSH	DRM_IO(0x49)
#define DRM_IOCTL_VIA_PCICMD	DRM_IOW(0x4A, drm_via_cmdbuffer_t)
#define DRM_IOCTL_VIA_CMDBUF_SIZE DRM_IOWR(0x4B, drm_via_cmdbuf_size_t)


/* Indices into buf.Setup where various bits of state are mirrored per
 * context and per buffer.  These can be fired at the card as a unit,
 * or in a piecewise fashion as required.
 */

#define VIA_TEX_SETUP_SIZE 8

/* Flags for clear ioctl
 */
#define VIA_FRONT   0x1
#define VIA_BACK    0x2
#define VIA_DEPTH   0x4
#define VIA_STENCIL 0x8
#define VIDEO 0
#define AGP 1
typedef struct {
	unsigned long offset;
	unsigned long size;
} drm_via_agp_t;

typedef struct {
	unsigned long offset;
	unsigned long size;
} drm_via_fb_t;

typedef struct {
	uint32_t context;
	uint32_t type;
	unsigned long size;
	unsigned long index;
	unsigned long offset;
} drm_via_mem_t;

typedef struct _drm_via_init {
	enum {
		VIA_INIT_MAP = 0x01,
		VIA_CLEANUP_MAP = 0x02
	} func;

	unsigned long sarea_priv_offset;
	unsigned long fb_offset;
	unsigned long mmio_offset;
	unsigned long agpAddr;
} drm_via_init_t;

typedef struct _drm_via_futex {
	enum {
		VIA_FUTEX_WAIT = 0x00,
		VIA_FUTEX_WAKE = 0X01
	} func;
	uint32_t ms;
	uint32_t lock;
	uint32_t val;
} drm_via_futex_t;

typedef struct _drm_via_dma_init {
	enum {
		VIA_INIT_DMA = 0x01,
		VIA_CLEANUP_DMA = 0x02,
                VIA_DMA_INITIALIZED = 0x03
	} func;

	unsigned long offset;
	unsigned long size;
	unsigned long reg_pause_addr;
} drm_via_dma_init_t;

typedef struct _drm_via_cmdbuffer {
	char *buf;
	unsigned long size;
} drm_via_cmdbuffer_t;

/* Warning: If you change the SAREA structure you must change the Xserver
 * structure as well */

typedef struct _drm_via_tex_region {
	unsigned char next, prev;	/* indices to form a circular LRU  */
	unsigned char inUse;	/* owned by a client, or free? */
	int age;		/* tracked by clients to update local LRU's */
} drm_via_tex_region_t;

typedef struct _drm_via_sarea {
	unsigned int dirty;
	unsigned int nbox;
	drm_clip_rect_t boxes[VIA_NR_SAREA_CLIPRECTS];
	drm_via_tex_region_t texList[VIA_NR_TEX_REGIONS + 1];
	int texAge;		/* last time texture was uploaded */
	int ctxOwner;		/* last context to upload state */
	int vertexPrim;

	/*
	 * Below is for XvMC.
	 * We want the lock integers alone on, and aligned to, a cache line.
	 * Therefore this somewhat strange construct.
	 */

	char XvMCLockArea[VIA_MAX_CACHELINE_SIZE * (VIA_NR_XVMC_LOCKS + 1)];

	unsigned int XvMCDisplaying[VIA_NR_XVMC_PORTS];
	unsigned int XvMCSubPicOn[VIA_NR_XVMC_PORTS];
	unsigned int XvMCCtxNoGrabbed;	/* Last context to hold decoder */

} drm_via_sarea_t;

typedef struct _drm_via_flush_agp {
	unsigned int offset;
	unsigned int size;
	unsigned int index;
	int discard;		/* client is finished with the buffer? */
} drm_via_flush_agp_t;

typedef struct _drm_via_flush_sys {
	unsigned int offset;
	unsigned int size;
	unsigned long index;
	int discard;		/* client is finished with the buffer? */
} drm_via_flush_sys_t;

typedef struct _drm_via_cmdbuf_size {
	enum {
		VIA_CMDBUF_SPACE = 0x01,
		VIA_CMDBUF_LAG = 0x02
	} func;
	int wait;
	uint32_t size;
} drm_via_cmdbuf_size_t;

#ifdef __KERNEL__

int via_fb_init(DRM_IOCTL_ARGS);
int via_mem_alloc(DRM_IOCTL_ARGS);
int via_mem_free(DRM_IOCTL_ARGS);
int via_agp_init(DRM_IOCTL_ARGS);
int via_map_init(DRM_IOCTL_ARGS);
int via_decoder_futex(DRM_IOCTL_ARGS);
int via_dma_init(DRM_IOCTL_ARGS);
int via_cmdbuffer(DRM_IOCTL_ARGS);
int via_flush_ioctl(DRM_IOCTL_ARGS);
int via_pci_cmdbuffer(DRM_IOCTL_ARGS);
int via_cmdbuf_size(DRM_IOCTL_ARGS);

#endif
#endif				/* _VIA_DRM_H_ */
