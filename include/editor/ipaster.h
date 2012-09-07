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

#ifndef __paster_h__
#define __paster_h__

#include "csutil/scf.h"

/// Structured used to paste and spawn new information.
struct PasteContents
{
  csString dynfactName;
  bool useTransform;
  csReversibleTransform trans;
  bool isStatic;
};

/**
 * A module responsible for handling pasting and related.
 */
struct iPaster : public virtual iBase
{
  SCF_INTERFACE(iPaster,0,0,1);

  /// Show the constrain marker.
  virtual void ShowConstrainMarker (bool constrainx, bool constrainy, bool constrainz) = 0;
  /// Move the constrain marker.
  virtual void MoveConstrainMarker (const csReversibleTransform& trans) = 0;
  /// Hide the constrain marker.
  virtual void HideConstrainMarker () = 0;

  // There are two paste buffers. One is the 'normal' paste buffer which is
  // filled by doing a copy. We call this the clipboard. The other is the paste
  // buffer that we're currently about to paste or spawn.

  /// Copy the selection to the clipboard.
  virtual void CopySelection () = 0;
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
  /// Return the number of items in the clipboard.
  virtual size_t GetClipboardSize () const = 0;

  /// Toggle grid movement on/off.
  virtual void ToggleGridMode () = 0;
  /// Return true if grid mode is enabled.
  virtual bool IsGridModeEnabled () const = 0;
  virtual float GetGridSize () const = 0;
};


#endif // __paster_h__

