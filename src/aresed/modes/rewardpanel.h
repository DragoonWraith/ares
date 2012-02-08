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

#ifndef __appares_rewardpanel_h
#define __appares_rewardpanel_h

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
struct iRewardFactory;
struct iParameter;
class UIDialog;
class UIManager;
class EntityMode;

class RewardPanel : public wxPanel, public Ares::View
{
private:
  iCelPlLayer* pl;
  UIManager* uiManager;
  EntityMode* emode;
  wxSizer* parentSizer;

  iRewardFactory* reward;
  csString GetCurrentRewardType ();

  void OnChoicebookPageChange (wxChoicebookEvent& event);
  void OnUpdateEvent (wxCommandEvent& event);

  void UpdateReward ();
  void UpdatePanel ();

  UIDialog* createentityDialog;
  UIDialog* messageDialog;

  csRef<Ares::Value> messageParameters;
  csRef<Ares::Value> actionParameters;
  csRef<Ares::Value> createentityParameters;

public:
  RewardPanel (wxWindow* parent, UIManager* uiManager, EntityMode* emode);
  ~RewardPanel();

  iCelPlLayer* GetPL () const { return pl; }

  /**
   * Possibly switch the type of the reward. Do nothing if the reward is
   * already of the right type. Otherwise clear all properties.
   */
  void SwitchReward (iRewardFactory* reward);
  iRewardFactory* GetCurrentReward () const { return reward; }

  void Show () { wxPanel::Show (); parentSizer->Layout (); }
  void Hide () { reward = 0; wxPanel::Hide (); parentSizer->Layout (); }
  bool IsVisible () const { return IsShown (); }

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_rewardpanel_h

