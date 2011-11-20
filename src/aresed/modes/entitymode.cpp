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

#include "../apparesed.h"
#include "../camerawin.h"
#include "entitymode.h"

#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/questmanager.h"

#include <wx/xrc/xmlres.h>
#include <wx/listbox.h>

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(EntityMode::Panel, wxPanel)
  EVT_LISTBOX (XRCID("templateList"), EntityMode::Panel :: OnTemplateSelect)
END_EVENT_TABLE()

//---------------------------------------------------------------------------

EntityMode::EntityMode (wxWindow* parent, AresEdit3DView* aresed3d)
  : EditingMode (aresed3d, "Entity")
{
  panel = new Panel (parent, this);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("EntityModePanel"));
  iMarkerManager* mgr = aresed3d->GetMarkerManager ();
  view = mgr->CreateGraphView ();
  view->Clear ();

  view->SetVisible (false);

  InitColors ();
}

EntityMode::~EntityMode ()
{
  aresed3d->GetMarkerManager ()->DestroyGraphView (view);
}

iMarkerColor* EntityMode::NewColor (const char* name,
    float r0, float g0, float b0, float r1, float g1, float b1, bool fill)
{
  iMarkerManager* mgr = aresed3d->GetMarkerManager ();
  iMarkerColor* col = mgr->CreateMarkerColor (name);
  col->SetRGBColor (SELECTION_NONE, r0, g0, b0, 1);
  col->SetRGBColor (SELECTION_SELECTED, r1, g1, b1, 1);
  col->SetRGBColor (SELECTION_ACTIVE, r1, g1, b1, 1);
  col->SetPenWidth (SELECTION_NONE, 1.2f);
  col->SetPenWidth (SELECTION_SELECTED, 1.2f);
  col->SetPenWidth (SELECTION_ACTIVE, 1.2f);
  col->EnableFill (SELECTION_NONE, fill);
  col->EnableFill (SELECTION_SELECTED, fill);
  col->EnableFill (SELECTION_ACTIVE, fill);
  return col;
}

void EntityMode::InitColors ()
{
  iMarkerManager* mgr = aresed3d->GetMarkerManager ();

  iMarkerColor* textColor = NewColor ("viewWhite", .7, .7, .7, 1, 1, 1, false);

  styleTemplate = mgr->CreateGraphNodeStyle ();
  styleTemplate->SetBorderColor (NewColor ("templateColorFG", .7, .7, .7, 1, 1, 1, false));
  styleTemplate->SetBackgroundColor (NewColor ("templateColorBG", .1, .4, .5, .2, .6, .7, true));
  styleTemplate->SetTextColor (textColor);

  stylePC = mgr->CreateGraphNodeStyle ();
  stylePC->SetBorderColor (NewColor ("pcColorFG", 0, 0, .7, 0, 0, 1, false));
  stylePC->SetBackgroundColor (NewColor ("pcColorBG", .1, .4, .5, .2, .6, .7, true));
  stylePC->SetTextColor (textColor);

  styleState = mgr->CreateGraphNodeStyle ();
  styleState->SetBorderColor (NewColor ("stateColorFG", 0, .7, 0, 0, 1, 0, false));
  styleState->SetBackgroundColor (NewColor ("stateColorBG", .1, .4, .5, .2, .6, .7, true));
  styleState->SetTextColor (textColor);

  styleResponse = mgr->CreateGraphNodeStyle ();
  styleResponse->SetBorderColor (NewColor ("respColorFG", 0, .7, .7, 0, 1, 1, false));
  styleResponse->SetBackgroundColor (NewColor ("respColorBG", .3, .6, .7, .4, .7, .8, true));
  styleResponse->SetRoundness (1);
  styleResponse->SetTextColor (NewColor ("respColorTxt", 0, 0, 0, 0, 0, 0, false));

  styleReward = mgr->CreateGraphNodeStyle ();
  styleReward->SetBorderColor (NewColor ("rewColorFG", 0, .7, .7, 0, 1, 1, false));
  styleReward->SetBackgroundColor (NewColor ("rewColorBG", .3, .6, .7, .4, .7, .8, true));
  styleReward->SetRoundness (1);
  styleReward->SetTextColor (textColor);

  view->SetDefaultNodeStyle (stylePC);

  iMarkerColor* thickLinkColor = mgr->CreateMarkerColor ("thickLink");
  thickLinkColor->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  thickLinkColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  thickLinkColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  thickLinkColor->SetPenWidth (SELECTION_NONE, 1.2f);
  thickLinkColor->SetPenWidth (SELECTION_SELECTED, 2.0f);
  thickLinkColor->SetPenWidth (SELECTION_ACTIVE, 2.0f);
  thinLinkColor = mgr->CreateMarkerColor ("thinLink");
  thinLinkColor->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  thinLinkColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  thinLinkColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  thinLinkColor->SetPenWidth (SELECTION_NONE, 0.8f);
  thinLinkColor->SetPenWidth (SELECTION_SELECTED, 0.8f);
  thinLinkColor->SetPenWidth (SELECTION_ACTIVE, 0.8f);
  arrowLinkColor = mgr->CreateMarkerColor ("arrowLink");
  arrowLinkColor->SetRGBColor (SELECTION_NONE, 0, .5, .5, .5);
  arrowLinkColor->SetRGBColor (SELECTION_SELECTED, 0, 1, 1, .5);
  arrowLinkColor->SetRGBColor (SELECTION_ACTIVE, 0, 1, 1, .5);
  arrowLinkColor->SetPenWidth (SELECTION_NONE, 0.5f);
  arrowLinkColor->SetPenWidth (SELECTION_SELECTED, 0.5f);
  arrowLinkColor->SetPenWidth (SELECTION_ACTIVE, 0.5f);

  view->SetLinkColor (thickLinkColor);
}

void EntityMode::SetupItems ()
{
  wxListBox* list = XRCCTRL (*panel, "templateList", wxListBox);
  list->Clear ();
  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  wxArrayString names;
  for (size_t i = 0 ; i < pl->GetEntityTemplateCount () ; i++)
  {
    iCelEntityTemplate* tpl = pl->GetEntityTemplate (i);
    wxString name = wxString (tpl->GetName (), wxConvUTF8);
    names.Add (name);
  }
  list->InsertItems (names, 0);
}

void EntityMode::Start ()
{
  aresed3d->GetApp ()->GetCameraWindow ()->Hide ();
  SetupItems ();
  view->SetVisible (true);
}

void EntityMode::Stop ()
{
  view->SetVisible (false);
}

void EntityMode::BuildNewStateConnections (iRewardFactoryArray* rewards,
    const char* parentKey, const char* pcNodeName)
{
  for (size_t j = 0 ; j < rewards->GetSize () ; j++)
  {
    iRewardFactory* reward = rewards->Get (j);
    csString rewKey;
    rewKey.Format ("%s %d", parentKey, j);
    view->CreateNode (rewKey, "Rew", styleReward);
    view->LinkNode (parentKey, rewKey, thinLinkColor);

    csRef<iNewStateQuestRewardFactory> newState = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    if (newState)
    {
      csString stateNameKey = pcNodeName;
      // @@@ No support for expressions here!
      stateNameKey += newState->GetStateParameter ();
      view->LinkNode (rewKey, stateNameKey, arrowLinkColor, true);
    }
  }
}

void EntityMode::BuildStateGraph (iQuestStateFactory* state,
    const char* stateNameKey, const char* pcNodeName)
{
  csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
  for (size_t i = 0 ; i < responses->GetSize () ; i++)
  {
    iQuestTriggerResponseFactory* response = responses->Get (i);
    csString responseKey;
    responseKey.Format ("R%s %d", stateNameKey, i);
    view->CreateNode (responseKey, "T", styleResponse);
    view->LinkNode (stateNameKey, responseKey);
    csRef<iRewardFactoryArray> rewards = response->GetRewardFactories ();
    BuildNewStateConnections (rewards, responseKey, pcNodeName);
  }

  csRef<iRewardFactoryArray> initRewards = state->GetInitRewardFactories ();
  if (initRewards->GetSize () > 0)
  {
    csString newKeyKey;
    newKeyKey.Format ("%s I", stateNameKey);
    view->CreateNode (newKeyKey, "I", styleResponse);
    view->LinkNode (stateNameKey, newKeyKey);
    BuildNewStateConnections (initRewards, newKeyKey, pcNodeName);
  }
  csRef<iRewardFactoryArray> exitRewards = state->GetExitRewardFactories ();
  if (exitRewards->GetSize () > 0)
  {
    csString newKeyKey;
    newKeyKey.Format ("%s E", stateNameKey);
    view->CreateNode (newKeyKey, "E", styleResponse);
    view->LinkNode (stateNameKey, newKeyKey);
    BuildNewStateConnections (exitRewards, newKeyKey, pcNodeName);
  }
}

csString EntityMode::GetQuestName (iCelPropertyClassTemplate* pctpl)
{
  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  csStringID newquestID = pl->FetchStringID ("NewQuest");
  size_t idx = pctpl->FindProperty (newquestID);
  if (idx != csArrayItemNotFound)
  {
    celData data;
    csRef<iCelParameterIterator> parit = pctpl->GetProperty (idx,
		    newquestID, data);
    csStringID nameID = pl->FetchStringID ("name");
    csString questName;
    while (parit->HasNext ())
    {
      csStringID parid;
      iParameter* par = parit->Next (parid);
      // @@@ We don't support expression parameters here. 'params'
      // for creating entities is missing.
      if (parid == nameID)
      {
	return par->Get (0);
      }
    }
  }
  return "";
}

void EntityMode::BuildQuestGraph (iCelPropertyClassTemplate* pctpl,
    const char* pcNodeName)
{
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  csRef<iQuestManager> quest_mgr = csQueryRegistryOrLoad<iQuestManager> (
    aresed3d->GetObjectRegistry (),
    "cel.manager.quests");
  iQuestFactory* questFact = quest_mgr->GetQuestFactory (questName);
  // @@@ Error check
  if (questFact)
  {
    csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
    while (it->HasNext ())
    {
      iQuestStateFactory* stateFact = it->Next ();
      csString stateNameKey = pcNodeName;
      stateNameKey += stateFact->GetName ();
      view->CreateNode (stateNameKey, stateFact->GetName (), styleState);
      view->LinkNode (pcNodeName, stateNameKey);
      BuildStateGraph (stateFact, stateNameKey, pcNodeName);
    }
  }
}

csString EntityMode::GetExtraPCInfo (iCelPropertyClassTemplate* pctpl)
{
  csString pcName = pctpl->GetName ();
  if (pcName == "pclogic.quest")
  {
    return GetQuestName (pctpl);
  }
  return "";
}

void EntityMode::BuildTemplateGraph (const char* templateName)
{
  view->Clear ();

  view->SetVisible (false);
  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (templateName);
  if (!tpl) return;

  view->CreateNode (templateName, 0, styleTemplate);

  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
    csString pcName = pctpl->GetName ();
    csString pcNodeName;
    size_t lastDot = pcName.FindLast ('.');
    if (lastDot != csArrayItemNotFound)
      pcNodeName = pcName.Slice (lastDot+1);
    else
      pcNodeName = pcName;

    if (pctpl->GetTag () != 0)
      pcNodeName.AppendFmt (" (%s)", pctpl->GetTag ());
    csString pcLabel = pcNodeName;
    csString extraInfo = GetExtraPCInfo (pctpl);
    if (!extraInfo.IsEmpty ()) { pcLabel += '\n'; pcLabel += extraInfo; }
    view->CreateNode (pcNodeName, pcLabel, stylePC);
    view->LinkNode (templateName, pcNodeName);
    if (pcName == "pclogic.quest")
      BuildQuestGraph (pctpl, pcNodeName);
  }
  view->SetVisible (true);
}

void EntityMode::OnTemplateSelect ()
{
  wxListBox* list = XRCCTRL (*panel, "templateList", wxListBox);
  csString templateName = (const char*)list->GetStringSelection ().mb_str(wxConvUTF8);
  BuildTemplateGraph (templateName);
}

void EntityMode::FramePre()
{
}

void EntityMode::Frame3D()
{
  aresed3d->GetG2D ()->Clear (aresed3d->GetG2D ()->FindRGB (100, 110, 120));
}

void EntityMode::Frame2D()
{
}

bool EntityMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  return false;
}

bool EntityMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  return false;
}

bool EntityMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
{
  return false;
}

bool EntityMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return false;
}

