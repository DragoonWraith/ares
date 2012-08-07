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

#include <crystalspace.h>
#include "edcommon/listctrltools.h"
#include "edcommon/inspect.h"
#include "entitymode.h"
#include "templatepanel.h"
#include "pcpanel.h"
#include "triggerpanel.h"
#include "rewardpanel.h"
#include "sequencepanel.h"
#include "editor/i3dview.h"
#include "editor/iapp.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"

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

SCF_IMPLEMENT_FACTORY (EntityMode)

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

EntityMode::EntityMode (iBase* parent) : scfImplementationType (this, parent)
{
  name = "Entity";
}

bool EntityMode::Initialize (iObjectRegistry* object_reg)
{
  if (!EditingMode::Initialize (object_reg)) return false;

  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  iGraphics2D* g2d = g3d->GetDriver2D ();
  font = g2d->GetFontServer ()->LoadFont ("DejaVuSans", 10);
  fontBold = g2d->GetFontServer ()->LoadFont ("DejaVuSansBold", 10);
  fontLarge = g2d->GetFontServer ()->LoadFont ("DejaVuSans", 13);

  questMgr = csQueryRegistryOrLoad<iQuestManager> (object_reg,
      "cel.manager.quests");

  return true;
}

void EntityMode::SetParent (wxWindow* parent)
{
  panel = new Panel (parent, this);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("EntityModePanel"));

  pcPanel = new PropertyClassPanel (panel, view3d->GetApplication ()->GetUI (), this);
  pcPanel->Hide ();

  triggerPanel = new TriggerPanel (panel, view3d->GetApplication ()->GetUI (), this);
  triggerPanel->Hide ();

  rewardPanel = new RewardPanel (panel, view3d->GetApplication ()->GetUI (), this);
  rewardPanel->Hide ();

  sequencePanel = new SequencePanel (panel, view3d->GetApplication ()->GetUI (), this);
  sequencePanel->Hide ();

  tplPanel = new EntityTemplatePanel (panel, view3d->GetApplication ()->GetUI (), this);
  tplPanel->Hide ();

  graphView = markerMgr->CreateGraphView ();
  graphView->Clear ();
  csRef<GraphNodeCallback> cb;
  cb.AttachNew (new GraphNodeCallback (this));
  graphView->AddNodeActivationCallback (cb);

  graphView->SetVisible (false);

  InitColors ();
  editQuestMode = false;
}

EntityMode::~EntityMode ()
{
  markerMgr->DestroyGraphView (graphView);
}

iAresEditor* EntityMode::GetApplication () const
{
  return view3d->GetApplication ();
}

iMarkerColor* EntityMode::NewColor (const char* name,
    float r0, float g0, float b0, float r1, float g1, float b1, bool fill)
{
  iMarkerColor* col = markerMgr->CreateMarkerColor (name);
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
  iMarkerColor* col = markerMgr->CreateMarkerColor (name);
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
  iMarkerColor* textColor = NewColor ("viewWhite", .8, .8, .8, 1, 1, 1, false);
  iMarkerColor* textSelColor = NewColor ("viewBlack", 0, 0, 0, 0, 0, 0, false);

  iMarkerColor* thickLinkColor = markerMgr->CreateMarkerColor ("thickLink");
  thickLinkColor->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  thickLinkColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  thickLinkColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  thickLinkColor->SetPenWidth (SELECTION_NONE, 1.2f);
  thickLinkColor->SetPenWidth (SELECTION_SELECTED, 2.0f);
  thickLinkColor->SetPenWidth (SELECTION_ACTIVE, 2.0f);
  iMarkerColor* thinLinkColor = markerMgr->CreateMarkerColor ("thinLink");
  thinLinkColor->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  thinLinkColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  thinLinkColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  thinLinkColor->SetPenWidth (SELECTION_NONE, 0.8f);
  thinLinkColor->SetPenWidth (SELECTION_SELECTED, 0.8f);
  thinLinkColor->SetPenWidth (SELECTION_ACTIVE, 0.8f);
  iMarkerColor* arrowLinkColor = markerMgr->CreateMarkerColor ("arrowLink");
  arrowLinkColor->SetRGBColor (SELECTION_NONE, .6, .6, .6, .5);
  arrowLinkColor->SetRGBColor (SELECTION_SELECTED, .7, .7, .7, .5);
  arrowLinkColor->SetRGBColor (SELECTION_ACTIVE, .7, .7, .7, .5);
  arrowLinkColor->SetPenWidth (SELECTION_NONE, 0.3f);
  arrowLinkColor->SetPenWidth (SELECTION_SELECTED, 0.3f);
  arrowLinkColor->SetPenWidth (SELECTION_ACTIVE, 0.3f);

  styleTemplate = markerMgr->CreateGraphNodeStyle ();
  styleTemplate->SetBorderColor (NewColor ("templateColorFG", .0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleTemplate->SetBackgroundColor (NewColor ("templateColorBG", .1, .4, .5, .2, .6, .7, true));
  styleTemplate->SetTextColor (textColor);
  styleTemplate->SetTextFont (fontLarge);

  stylePC = markerMgr->CreateGraphNodeStyle ();
  stylePC->SetBorderColor (NewColor ("pcColorFG", 0, 0, .7, 0, 0, 1, 1, 1, 1, false));
  stylePC->SetBackgroundColor (NewColor ("pcColorBG", .1, .4, .5, .2, .6, .7, true));
  stylePC->SetTextColor (textColor);
  stylePC->SetTextFont (font);

  iMarkerColor* colStateFG = NewColor ("stateColorFG", 0, .7, 0, 0, 1, 0, 1, 1, 1, false);
  iMarkerColor* colStateBG = NewColor ("stateColorBG", .1, .4, .5, .2, .6, .7, true);
  styleState = markerMgr->CreateGraphNodeStyle ();
  styleState->SetBorderColor (colStateFG);
  styleState->SetBackgroundColor (colStateBG);
  styleState->SetTextColor (textColor);
  styleState->SetTextFont (font);
  styleStateDefault = markerMgr->CreateGraphNodeStyle ();
  styleStateDefault->SetBorderColor (colStateFG);
  styleStateDefault->SetBackgroundColor (colStateBG);
  styleStateDefault->SetTextColor (textSelColor);
  styleStateDefault->SetTextFont (fontBold);

  iMarkerColor* colSeqFG = NewColor ("seqColorFG", 0, 0, 0, 0, 0, 0, 1, 1, 1, false);
  iMarkerColor* colSeqBG = NewColor ("seqColorBG", .8, 0, 0, 1, 0, 0, true);
  styleSequence = markerMgr->CreateGraphNodeStyle ();
  styleSequence->SetBorderColor (colSeqFG);
  styleSequence->SetBackgroundColor (colSeqBG);
  styleSequence->SetTextColor (textColor);
  styleSequence->SetTextFont (font);

  styleResponse = markerMgr->CreateGraphNodeStyle ();
  styleResponse->SetBorderColor (NewColor ("respColorFG", 0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleResponse->SetBackgroundColor (NewColor ("respColorBG", .3, .6, .7, .4, .7, .8, true));
  styleResponse->SetRoundness (5);
  styleResponse->SetTextColor (NewColor ("respColorTxt", 0, 0, 0, 0, 0, 0, false));
  styleResponse->SetTextFont (font);

  styleReward = markerMgr->CreateGraphNodeStyle ();
  styleReward->SetBorderColor (NewColor ("rewColorFG", 0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleReward->SetBackgroundColor (NewColor ("rewColorBG", .3, .6, .7, .4, .7, .8, true));
  styleReward->SetRoundness (1);
  styleReward->SetTextColor (textColor);
  styleReward->SetTextFont (font);
  styleReward->SetConnectorStyle (CONNECTOR_RIGHT);

  graphView->SetDefaultNodeStyle (stylePC);

  styleThickLink = markerMgr->CreateGraphLinkStyle ();
  styleThickLink->SetColor (thickLinkColor);
  styleThinLink = markerMgr->CreateGraphLinkStyle ();
  styleThinLink->SetColor (thinLinkColor);
  styleArrowLink = markerMgr->CreateGraphLinkStyle ();
  styleArrowLink->SetColor (arrowLinkColor);
  styleArrowLink->SetArrow (true);
  styleArrowLink->SetSoft (true);
  styleArrowLink->SetLinkStrength (0.0);

  graphView->SetDefaultLinkStyle (styleThickLink);
}

void EntityMode::SetupItems ()
{
  wxListBox* list = XRCCTRL (*panel, "templateList", wxListBox);
  list->Clear ();
  wxArrayString names;
  csRef<iCelEntityTemplateIterator> it = pl->GetEntityTemplates ();
  while (it->HasNext ())
  {
    iCelEntityTemplate* tpl = it->Next ();
    wxString name = wxString::FromUTF8 (tpl->GetName ());
    names.Add (name);
  }
  list->InsertItems (names, 0);
}

void EntityMode::Start ()
{
  view3d->GetApplication ()->HideCameraWindow ();
  SetupItems ();
  graphView->SetVisible (true);
  pcPanel->Hide ();
  triggerPanel->Hide ();
  rewardPanel->Hide ();
  sequencePanel->Hide ();
  tplPanel->Hide ();
  contextMenuNode = "";
}

void EntityMode::Stop ()
{
  graphView->SetVisible (false);
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
    csString rewKey; rewKey.Format ("r:%zu,%s", j, parentKey);
    csString rewLabel; rewLabel.Format ("%zu:%s", j+1, GetRewardType (reward));
    graphView->CreateSubNode (parentKey, rewKey, rewLabel, styleReward);
    //graphView->LinkNode (parentKey, rewKey, styleThinLink);

    csRef<iNewStateQuestRewardFactory> newState = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    if (newState)
    {
      // @@@ No support for expressions here!
      csString stateKey; stateKey.Format ("S:%s,%s", newState->GetStateParameter (), pcKey);
      graphView->LinkNode (rewKey, stateKey, styleArrowLink);
    }
  }
}

csString EntityMode::GetRewardsLabel (iRewardFactoryArray* rewards)
{
  csString label;
  for (size_t j = 0 ; j < rewards->GetSize () ; j++)
  {
    label += csString ("\n    ");// + GetRewardType (reward);
  }
  return label;
}

void EntityMode::BuildStateGraph (iQuestStateFactory* state,
    const char* stateKey, const char* pcKey)
{
  csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
  for (size_t i = 0 ; i < responses->GetSize () ; i++)
  {
    iQuestTriggerResponseFactory* response = responses->Get (i);
    csString responseKey; responseKey.Format ("t:%zu,%s", i, stateKey);
    csString triggerLabel = GetTriggerType (response->GetTriggerFactory ());
    triggerLabel += GetRewardsLabel (response->GetRewardFactories ());
    graphView->CreateNode (responseKey, triggerLabel, styleResponse);
    graphView->LinkNode (stateKey, responseKey);
    csRef<iRewardFactoryArray> rewards = response->GetRewardFactories ();
    BuildRewardGraph (rewards, responseKey, pcKey);
  }

  csRef<iRewardFactoryArray> initRewards = state->GetInitRewardFactories ();
  if (initRewards->GetSize () > 0)
  {
    csString newKeyKey; newKeyKey.Format ("i:,%s", stateKey);
    csString label = "Oninit:";
    label += GetRewardsLabel (initRewards);
    graphView->CreateNode (newKeyKey, label, styleResponse);
    graphView->LinkNode (stateKey, newKeyKey);
    BuildRewardGraph (initRewards, newKeyKey, pcKey);
  }
  csRef<iRewardFactoryArray> exitRewards = state->GetExitRewardFactories ();
  if (exitRewards->GetSize () > 0)
  {
    csString newKeyKey; newKeyKey.Format ("e:,%s", stateKey);
    csString label = "Onexit:";
    label += GetRewardsLabel (exitRewards);
    graphView->CreateNode (newKeyKey, label, styleResponse);
    graphView->LinkNode (stateKey, newKeyKey);
    BuildRewardGraph (exitRewards, newKeyKey, pcKey);
  }
}

csString EntityMode::GetQuestName (iCelPropertyClassTemplate* pctpl)
{
  return InspectTools::GetActionParameterValueString (pl, pctpl,
      "NewQuest", "name");
}

void EntityMode::BuildQuestGraph (iQuestFactory* questFact, const char* pcKey,
    bool fullquest, const csString& defaultState)
{
  csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
  while (it->HasNext ())
  {
    iQuestStateFactory* stateFact = it->Next ();
    csString stateKey; stateKey.Format ("S:%s,%s", stateFact->GetName (), pcKey);
    graphView->CreateNode (stateKey, stateFact->GetName (),
	defaultState == stateFact->GetName () ? styleStateDefault : styleState);
    graphView->LinkNode (pcKey, stateKey);
    if (fullquest)
      BuildStateGraph (stateFact, stateKey, pcKey);
  }
  csRef<iCelSequenceFactoryIterator> seqIt = questFact->GetSequences ();
  while (seqIt->HasNext ())
  {
    iCelSequenceFactory* seqFact = seqIt->Next ();
    csString seqKey; seqKey.Format ("s:%s,%s", seqFact->GetName (), pcKey);
    graphView->CreateNode (seqKey, seqFact->GetName (), styleSequence);
    graphView->LinkNode (pcKey, seqKey);
  }
}

void EntityMode::BuildQuestGraph (iCelPropertyClassTemplate* pctpl,
    const char* pcKey)
{
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  csString defaultState = InspectTools::GetPropertyValueString (pl, pctpl, "state");

  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  // @@@ Error check
  if (questFact) BuildQuestGraph (questFact, pcKey, false, defaultState);
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

  graphView->StartRefresh ();

  graphView->SetVisible (false);
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (templateName);
  if (!tpl) { graphView->FinishRefresh (); return; }

  csString tplKey; tplKey.Format ("T:%s", templateName);
  graphView->CreateNode (tplKey, templateName, styleTemplate);

  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);

    // Extract the last part of the name (everything after the last '.').
    csString pcKey, pcLabel;
    GetPCKeyLabel (pctpl, pcKey, pcLabel);
    graphView->CreateNode (pcKey, pcLabel, stylePC);
    graphView->LinkNode (tplKey, pcKey);
    csString pcName = pctpl->GetName ();
    if (pcName == "pclogic.quest")
      BuildQuestGraph (pctpl, pcKey);
  }
  graphView->FinishRefresh ();
  graphView->SetVisible (true);
}

void EntityMode::RefreshView (iCelPropertyClassTemplate* pctpl)
{
  if (editQuestMode)
  {
    if (!pctpl)
    {
      if (GetContextMenuNode ().IsEmpty ()) return;
      pctpl = GetPCTemplate (GetContextMenuNode ());
    }
    csString questName = GetQuestName (pctpl);
    if (questName.IsEmpty ()) return;

    iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
    if (!questFact)
      questFact = questMgr->CreateQuestFactory (questName);

    graphView->StartRefresh ();
    graphView->SetVisible (false);

    csString pcKey, pcLabel;
    GetPCKeyLabel (pctpl, pcKey, pcLabel);
    graphView->CreateNode (pcKey, pcLabel, stylePC);

    csString defaultState;	// Empty: we have no default state here.
    BuildQuestGraph (questFact, pcKey, true, defaultState);
    graphView->FinishRefresh ();
    graphView->SetVisible (true);
  }
  else
    BuildTemplateGraph (currentTemplate);
}

iQuestFactory* EntityMode::GetSelectedQuest (const char* key)
{
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (key);
  csString questName = GetQuestName (pctpl);
  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  return questFact;
}

bool EntityMode::IsOnInit (const char* key)
{
  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'i') return true;
  }
  return 0;
}

bool EntityMode::IsOnExit (const char* key)
{
  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'e') return true;
  }
  return 0;
}

iCelSequenceFactory* EntityMode::GetSelectedSequence (const char* key)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (!tpl) return 0;

  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 's')
    {
      csStringArray tokens (op, ":");
      csString sequenceName = tokens[1];
      iQuestFactory* questFact = GetSelectedQuest (key);
      return questFact->GetSequence (sequenceName);
    }
  }
  return 0;
}

csRef<iRewardFactoryArray> EntityMode::GetSelectedReward (const char* key,
    size_t& idx)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (!tpl) return 0;

  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'r')
    {
      csRef<iRewardFactoryArray> array;
      csStringArray tokens (op, ":");
      csString triggerNum = tokens[1];
      int index;
      csScanStr (triggerNum, "%d", &index);
      idx = index;
      if (IsOnInit (key))
      {
        iQuestStateFactory* state = GetSelectedState (key);
	return state->GetInitRewardFactories ();
      }
      else if (IsOnExit (key))
      {
        iQuestStateFactory* state = GetSelectedState (key);
	return state->GetExitRewardFactories ();
      }
      else
      {
        iQuestTriggerResponseFactory* resp = GetSelectedTriggerResponse (key);
        if (!resp) return 0;
        return resp->GetRewardFactories ();
      }
    }
  }
  return 0;
}

iQuestTriggerResponseFactory* EntityMode::GetSelectedTriggerResponse (const char* key)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (!tpl) return 0;

  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 't')
    {
      csStringArray tokens (op, ":");
      csString triggerNum = tokens[1];
      int num;
      csScanStr (triggerNum, "%d", &num);
      iQuestStateFactory* state = GetSelectedState (key);
      if (!state) return 0;
      return state->GetTriggerResponseFactories ()->Get (num);
    }
  }
  return 0;
}

csString EntityMode::GetSelectedStateName (const char* key)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (!tpl) return "";

  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'S')
    {
      csStringArray tokens (op, ":");
      csString stateName = tokens[1];
      return stateName;
    }
  }
  return "";
}

iQuestStateFactory* EntityMode::GetSelectedState (const char* key)
{
  csString n = GetSelectedStateName (key);
  if (!n) return 0;
  iQuestFactory* questFact = GetSelectedQuest (key);
  if (!questFact) return 0;
  return questFact->GetState (n);
}


iCelPropertyClassTemplate* EntityMode::GetPCTemplate (const char* key)
{
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
  editQuestMode = false;
  BuildTemplateGraph (templateName);
}

void EntityMode::OnTemplateAdd ()
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New Template", "Name:");
  if (!name->IsEmpty ())
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (name->GetData ());
    if (tpl)
      ui->Error ("A template with this name already exists!");
    else
    {
      tpl = pl->CreateEntityTemplate (name->GetData ());
      currentTemplate = name->GetData ();
      editQuestMode = false;
      BuildTemplateGraph (currentTemplate);
      SetupItems ();
    }
  }
}

void EntityMode::OnTemplateDel ()
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  ui->Message ("Not implemented yet!");
}

void EntityMode::OnDelete ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  const char type = contextMenuNode.operator[] (0);
  if (type == 'T')
  {
    // Delete template.
    ui->Message ("Not implemented yet!");
  }
  else if (type == 'P')
  {
    // Delete property class.
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());
    tpl->RemovePropertyClassTemplate (pctpl);
    editQuestMode = false;
    RefreshView ();
  }
  else if (type == 'S')
  {
    // Delete state.
    csString state = GetSelectedStateName (GetContextMenuNode ());
    iQuestFactory* questFact = GetSelectedQuest (GetContextMenuNode ());
    questFact->RemoveState (state);
    RefreshView ();
  }
  else if (type == 't')
  {
    // Delete trigger.
    csString state = GetSelectedStateName (GetContextMenuNode ());
    iQuestFactory* questFact = GetSelectedQuest (GetContextMenuNode ());
    iQuestStateFactory* questState = questFact->GetState (state);
    csRef<iQuestTriggerResponseFactoryArray> responses = questState->GetTriggerResponseFactories ();
    iQuestTriggerResponseFactory* resp = GetSelectedTriggerResponse (GetContextMenuNode ());
    responses->Delete (resp);
    RefreshView ();
  }
  else if (type == 'r')
  {
    // Delete reward.
    size_t idx;
    csRef<iRewardFactoryArray> array = GetSelectedReward (GetContextMenuNode (), idx);
    if (!array) return;
    array->DeleteIndex (idx);
    RefreshView ();
  }
  else if (type == 's')
  {
    // Delete sequence.
    iCelSequenceFactory* sequence = GetSelectedSequence (GetContextMenuNode ());
    iQuestFactory* questFact = GetSelectedQuest (GetContextMenuNode ());
    questFact->RemoveSequence (sequence->GetName ());
    RefreshView ();
  }
}

void EntityMode::OnCreatePC ()
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New PropertyClass");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddChoice ("name", "pcobject.mesh", "pctools.properties",
      "pctools.inventory", "pclogic.quest", "pclogic.spawn",
      "pclogic.wire", "pctools.messenger",
      "pcinput.standard", "pcphysics.object", "pcphysics.system", "pccamera.old",
      "pcmove.actor.dynamic", "pcmove.actor.standard", "pcmove.actor.wasd",
      "pcworld.dynamic", "ares.gamecontrol",
      (const char*)0);
  dialog->AddRow ();
  dialog->AddLabel ("Tag:");
  dialog->AddText ("tag");
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    csString tag = fields.Get ("tag", "");
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

      RefreshView ();
    }

  }
}

void EntityMode::PCWasEdited (iCelPropertyClassTemplate* pctpl)
{
  RefreshView (pctpl);
}

void EntityMode::ActivateNode (const char* nodeName)
{
  tplPanel->Hide ();
  pcPanel->Hide ();
  triggerPanel->Hide ();
  rewardPanel->Hide ();
  sequencePanel->Hide ();
  if (!nodeName) return;
  csString activeNode = nodeName;
printf ("node:%s\n", nodeName); fflush (stdout);
  const char type = activeNode.operator[] (0);
  if (type == 'P')
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    iCelPropertyClassTemplate* pctpl = GetPCTemplate (activeNode);
    pcPanel->SwitchToPC (tpl, pctpl);
    pcPanel->Show ();
  }
  else if (type == 'T')
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    tplPanel->SwitchToTpl (tpl);
    tplPanel->Show ();
  }
  else if (type == 't')
  {
    iQuestTriggerResponseFactory* resp = GetSelectedTriggerResponse (activeNode);
    if (!resp || !resp->GetTriggerFactory ()) return;
    triggerPanel->SwitchTrigger (resp);
    triggerPanel->Show ();
  }
  else if (type == 'r')
  {
    size_t idx;
    csRef<iRewardFactoryArray> array = GetSelectedReward (activeNode, idx);
    if (!array) return;
    iRewardFactory* reward = array->Get (idx);
    rewardPanel->SwitchReward (GetSelectedQuest (activeNode), array, idx, reward);
    rewardPanel->Show ();
  }
  else if (type == 's')
  {
    iCelSequenceFactory* sequence = GetSelectedSequence (activeNode);
    if (!sequence) return;
    sequencePanel->SwitchSequence (sequence);
    sequencePanel->Show ();
  }
}

void EntityMode::OnCreateReward (int type)
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Reward");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddChoice ("name", "newstate", "debugprint", "action", "changeproperty",
      "createentity", "destroyentity", "inventory", "message", "cssequence",
      "sequence", "sequencefinish", (const char*)0);
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    iRewardType* rewardType = questMgr->GetRewardType ("cel.rewards."+name);
    csRef<iRewardFactory> fact = rewardType->CreateRewardFactory ();
    if (type == 0)
    {
      iQuestTriggerResponseFactory* resp = GetSelectedTriggerResponse (GetContextMenuNode ());
      if (!resp) return;
      resp->AddRewardFactory (fact);
    }
    else
    {
      iQuestStateFactory* state = GetSelectedState (GetContextMenuNode ());
      if (type == 1) state->AddInitRewardFactory (fact);
      else state->AddExitRewardFactory (fact);
    }
    RefreshView ();
  }
}

void EntityMode::OnCreateTrigger ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Trigger");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddChoice ("name", "entersector", "meshentersector", "inventory",
      "meshselect", "message", "operation", "propertychange", "sequencefinish",
      "timeout", "trigger", "watch", (const char*)0);
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    csString state = GetSelectedStateName (GetContextMenuNode ());
    iQuestFactory* questFact = GetSelectedQuest (GetContextMenuNode ());
    iQuestStateFactory* questState = questFact->GetState (state);
    iQuestTriggerResponseFactory* resp = questState->CreateTriggerResponseFactory ();
    iTriggerType* triggertype = questMgr->GetTriggerType ("cel.triggers."+name);
    csRef<iTriggerFactory> triggerFact = triggertype->CreateTriggerFactory ();
    resp->SetTriggerFactory (triggerFact);
    RefreshView ();
  }
}

void EntityMode::OnDefaultState ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());

  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  if (!questFact) return;

  csStringArray tokens (GetContextMenuNode (), ",");
  csString state = tokens[0];
  state = state.Slice (2);
  pctpl->RemoveProperty (pl->FetchStringID ("state"));
  pctpl->SetProperty (pl->FetchStringID ("state"), state.GetData ());
  RefreshView (pctpl);
}

void EntityMode::OnNewSequence ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New Sequence", "Name:");
  if (name->IsEmpty ()) return;

  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  if (!questFact)
    questFact = questMgr->CreateQuestFactory (questName);
  if (questFact->GetSequence (name->GetData ()))
  {
    ui->Error ("Sequence already exists with this name!");
    return;
  }
  questFact->CreateSequence (name->GetData ());
  RefreshView (pctpl);
}

void EntityMode::OnNewState ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New State", "Name:");
  if (name->IsEmpty ()) return;

  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  if (!questFact)
    questFact = questMgr->CreateQuestFactory (questName);
  if (questFact->GetState (name->GetData ()))
  {
    ui->Error ("State already exists with this name!");
    return;
  }
  questFact->CreateState (name->GetData ());
  RefreshView (pctpl);
}

void EntityMode::OnEditQuest ()
{
  editQuestMode = true;
  RefreshView ();
}

void EntityMode::AllocContextHandlers (wxFrame* frame)
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();

  idDelete = ui->AllocContextMenuID ();
  frame->Connect (idDelete, wxEVT_COMMAND_MENU_SELECTED,
	      wxCommandEventHandler (EntityMode::Panel::OnDelete), 0, panel);
  idCreate = ui->AllocContextMenuID ();
  frame->Connect (idCreate, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreatePC), 0, panel);
  idEditQuest = ui->AllocContextMenuID ();
  frame->Connect (idEditQuest, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnEditQuest), 0, panel);
  idNewState = ui->AllocContextMenuID ();
  frame->Connect (idNewState, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnNewState), 0, panel);
  idNewSequence = ui->AllocContextMenuID ();
  frame->Connect (idNewSequence, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnNewSequence), 0, panel);
  idDefaultState = ui->AllocContextMenuID ();
  frame->Connect (idDefaultState, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnDefaultState), 0, panel);
  idCreateTrigger = ui->AllocContextMenuID ();
  frame->Connect (idCreateTrigger, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreateTrigger), 0, panel);
  idCreateReward = ui->AllocContextMenuID ();
  frame->Connect (idCreateReward, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreateReward), 0, panel);
  idCreateRewardOnInit = ui->AllocContextMenuID ();
  frame->Connect (idCreateRewardOnInit, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreateRewardOnInit), 0, panel);
  idCreateRewardOnExit = ui->AllocContextMenuID ();
  frame->Connect (idCreateRewardOnExit, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreateRewardOnExit), 0, panel);
}

csString EntityMode::GetContextMenuNode ()
{
  if (!contextMenuNode) return "";
  if (!graphView->NodeExists (contextMenuNode))
  {
    contextMenuNode = "";
  }
  return contextMenuNode;
}

void EntityMode::AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY)
{
  contextMenuNode = graphView->FindHitNode (mouseX, mouseY);
  if (!contextMenuNode.IsEmpty ())
  {
    contextMenu->AppendSeparator ();

    const char type = contextMenuNode.operator[] (0);
    if (strchr ("TPSstier", type))
      contextMenu->Append (idDelete, wxT ("Delete"));
    if (type == 'T')
      contextMenu->Append (idCreate, wxT ("Create Property Class..."));
    if (type == 'P' && contextMenuNode.StartsWith ("P:pclogic.quest"))
    {
      contextMenu->Append (idEditQuest, wxT ("Edit quest"));
      contextMenu->Append (idNewState, wxT ("New state..."));
      contextMenu->Append (idNewSequence, wxT ("New sequence..."));
    }
    if (type == 'S')
    {
      contextMenu->Append (idDefaultState, wxT ("Set default state"));
      contextMenu->Append (idCreateTrigger, wxT ("Create trigger..."));
      contextMenu->Append (idCreateRewardOnInit, wxT ("Create on-init reward..."));
      contextMenu->Append (idCreateRewardOnExit, wxT ("Create on-exit reward..."));
    }
    if (type == 'i')
    {
      contextMenu->Append (idCreateRewardOnInit, wxT ("Create on-init reward..."));
    }
    if (type == 'e')
    {
      contextMenu->Append (idCreateRewardOnExit, wxT ("Create on-exit reward..."));
    }
    if (type == 't')
    {
      contextMenu->Append (idCreateReward, wxT ("Create reward..."));
    }
  }
}

void EntityMode::FramePre()
{
}

void EntityMode::Frame3D()
{
  g3d->GetDriver2D ()->Clear (g3d->GetDriver2D ()->FindRGB (100, 110, 120));
}

void EntityMode::Frame2D()
{
}

bool EntityMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  if (code == '1')
  {
    float f = graphView->GetNodeForceFactor ();
    f -= 5.0f;
    graphView->SetNodeForceFactor (f);
    printf ("Node force factor %g\n", f); fflush (stdout);
  }
  if (code == '2')
  {
    float f = graphView->GetNodeForceFactor ();
    f += 5.0f;
    graphView->SetNodeForceFactor (f);
    printf ("Node force factor %g\n", f); fflush (stdout);
  }
  if (code == '3')
  {
    float f = graphView->GetLinkForceFactor ();
    f -= 0.01f;
    graphView->SetLinkForceFactor (f);
    printf ("Link force factor %g\n", f); fflush (stdout);
  }
  if (code == '4')
  {
    float f = graphView->GetLinkForceFactor ();
    f += 0.01f;
    graphView->SetLinkForceFactor (f);
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

