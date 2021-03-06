
/*
Copyright (c) 2003-2004 Roland Mainz <roland.mainz@nrubsig.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>

#include "regionstr.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "fontxlfd.h"
#include "fntfil.h"
#include "fntfilst.h"

#include "Ps.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "ft.h"
#include "ftfuncs.h"

char *
PsGetFTFontFileName(FontPtr pFont)
{
  FTFontPtr tf = (FTFontPtr)pFont->fontPrivate;
  return tf->instance->face->filename;
}
   
Bool
PsIsFreeTypeFont(FontPtr pFont)
{
  int         i;
  int         nprops = pFont->info.nprops;
  FontPropPtr props  = pFont->info.props;
  /* "RASTERIZER_NAME" must match the rasterizer name set in
   * xc/lib/font/FreeType/ftfuncs.c */
  Atom        name   = MakeAtom("RASTERIZER_NAME", 15, True); 
  Atom        value  = (Atom)0;
  char       *rv;

  for( i=0 ; i<nprops ; i++ )
  {
    if( props[i].name==name )
      { value = props[i].value; break; }
  }
  if( !value )
    return False; 

  rv = NameForAtom(value);
  if( !rv )
    return False; 

  if( memcmp(rv, "FreeType", 8) == 0 )
    return  True;

  return False;
}

