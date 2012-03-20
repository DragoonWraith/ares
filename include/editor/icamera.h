/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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

#ifndef __icamera_h__
#define __icamera_h__

#include "csutil/scf.h"

/**
 * The camera.
 */
struct iEditorCamera : public virtual iBase
{
  SCF_INTERFACE(iEditorCamera,0,0,1);

  /**
   * Handle a frame for the editor camera.
   */
  virtual void Frame (float elapsed, int mouseX, int mouseY) = 0;

  /**
   * Various event functions.
   */
  virtual bool OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY) = 0;
  virtual bool OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY) = 0;
  virtual bool OnMouseMove (iEvent& ev, int mouseX, int mouseY) = 0;
  virtual void OnFocusLost () = 0;

  /**
   * Camera panning control.
   */
  virtual bool IsPanningEnabled () const = 0;
  virtual void EnablePanning (const csVector3& center) = 0;
  virtual void DisablePanning () = 0;
};


#endif // __icamera_h__

