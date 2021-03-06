/* $XFree86: xc/programs/Xserver/GL/glx/g_single.c,v 1.4 2001/03/21 16:29:35 dawes Exp $ */
/* DO NOT EDIT - THIS FILE IS AUTOMATICALLY GENERATED */
/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: This software was created using the
** OpenGL(R) version 1.2.1 Sample Implementation published by SGI, but has
** not been independently verified as being compliant with the OpenGL(R)
** version 1.2.1 Specification.
*/

#include "dmx.h"
#include "dmxwindow.h"
#include "dmxpixmap.h"
#include "dmxfont.h"

#undef Xmalloc
#undef Xcalloc
#undef Xrealloc
#undef Xfree

#define NEED_REPLIES
#include "glxserver.h"
#include "glxext.h"
#include "g_disptab.h"
/* #include "g_disptab_EXT.h" */
#include "unpack.h"
#include "glxutil.h"

#include "GL/glxproto.h"

#ifdef PANORAMIX
#include "panoramiXsrv.h"
#endif

/*
 * GetReqVendorPrivate - this is the equivalent of GetReq macro
 *    from Xlibint.h but it does not set the reqType field (the opcode).
 *    this is because the GL single opcodes has different naming convension
 *    the other X opcodes (ie. X_GLsop_GetFloatv).
 */
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define GetReqVendorPrivate(name, req) \
        WORD64ALIGN\
	if ((dpy->bufptr + SIZEOF(x##name##Req)) > dpy->bufmax)\
		_XFlush(dpy);\
	req = (x##name##Req *)(dpy->last_req = dpy->bufptr);\
	req->length = (SIZEOF(x##name##Req))>>2;\
	dpy->bufptr += SIZEOF(x##name##Req);\
	dpy->request++

#else  /* non-ANSI C uses empty comment instead of "##" for token concatenation */
#define GetReqVendorPrivate(name, req) \
        WORD64ALIGN\
	if ((dpy->bufptr + SIZEOF(x/**/name/**/Req)) > dpy->bufmax)\
		_XFlush(dpy);\
	req = (x/**/name/**/Req *)(dpy->last_req = dpy->bufptr);\
	req->length = (SIZEOF(x/**/name/**/Req))>>2;\
	dpy->bufptr += SIZEOF(x/**/name/**/Req);\
	dpy->request++
#endif

extern Display *GetBackEndDisplay( __GLXclientState *cl, int s );
extern int GetCurrentBackEndTag(__GLXclientState *cl, GLXContextTag tag, int s);

static int swap_vec_element_size = 0;

static void SendSwappedReply( ClientPtr client,
                              xGLXVendorPrivReply *reply, 
			      char *buf,
			      int   buf_size )
{
   __GLX_DECLARE_SWAP_VARIABLES;
   __GLX_SWAP_SHORT(&reply->sequenceNumber);
   __GLX_SWAP_INT(&reply->length);
   __GLX_SWAP_INT(&reply->retval);
   __GLX_SWAP_INT(&reply->size);

   if ( (buf_size == 0) && (swap_vec_element_size > 0) ) {
      /*
       * the reply has single component - need to swap pad3
       */
      if (swap_vec_element_size == 2) {
	 __GLX_SWAP_SHORT(&reply->pad3);
      }
      else if (swap_vec_element_size == 4) {
	 __GLX_SWAP_INT(&reply->pad3);
	 __GLX_SWAP_INT(&reply->pad4);
      }
      else if (swap_vec_element_size == 8) {
	 __GLX_SWAP_DOUBLE(&reply->pad3);
      }
   }
   else if ( (buf_size > 0) && (swap_vec_element_size > 0) ) {
      /*
       * the reply has vector of elements which needs to be swapped
       */
      int vsize = buf_size / swap_vec_element_size;
      char *p = buf;
      int i;

      for (i=0; i<vsize; i++) {
	 if (swap_vec_element_size == 2) {
	    __GLX_SWAP_SHORT(p);
	 }
	 else if (swap_vec_element_size == 4) {
	    __GLX_SWAP_INT(p);
	 }
	 else if (swap_vec_element_size == 8) {
	    __GLX_SWAP_DOUBLE(p);
	 }

	 p += swap_vec_element_size;
      }

      __GLX_SWAP_INT(&reply->pad3);
      __GLX_SWAP_INT(&reply->pad4);
      __GLX_SWAP_INT(&reply->pad5);
      __GLX_SWAP_INT(&reply->pad6);

   }

    WriteToClient(client, sizeof(xGLXVendorPrivReply),(char *)reply);
    if (buf_size > 0)
       WriteToClient(client, buf_size, (char *)buf);

}

int __glXVForwardSingleReq( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   xGLXVendorPrivateReq *be_req;
    __GLXcontext *glxc;
   int from_screen = 0;
   int to_screen = 0;
   int buf_size;
   int s;

    glxc = __glXLookupContextByTag(cl, req->contextTag);
    if (!glxc) {
	return 0;
    }
    from_screen = to_screen = glxc->pScreen->myNum;

#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
       from_screen = 0;
       to_screen = screenInfo.numScreens - 1;
    }
#endif

    pc += sz_xGLXVendorPrivateReq;
    buf_size = (req->length << 2) - sz_xGLXVendorPrivateReq;

    /*
     * just forward the request to back-end server(s)
     */
    for (s=from_screen; s<=to_screen; s++) {
       DMXScreenInfo *dmxScreen = &dmxScreens[s];
       Display *dpy = GetBackEndDisplay(cl,s);

       LockDisplay(dpy);
       GetReqVendorPrivate(GLXVendorPrivate,be_req);
       be_req->reqType = dmxScreen->glxMajorOpcode;
       be_req->glxCode = req->glxCode;
       be_req->length = req->length;
       be_req->vendorCode = req->vendorCode;
       be_req->contextTag = GetCurrentBackEndTag(cl,req->contextTag,s);
       if (buf_size > 0) 
	  _XSend(dpy, (const char *)pc, buf_size);
       UnlockDisplay(dpy);
       SyncHandle();
    }

    return Success;
}

int __glXVForwardPipe0WithReply( __GLXclientState *cl, GLbyte *pc )
{
   ClientPtr client = cl->client;
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   xGLXVendorPrivateReq *be_req;
   xGLXVendorPrivReply reply;
   xGLXVendorPrivReply be_reply;
    __GLXcontext *glxc;
   int buf_size;
   char *be_buf;
   int   be_buf_size;
   DMXScreenInfo *dmxScreen;
   Display *dpy;

    glxc = __glXLookupContextByTag(cl, req->contextTag);
    if (!glxc) {
	return __glXBadContext;
    }

    pc += sz_xGLXVendorPrivateReq;
    buf_size = (req->length << 2) - sz_xGLXVendorPrivateReq;

    dmxScreen = &dmxScreens[glxc->pScreen->myNum];
    dpy = GetBackEndDisplay(cl, glxc->pScreen->myNum);

    /* 
     * send the request to the first back-end server
     */
    LockDisplay(dpy);
    GetReqVendorPrivate(GLXVendorPrivate,be_req);
    be_req->reqType = dmxScreen->glxMajorOpcode;
    be_req->glxCode = req->glxCode;
    be_req->length = req->length;
    be_req->vendorCode = req->vendorCode;
    be_req->contextTag = GetCurrentBackEndTag(cl,req->contextTag, glxc->pScreen->myNum);
    if (buf_size > 0) 
       _XSend(dpy, (const char *)pc, buf_size);

    /*
     * get the reply from the back-end server
     */
    _XReply(dpy, (xReply*) &be_reply, 0, False);
    be_buf_size = be_reply.length << 2;
    if (be_buf_size > 0) {
       be_buf = (char *)Xalloc( be_buf_size );
       if (be_buf) {
	  _XRead(dpy, be_buf, be_buf_size);
       }
       else {
	  /* Throw data on the floor */
	  _XEatData(dpy, be_buf_size);
	  return BadAlloc;
       }
    }

    UnlockDisplay(dpy);
    SyncHandle();

    /*
     * send the reply to the client
     */
    memcpy( &reply, &be_reply, sz_xGLXVendorPrivReply );
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if (client->swapped) {
       SendSwappedReply( client, &reply, be_buf, be_buf_size );
    }
    else {
       WriteToClient(client, sizeof(xGLXVendorPrivReply),(char *)&reply);
       if (be_buf_size > 0)
	  WriteToClient(client, be_buf_size, (char *)be_buf);
    }

    if (be_buf_size > 0) Xfree(be_buf);

    return Success;
}

int __glXVForwardAllWithReply( __GLXclientState *cl, GLbyte *pc )
{
   ClientPtr client = cl->client;
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   xGLXVendorPrivateReq *be_req;
   xGLXVendorPrivReply reply;
   xGLXVendorPrivReply be_reply;
    __GLXcontext *glxc;
   int buf_size;
   char *be_buf;
   int   be_buf_size;
   int from_screen = 0;
   int to_screen = 0;
   int s;

   DMXScreenInfo *dmxScreen;
   Display *dpy;

    glxc = __glXLookupContextByTag(cl, req->contextTag);
    if (!glxc) {
	return 0;
    }
    from_screen = to_screen = glxc->pScreen->myNum;

#ifdef PANORAMIX
    if (!noPanoramiXExtension) {
       from_screen = 0;
       to_screen = screenInfo.numScreens - 1;
    }
#endif

    pc += sz_xGLXVendorPrivateReq;
    buf_size = (req->length << 2) - sz_xGLXVendorPrivateReq;

    /* 
     * send the request to the first back-end server(s)
     */
    for (s=to_screen; s>=from_screen; s--) {
       dmxScreen = &dmxScreens[s];
       dpy = GetBackEndDisplay(cl,s);

       LockDisplay(dpy);
       GetReqVendorPrivate(GLXVendorPrivate,be_req);
       be_req->reqType = dmxScreen->glxMajorOpcode;
       be_req->glxCode = req->glxCode;
       be_req->length = req->length;
       be_req->vendorCode = req->vendorCode;
       be_req->contextTag = GetCurrentBackEndTag(cl,req->contextTag,s);
       if (buf_size > 0) 
	  _XSend(dpy, (const char *)pc, buf_size);

       /*
	* get the reply from the back-end server
	*/
       _XReply(dpy, (xReply*) &be_reply, 0, False);
       be_buf_size = be_reply.length << 2;
       if (be_buf_size > 0) {
	  be_buf = (char *)Xalloc( be_buf_size );
	  if (be_buf) {
	     _XRead(dpy, be_buf, be_buf_size);
	  }
	  else {
	     /* Throw data on the floor */
	     _XEatData(dpy, be_buf_size);
	     return BadAlloc;
	  }
       }

       UnlockDisplay(dpy);
       SyncHandle();

       if (s > from_screen && be_buf_size > 0) {
	  Xfree(be_buf);
       }
    }

    /*
     * send the reply to the client
     */
    memcpy( &reply, &be_reply, sz_xGLXVendorPrivReply );
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if (client->swapped) {
       SendSwappedReply( client, &reply, be_buf, be_buf_size );
    }
    else {
       WriteToClient(client, sizeof(xGLXVendorPrivReply),(char *)&reply);
       if (be_buf_size > 0)
	  WriteToClient(client, be_buf_size, (char *)be_buf);
    }

    if (be_buf_size > 0) Xfree(be_buf);

    return Success;
}

int __glXVForwardSingleReqSwap( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 0;

   return( __glXVForwardSingleReq( cl, pc ) );
}

int __glXVForwardPipe0WithReplySwap( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 0;

   /*
    * swap extra data in request - assuming all data
    * (if available) are arrays of 4 bytes components !
    */
   if (req->length > sz_xGLXVendorPrivateReq/4) {
      int *data = (int *)(req+1);
      int count = req->length - sz_xGLXVendorPrivateReq/4;
      __GLX_SWAP_INT_ARRAY(data, count );
   }

   return( __glXVForwardPipe0WithReply( cl, pc ) );
}

int __glXVForwardPipe0WithReplySwapsv( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 2;

   /*
    * swap extra data in request - assuming all data
    * (if available) are arrays of 4 bytes components !
    */
   if (req->length > sz_xGLXVendorPrivateReq/4) {
      int *data = (int *)(req+1);
      int count = req->length - sz_xGLXVendorPrivateReq/4;
      __GLX_SWAP_INT_ARRAY(data, count );
   }

   return( __glXVForwardPipe0WithReply( cl, pc ) );
}

int __glXVForwardPipe0WithReplySwapiv( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 4;

   /*
    * swap extra data in request - assuming all data
    * (if available) are arrays of 4 bytes components !
    */
   if (req->length > sz_xGLXVendorPrivateReq/4) {
      int *data = (int *)(req+1);
      int count = req->length - sz_xGLXVendorPrivateReq/4;
      __GLX_SWAP_INT_ARRAY(data, count );
   }

   return( __glXVForwardPipe0WithReply( cl, pc ) );
}

int __glXVForwardPipe0WithReplySwapdv( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 8;

   /*
    * swap extra data in request - assuming all data
    * (if available) are arrays of 4 bytes components !
    */
   if (req->length > sz_xGLXVendorPrivateReq/4) {
      int *data = (int *)(req+1);
      int count = req->length - sz_xGLXVendorPrivateReq/4;
      __GLX_SWAP_INT_ARRAY(data, count );
   }

   return( __glXVForwardPipe0WithReply( cl, pc ) );
}

int __glXVForwardAllWithReplySwap( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 0;

   /*
    * swap extra data in request - assuming all data
    * (if available) are arrays of 4 bytes components !
    */
   if (req->length > sz_xGLXVendorPrivateReq/4) {
      int *data = (int *)(req+1);
      int count = req->length - sz_xGLXVendorPrivateReq/4;
      __GLX_SWAP_INT_ARRAY(data, count );
   }

   return( __glXVForwardAllWithReply( cl, pc ) );
}

int __glXVForwardAllWithReplySwapsv( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 2;

   /*
    * swap extra data in request - assuming all data
    * (if available) are arrays of 4 bytes components !
    */
   if (req->length > sz_xGLXVendorPrivateReq/4) {
      int *data = (int *)(req+1);
      int count = req->length - sz_xGLXVendorPrivateReq/4;
      __GLX_SWAP_INT_ARRAY(data, count );
   }

   return( __glXVForwardAllWithReply( cl, pc ) );
}

int __glXVForwardAllWithReplySwapiv( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 4;

   /*
    * swap extra data in request - assuming all data
    * (if available) are arrays of 4 bytes components !
    */
   if (req->length > sz_xGLXVendorPrivateReq/4) {
      int *data = (int *)(req+1);
      int count = req->length - sz_xGLXVendorPrivateReq/4;
      __GLX_SWAP_INT_ARRAY(data, count );
   }

   return( __glXVForwardAllWithReply( cl, pc ) );
}

int __glXVForwardAllWithReplySwapdv( __GLXclientState *cl, GLbyte *pc )
{
   xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *)pc;
   __GLX_DECLARE_SWAP_VARIABLES;

   __GLX_SWAP_SHORT(&req->length);
   __GLX_SWAP_INT(&req->vendorCode);
   __GLX_SWAP_INT(&req->contextTag);

   swap_vec_element_size = 8;

   /*
    * swap extra data in request - assuming all data
    * (if available) are arrays of 4 bytes components !
    */
   if (req->length > sz_xGLXVendorPrivateReq/4) {
      int *data = (int *)(req+1);
      int count = req->length - sz_xGLXVendorPrivateReq/4;
      __GLX_SWAP_INT_ARRAY(data, count );
   }

   return( __glXVForwardAllWithReply( cl, pc ) );
}

