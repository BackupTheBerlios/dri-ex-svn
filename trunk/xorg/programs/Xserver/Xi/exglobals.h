/* $XFree86: xc/programs/Xserver/Xi/exglobals.h,v 3.2 1996/05/06 05:56:03 dawes Exp $ */
/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/*****************************************************************
 *
 * Globals referenced elsewhere in the server.
 *
 */
#ifndef EXGLOBALS_H
#define EXGLOBALS_H 1

extern	int 	IReqCode;
extern	int	BadDevice;
extern	int	BadEvent;
extern	int	BadMode;
extern	int	DeviceBusy;
extern	int	BadClass;

extern	Mask	DevicePointerMotionMask;
extern	Mask	DevicePointerMotionHintMask;
extern	Mask	DeviceFocusChangeMask;
extern	Mask	DeviceStateNotifyMask;
extern	Mask	ChangeDeviceNotifyMask;
extern	Mask	DeviceMappingNotifyMask;
extern	Mask	DeviceOwnerGrabButtonMask;
extern	Mask	DeviceButtonGrabMask;
extern	Mask	DeviceButtonMotionMask;
extern	Mask	PropagateMask[];

extern	int	DeviceValuator;
extern	int	DeviceKeyPress;
extern	int	DeviceKeyRelease;
extern	int	DeviceButtonPress;
extern	int	DeviceButtonRelease;
extern	int	DeviceMotionNotify;
extern	int	DeviceFocusIn;
extern	int	DeviceFocusOut;
extern	int	ProximityIn;
extern	int	ProximityOut;
extern	int	DeviceStateNotify;
extern	int	DeviceKeyStateNotify;
extern	int	DeviceButtonStateNotify;
extern	int	DeviceMappingNotify;
extern	int	ChangeDeviceNotify;

extern	int	RT_INPUTCLIENT;

#if 0
/* FIXME: in dix */
extern	InputInfo inputInfo;
#endif

#endif /* EXGLOBALS_H */
