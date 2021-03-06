#if !defined(_WINWINDOW_H_)
#define _WINWINDOW_H_
/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Kensuke Matsuzaki
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winwindow.h,v 1.3 2003/10/02 13:30:11 eich Exp $ */

#ifndef NO
#define NO			0
#endif
#ifndef YES
#define YES			1
#endif

/* Constant strings */
#define WINDOW_CLASS		"cygwin/x"
#define WINDOW_TITLE		"Cygwin/X - %s:%d"
#define WINDOW_TITLE_XDMCP	"Cygwin/X - %s"
#define WIN_SCR_PROP		"cyg_screen_prop rl"
#define WINDOW_CLASS_X		"cygwin/x X rl"
#define WINDOW_TITLE_X		"Cygwin/X X"
#define WIN_WINDOW_PROP		"cyg_window_prop_rl"
#ifdef HAS_DEVWINDOWS
# define WIN_MSG_QUEUE_FNAME	"/dev/windows"
#endif
#define WIN_WID_PROP		"cyg_wid_prop_rl"
#define WIN_NEEDMANAGE_PROP	"cyg_override_redirect_prop_rl"
#ifndef CYGMULTIWINDOW_DEBUG
#define CYGMULTIWINDOW_DEBUG    NO
#endif
#ifndef CYGWINDOWING_DEBUG
#define CYGWINDOWING_DEBUG	NO
#endif

typedef struct _winPrivScreenRec *winPrivScreenPtr;


/*
 * Window privates
 */

typedef struct
{
  DWORD			dwDummy;
  HRGN			hRgn;
  HWND			hWnd;
  winPrivScreenPtr	pScreenPriv;
  Bool			fXKilled;

  /* Privates used by primary fb DirectDraw server */
  LPDDSURFACEDESC	pddsdPrimary;

  /* Privates used by shadow fb DirectDraw Nonlocking server */
  LPDIRECTDRAWSURFACE4	pddsPrimary4;

  /* Privates used by both shadow fb DirectDraw servers */
  LPDIRECTDRAWCLIPPER	pddcPrimary;
} winPrivWinRec, *winPrivWinPtr;

#ifdef XWIN_MULTIWINDOW
typedef struct _winWMMessageRec{
  DWORD			dwID;
  DWORD			msg;
  int			iWindow;
  HWND			hwndWindow;
  int			iX, iY;
  int			iWidth, iHeight;
} winWMMessageRec, *winWMMessagePtr;


/*
 * winmultiwindowwm.c
 */

#define		WM_WM_MOVE		(WM_USER + 1)
#define		WM_WM_SIZE		(WM_USER + 2)
#define		WM_WM_RAISE		(WM_USER + 3)
#define		WM_WM_LOWER		(WM_USER + 4)
#define		WM_WM_MAP		(WM_USER + 5)
#define		WM_WM_UNMAP		(WM_USER + 6)
#define		WM_WM_KILL		(WM_USER + 7)
#define		WM_WM_ACTIVATE		(WM_USER + 8)
#define		WM_WM_NAME_EVENT	(WM_USER + 9)
#define		WM_WM_HINTS_EVENT	(WM_USER + 10)
#define		WM_WM_CHANGE_STATE	(WM_USER + 11)
#define		WM_MANAGE		(WM_USER + 100)
#define		WM_UNMANAGE		(WM_USER + 102)

void
winSendMessageToWM (void *pWMInfo, winWMMessagePtr msg);

Bool
winInitWM (void **ppWMInfo,
	   pthread_t *ptWMProc,
	   pthread_t *ptXMsgProc,
	   pthread_mutex_t *ppmServerStarted,
	   int dwScreen,
	   HWND hwndScreen);

void
winDeinitMultiWindowWM (void);

void
winMinimizeWindow (Window id);


/*
 * winmultiwindowicons.c
 */

void
winUpdateIcon (Window id);

void 
winInitGlobalIcons (void);

void 
winDestroyIcon(HICON hIcon);

#endif /* XWIN_MULTIWINDOW */
#endif
