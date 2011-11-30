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
#include "../ui/uimanager.h"
#include "../inspect.h"
#include "pcpanel.h"
#include "../ui/listctrltools.h"

#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/questmanager.h"

#include <wx/xrc/xmlres.h>
#include <wx/listbox.h>

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(EntityMode::Panel, wxPanel)
  EVT_LISTBOX (XRCID("templateList"), EntityMode::Panel :: OnTemplateSelect)
  EVT_CONTEXT_MENU (EntityMode::Panel :: OnContextMenu)
  EVT_MENU (ID_Template_Add, EntityMode::Panel :: OnTemplateAdd)
  EVT_MENU (ID_Template_Delete, EntityMode::Panel :: OnTemplateDel)
END_EVENT_TABLE()

//---------------------------------------------------------------------------

class GraphNodeCallback : public iGraphNodeCallback
{
private:
  EntityMode* emode;

public:
  GraphNodeCallback (EntityMode* emode) : emode (emode) { }
  virtual ~GraphNodeCallback () { }
  virtual void ActivateNode (const char* nodeName)
  {
    emode->ActivateNode (nodeName);
  }
};

//---------------------------------------------------------------------------

EntityMode::EntityMode (wxWindow* parent, AresEdit3DView* aresed3d)
  : EditingMode (aresed3d, "Entity")
{
  panel = new Panel (parent, this);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("EntityModePanel"));

  pcPanel = new PropertyClassPanel (panel, aresed3d->GetApp ()->GetUIManager (),
      this);
  pcPanel->Hide ();

  iMarkerManager* mgr = aresed3d->GetMarkerManager ();
  view = mgr->CreateGraphView ();
  view->Clear ();
  csRef<GraphNodeCallback> cb;
  cb.AttachNew (new GraphNodeCallback (this));
  view->AddNodeActivationCallback (cb);

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

iMarkerColor* EntityMode::NewColor (const char* name,
    float r0, float g0, float b0, float r1, float g1, float b1,
    float r2, float g2, float b2, bool fill)
{
  iMarkerManager* mgr = aresed3d->GetMarkerManager ();
  iMarkerColor* col = mgr->CreateMarkerColor (name);
  col->SetRGBColor (SELECTION_NONE, r0, g0, b0, 1);
  col->SetRGBColor (SELECTION_SELECTED, r1, g1, b1, 1);
  col->SetRGBColor (SELECTION_ACTIVE, r2, g2, b2, 1);
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

  iMarkerColor* thickLinkColor = mgr->CreateMarkerColor ("thickLink");
  thickLinkColor->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  thickLinkColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  thickLinkColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  thickLinkColor->SetPenWidth (SELECTION_NONE, 1.2f);
  thickLinkColor->SetPenWidth (SELECTION_SELECTED, 2.0f);
  thickLinkColor->SetPenWidth (SELECTION_ACTIVE, 2.0f);
  iMarkerColor* thinLinkColor = mgr->CreateMarkerColor ("thinLink");
  thinLinkColor->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  thinLinkColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  thinLinkColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  thinLinkColor->SetPenWidth (SELECTION_NONE, 0.8f);
  thinLinkColor->SetPenWidth (SELECTION_SELECTED, 0.8f);
  thinLinkColor->SetPenWidth (SELECTION_ACTIVE, 0.8f);
  iMarkerColor* arrowLinkColor = mgr->CreateMarkerColor ("arrowLink");
  arrowLinkColor->SetRGBColor (SELECTION_NONE, 0, .5, .5, .5);
  arrowLinkColor->SetRGBColor (SELECTION_SELECTED, 0, 1, 1, .5);
  arrowLinkColor->SetRGBColor (SELECTION_ACTIVE, 0, 1, 1, .5);
  arrowLinkColor->SetPenWidth (SELECTION_NONE, 0.5f);
  arrowLinkColor->SetPenWidth (SELECTION_SELECTED, 0.5f);
  arrowLinkColor->SetPenWidth (SELECTION_ACTIVE, 0.5f);

  styleTemplate = mgr->CreateGraphNodeStyle ();
  styleTemplate->SetBorderColor (NewColor ("templateColorFG", .0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleTemplate->SetBackgroundColor (NewColor ("templateColorBG", .1, .4, .5, .2, .6, .7, true));
  styleTemplate->SetTextColor (textColor);

  stylePC = mgr->CreateGraphNodeStyle ();
  stylePC->SetBorderColor (NewColor ("pcColorFG", 0, 0, .7, 0, 0, 1, 1, 1, 1, false));
  stylePC->SetBackgroundColor (NewColor ("pcColorBG", .1, .4, .5, .2, .6, .7, true));
  stylePC->SetTextColor (textColor);

  styleState = mgr->CreateGraphNodeStyle ();
  styleState->SetBorderColor (NewColor ("stateColorFG", 0, .7, 0, 0, 1, 0, 1, 1, 1, false));
  styleState->SetBackgroundColor (NewColor ("stateColorBG", .1, .4, .5, .2, .6, .7, true));
  styleState->SetTextColor (textColor);

  styleResponse = mgr->CreateGraphNodeStyle ();
  styleResponse->SetBorderColor (NewColor ("respColorFG", 0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleResponse->SetBackgroundColor (NewColor ("respColorBG", .3, .6, .7, .4, .7, .8, true));
  styleResponse->SetRoundness (1);
  styleResponse->SetTextColor (NewColor ("respColorTxt", 0, 0, 0, 0, 0, 0, false));

  styleReward = mgr->CreateGraphNodeStyle ();
  styleReward->SetBorderColor (NewColor ("rewColorFG", 0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleReward->SetBackgroundColor (NewColor ("rewColorBG", .3, .6, .7, .4, .7, .8, true));
  styleReward->SetRoundness (1);
  styleReward->SetTextColor (textColor);

  styleInvisible = mgr->CreateGraphNodeStyle ();
  styleInvisible->SetBorderColor (arrowLinkColor);
  styleInvisible->SetBackgroundColor (arrowLinkColor);
  styleInvisible->SetRoundness (1);
  styleInvisible->SetTextColor (arrowLinkColor);
  styleInvisible->SetWeightFactor (0.4f);

  view->SetDefaultNodeStyle (stylePC);

  styleThickLink = mgr->CreateGraphLinkStyle ();
  styleThickLink->SetColor (thickLinkColor);
  styleThinLink = mgr->CreateGraphLinkStyle ();
  styleThinLink->SetColor (thinLinkColor);
  styleArrowLink = mgr->CreateGraphLinkStyle ();
  styleArrowLink->SetColor (arrowLinkColor);
  styleArrowLink->SetArrow (true);
  styleArrow0Link = mgr->CreateGraphLinkStyle ();
  styleArrow0Link->SetColor (arrowLinkColor);

  view->SetDefaultLinkStyle (styleThickLink);
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
    wxString name = wxString::FromUTF8 (tpl->GetName ());
    names.Add (name);
  }
  list->InsertItems (names, 0);
}

void EntityMode::Start ()
{
  aresed3d->GetApp ()->GetCameraWindow ()->Hide ();
  SetupItems ();
  view->SetVisible (true);
  pcPanel->Hide ();
  contextMenuNode = "";
}

void EntityMode::Stop ()
{
  view->SetVisible (false);
}

void EntityMode::OnContextMenu (wxContextMenuEvent& event)
{
  wxListBox* list = XRCCTRL (*panel, "templateList", wxListBox);
  bool hasItem;
  if (ListCtrlTools::CheckHitList (list, hasItem, event.GetPosition ()))
  {
    wxMenu contextMenu;
    contextMenu.Append(ID_Template_Add, wxT ("&Add Template..."));
    if (hasItem)
    {
      contextMenu.Append(ID_Template_Delete, wxT ("&Delete"));
    }
    panel->PopupMenu (&contextMenu);
  }
}

const char* EntityMode::GetTriggerType (iTriggerFactory* trigger)
{
  {
    csRef<iTimeoutTriggerFactory> s = scfQueryInterface<iTimeoutTriggerFactory> (trigger);
    if (s) return "Timeout";
  }
  {
    csRef<iEnterSectorTriggerFactory> s = scfQueryInterface<iEnterSectorTriggerFactory> (trigger);
    if (s) return "EnterSect";
  }
  {
    csRef<iSequenceFinishTriggerFactory> s = scfQueryInterface<iSequenceFinishTriggerFactory> (trigger);
    if (s) return "SeqFinish";
  }
  {
    csRef<iPropertyChangeTriggerFactory> s = scfQueryInterface<iPropertyChangeTriggerFactory> (trigger);
    if (s) return "PropChange";
  }
  {
    csRef<iTriggerTriggerFactory> s = scfQueryInterface<iTriggerTriggerFactory> (trigger);
    if (s) return "Trigger";
  }
  {
    csRef<iWatchTriggerFactory> s = scfQueryInterface<iWatchTriggerFactory> (trigger);
    if (s) return "Watch";
  }
  {
    csRef<iOperationTriggerFactory> s = scfQueryInterface<iOperationTriggerFactory> (trigger);
    if (s) return "Operation";
  }
  {
    csRef<iInventoryTriggerFactory> s = scfQueryInterface<iInventoryTriggerFactory> (trigger);
    if (s) return "Inventory";
  }
  {
    csRef<iMessageTriggerFactory> s = scfQueryInterface<iMessageTriggerFactory> (trigger);
    if (s) return "Message";
  }
  {
    csRef<iMeshSelectTriggerFactory> s = scfQueryInterface<iMeshSelectTriggerFactory> (trigger);
    if (s) return "MeshSel";
  }
  return "?";
}

const char* EntityMode::GetRewardType (iRewardFactory* reward)
{
  {
    csRef<iNewStateQuestRewardFactory> s = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    if (s) return "NewState";
  }
  {
    csRef<iDebugPrintRewardFactory> s = scfQueryInterface<iDebugPrintRewardFactory> (reward);
    if (s) return "DbPrint";
  }
  {
    csRef<iInventoryRewardFactory> s = scfQueryInterface<iInventoryRewardFactory> (reward);
    if (s) return "Inventory";
  }
  {
    csRef<iSequenceRewardFactory> s = scfQueryInterface<iSequenceRewardFactory> (reward);
    if (s) return "Sequence";
  }
  {
    csRef<iCsSequenceRewardFactory> s = scfQueryInterface<iCsSequenceRewardFactory> (reward);
    if (s) return "CsSequence";
  }
  {
    csRef<iSequenceFinishRewardFactory> s = scfQueryInterface<iSequenceFinishRewardFactory> (reward);
    if (s) return "SeqFinish";
  }
  {
    csRef<iChangePropertyRewardFactory> s = scfQueryInterface<iChangePropertyRewardFactory> (reward);
    if (s) return "ChangeProp";
  }
  {
    csRef<iCreateEntityRewardFactory> s = scfQueryInterface<iCreateEntityRewardFactory> (reward);
    if (s) return "CreateEnt";
  }
  {
    csRef<iDestroyEntityRewardFactory> s = scfQueryInterface<iDestroyEntityRewardFactory> (reward);
    if (s) return "DestroyEnt";
  }
  {
    csRef<iActionRewardFactory> s = scfQueryInterface<iActionRewardFactory> (reward);
    if (s) return "Action";
  }
  {
    csRef<iMessageRewardFactory> s = scfQueryInterface<iMessageRewardFactory> (reward);
    if (s) return "Message";
  }
  return "?";
}

void EntityMode::BuildRewardGraph (iRewardFactoryArray* rewards,
    const char* parentKey, const char* pcKey)
{
  for (size_t j = 0 ; j < rewards->GetSize () ; j++)
  {
    iRewardFactory* reward = rewards->Get (j);
    csString rewKey; rewKey.Format ("r:%d,%s", j, parentKey);
    csString rewLabel; rewLabel.Format ("%d:%s", j+1, GetRewardType (reward));
    view->CreateNode (rewKey, rewLabel, styleReward);
    view->LinkNode (parentKey, rewKey, styleThinLink);

    csRef<iNewStateQuestRewardFactory> newState = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    if (newState)
    {
      // @@@ No support for expressions here!
      csString stateKey; stateKey.Format ("S:%s,%s", newState->GetStateParameter (), pcKey);
      csString i1; i1.Format (".:%s:%s:1", rewKey.GetData (), stateKey.GetData ());
      csString i2; i2.Format (".:%s:%s:2", rewKey.GetData (), stateKey.GetData ());
      csString i3; i3.Format (".:%s:%s:3", rewKey.GetData (), stateKey.GetData ());
      csString i4; i4.Format (".:%s:%s:4", rewKey.GetData (), stateKey.GetData ());
      csString i5; i5.Format (".:%s:%s:5", rewKey.GetData (), stateKey.GetData ());
      csString i6; i6.Format (".:%s:%s:6", rewKey.GetData (), stateKey.GetData ());
      view->CreateNode (i1, "", styleInvisible);
      view->CreateNode (i2, "", styleInvisible);
      view->CreateNode (i3, "", styleInvisible);
      view->CreateNode (i4, "", styleInvisible);
      view->CreateNode (i5, "", styleInvisible);
      view->CreateNode (i6, "", styleInvisible);
      view->LinkNode (rewKey, i1, styleArrow0Link);
      view->LinkNode (i1, i2, styleArrow0Link);
      view->LinkNode (i2, i3, styleArrow0Link);
      view->LinkNode (i3, i4, styleArrowLink);
      view->LinkNode (i4, i5, styleArrow0Link);
      view->LinkNode (i5, i6, styleArrow0Link);
      view->LinkNode (i6, stateKey, styleArrow0Link);
    }
  }
}

void EntityMode::BuildStateGraph (iQuestStateFactory* state,
    const char* stateKey, const char* pcKey)
{
  csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
  for (size_t i = 0 ; i < responses->GetSize () ; i++)
  {
    iQuestTriggerResponseFactory* response = responses->Get (i);
    csString responseKey; responseKey.Format ("t:%d,%s", i, stateKey);
    view->CreateNode (responseKey, GetTriggerType (response->GetTriggerFactory ()), styleResponse);
    view->LinkNode (stateKey, responseKey);
    csRef<iRewardFactoryArray> rewards = response->GetRewardFactories ();
    BuildRewardGraph (rewards, responseKey, pcKey);
  }

  csRef<iRewardFactoryArray> initRewards = state->GetInitRewardFactories ();
  if (initRewards->GetSize () > 0)
  {
    csString newKeyKey; newKeyKey.Format ("i:%s", stateKey);
    view->CreateNode (newKeyKey, "I", styleResponse);
    view->LinkNode (stateKey, newKeyKey);
    BuildRewardGraph (initRewards, newKeyKey, pcKey);
  }
  csRef<iRewardFactoryArray> exitRewards = state->GetExitRewardFactories ();
  if (exitRewards->GetSize () > 0)
  {
    csString newKeyKey; newKeyKey.Format ("e:%s", stateKey);
    view->CreateNode (newKeyKey, "E", styleResponse);
    view->LinkNode (stateKey, newKeyKey);
    BuildRewardGraph (exitRewards, newKeyKey, pcKey);
  }
}

csString EntityMode::GetQuestName (iCelPropertyClassTemplate* pctpl)
{
  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  return InspectTools::GetActionParameterValueString (pl, pctpl,
      "NewQuest", "name");
}

void EntityMode::BuildQuestGraph (iQuestFactory* questFact, const char* pcKey, bool fullquest)
{
  csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
  while (it->HasNext ())
  {
    iQuestStateFactory* stateFact = it->Next ();
    csString stateKey; stateKey.Format ("S:%s,%s", stateFact->GetName (), pcKey);
    view->CreateNode (stateKey, stateFact->GetName (), styleState);
    view->LinkNode (pcKey, stateKey);
    if (fullquest)
      BuildStateGraph (stateFact, stateKey, pcKey);
  }
}

void EntityMode::BuildQuestGraph (iCelPropertyClassTemplate* pctpl,
    const char* pcKey)
{
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  csRef<iQuestManager> quest_mgr = csQueryRegistryOrLoad<iQuestManager> (
    aresed3d->GetObjectRegistry (),
    "cel.manager.quests");
  iQuestFactory* questFact = quest_mgr->GetQuestFactory (questName);
  // @@@ Error check
  if (questFact) BuildQuestGraph (questFact, pcKey, false);
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

void EntityMode::GetPCKeyLabel (iCelPropertyClassTemplate* pctpl, csString& pcKey, csString& pcLabel)
{
  csString pcShortName;
  csString pcName = pctpl->GetName ();
  size_t lastDot = pcName.FindLast ('.');
  if (lastDot != csArrayItemNotFound)
    pcShortName = pcName.Slice (lastDot+1);

  pcKey.Format ("P:%s", pcName.GetData ());
  pcLabel = pcShortName;
  if (pctpl->GetTag () != 0)
  {
    pcKey.AppendFmt (":%s", pctpl->GetTag ());
    pcLabel.AppendFmt (" (%s)", pctpl->GetTag ());
  }

  csString extraInfo = GetExtraPCInfo (pctpl);
  if (!extraInfo.IsEmpty ()) { pcLabel += '\n'; pcLabel += extraInfo; }
}

void EntityMode::BuildTemplateGraph (const char* templateName)
{
  currentTemplate = templateName;
  pcPanel->Hide ();

  view->Clear ();

  view->SetVisible (false);
  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (templateName);
  if (!tpl) return;

  csString tplKey; tplKey.Format ("T:%s", templateName);
  view->CreateNode (tplKey, templateName, styleTemplate);

  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);

    // Extract the last part of the name (everything after the last '.').
    csString pcKey, pcLabel;
    GetPCKeyLabel (pctpl, pcKey, pcLabel);
    view->CreateNode (pcKey, pcLabel, stylePC);
    view->LinkNode (tplKey, pcKey);
    csString pcName = pctpl->GetName ();
    if (pcName == "pclogic.quest")
      BuildQuestGraph (pctpl, pcKey);
  }
  view->SetVisible (true);
}

iCelPropertyClassTemplate* EntityMode::GetPCTemplate (const char* key)
{
  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (!tpl) return 0;

  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'P')
    {
      csStringArray tokens (op, ":");
      csString pcName = tokens[1];
      csString tagName;
      if (tokens.GetSize () >= 3) tagName = tokens[2];
      return tpl->FindPropertyClassTemplate (pcName, tagName);
    }
  }
  return 0;
}

void EntityMode::OnTemplateSelect ()
{
  wxListBox* list = XRCCTRL (*panel, "templateList", wxListBox);
  csString templateName = (const char*)list->GetStringSelection ().mb_str(wxConvUTF8);
  BuildTemplateGraph (templateName);
}

void EntityMode::OnTemplateAdd ()
{
  UIManager* ui = aresed3d->GetApp ()->GetUIManager ();
  UIDialog* dialog = ui->CreateDialog ("New Template");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("name");
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    iCelPlLayer* pl = aresed3d->GetPlLayer ();
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (name);
    if (tpl)
      ui->Error ("A template with this name already exists!");
    else
    {
      tpl = pl->CreateEntityTemplate (name);
      currentTemplate = name;
      BuildTemplateGraph (currentTemplate);
      SetupItems ();
    }
  }
  delete dialog;
}

void EntityMode::OnTemplateDel ()
{
  UIManager* ui = aresed3d->GetApp ()->GetUIManager ();
  ui->Message ("Not implemented yet!");
}

void EntityMode::OnDelete ()
{
  UIManager* ui = aresed3d->GetApp ()->GetUIManager ();
  ui->Message ("Not implemented yet!");
}

void EntityMode::OnCreatePC ()
{
  UIManager* ui = aresed3d->GetApp ()->GetUIManager ();
  UIDialog* dialog = ui->CreateDialog ("New PropertyClass");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddChoice ("name", "pcobject.mesh", "pctools.properties",
      "pctools.inventory", "pclogic.quest", "pclogic.spawn",
      "pclogic.wire", (const char*)0);
  dialog->AddRow ();
  dialog->AddLabel ("Tag:");
  dialog->AddText ("tag");
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    csString tag = fields.Get ("tag", "");
    printf ("name=%s tag=%s\n", name.GetData (), tag.GetData ());
    iCelPlLayer* pl = aresed3d->GetPlLayer ();
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    iCelPropertyClassTemplate* pc = tpl->FindPropertyClassTemplate (name, tag);
    if (pc)
      ui->Error ("Property class with this name and tag already exists!");
    else
    {
      pc = tpl->CreatePropertyClassTemplate ();
      pc->SetName (name);
      if (tag && *tag)
        pc->SetTag (tag);

      BuildTemplateGraph (currentTemplate);
    }

  }
  delete dialog;
}

void EntityMode::PCWasEdited (iCelPropertyClassTemplate* pctpl)
{
  csString activeNode = view->GetActiveNode ();

  csString newKey, newLabel;
  GetPCKeyLabel (pctpl, newKey, newLabel);
  view->ReplaceNode (activeNode, newKey, newLabel, stylePC);
}

void EntityMode::ActivateNode (const char* nodeName)
{
  if (!nodeName) { pcPanel->Hide (); return; }
  csString activeNode = nodeName;
  const char type = activeNode.operator[] (0);
  if (type == 'P')
  {
    iCelPlLayer* pl = aresed3d->GetPlLayer ();
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    iCelPropertyClassTemplate* pctpl = GetPCTemplate (activeNode);
    pcPanel->SwitchToPC (tpl, pctpl);
    pcPanel->Show ();
  }
  else
  {
    pcPanel->Hide ();
  }
}

void EntityMode::OnEditQuest ()
{
  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (!tpl) return;

  csStringArray tokens (contextMenuNode, ":");
  csString pcName = tokens[1];
  csString tagName;
  if (tokens.GetSize () >= 3) tagName = tokens[2];
  iCelPropertyClassTemplate* pctpl = tpl->FindPropertyClassTemplate (
      pcName, tagName);
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  csRef<iQuestManager> quest_mgr = csQueryRegistryOrLoad<iQuestManager> (
    aresed3d->GetObjectRegistry (),
    "cel.manager.quests");
  iQuestFactory* questFact = quest_mgr->GetQuestFactory (questName);
  // @@@ Error check
  if (questFact)
  {
    view->Clear ();
    view->SetVisible (false);

    csString pcKey, pcLabel;
    GetPCKeyLabel (pctpl, pcKey, pcLabel);
    view->CreateNode (pcKey, pcLabel, stylePC);

    BuildQuestGraph (questFact, pcKey, true);
    view->SetVisible (true);
  }
}

void EntityMode::AllocContextHandlers (wxFrame* frame)
{
  UIManager* ui = aresed3d->GetApp ()->GetUIManager ();

  idDelete = ui->AllocContextMenuID ();
  frame->Connect (idDelete, wxEVT_COMMAND_MENU_SELECTED,
	      wxCommandEventHandler (EntityMode::Panel::OnDelete), 0, panel);
  idCreate = ui->AllocContextMenuID ();
  frame->Connect (idCreate, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreatePC), 0, panel);
  idEditQuest = ui->AllocContextMenuID ();
  frame->Connect (idEditQuest, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnEditQuest), 0, panel);
}

void EntityMode::AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY)
{
  contextMenuNode = view->FindHitNode (mouseX, mouseY);
  if (!contextMenuNode.IsEmpty ())
  {
    contextMenu->AppendSeparator ();

    const char type = contextMenuNode.operator[] (0);
    if (strchr ("TPStier", type))
      contextMenu->Append (idDelete, wxT ("Delete"));
    if (type == 'T')
      contextMenu->Append (idCreate, wxT ("Create Property Class..."));
    if (type == 'P' && contextMenuNode.StartsWith ("P:pclogic.quest"))
      contextMenu->Append (idEditQuest, wxT ("Edit quest"));
  }
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
  if (code == '1')
  {
    float f = view->GetNodeForceFactor ();
    f -= 5.0f;
    view->SetNodeForceFactor (f);
    printf ("Node force factor %g\n", f); fflush (stdout);
  }
  if (code == '2')
  {
    float f = view->GetNodeForceFactor ();
    f += 5.0f;
    view->SetNodeForceFactor (f);
    printf ("Node force factor %g\n", f); fflush (stdout);
  }
  if (code == '3')
  {
    float f = view->GetLinkForceFactor ();
    f -= 0.01f;
    view->SetLinkForceFactor (f);
    printf ("Link force factor %g\n", f); fflush (stdout);
  }
  if (code == '4')
  {
    float f = view->GetLinkForceFactor ();
    f += 0.01f;
    view->SetLinkForceFactor (f);
    printf ("Link force factor %g\n", f); fflush (stdout);
  }
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

