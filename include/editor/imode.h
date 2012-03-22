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

#ifndef __imode_h__
#define __imode_h__

#include "csutil/scf.h"
#include <wx/wx.h>

#include "iplugin.h"

struct iDynamicObject;
struct i3DView;

/**
 * An 'editor' mode.
 */
struct iEditingMode : public virtual iBase
{
  SCF_INTERFACE(iEditingMode,0,0,1);

  /**
   * Allocate context handlers for the context menu of this mode.
   */
  virtual void AllocContextHandlers (wxFrame* frame) = 0;

  /**
   * Add context menu items to a context menu.
   */
  virtual void AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY) = 0;

  /**
   * Activate the mode.
   */
  virtual void Start () = 0;

  /**
   * Stop the mode.
   */
  virtual void Stop () = 0;

  /**
   * Refresh the mode in case something big changes (new file
   * is loaded, new dynamic factories are created, ...)
   */
  virtual void Refresh () = 0;

  /**
   * Get the current status line for this mode.
   */
  virtual csRef<iString> GetStatusLine () = 0;

  //-----------------------------------------------------------------------
  // Callback functions from the editor system.
  //-----------------------------------------------------------------------

  /**
   * When the selection changes the editor will call this function.
   */
  virtual void CurrentObjectsChanged (const csArray<iDynamicObject*>& current) = 0;

  /**
   * Called by the editor framework right before the frame starts rendering.
   */
  virtual void FramePre () = 0;

  /**
   * Called by the editor framework when it is time to render 3D stuff.
   */
  virtual void Frame3D () = 0;

  /**
   * Called by the editor framework when it is time to render 2D stuff.
   */
  virtual void Frame2D () = 0;

  /**
   * Called by the editor framework in case of a keyboard event.
   */
  virtual bool OnKeyboard (iEvent& ev, utf32_char code) = 0;

  /**
   * Called by the editor framework in case of a mouse down event.
   */
  virtual bool OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY) = 0;

  /**
   * Called by the editor framework in case of a mouse up event.
   */
  virtual bool OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY) = 0;

  /**
   * Called by the editor framework in case of a mouse move event.
   */
  virtual bool OnMouseMove (iEvent& ev, int mouseX, int mouseY) = 0;

  /**
   * Called by the editor framework in case focus was lost.
   */
  virtual void OnFocusLost () = 0;

  /**
   * Called by the editor framework when the user starts dragging a marker.
   */
  virtual void MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers) = 0;

  /**
   * Called by the editor framework when a marker wants to move.
   */
  virtual void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos) = 0;

  /**
   * Called by the editor framework when a marker wants to rotate.
   */
  virtual void MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform) = 0;

  /**
   * Called by the editor framework when a marker stops dragging.
   */
  virtual void MarkerStopDragging (iMarker* marker, iMarkerHitArea* area) = 0;
};


#endif // __imode_h__

