/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/getvalidbios.c,v 1.1 2000/02/13 03:06:38 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

memType
getValidBIOSBase(PCITAG tag, int num)
{
    return 0;
}
