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

#include "edcommon/model.h"

struct iCelPlLayer;
struct iCelEntityTemplate;
struct iCelPropertyClassTemplate;
struct iCelParameterIterator;
struct iCelSequenceFactory;
struct iQuestFactory;
struct iSeqOpFactory;
struct iQuestManager;
struct iUIDialog;
struct iUIManager;
class EntityMode;

class SequencePanel : public wxPanel, public Ares::View
{
private:
  iCelPlLayer* pl;
  iUIManager* uiManager;
  EntityMode* emode;
  wxSizer* parentSizer;

  iQuestFactory* questFact;
  iCelSequenceFactory* sequence;
  csString GetCurrentSequenceType ();

  void UpdateSequence ();
  void UpdatePanel ();

  csRef<iUIDialog> newopDialog;

  csRef<Ares::ListSelectedValue> operationsSelectedValue;
  wxListCtrl* operationsList;
  csRef<Ares::Value> operations;

public:
  SequencePanel (wxWindow* parent, iUIManager* uiManager, EntityMode* emode);
  ~SequencePanel();

  void RegisterModification ();

  iCelPlLayer* GetPL () const { return pl; }
  EntityMode* GetEntityMode () const { return emode; }
  iQuestFactory* GetQuestFactory () const { return questFact; }

  /**
   * Possibly switch the type of the sequence. Do nothing if the sequence is
   * already of the right type. Otherwise clear all properties.
   */
  void SwitchSequence (iQuestFactory* questFact, iCelSequenceFactory* sequence);
  iCelSequenceFactory* GetCurrentSequence () const { return sequence; }
  iSeqOpFactory* GetSeqOpFactory ();
  iQuestManager* GetQuestManager () const;
  long GetSeqOpSelection () const;

  void Show () { wxPanel::Show (); parentSizer->Layout (); }
  void Hide () { sequence = 0; wxPanel::Hide (); parentSizer->Layout (); }
  bool IsVisible () const { return IsShown (); }
};

#endif // __appares_sequencepanel_h

