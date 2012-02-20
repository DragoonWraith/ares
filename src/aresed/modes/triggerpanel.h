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

#ifndef __appares_triggerpanel_h
#define __appares_triggerpanel_h

#include <crystalspace.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/choicebk.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

struct iCelPlLayer;
struct iCelEntityTemplate;
struct iCelPropertyClassTemplate;
struct iCelParameterIterator;
struct iParameter;
struct iQuestTriggerResponseFactory;
class UIDialog;
class UIManager;
class EntityMode;

class TriggerPanel : public wxPanel
{
private:
  iCelPlLayer* pl;
  UIManager* uiManager;
  EntityMode* emode;
  wxSizer* parentSizer;

  iQuestTriggerResponseFactory* triggerResp;
  csString GetCurrentTriggerType ();

  void OnChoicebookPageChange (wxChoicebookEvent& event);
  void OnUpdateEvent (wxCommandEvent& event);
  void OnSetThisInventory (wxCommandEvent& event);
  void OnSetThisMeshSelect (wxCommandEvent& event);
  void OnSetThisProperty (wxCommandEvent& event);
  void OnSetThisQuest (wxCommandEvent& event);
  void OnSetThisTrigger (wxCommandEvent& event);
  void OnSetThisWatch (wxCommandEvent& event);
  void OnSetThisMeshEnter (wxCommandEvent& event);
  void OnSetThisSectorEnter (wxCommandEvent& event);

  void UpdateTrigger ();
  void UpdatePanel ();

public:
  TriggerPanel (wxWindow* parent, UIManager* uiManager, EntityMode* emode);
  ~TriggerPanel();

  /**
   * Possibly switch the type of the trigger. Do nothing if the trigger is
   * already of the right type. Otherwise clear all properties.
   */
  void SwitchTrigger (iQuestTriggerResponseFactory* triggerResp);

  void Show () { wxPanel::Show (); parentSizer->Layout (); }
  void Hide () { wxPanel::Hide (); parentSizer->Layout (); }
  bool IsVisible () const { return IsShown (); }

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_triggerpanel_h

