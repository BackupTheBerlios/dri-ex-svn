/* drm_pci.h -- PCI DMA memory management wrappers for DRM -*- linux-c -*- */
/**
 * \file drm_pci.h
 * \brief Functions and ioctls to manage PCI memory
 * 
 * \warning These interfaces aren't stable yet.
 * 
 * \todo Implement the remaining ioctl's for the PCI pools.
 * \todo Add support to map these buffers.
 * \todo The wrappers here are so thin that they would be better off inlined..
 *
 * \author Jos� Fonseca <jrfonseca@tungstengraphics.com>
 * \author Leif Delgass <ldelgass@retinalburn.net>
 */

/*
 * Copyright 2003 Jos� Fonseca.
 * Copyright 2003 Leif Delgass.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <linux/pci.h>
#include "drmP.h"

/**********************************************************************/
/** \name PCI memory */
/*@{*/


/**
 * \brief Allocate a PCI consistent memory block, for DMA.
 */
void *
DRM(pci_alloc)(drm_device_t *dev, size_t size, size_t align, 
	       dma_addr_t maxaddr, dma_addr_t *busaddr)
{
	void *address;
#if 0
	unsigned long addr;
	size_t sz;
#endif
#if DRM_DEBUG_MEMORY
	int area = DRM_MEM_DMA;

	spin_lock(&DRM(mem_lock));
	if ((DRM(ram_used) >> PAGE_SHIFT)
	    > (DRM_RAM_PERCENT * DRM(ram_available)) / 100) {
		spin_unlock(&DRM(mem_lock));
		return 0;
	}
	spin_unlock(&DRM(mem_lock));
#endif

	/* pci_alloc_consistent only guarantees alignment to the smallest 
	 * PAGE_SIZE order which is greater than or equal to the requested size.
	 * Return NULL here for now to make sure nobody tries for larger alignment
	 */
	if (align > size)
		return NULL;

	if (pci_set_dma_mask( dev->pdev, maxaddr ) != 0) {
		DRM_ERROR( "Setting pci dma mask failed\n" );
		return NULL;
	}

	address = pci_alloc_consistent( dev->pdev, size, busaddr );

#if DRM_DEBUG_MEMORY
	if (address == NULL) {
		spin_lock(&DRM(mem_lock));
		++DRM(mem_stats)[area].fail_count;
		spin_unlock(&DRM(mem_lock));
		return NULL;
	}

	spin_lock(&DRM(mem_lock));
	++DRM(mem_stats)[area].succeed_count;
	DRM(mem_stats)[area].bytes_allocated += size;
	DRM(ram_used)		             += size;
	spin_unlock(&DRM(mem_lock));
#else
	if (address == NULL)
		return NULL;
#endif

	memset(address, 0, size);

#if 0
	/* XXX - Is virt_to_page() legal for consistent mem? */
				/* Reserve */
	for (addr = (unsigned long)address, sz = size;
	     sz > 0;
	     addr += PAGE_SIZE, sz -= PAGE_SIZE) {
		SetPageReserved(virt_to_page(addr));
	}
#endif

	return address;
}

/**
 * \brief Free a PCI consistent memory block.
 */
void
DRM(pci_free)(drm_device_t *dev, size_t size, void *vaddr, dma_addr_t busaddr)
{
#if 0
	unsigned long addr;
	size_t sz;
#endif
#if DRM_DEBUG_MEMORY
	int area = DRM_MEM_DMA;
	int alloc_count;
	int free_count;
#endif

	if (!vaddr) {
#if DRM_DEBUG_MEMORY
		DRM_MEM_ERROR(area, "Attempt to free address 0\n");
#endif
	} else {
#if 0
		/* XXX - Is virt_to_page() legal for consistent mem? */
				/* Unreserve */
		for (addr = (unsigned long)vaddr, sz = size;
		     sz > 0;
		     addr += PAGE_SIZE, sz -= PAGE_SIZE) {
			ClearPageReserved(virt_to_page(addr));
		}
#endif
		pci_free_consistent( dev->pdev, size, vaddr, busaddr );
	}

#if DRM_DEBUG_MEMORY
	spin_lock(&DRM(mem_lock));
	free_count  = ++DRM(mem_stats)[area].free_count;
	alloc_count =	DRM(mem_stats)[area].succeed_count;
	DRM(mem_stats)[area].bytes_freed += size;
	DRM(ram_used)			 -= size;
	spin_unlock(&DRM(mem_lock));
	if (free_count > alloc_count) {
		DRM_MEM_ERROR(area,
			      "Excess frees: %d frees, %d allocs\n",
			      free_count, alloc_count);
	}
#endif

}

/*@}*/
