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

#ifndef __i3dview_h__
#define __i3dview_h__

#include "csutil/scf.h"
#include <wx/wx.h>

struct iDynamicObject;
struct iDynamicFactory;
struct iSelection;
struct iDynamicCell;
struct iPcDynamicWorld;
struct iAresEditor;
struct iELCM;
struct iEditorCamera;

namespace Ares
{
  class Value;
}

/// Structured used to paste and spawn new information.
struct PasteContents
{
  csString dynfactName;
  bool useTransform;
  csReversibleTransform trans;
  bool isStatic;
};

/**
 * The 3D view in the editor.
 */
struct i3DView : public virtual iBase
{
  SCF_INTERFACE(i3DView,0,0,1);

  /// Get the CS camera for the view.
  virtual iCamera* GetCsCamera () const = 0;

  /// Get the view.
  virtual iView* GetView () const = 0;

  /// Get the editor camera.
  virtual iEditorCamera* GetEditorCamera () const = 0;

  /// Get the light that moves with the camera.
  virtual iLight* GetCameraLight () const = 0;

  /// Show the constrain marker.
  virtual void ShowConstrainMarker (bool constrainx, bool constrainy, bool constrainz) = 0;
  /// Move the constrain marker.
  virtual void MoveConstrainMarker (const csReversibleTransform& trans) = 0;
  /// Hide the constrain marker.
  virtual void HideConstrainMarker () = 0;

  /// Return true if we're in debug mode.
  virtual bool IsDebugMode () const = 0;
  virtual void SetDebugMode (bool b) = 0;

  /// Time settings.
  virtual bool IsAutoTime () const = 0;
  virtual void SetAutoTime (bool a) = 0;
  virtual void ModifyCurrentTime (csTicks t) = 0;
  virtual csTicks GetCurrentTime () const = 0;
  virtual bool IsSimulation () const = 0;

  /**
   * Calculate a segment representing a beam that starts from camera
   * position towards a given point on screen.
   */
  virtual csSegment3 GetBeam (int x, int y, float maxdist = 1000.0f) = 0;
  
  /**
   * Calculate a segment representing a beam that starts from camera
   * position towards a given point on screen as pointed to by the mouse.
   */
  virtual csSegment3 GetMouseBeam (float maxdist = 1000.0f) = 0;
  
  /**
   * Given a beam, calculate the dynamic object at that position.
   */
  virtual iDynamicObject* TraceBeam (const csSegment3& beam, csVector3& isect) = 0;

  /**
   * Given a beam, see if something is selected at the position.
   * This can be a dynamic object but also static geometry.
   */
  virtual bool TraceBeamHit (const csSegment3& beam, csVector3& isect) = 0;

  /**
   * Hit a beam with the terrain and return the intersection point.
   */
  virtual bool TraceBeamTerrain (const csVector3& start, const csVector3& end,
      csVector3& isect) = 0;

  /**
   * Get the value for the collection of dynamic factories.
   */
  virtual Ares::Value* GetDynfactCollectionValue () const = 0;

  /**
   * Get the current selection.
   */
  virtual iSelection* GetSelection () const = 0;

  /**
   * Set the static state of the current selected objects.
   */
  virtual void SetStaticSelectedObjects (bool st) = 0;

  /**
   * Set the name of the current selected objects (only works for the first
   * selected object).
   */
  virtual void ChangeNameSelectedObject (const char* name) = 0;

  /**
   * Get the dynamics system.
   */
  virtual iDynamicSystem* GetDynamicSystem () const = 0;

  /**
   * Get the physics bullet system.
   */
  virtual CS::Physics::Bullet::iDynamicSystem* GetBulletSystem () const = 0;

  /**
   * Spawn an item. 'trans' is an optional relative transform to use for
   * the new item.
   */
  virtual iDynamicObject* SpawnItem (const csString& name,
      csReversibleTransform* trans = 0) = 0;

  /**
   * Get the current dynamic cell.
   */
  virtual iDynamicCell* GetDynamicCell () const = 0;

  /**
   * Get a reference to the dynamic world property class.
   */
  virtual iPcDynamicWorld* GetDynamicWorld () const = 0;

  /**
   * Get the size of the 3D view.
   */
  virtual int GetViewWidth () const = 0;
  virtual int GetViewHeight () const = 0;

  /**
   * Get the ares editor.
   */
  virtual iAresEditor* GetApplication  () = 0;

  /**
   * Get the ELCM.
   */
  virtual iELCM* GetELCM () const = 0;

  // There are two paste buffers. One is the 'normal' paste buffer which is
  // filled by doing a copy. We call this the clipboard. The other is the paste
  // buffer that we're currently about to paste or spawn.

  /// Start paste mode.
  virtual void StartPasteSelection () = 0;
  /// Start paste mode for a specific object.
  virtual void StartPasteSelection (const char* name) = 0;
  /// Paste mode active.
  virtual bool IsPasteSelectionActive () const = 0;
  /// Set the paste constrain mode. Use one of the CONSTRAIN_ constants.
  virtual void SetPasteConstrain (int mode) = 0;
  /// Get the current paste contrain mode.
  virtual int GetPasteConstrain () const = 0;
  /// Is there something in the clipboard?
  virtual bool IsClipboardFull () const = 0;

  /// Toggle grid movement on/off.
  virtual void ToggleGridMode () = 0;

  /**
   * Move an item to another category. If the item doesn't already exist
   * then this is equivalent to calling AddItem().
   */
  virtual void ChangeCategory (const char* newCategory, const char* itemname) = 0;

  /**
   * When the physical properties of a factory change or a new factory is created
   * we need to change various internal settings for this.
   */
  virtual void RefreshFactorySettings (iDynamicFactory* fact) = 0;
};


#endif // __i3dview_h__

