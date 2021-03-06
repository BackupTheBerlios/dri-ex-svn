/* $XFree86$ */
/*  This is the xf86 module code for the DEC_XTRAP extension.
 */

#include "xf86Module.h"

#include <X11/extensions/xtrapdi.h>

extern void DEC_XTRAPInit(INITARGS);

#ifdef XFree86LOADER

static MODULESETUPPROTO(xtrapSetup);

ExtensionModule xtrapExt =
{
    DEC_XTRAPInit,
    XTrapExtName,
    NULL,
    NULL,
    NULL
};

static XF86ModuleVersionInfo xtrapVersRec =
{
    "xtrap",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_EXTENSION,         /* needs the server extension ABI */
    ABI_EXTENSION_VERSION,
    MOD_CLASS_EXTENSION,
    {0,0,0,0}
};

XF86ModuleData xtrapModuleData = { &xtrapVersRec, xtrapSetup, NULL };

static pointer
xtrapSetup(pointer module, pointer opts, int *errmaj, int *errmin) {
    LoadExtension(&xtrapExt, FALSE);
    /* Need a non-NULL return value to indicate success */
    return (pointer)1;
}

#endif /* XFree86LOADER */
