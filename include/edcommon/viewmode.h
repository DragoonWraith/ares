/*
The MIT License

Copyright (c) 2011 by Jorrit Tyberghein

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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */

#ifndef __aresed_viewmode_h
#define __aresed_viewmode_h

#include "csutil/csstring.h"
#include "csutil/scfstr.h"
#include "edcommon/editmodes.h"
#include "inature.h"

struct iGeometryGenerator;
struct iNature;

/**
 * Superclass for all modes that require the general 3D view.
 */
class ARES_EDCOMMON_EXPORT ViewMode : public EditingMode
{
protected:
  csRef<iFont> font;
  csRef<iNature> nature;
  csRef<iVirtualClock> vc;
  csRef<CS::Physics::iPhysicalSystem> dyn;

public:
  ViewMode (i3DView* view, iObjectRegistry* object_reg, const char* name);
  ViewMode (iBase* parent);
  virtual ~ViewMode () { }
  virtual bool Initialize (iObjectRegistry* object_reg);

  virtual void Start ();
  virtual void Stop ();

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);
  virtual void OnFocusLost ();

  virtual csRef<iString> GetStatusLine ()
  {
    csRef<iString> str;
    str.AttachNew (new scfString ("MMB: rotate camera, shift-MMB: pan camera, RMB: context menu"));
    return str;
  }
};

#endif // __aresed_viewmode_h

