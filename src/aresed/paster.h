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

#ifndef __paster_h
#define __paster_h

#include <crystalspace.h>
#include "editor/i3dview.h"
#include "editor/iapp.h"
#include "editor/ipaster.h"

class AppAresEditWX;
class AresEdit3DView;
struct iMarker;

/**
 * Everything for pasting.
 */
class Paster : public scfImplementation1<Paster, iPaster>
{
private:
  AppAresEditWX* app;
  AresEdit3DView* view3d;

  /// Marker used for pasting.
  iMarker* pasteMarker;
  iMarker* constrainMarker;
  csString currentPasteMarkerContext;	// Name of the dynfact mesh currently in pasteMarker.
  int pasteConstrainMode;		// Current paste constrain mode.
  csVector3 pasteConstrain;
  bool gridMode;
  float gridSize;

  /// A paste buffer.
  csArray<PasteContents> pastebuffer;

  /// When there are items in this array we are waiting to spawn stuff.
  csArray<PasteContents> todoSpawn;

  /**
   * Create the paste marker based on the current paste buffer (if needed).
   */
  void CreatePasteMarker ();

  /// Return where an item would be spawned if we were to spawn it now.
  csReversibleTransform GetSpawnTransformation ();

public:
  Paster ();
  virtual ~Paster ();

  bool Setup (AppAresEditWX* app, AresEdit3DView* view3d);

  /**
   * Make sure the paste marker is at the correct spot and active.
   */
  void PlacePasteMarker ();

  /**
   * Paste the current paste buffer at the mouse position. Usually you
   * would not use this but use StartPasteSelection() instead.
   */
  void PasteSelection ();

  /**
   * Stop paste mode.
   */
  void StopPasteMode ();

  /**
   * Constrain a transform according to the current mode.
   */
  void ConstrainTransform (csReversibleTransform& tr);

  /**
   * Cleanup everything from the paster.
   */
  void Cleanup ();

  virtual void ShowConstrainMarker (bool constrainx, bool constrainy, bool constrainz);
  virtual void MoveConstrainMarker (const csReversibleTransform& trans);
  virtual void HideConstrainMarker ();

  virtual void CopySelection ();
  virtual void StartPasteSelection ();
  virtual void StartPasteSelection (const char* name);
  virtual bool IsPasteSelectionActive () const { return todoSpawn.GetSize () > 0; }
  virtual void SetPasteConstrain (int mode);
  virtual int GetPasteConstrain () const { return pasteConstrainMode; }
  virtual size_t GetClipboardSize () const { return pastebuffer.GetSize (); }

  virtual void ToggleGridMode ();
  virtual bool IsGridModeEnabled () const { return gridMode; }
  virtual float GetGridSize () const { return gridSize; }
};

#endif // __paster_h

