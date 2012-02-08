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

#ifndef __appares_sequencepanel_h
#define __appares_sequencepanel_h

#include <crystalspace.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/choicebk.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

#include "../models/model.h"

struct iCelPlLayer;
struct iCelEntityTemplate;
struct iCelPropertyClassTemplate;
struct iCelParameterIterator;
struct iCelSequenceFactory;
class UIDialog;
class UIManager;
class EntityMode;

class SequencePanel : public wxPanel, public Ares::View
{
private:
  iCelPlLayer* pl;
  UIManager* uiManager;
  EntityMode* emode;
  wxSizer* parentSizer;

  iCelSequenceFactory* sequence;
  csString GetCurrentSequenceType ();

  void OnChoicebookPageChange (wxChoicebookEvent& event);
  void OnUpdateEvent (wxCommandEvent& event);

  void UpdateSequence ();
  void UpdatePanel ();

  csRef<Ares::Value> seqopCollection;

public:
  SequencePanel (wxWindow* parent, UIManager* uiManager, EntityMode* emode);
  ~SequencePanel();

  iCelPlLayer* GetPL () const { return pl; }

  /**
   * Possibly switch the type of the sequence. Do nothing if the sequence is
   * already of the right type. Otherwise clear all properties.
   */
  void SwitchSequence (iCelSequenceFactory* sequence);
  iCelSequenceFactory* GetCurrentSequence () const { return sequence; }

  void Show () { wxPanel::Show (); parentSizer->Layout (); }
  void Hide () { sequence = 0; wxPanel::Hide (); parentSizer->Layout (); }
  bool IsVisible () const { return IsShown (); }

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_sequencepanel_h

