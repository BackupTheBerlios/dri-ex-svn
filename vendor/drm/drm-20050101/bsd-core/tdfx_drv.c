/* tdfx_drv.c -- tdfx driver -*- linux-c -*-
 * Created: Thu Oct  7 10:38:32 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Rickard E. (Rik) Faith <faith@valinux.com>
 *    Daryll Strauss <daryll@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 *
 */

#include "tdfx_drv.h"
#include "drmP.h"
#include "drm_pciids.h"

/* drv_PCI_IDs comes from drm_pciids.h, generated from drm_pciids.txt. */
static drm_pci_id_list_t tdfx_pciidlist[] = {
	tdfx_PCI_IDS
};

extern drm_ioctl_desc_t tdfx_ioctls[];
extern int tdfx_max_ioctl;

static void tdfx_configure(drm_device_t *dev)
{
	dev->dev_priv_size = 1; /* No dev_priv */

	dev->max_driver_ioctl = 0;

	dev->driver_name = DRIVER_NAME;
	dev->driver_desc = DRIVER_DESC;
	dev->driver_date = DRIVER_DATE;
	dev->driver_major = DRIVER_MAJOR;
	dev->driver_minor = DRIVER_MINOR;
	dev->driver_patchlevel = DRIVER_PATCHLEVEL;

	dev->use_mtrr = 1;
}

#ifdef __FreeBSD__
static int
tdfx_probe(device_t dev)
{
	return drm_probe(dev, tdfx_pciidlist);
}

static int
tdfx_attach(device_t nbdev)
{
	drm_device_t *dev = device_get_softc(nbdev);

	bzero(dev, sizeof(drm_device_t));
	tdfx_configure(dev);
	return drm_attach(nbdev, tdfx_pciidlist);
}

static device_method_t tdfx_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,		tdfx_probe),
	DEVMETHOD(device_attach,	tdfx_attach),
	DEVMETHOD(device_detach,	drm_detach),

	{ 0, 0 }
};

static driver_t tdfx_driver = {
	"drm",
	tdfx_methods,
	sizeof(drm_device_t)
};

extern devclass_t drm_devclass;
DRIVER_MODULE(tdfx, pci, tdfx_driver, drm_devclass, 0, 0);
MODULE_DEPEND(tdfx, drm, 1, 1, 1);

#elif defined(__NetBSD__) || defined(__OpenBSD__)
CFDRIVER_DECL(tdfx, DV_TTY, NULL);
#endif
