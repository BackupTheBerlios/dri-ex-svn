/* $XFree86: xc/programs/Xserver/Xi/getdctl.h,v 3.1 1996/04/15 11:18:38 dawes Exp $ */
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

#ifndef GETDCTL_H
#define GETDCTL_H 1

int
SProcXGetDeviceControl (
	ClientPtr              /* client */
	);

int
ProcXGetDeviceControl (
	ClientPtr              /* client */
	);

void
CopySwapDeviceResolution (
	ClientPtr              /* client */,
	ValuatorClassPtr       /* v */,
	char *                 /* buf */,
	int                    /* length */
	);

void
SRepXGetDeviceControl (
	ClientPtr              /* client */,
	int                    /* size */,
	xGetDeviceControlReply * /* rep */
	);

#endif /* GETDCTL_H */
