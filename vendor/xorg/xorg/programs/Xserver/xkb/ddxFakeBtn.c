/* $Xorg: ddxFakeBtn.c,v 1.3 2000/08/17 19:53:45 cpqbld Exp $ */
/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be 
used in advertising or publicity pertaining to distribution 
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability 
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/
/* $XFree86$ */

#include <stdio.h>
#define	NEED_EVENTS 1
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include "inputstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "XKBsrv.h"
#include "XI.h"

void
XkbDDXFakePointerButton(int event,int button)
{
xEvent	ev;
int	x,y;
DevicePtr ptr;

    if ((ptr = LookupPointerDevice())==NULL)
	return;
    GetSpritePosition(&x,&y);
    ev.u.u.type = event;
    ev.u.u.detail = button;
    ev.u.keyButtonPointer.time = GetTimeInMillis();
    ev.u.keyButtonPointer.rootX = x;
    ev.u.keyButtonPointer.rootY = y;
    (*ptr->processInputProc)( &ev, (DeviceIntPtr)ptr, 1 );
    return;
}
