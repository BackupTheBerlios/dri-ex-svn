/**
 * \file drm_bufs.c
 * Generic buffer template
 *
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 * \author Gareth Hughes <gareth@valinux.com>
 */

/*
 * Created: Thu Nov 23 03:10:50 2000 by gareth@valinux.com
 *
 * Copyright 1999, 2000 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/vmalloc.h>
#include "drmP.h"

 /**
 * Adjusts the memory offset to its absolute value according to the mapping
 * type.  Adds the map to the map list drm_device::maplist. Adds MTRR's where
 * applicable and if supported by the kernel.
 */
int drm_initmap(drm_device_t * dev, unsigned int offset, unsigned int size,
		int type, int flags)
{
	drm_map_t *map;
	drm_map_list_t *list;

	DRM_DEBUG("\n");

	if ((offset & (~PAGE_MASK)) || (size & (~PAGE_MASK)))
		return -EINVAL;
#if !defined(__sparc__) && !defined(__alpha__)
	if (offset + size < offset || offset < virt_to_phys(high_memory))
		return -EINVAL;
#endif
	if (!(list = drm_alloc(sizeof(*list), DRM_MEM_MAPS)))
		return -ENOMEM;
	memset(list, 0, sizeof(*list));

	if (!(map = drm_alloc(sizeof(*map), DRM_MEM_MAPS))) {
		drm_free(list, sizeof(*list), DRM_MEM_MAPS);
		return -ENOMEM;
	}

	*map = (drm_map_t) {
	.offset = offset,.size = size,.type = type,.flags =
		    flags,.mtrr = -1,.handle = 0,};
	list->map = map;

	DRM_DEBUG("initmap offset = 0x%08lx, size = 0x%08lx, type = %d\n",
		  map->offset, map->size, map->type);

#ifdef __alpha__
	map->offset += dev->hose->mem_space->start;
#endif
	if (drm_core_has_MTRR(dev)) {
		if (map->type == _DRM_FRAME_BUFFER ||
		    (map->flags & _DRM_WRITE_COMBINING)) {
			map->mtrr = mtrr_add(map->offset, map->size,
					     MTRR_TYPE_WRCOMB, 1);
		}
	}

	if (map->type == _DRM_REGISTERS)
		map->handle = drm_ioremap(map->offset, map->size, dev);

	down(&dev->struct_sem);
	list_add(&list->head, &dev->maplist->head);
	up(&dev->struct_sem);

	dev->driver->permanent_maps = 1;
	DRM_DEBUG("finished\n");

	return 0;
}
EXPORT_SYMBOL(drm_initmap);

/**
 * Ioctl to specify a range of memory that is available for mapping by a non-root process.
 *
 * \param inode device inode.
 * \param filp file pointer.
 * \param cmd command.
 * \param arg pointer to a drm_map structure.
 * \return zero on success or a negative value on error.
 *
 * Adjusts the memory offset to its absolute value according to the mapping
 * type.  Adds the map to the map list drm_device::maplist. Adds MTRR's where
 * applicable and if supported by the kernel.
 */
int drm_addmap(struct inode *inode, struct file *filp,
	       unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	drm_map_t *map;
	drm_map_t __user *argp = (void __user *)arg;
	drm_map_list_t *list;

	if (!(filp->f_mode & 3))
		return -EACCES;	/* Require read/write */

	map = drm_alloc(sizeof(*map), DRM_MEM_MAPS);
	if (!map)
		return -ENOMEM;

	if (copy_from_user(map, argp, sizeof(*map))) {
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		return -EFAULT;
	}

	/* Only allow shared memory to be removable since we only keep enough
	 * book keeping information about shared memory to allow for removal
	 * when processes fork.
	 */
	if ((map->flags & _DRM_REMOVABLE) && map->type != _DRM_SHM) {
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		return -EINVAL;
	}
	DRM_DEBUG("offset = 0x%08lx, size = 0x%08lx, type = %d\n",
		  map->offset, map->size, map->type);
	if ((map->offset & (~PAGE_MASK)) || (map->size & (~PAGE_MASK))) {
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		return -EINVAL;
	}
	map->mtrr = -1;
	map->handle = NULL;

	switch (map->type) {
	case _DRM_REGISTERS:
	case _DRM_FRAME_BUFFER:{
			/* after all the drivers switch to permanent mapping this should just return an error */
			struct list_head *_list;

			/* If permanent maps are implemented, maps must match */
			if (dev->driver->permanent_maps) {
				DRM_DEBUG
				    ("Looking for: offset = 0x%08lx, size = 0x%08lx, type = %d\n",
				     map->offset, map->size, map->type);
				list_for_each(_list, &dev->maplist->head) {
					drm_map_list_t *_entry =
					    list_entry(_list, drm_map_list_t,
						       head);
					DRM_DEBUG
					    ("Checking: offset = 0x%08lx, size = 0x%08lx, type = %d\n",
					     _entry->map->offset,
					     _entry->map->size,
					     _entry->map->type);
					if (_entry->map
					    && map->type == _entry->map->type
					    && map->offset ==
					    _entry->map->offset) {
						_entry->map->size = map->size;
						drm_free(map, sizeof(*map),
							 DRM_MEM_MAPS);
						map = _entry->map;
						DRM_DEBUG
						    ("Found existing: offset = 0x%08lx, size = 0x%08lx, type = %d\n",
						     map->offset, map->size,
						     map->type);
						goto found_it;
					}
				}
				/* addmap didn't match an existing permanent map, that's an error */
				return -EINVAL;
			}
#if !defined(__sparc__) && !defined(__alpha__) && !defined(__ia64__)
			if (map->offset + map->size < map->offset ||
			    map->offset < virt_to_phys(high_memory)) {
				drm_free(map, sizeof(*map), DRM_MEM_MAPS);
				return -EINVAL;
			}
#endif
#ifdef __alpha__
			map->offset += dev->hose->mem_space->start;
#endif
			if (drm_core_has_MTRR(dev)) {
				if (map->type == _DRM_FRAME_BUFFER ||
				    (map->flags & _DRM_WRITE_COMBINING)) {
					map->mtrr =
					    mtrr_add(map->offset, map->size,
						     MTRR_TYPE_WRCOMB, 1);
				}
			}
			if (map->type == _DRM_REGISTERS)
				map->handle =
				    drm_ioremap(map->offset, map->size, dev);
			break;
		}
	case _DRM_SHM:
		map->handle = vmalloc_32(map->size);
		DRM_DEBUG("%lu %d %p\n",
			  map->size, drm_order(map->size), map->handle);
		if (!map->handle) {
			drm_free(map, sizeof(*map), DRM_MEM_MAPS);
			return -ENOMEM;
		}
		map->offset = (unsigned long)map->handle;
		if (map->flags & _DRM_CONTAINS_LOCK) {
			/* Prevent a 2nd X Server from creating a 2nd lock */
			if (dev->lock.hw_lock != NULL) {
				vfree(map->handle);
				drm_free(map, sizeof(*map), DRM_MEM_MAPS);
				return -EBUSY;
			}
			dev->sigdata.lock = dev->lock.hw_lock = map->handle;	/* Pointer to lock */
		}
		break;
	case _DRM_AGP:
		if (drm_core_has_AGP(dev)) {
#ifdef __alpha__
			map->offset += dev->hose->mem_space->start;
#endif
			map->offset += dev->agp->base;
			map->mtrr = dev->agp->agp_mtrr;	/* for getmap */
		}
		break;
	case _DRM_SCATTER_GATHER:
		if (!dev->sg) {
			drm_free(map, sizeof(*map), DRM_MEM_MAPS);
			return -EINVAL;
		}
		map->offset += dev->sg->handle;
		break;

	default:
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		return -EINVAL;
	}

	list = drm_alloc(sizeof(*list), DRM_MEM_MAPS);
	if (!list) {
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
		return -EINVAL;
	}
	memset(list, 0, sizeof(*list));
	list->map = map;

	down(&dev->struct_sem);
	list_add(&list->head, &dev->maplist->head);
	up(&dev->struct_sem);
      found_it:
	if (copy_to_user(argp, map, sizeof(*map)))
		return -EFAULT;
	if (map->type != _DRM_SHM) {
		if (copy_to_user(&argp->handle,
				 &map->offset, sizeof(map->offset)))
			return -EFAULT;
	}
	return 0;
}

/**
 * Remove a map private from list and deallocate resources if the mapping
 * isn't in use.
 *
 * \param inode device inode.
 * \param filp file pointer.
 * \param cmd command.
 * \param arg pointer to a drm_map_t structure.
 * \return zero on success or a negative value on error.
 *
 * Searches the map on drm_device::maplist, removes it from the list, see if
 * its being used, and free any associate resource (such as MTRR's) if it's not
 * being on use.
 *
 * \sa addmap().
 */
int drm_rmmap(struct inode *inode, struct file *filp,
	      unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	struct list_head *list;
	drm_map_list_t *r_list = NULL;
	drm_vma_entry_t *pt, *prev;
	drm_map_t *map;
	drm_map_t request;
	int found_maps = 0;

	if (copy_from_user(&request, (drm_map_t __user *) arg, sizeof(request))) {
		return -EFAULT;
	}

	down(&dev->struct_sem);
	list = &dev->maplist->head;
	list_for_each(list, &dev->maplist->head) {
		r_list = list_entry(list, drm_map_list_t, head);

		if (r_list->map &&
		    r_list->map->handle == request.handle &&
		    r_list->map->flags & _DRM_REMOVABLE)
			break;
	}

	/* List has wrapped around to the head pointer, or its empty we didn't
	 * find anything.
	 */
	if (list == (&dev->maplist->head)) {
		up(&dev->struct_sem);
		return -EINVAL;
	}
	map = r_list->map;

	/* Register and framebuffer maps are permanent */
	if ((map->type == _DRM_REGISTERS) || (map->type == _DRM_FRAME_BUFFER)) {
		up(&dev->struct_sem);
		return 0;
	}
	list_del(list);
	drm_free(list, sizeof(*list), DRM_MEM_MAPS);

	for (pt = dev->vmalist, prev = NULL; pt; prev = pt, pt = pt->next) {
		if (pt->vma->vm_private_data == map)
			found_maps++;
	}

	if (!found_maps) {
		switch (map->type) {
		case _DRM_REGISTERS:
		case _DRM_FRAME_BUFFER:
			break;	/* Can't get here, make compiler happy */
		case _DRM_SHM:
			vfree(map->handle);
			break;
		case _DRM_AGP:
		case _DRM_SCATTER_GATHER:
			break;
		}
		drm_free(map, sizeof(*map), DRM_MEM_MAPS);
	}
	up(&dev->struct_sem);
	return 0;
}

/**
 * Cleanup after an error on one of the addbufs() functions.
 *
 * \param dev DRM device.
 * \param entry buffer entry where the error occurred.
 *
 * Frees any pages and buffers associated with the given entry.
 */
static void drm_cleanup_buf_error(drm_device_t * dev, drm_buf_entry_t * entry)
{
	int i;

	if (entry->seg_count) {
		for (i = 0; i < entry->seg_count; i++) {
			if (entry->seglist[i]) {
				drm_free_pages(entry->seglist[i],
					       entry->page_order, DRM_MEM_DMA);
			}
		}
		drm_free(entry->seglist,
			 entry->seg_count *
			 sizeof(*entry->seglist), DRM_MEM_SEGS);

		entry->seg_count = 0;
	}

	if (entry->buf_count) {
		for (i = 0; i < entry->buf_count; i++) {
			if (entry->buflist[i].dev_private) {
				drm_free(entry->buflist[i].dev_private,
					 entry->buflist[i].dev_priv_size,
					 DRM_MEM_BUFS);
			}
		}
		drm_free(entry->buflist,
			 entry->buf_count *
			 sizeof(*entry->buflist), DRM_MEM_BUFS);

		entry->buf_count = 0;
	}
}

#if __OS_HAS_AGP
/**
 * Add AGP buffers for DMA transfers (ioctl).
 *
 * \param inode device inode.
 * \param filp file pointer.
 * \param cmd command.
 * \param arg pointer to a drm_buf_desc_t request.
 * \return zero on success or a negative number on failure.
 *
 * After some sanity checks creates a drm_buf structure for each buffer and
 * reallocates the buffer list of the same size order to accommodate the new
 * buffers.
 */
int drm_addbufs_agp(struct inode *inode, struct file *filp,
		    unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	drm_device_dma_t *dma = dev->dma;
	drm_buf_desc_t request;
	drm_buf_entry_t *entry;
	drm_buf_t *buf;
	unsigned long offset;
	unsigned long agp_offset;
	int count;
	int order;
	int size;
	int alignment;
	int page_order;
	int total;
	int byte_count;
	int i;
	drm_buf_t **temp_buflist;
	drm_buf_desc_t __user *argp = (void __user *)arg;

	if (!dma)
		return -EINVAL;

	if (copy_from_user(&request, argp, sizeof(request)))
		return -EFAULT;

	count = request.count;
	order = drm_order(request.size);
	size = 1 << order;

	alignment = (request.flags & _DRM_PAGE_ALIGN)
	    ? PAGE_ALIGN(size) : size;
	page_order = order - PAGE_SHIFT > 0 ? order - PAGE_SHIFT : 0;
	total = PAGE_SIZE << page_order;

	byte_count = 0;
	agp_offset = dev->agp->base + request.agp_start;

	DRM_DEBUG("count:      %d\n", count);
	DRM_DEBUG("order:      %d\n", order);
	DRM_DEBUG("size:       %d\n", size);
	DRM_DEBUG("agp_offset: %lu\n", agp_offset);
	DRM_DEBUG("alignment:  %d\n", alignment);
	DRM_DEBUG("page_order: %d\n", page_order);
	DRM_DEBUG("total:      %d\n", total);

	if (order < DRM_MIN_ORDER || order > DRM_MAX_ORDER)
		return -EINVAL;
	if (dev->queue_count)
		return -EBUSY;	/* Not while in use */

	spin_lock(&dev->count_lock);
	if (dev->buf_use) {
		spin_unlock(&dev->count_lock);
		return -EBUSY;
	}
	atomic_inc(&dev->buf_alloc);
	spin_unlock(&dev->count_lock);

	down(&dev->struct_sem);
	entry = &dma->bufs[order];
	if (entry->buf_count) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;	/* May only call once for each order */
	}

	if (count < 0 || count > 4096) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -EINVAL;
	}

	entry->buflist = drm_alloc(count * sizeof(*entry->buflist),
				   DRM_MEM_BUFS);
	if (!entry->buflist) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	memset(entry->buflist, 0, count * sizeof(*entry->buflist));

	entry->buf_size = size;
	entry->page_order = page_order;

	offset = 0;

	while (entry->buf_count < count) {
		buf = &entry->buflist[entry->buf_count];
		buf->idx = dma->buf_count + entry->buf_count;
		buf->total = alignment;
		buf->order = order;
		buf->used = 0;

		buf->offset = (dma->byte_count + offset);
		buf->bus_address = agp_offset + offset;
		buf->address = (void *)(agp_offset + offset);
		buf->next = NULL;
		buf->waiting = 0;
		buf->pending = 0;
		init_waitqueue_head(&buf->dma_wait);
		buf->filp = NULL;

		buf->dev_priv_size = dev->driver->dev_priv_size;
		buf->dev_private = drm_alloc(buf->dev_priv_size, DRM_MEM_BUFS);
		if (!buf->dev_private) {
			/* Set count correctly so we free the proper amount. */
			entry->buf_count = count;
			drm_cleanup_buf_error(dev, entry);
			up(&dev->struct_sem);
			atomic_dec(&dev->buf_alloc);
			return -ENOMEM;
		}
		memset(buf->dev_private, 0, buf->dev_priv_size);

		DRM_DEBUG("buffer %d @ %p\n", entry->buf_count, buf->address);

		offset += alignment;
		entry->buf_count++;
		byte_count += PAGE_SIZE << page_order;
	}

	DRM_DEBUG("byte_count: %d\n", byte_count);

	temp_buflist = drm_realloc(dma->buflist,
				   dma->buf_count * sizeof(*dma->buflist),
				   (dma->buf_count + entry->buf_count)
				   * sizeof(*dma->buflist), DRM_MEM_BUFS);
	if (!temp_buflist) {
		/* Free the entry because it isn't valid */
		drm_cleanup_buf_error(dev, entry);
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	dma->buflist = temp_buflist;

	for (i = 0; i < entry->buf_count; i++) {
		dma->buflist[i + dma->buf_count] = &entry->buflist[i];
	}

	dma->buf_count += entry->buf_count;
	dma->byte_count += byte_count;

	DRM_DEBUG("dma->buf_count : %d\n", dma->buf_count);
	DRM_DEBUG("entry->buf_count : %d\n", entry->buf_count);

	up(&dev->struct_sem);

	request.count = entry->buf_count;
	request.size = size;

	if (copy_to_user(argp, &request, sizeof(request)))
		return -EFAULT;

	dma->flags = _DRM_DMA_USE_AGP;

	atomic_dec(&dev->buf_alloc);
	return 0;
}
#endif				/* __OS_HAS_AGP */

int drm_addbufs_pci(struct inode *inode, struct file *filp,
		    unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	drm_device_dma_t *dma = dev->dma;
	drm_buf_desc_t request;
	int count;
	int order;
	int size;
	int total;
	int page_order;
	drm_buf_entry_t *entry;
	unsigned long page;
	drm_buf_t *buf;
	int alignment;
	unsigned long offset;
	int i;
	int byte_count;
	int page_count;
	unsigned long *temp_pagelist;
	drm_buf_t **temp_buflist;
	drm_buf_desc_t __user *argp = (void __user *)arg;

	if (!drm_core_check_feature(dev, DRIVER_PCI_DMA))
		return -EINVAL;

	if (!dma)
		return -EINVAL;

	if (copy_from_user(&request, argp, sizeof(request)))
		return -EFAULT;

	count = request.count;
	order = drm_order(request.size);
	size = 1 << order;

	DRM_DEBUG("count=%d, size=%d (%d), order=%d, queue_count=%d\n",
		  request.count, request.size, size, order, dev->queue_count);

	if (order < DRM_MIN_ORDER || order > DRM_MAX_ORDER)
		return -EINVAL;
	if (dev->queue_count)
		return -EBUSY;	/* Not while in use */

	alignment = (request.flags & _DRM_PAGE_ALIGN)
	    ? PAGE_ALIGN(size) : size;
	page_order = order - PAGE_SHIFT > 0 ? order - PAGE_SHIFT : 0;
	total = PAGE_SIZE << page_order;

	spin_lock(&dev->count_lock);
	if (dev->buf_use) {
		spin_unlock(&dev->count_lock);
		return -EBUSY;
	}
	atomic_inc(&dev->buf_alloc);
	spin_unlock(&dev->count_lock);

	down(&dev->struct_sem);
	entry = &dma->bufs[order];
	if (entry->buf_count) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;	/* May only call once for each order */
	}

	if (count < 0 || count > 4096) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -EINVAL;
	}

	entry->buflist = drm_alloc(count * sizeof(*entry->buflist),
				   DRM_MEM_BUFS);
	if (!entry->buflist) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	memset(entry->buflist, 0, count * sizeof(*entry->buflist));

	entry->seglist = drm_alloc(count * sizeof(*entry->seglist),
				   DRM_MEM_SEGS);
	if (!entry->seglist) {
		drm_free(entry->buflist,
			 count * sizeof(*entry->buflist), DRM_MEM_BUFS);
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	memset(entry->seglist, 0, count * sizeof(*entry->seglist));

	/* Keep the original pagelist until we know all the allocations
	 * have succeeded
	 */
	temp_pagelist = drm_alloc((dma->page_count + (count << page_order))
				  * sizeof(*dma->pagelist), DRM_MEM_PAGES);
	if (!temp_pagelist) {
		drm_free(entry->buflist,
			 count * sizeof(*entry->buflist), DRM_MEM_BUFS);
		drm_free(entry->seglist,
			 count * sizeof(*entry->seglist), DRM_MEM_SEGS);
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	memcpy(temp_pagelist,
	       dma->pagelist, dma->page_count * sizeof(*dma->pagelist));
	DRM_DEBUG("pagelist: %d entries\n",
		  dma->page_count + (count << page_order));

	entry->buf_size = size;
	entry->page_order = page_order;
	byte_count = 0;
	page_count = 0;

	while (entry->buf_count < count) {
		page = drm_alloc_pages(page_order, DRM_MEM_DMA);
		if (!page) {
			/* Set count correctly so we free the proper amount. */
			entry->buf_count = count;
			entry->seg_count = count;
			drm_cleanup_buf_error(dev, entry);
			drm_free(temp_pagelist,
				 (dma->page_count + (count << page_order))
				 * sizeof(*dma->pagelist), DRM_MEM_PAGES);
			up(&dev->struct_sem);
			atomic_dec(&dev->buf_alloc);
			return -ENOMEM;
		}
		entry->seglist[entry->seg_count++] = page;
		for (i = 0; i < (1 << page_order); i++) {
			DRM_DEBUG("page %d @ 0x%08lx\n",
				  dma->page_count + page_count,
				  page + PAGE_SIZE * i);
			temp_pagelist[dma->page_count + page_count++]
			    = page + PAGE_SIZE * i;
		}
		for (offset = 0;
		     offset + size <= total && entry->buf_count < count;
		     offset += alignment, ++entry->buf_count) {
			buf = &entry->buflist[entry->buf_count];
			buf->idx = dma->buf_count + entry->buf_count;
			buf->total = alignment;
			buf->order = order;
			buf->used = 0;
			buf->offset = (dma->byte_count + byte_count + offset);
			buf->address = (void *)(page + offset);
			buf->bus_address = virt_to_bus(buf->address);
			buf->next = NULL;
			buf->waiting = 0;
			buf->pending = 0;
			init_waitqueue_head(&buf->dma_wait);
			buf->filp = NULL;

			buf->dev_priv_size = dev->driver->dev_priv_size;
			buf->dev_private = drm_alloc(dev->driver->dev_priv_size,
						     DRM_MEM_BUFS);
			if (!buf->dev_private) {
				/* Set count correctly so we free the proper amount. */
				entry->buf_count = count;
				entry->seg_count = count;
				drm_cleanup_buf_error(dev, entry);
				drm_free(temp_pagelist,
					 (dma->page_count +
					  (count << page_order))
					 * sizeof(*dma->pagelist),
					 DRM_MEM_PAGES);
				up(&dev->struct_sem);
				atomic_dec(&dev->buf_alloc);
				return -ENOMEM;
			}
			memset(buf->dev_private, 0, buf->dev_priv_size);

			DRM_DEBUG("buffer %d @ %p\n",
				  entry->buf_count, buf->address);
		}
		byte_count += PAGE_SIZE << page_order;
	}

	temp_buflist = drm_realloc(dma->buflist,
				   dma->buf_count * sizeof(*dma->buflist),
				   (dma->buf_count + entry->buf_count)
				   * sizeof(*dma->buflist), DRM_MEM_BUFS);
	if (!temp_buflist) {
		/* Free the entry because it isn't valid */
		drm_cleanup_buf_error(dev, entry);
		drm_free(temp_pagelist,
			 (dma->page_count + (count << page_order))
			 * sizeof(*dma->pagelist), DRM_MEM_PAGES);
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	dma->buflist = temp_buflist;

	for (i = 0; i < entry->buf_count; i++) {
		dma->buflist[i + dma->buf_count] = &entry->buflist[i];
	}

	/* No allocations failed, so now we can replace the orginal pagelist
	 * with the new one.
	 */
	if (dma->page_count) {
		drm_free(dma->pagelist,
			 dma->page_count * sizeof(*dma->pagelist),
			 DRM_MEM_PAGES);
	}
	dma->pagelist = temp_pagelist;

	dma->buf_count += entry->buf_count;
	dma->seg_count += entry->seg_count;
	dma->page_count += entry->seg_count << page_order;
	dma->byte_count += PAGE_SIZE * (entry->seg_count << page_order);

	up(&dev->struct_sem);

	request.count = entry->buf_count;
	request.size = size;

	if (copy_to_user(argp, &request, sizeof(request)))
		return -EFAULT;

	atomic_dec(&dev->buf_alloc);
	return 0;

}

int drm_addbufs_sg(struct inode *inode, struct file *filp,
		   unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	drm_device_dma_t *dma = dev->dma;
	drm_buf_desc_t __user *argp = (void __user *)arg;
	drm_buf_desc_t request;
	drm_buf_entry_t *entry;
	drm_buf_t *buf;
	unsigned long offset;
	unsigned long agp_offset;
	int count;
	int order;
	int size;
	int alignment;
	int page_order;
	int total;
	int byte_count;
	int i;
	drm_buf_t **temp_buflist;

	if (!drm_core_check_feature(dev, DRIVER_SG))
		return -EINVAL;

	if (!dma)
		return -EINVAL;

	if (copy_from_user(&request, argp, sizeof(request)))
		return -EFAULT;

	count = request.count;
	order = drm_order(request.size);
	size = 1 << order;

	alignment = (request.flags & _DRM_PAGE_ALIGN)
	    ? PAGE_ALIGN(size) : size;
	page_order = order - PAGE_SHIFT > 0 ? order - PAGE_SHIFT : 0;
	total = PAGE_SIZE << page_order;

	byte_count = 0;
	agp_offset = request.agp_start;

	DRM_DEBUG("count:      %d\n", count);
	DRM_DEBUG("order:      %d\n", order);
	DRM_DEBUG("size:       %d\n", size);
	DRM_DEBUG("agp_offset: %lu\n", agp_offset);
	DRM_DEBUG("alignment:  %d\n", alignment);
	DRM_DEBUG("page_order: %d\n", page_order);
	DRM_DEBUG("total:      %d\n", total);

	if (order < DRM_MIN_ORDER || order > DRM_MAX_ORDER)
		return -EINVAL;
	if (dev->queue_count)
		return -EBUSY;	/* Not while in use */

	spin_lock(&dev->count_lock);
	if (dev->buf_use) {
		spin_unlock(&dev->count_lock);
		return -EBUSY;
	}
	atomic_inc(&dev->buf_alloc);
	spin_unlock(&dev->count_lock);

	down(&dev->struct_sem);
	entry = &dma->bufs[order];
	if (entry->buf_count) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;	/* May only call once for each order */
	}

	if (count < 0 || count > 4096) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -EINVAL;
	}

	entry->buflist = drm_alloc(count * sizeof(*entry->buflist),
				   DRM_MEM_BUFS);
	if (!entry->buflist) {
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	memset(entry->buflist, 0, count * sizeof(*entry->buflist));

	entry->buf_size = size;
	entry->page_order = page_order;

	offset = 0;

	while (entry->buf_count < count) {
		buf = &entry->buflist[entry->buf_count];
		buf->idx = dma->buf_count + entry->buf_count;
		buf->total = alignment;
		buf->order = order;
		buf->used = 0;

		buf->offset = (dma->byte_count + offset);
		buf->bus_address = agp_offset + offset;
		buf->address = (void *)(agp_offset + offset + dev->sg->handle);
		buf->next = NULL;
		buf->waiting = 0;
		buf->pending = 0;
		init_waitqueue_head(&buf->dma_wait);
		buf->filp = NULL;

		buf->dev_priv_size = dev->driver->dev_priv_size;
		buf->dev_private = drm_alloc(dev->driver->dev_priv_size,
					     DRM_MEM_BUFS);
		if (!buf->dev_private) {
			/* Set count correctly so we free the proper amount. */
			entry->buf_count = count;
			drm_cleanup_buf_error(dev, entry);
			up(&dev->struct_sem);
			atomic_dec(&dev->buf_alloc);
			return -ENOMEM;
		}

		memset(buf->dev_private, 0, buf->dev_priv_size);

		DRM_DEBUG("buffer %d @ %p\n", entry->buf_count, buf->address);

		offset += alignment;
		entry->buf_count++;
		byte_count += PAGE_SIZE << page_order;
	}

	DRM_DEBUG("byte_count: %d\n", byte_count);

	temp_buflist = drm_realloc(dma->buflist,
				   dma->buf_count * sizeof(*dma->buflist),
				   (dma->buf_count + entry->buf_count)
				   * sizeof(*dma->buflist), DRM_MEM_BUFS);
	if (!temp_buflist) {
		/* Free the entry because it isn't valid */
		drm_cleanup_buf_error(dev, entry);
		up(&dev->struct_sem);
		atomic_dec(&dev->buf_alloc);
		return -ENOMEM;
	}
	dma->buflist = temp_buflist;

	for (i = 0; i < entry->buf_count; i++) {
		dma->buflist[i + dma->buf_count] = &entry->buflist[i];
	}

	dma->buf_count += entry->buf_count;
	dma->byte_count += byte_count;

	DRM_DEBUG("dma->buf_count : %d\n", dma->buf_count);
	DRM_DEBUG("entry->buf_count : %d\n", entry->buf_count);

	up(&dev->struct_sem);

	request.count = entry->buf_count;
	request.size = size;

	if (copy_to_user(argp, &request, sizeof(request)))
		return -EFAULT;

	dma->flags = _DRM_DMA_USE_SG;

	atomic_dec(&dev->buf_alloc);
	return 0;
}

/**
 * Add buffers for DMA transfers (ioctl).
 *
 * \param inode device inode.
 * \param filp file pointer.
 * \param cmd command.
 * \param arg pointer to a drm_buf_desc_t request.
 * \return zero on success or a negative number on failure.
 *
 * According with the memory type specified in drm_buf_desc::flags and the
 * build options, it dispatches the call either to addbufs_agp(),
 * addbufs_sg() or addbufs_pci() for AGP, scatter-gather or consistent
 * PCI memory respectively.
 */
int drm_addbufs(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	drm_buf_desc_t request;
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;

	if (!drm_core_check_feature(dev, DRIVER_HAVE_DMA))
		return -EINVAL;

	if (copy_from_user(&request, (drm_buf_desc_t __user *) arg,
			   sizeof(request)))
		return -EFAULT;

#if __OS_HAS_AGP
	if (request.flags & _DRM_AGP_BUFFER)
		return drm_addbufs_agp(inode, filp, cmd, arg);
	else
#endif
	if (request.flags & _DRM_SG_BUFFER)
		return drm_addbufs_sg(inode, filp, cmd, arg);
	else
		return drm_addbufs_pci(inode, filp, cmd, arg);
}

/**
 * Get information about the buffer mappings.
 *
 * This was originally mean for debugging purposes, or by a sophisticated
 * client library to determine how best to use the available buffers (e.g.,
 * large buffers can be used for image transfer).
 *
 * \param inode device inode.
 * \param filp file pointer.
 * \param cmd command.
 * \param arg pointer to a drm_buf_info structure.
 * \return zero on success or a negative number on failure.
 *
 * Increments drm_device::buf_use while holding the drm_device::count_lock
 * lock, preventing of allocating more buffers after this call. Information
 * about each requested buffer is then copied into user space.
 */
int drm_infobufs(struct inode *inode, struct file *filp,
		 unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	drm_device_dma_t *dma = dev->dma;
	drm_buf_info_t request;
	drm_buf_info_t __user *argp = (void __user *)arg;
	int i;
	int count;

	if (!drm_core_check_feature(dev, DRIVER_HAVE_DMA))
		return -EINVAL;

	if (!dma)
		return -EINVAL;

	spin_lock(&dev->count_lock);
	if (atomic_read(&dev->buf_alloc)) {
		spin_unlock(&dev->count_lock);
		return -EBUSY;
	}
	++dev->buf_use;		/* Can't allocate more after this call */
	spin_unlock(&dev->count_lock);

	if (copy_from_user(&request, argp, sizeof(request)))
		return -EFAULT;

	for (i = 0, count = 0; i < DRM_MAX_ORDER + 1; i++) {
		if (dma->bufs[i].buf_count)
			++count;
	}

	DRM_DEBUG("count = %d\n", count);

	if (request.count >= count) {
		for (i = 0, count = 0; i < DRM_MAX_ORDER + 1; i++) {
			if (dma->bufs[i].buf_count) {
				drm_buf_desc_t __user *to =
				    &request.list[count];
				drm_buf_entry_t *from = &dma->bufs[i];
				drm_freelist_t *list = &dma->bufs[i].freelist;
				if (copy_to_user(&to->count,
						 &from->buf_count,
						 sizeof(from->buf_count)) ||
				    copy_to_user(&to->size,
						 &from->buf_size,
						 sizeof(from->buf_size)) ||
				    copy_to_user(&to->low_mark,
						 &list->low_mark,
						 sizeof(list->low_mark)) ||
				    copy_to_user(&to->high_mark,
						 &list->high_mark,
						 sizeof(list->high_mark)))
					return -EFAULT;

				DRM_DEBUG("%d %d %d %d %d\n",
					  i,
					  dma->bufs[i].buf_count,
					  dma->bufs[i].buf_size,
					  dma->bufs[i].freelist.low_mark,
					  dma->bufs[i].freelist.high_mark);
				++count;
			}
		}
	}
	request.count = count;

	if (copy_to_user(argp, &request, sizeof(request)))
		return -EFAULT;

	return 0;
}

/**
 * Specifies a low and high water mark for buffer allocation
 *
 * \param inode device inode.
 * \param filp file pointer.
 * \param cmd command.
 * \param arg a pointer to a drm_buf_desc structure.
 * \return zero on success or a negative number on failure.
 *
 * Verifies that the size order is bounded between the admissible orders and
 * updates the respective drm_device_dma::bufs entry low and high water mark.
 *
 * \note This ioctl is deprecated and mostly never used.
 */
int drm_markbufs(struct inode *inode, struct file *filp,
		 unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	drm_device_dma_t *dma = dev->dma;
	drm_buf_desc_t request;
	int order;
	drm_buf_entry_t *entry;

	if (!drm_core_check_feature(dev, DRIVER_HAVE_DMA))
		return -EINVAL;

	if (!dma)
		return -EINVAL;

	if (copy_from_user(&request,
			   (drm_buf_desc_t __user *) arg, sizeof(request)))
		return -EFAULT;

	DRM_DEBUG("%d, %d, %d\n",
		  request.size, request.low_mark, request.high_mark);
	order = drm_order(request.size);
	if (order < DRM_MIN_ORDER || order > DRM_MAX_ORDER)
		return -EINVAL;
	entry = &dma->bufs[order];

	if (request.low_mark < 0 || request.low_mark > entry->buf_count)
		return -EINVAL;
	if (request.high_mark < 0 || request.high_mark > entry->buf_count)
		return -EINVAL;

	entry->freelist.low_mark = request.low_mark;
	entry->freelist.high_mark = request.high_mark;

	return 0;
}

/**
 * Unreserve the buffers in list, previously reserved using drmDMA.
 *
 * \param inode device inode.
 * \param filp file pointer.
 * \param cmd command.
 * \param arg pointer to a drm_buf_free structure.
 * \return zero on success or a negative number on failure.
 *
 * Calls free_buffer() for each used buffer.
 * This function is primarily used for debugging.
 */
int drm_freebufs(struct inode *inode, struct file *filp,
		 unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	drm_device_dma_t *dma = dev->dma;
	drm_buf_free_t request;
	int i;
	int idx;
	drm_buf_t *buf;

	if (!drm_core_check_feature(dev, DRIVER_HAVE_DMA))
		return -EINVAL;

	if (!dma)
		return -EINVAL;

	if (copy_from_user(&request,
			   (drm_buf_free_t __user *) arg, sizeof(request)))
		return -EFAULT;

	DRM_DEBUG("%d\n", request.count);
	for (i = 0; i < request.count; i++) {
		if (copy_from_user(&idx, &request.list[i], sizeof(idx)))
			return -EFAULT;
		if (idx < 0 || idx >= dma->buf_count) {
			DRM_ERROR("Index %d (of %d max)\n",
				  idx, dma->buf_count - 1);
			return -EINVAL;
		}
		buf = dma->buflist[idx];
		if (buf->filp != filp) {
			DRM_ERROR("Process %d freeing buffer not owned\n",
				  current->pid);
			return -EINVAL;
		}
		drm_free_buffer(dev, buf);
	}

	return 0;
}

/**
 * Maps all of the DMA buffers into client-virtual space (ioctl).
 *
 * \param inode device inode.
 * \param filp file pointer.
 * \param cmd command.
 * \param arg pointer to a drm_buf_map structure.
 * \return zero on success or a negative number on failure.
 *
 * Maps the AGP or SG buffer region with do_mmap(), and copies information
 * about each buffer into user space. The PCI buffers are already mapped on the
 * addbufs_pci() call.
 */
int drm_mapbufs(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	drm_file_t *priv = filp->private_data;
	drm_device_t *dev = priv->head->dev;
	drm_device_dma_t *dma = dev->dma;
	drm_buf_map_t __user *argp = (void __user *)arg;
	int retcode = 0;
	const int zero = 0;
	unsigned long virtual;
	unsigned long address;
	drm_buf_map_t request;
	int i;

	if (!drm_core_check_feature(dev, DRIVER_HAVE_DMA))
		return -EINVAL;

	if (!dma)
		return -EINVAL;

	spin_lock(&dev->count_lock);
	if (atomic_read(&dev->buf_alloc)) {
		spin_unlock(&dev->count_lock);
		return -EBUSY;
	}
	dev->buf_use++;		/* Can't allocate more after this call */
	spin_unlock(&dev->count_lock);

	if (copy_from_user(&request, argp, sizeof(request)))
		return -EFAULT;

	if (request.count >= dma->buf_count) {
		if ((drm_core_has_AGP(dev) && (dma->flags & _DRM_DMA_USE_AGP))
		    || (drm_core_check_feature(dev, DRIVER_SG)
			&& (dma->flags & _DRM_DMA_USE_SG))) {
			drm_map_t *map = dev->agp_buffer_map;

			if (!map) {
				retcode = -EINVAL;
				goto done;
			}
#if LINUX_VERSION_CODE <= 0x020402
			down(&current->mm->mmap_sem);
#else
			down_write(&current->mm->mmap_sem);
#endif
			virtual = do_mmap(filp, 0, map->size,
					  PROT_READ | PROT_WRITE,
					  MAP_SHARED,
					  (unsigned long)map->offset);
#if LINUX_VERSION_CODE <= 0x020402
			up(&current->mm->mmap_sem);
#else
			up_write(&current->mm->mmap_sem);
#endif
		} else {
#if LINUX_VERSION_CODE <= 0x020402
			down(&current->mm->mmap_sem);
#else
			down_write(&current->mm->mmap_sem);
#endif
			virtual = do_mmap(filp, 0, dma->byte_count,
					  PROT_READ | PROT_WRITE,
					  MAP_SHARED, 0);
#if LINUX_VERSION_CODE <= 0x020402
			up(&current->mm->mmap_sem);
#else
			up_write(&current->mm->mmap_sem);
#endif
		}
		if (virtual > -1024UL) {
			/* Real error */
			retcode = (signed long)virtual;
			goto done;
		}
		request.virtual = (void __user *)virtual;

		for (i = 0; i < dma->buf_count; i++) {
			if (copy_to_user(&request.list[i].idx,
					 &dma->buflist[i]->idx,
					 sizeof(request.list[0].idx))) {
				retcode = -EFAULT;
				goto done;
			}
			if (copy_to_user(&request.list[i].total,
					 &dma->buflist[i]->total,
					 sizeof(request.list[0].total))) {
				retcode = -EFAULT;
				goto done;
			}
			if (copy_to_user(&request.list[i].used,
					 &zero, sizeof(zero))) {
				retcode = -EFAULT;
				goto done;
			}
			address = virtual + dma->buflist[i]->offset;	/* *** */
			if (copy_to_user(&request.list[i].address,
					 &address, sizeof(address))) {
				retcode = -EFAULT;
				goto done;
			}
		}
	}
      done:
	request.count = dma->buf_count;
	DRM_DEBUG("%d buffers, retcode = %d\n", request.count, retcode);

	if (copy_to_user(argp, &request, sizeof(request)))
		return -EFAULT;

	return retcode;
}

/**
 * Compute size order.  Returns the exponent of the smaller power of two which
 * is greater or equal to given number.
 *
 * \param size size.
 * \return order.
 *
 * \todo Can be made faster.
 */
int drm_order( unsigned long size )
{
	int order;
	unsigned long tmp;

	for (order = 0, tmp = size >> 1; tmp; tmp >>= 1, order++)
		;

	if (size & (size - 1))
		++order;

	return order;
}
EXPORT_SYMBOL(drm_order);

