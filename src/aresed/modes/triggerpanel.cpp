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

#include "triggerpanel.h"
#include "entitymode.h"
#include "../ui/uimanager.h"
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "../apparesed.h"
#include "edcommon/uitools.h"
#include "edcommon/inspect.h"
#include "edcommon/tools.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(TriggerPanel, wxPanel)
  EVT_CHOICEBOOK_PAGE_CHANGED (XRCID("triggerChoicebook"), TriggerPanel :: OnChoicebookPageChange)

  EVT_CHECKBOX (XRCID("onChange_Pc_Check"), TriggerPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("leave_Tr_Check"), TriggerPanel :: OnUpdateEvent)
  EVT_CHOICE (XRCID("operation_Pc_Choice"), TriggerPanel :: OnUpdateEvent)

  EVT_TEXT_ENTER (XRCID("entity_Es_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Es_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("sector_Es_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_BUTTON (XRCID("this_Es_Button"), TriggerPanel :: OnSetThisSectorEnter)

  EVT_TEXT_ENTER (XRCID("entity_In_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_In_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("child_In_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("childTemplate_In_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_BUTTON (XRCID("this_In_Button"), TriggerPanel :: OnSetThisInventory)

  EVT_TEXT_ENTER (XRCID("entity_Ms_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Ms_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Me_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_BUTTON (XRCID("this_Ms_Button"), TriggerPanel :: OnSetThisMeshSelect)

  EVT_TEXT_ENTER (XRCID("mask_Me_Combo"), TriggerPanel :: OnUpdateEvent)
  EVT_COMBOBOX (XRCID("mask_Me_Combo"), TriggerPanel :: OnUpdateEvent)

  EVT_TEXT_ENTER (XRCID("entity_Pc_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Pc_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("property_Pc_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("value_Pc_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_BUTTON (XRCID("this_Pc_Button"), TriggerPanel :: OnSetThisProperty)

  EVT_TEXT_ENTER (XRCID("entity_Sf_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Sf_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("sequence_Sf_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_BUTTON (XRCID("this_Sf_Button"), TriggerPanel :: OnSetThisQuest)

  EVT_TEXT_ENTER (XRCID("timeout_To_Text"), TriggerPanel :: OnUpdateEvent)

  EVT_TEXT_ENTER (XRCID("entity_Tr_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Tr_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_BUTTON (XRCID("this_Tr_Button"), TriggerPanel :: OnSetThisTrigger)

  EVT_TEXT_ENTER (XRCID("entity_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("target_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("targettag_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("checktime_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("radius_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("offsetX_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("offsetY_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("offsetZ_Wa_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_BUTTON (XRCID("this_Wa_Button"), TriggerPanel :: OnSetThisWatch)

  EVT_TEXT_ENTER (XRCID("entity_Me_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Me_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("sector_Me_Text"), TriggerPanel :: OnUpdateEvent)
  EVT_BUTTON (XRCID("this_Me_Button"), TriggerPanel :: OnSetThisMeshEnter)

END_EVENT_TABLE()

//--------------------------------------------------------------------------

void TriggerPanel::OnUpdateEvent (wxCommandEvent& event)
{
  printf ("Update Trigger!\n"); fflush (stdout);
  UpdateTrigger ();
}

void TriggerPanel::OnChoicebookPageChange (wxChoicebookEvent& event)
{
  UpdateTrigger ();
}

void TriggerPanel::OnSetThisInventory (wxCommandEvent& event)
{
  UITools::SetValue (this, "entity_In_Text", "$this");
  UITools::SetValue (this, "tag_In_Text", "");
}

void TriggerPanel::OnSetThisMeshSelect (wxCommandEvent& event)
{
  UITools::SetValue (this, "entity_Ms_Text", "$this");
  UITools::SetValue (this, "tag_Ms_Text", "");
}

void TriggerPanel::OnSetThisMeshEnter (wxCommandEvent& event)
{
  UITools::SetValue (this, "entity_Me_Text", "$this");
  UITools::SetValue (this, "tag_Me_Text", "");
}

void TriggerPanel::OnSetThisSectorEnter (wxCommandEvent& event)
{
  UITools::SetValue (this, "entity_Es_Text", "$this");
  UITools::SetValue (this, "tag_Es_Text", "");
}

void TriggerPanel::OnSetThisProperty (wxCommandEvent& event)
{
  UITools::SetValue (this, "entity_Pc_Text", "$this");
  UITools::SetValue (this, "tag_Pc_Text", "");
}

void TriggerPanel::OnSetThisTrigger (wxCommandEvent& event)
{
  UITools::SetValue (this, "entity_Tr_Text", "$this");
  UITools::SetValue (this, "tag_Tr_Text", "");
}

void TriggerPanel::OnSetThisWatch (wxCommandEvent& event)
{
  UITools::SetValue (this, "entity_Wa_Text", "$this");
  UITools::SetValue (this, "tag_Wa_Text", "");
}

void TriggerPanel::OnSetThisQuest (wxCommandEvent& event)
{
  UITools::SetValue (this, "entity_Sf_Text", "$this");
  UITools::SetValue (this, "tag_Sf_Text", emode->GetSelectedPC ()->GetTag ());
}

csString TriggerPanel::GetCurrentTriggerType ()
{
  if (!triggerResp) return "";
  csString trigger = triggerResp->GetTriggerFactory ()->GetTriggerType ()->GetName ();
  csString triggerS = trigger;
  if (triggerS.StartsWith ("cel.triggers."))
    triggerS = trigger.GetData ()+13;
  return triggerS;
}

void TriggerPanel::SwitchTrigger (iQuestTriggerResponseFactory* triggerResp)
{
  TriggerPanel::triggerResp = triggerResp;
  UITools::SwitchPage (this, "triggerChoicebook", GetCurrentTriggerType ());
  UpdatePanel ();
}

void TriggerPanel::UpdatePanel ()
{
  csString type = GetCurrentTriggerType ();
  if (type == "entersector" || type == "meshentersector")
  {
    csRef<iEnterSectorTriggerFactory> tf = scfQueryInterface<iEnterSectorTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "entity_Es_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_Es_Text", tf->GetTag ());
    UITools::SetValue (this, "sector_Es_Text", tf->GetSector ());
  }
  else if (type == "inventory")
  {
    csRef<iInventoryTriggerFactory> tf = scfQueryInterface<iInventoryTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "entity_In_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_In_Text", tf->GetTag ());
    UITools::SetValue (this, "child_In_Text", tf->GetChildEntity ());
    UITools::SetValue (this, "childTemplate_In_Text", tf->GetChildTemplate ());
  }
  else if (type == "meshselect")
  {
    csRef<iMeshSelectTriggerFactory> tf = scfQueryInterface<iMeshSelectTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "entity_Ms_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_Ms_Text", tf->GetTag ());
  }
  else if (type == "message")
  {
    csRef<iMessageTriggerFactory> tf = scfQueryInterface<iMessageTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "entity_Me_Text", tf->GetEntity ());
    UITools::ClearChoices (this, "mask_Me_Combo");

    const AresConfig& config = uiManager->GetApp ()->GetConfig ();
    const csArray<KnownMessage>& messages = config.GetMessages ();
    for (size_t i = 0 ; i < messages.GetSize () ; i++)
      UITools::AddChoices (this, "mask_Me_Combo", messages.Get (i).name.GetData (),
	(const char*)0);
    UITools::SetValue (this, "mask_Me_Combo", tf->GetMask ());
  }
  else if (type == "operation")
  {
    // @@@ TODO
  }
  else if (type == "propertychange")
  {
    csRef<iPropertyChangeTriggerFactory> tf = scfQueryInterface<iPropertyChangeTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "entity_Pc_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_Pc_Text", tf->GetTag ());
    UITools::SetValue (this, "property_Pc_Text", tf->GetProperty ());
    UITools::SetValue (this, "value_Pc_Text", tf->GetValue ());
    UITools::SetValue (this, "operation_Pc_Choice", tf->GetOperation ());
    wxCheckBox* check = XRCCTRL (*this, "onChange_Pc_Check", wxCheckBox);
    check->SetValue (tf->IsOnChangeOnly ());
  }
  else if (type == "sequencefinish")
  {
    csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "entity_Sf_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_Sf_Text", tf->GetTag ());
    UITools::SetValue (this, "sequence_Sf_Text", tf->GetSequence ());
  }
  else if (type == "timeout")
  {
    csRef<iTimeoutTriggerFactory> tf = scfQueryInterface<iTimeoutTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "timeout_To_Text", tf->GetTimeout ());
  }
  else if (type == "trigger")
  {
    csRef<iTriggerTriggerFactory> tf = scfQueryInterface<iTriggerTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "entity_Tr_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_Tr_Text", tf->GetTag ());
    wxCheckBox* check = XRCCTRL (*this, "leave_Tr_Check", wxCheckBox);
    check->SetValue (tf->IsLeaveEnabled ());
  }
  else if (type == "watch")
  {
    csRef<iWatchTriggerFactory> tf = scfQueryInterface<iWatchTriggerFactory> (triggerResp->GetTriggerFactory ());
    UITools::SetValue (this, "entity_Wa_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_Wa_Text", tf->GetTag ());
    UITools::SetValue (this, "target_Wa_Text", tf->GetTargetEntity ());
    UITools::SetValue (this, "targettag_Wa_Text", tf->GetTargetTag ());
    UITools::SetValue (this, "checktime_Wa_Text", tf->GetChecktime ());
    UITools::SetValue (this, "radius_Wa_Text", tf->GetRadius ());
    UITools::SetValue (this, "offsetX_Wa_Text", tf->GetOffsetX ());
    UITools::SetValue (this, "offsetY_Wa_Text", tf->GetOffsetY ());
    UITools::SetValue (this, "offsetZ_Wa_Text", tf->GetOffsetZ ());
  }
  else
  {
    printf ("Internal error: unknown type '%s'\n", type.GetData ());
  }
}

void TriggerPanel::UpdateTrigger ()
{
  if (!triggerResp) return;
  wxChoicebook* book = XRCCTRL (*this, "triggerChoicebook", wxChoicebook);
  int pageSel = book->GetSelection ();
  if (pageSel == wxNOT_FOUND)
  {
    uiManager->Error ("Internal error! Page not found!");
    return;
  }
  wxString pageTxt = book->GetPageText (pageSel);
  iQuestManager* questMgr = emode->GetQuestManager ();
  csString type = (const char*)pageTxt.mb_str (wxConvUTF8);
  if (type != GetCurrentTriggerType ())
  {
    iTriggerType* triggertype = questMgr->GetTriggerType ("cel.triggers."+type);
    csRef<iTriggerFactory> triggerfact = triggertype->CreateTriggerFactory ();
    triggerResp->SetTriggerFactory (triggerfact);
    UpdatePanel ();
  }
  else
  {
    if (type == "entersector" || type == "meshentersector")
    {
      csRef<iEnterSectorTriggerFactory> tf = scfQueryInterface<iEnterSectorTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Es_Text"),
	  UITools::GetValue (this, "tag_Es_Text"));
      tf->SetSectorParameter (UITools::GetValue (this, "sector_Es_Text"));
    }
    else if (type == "inventory")
    {
      csRef<iInventoryTriggerFactory> tf = scfQueryInterface<iInventoryTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetEntityParameter (UITools::GetValue (this, "entity_In_Text"),
	  UITools::GetValue (this, "tag_In_Text"));
      tf->SetChildEntityParameter (UITools::GetValue (this, "child_In_Text"));
      tf->SetChildTemplateParameter (UITools::GetValue (this, "childTemplate_In_Text"));
    }
    else if (type == "meshselect")
    {
      csRef<iMeshSelectTriggerFactory> tf = scfQueryInterface<iMeshSelectTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Ms_Text"),
	  UITools::GetValue (this, "tag_Ms_Text"));
    }
    else if (type == "message")
    {
      csRef<iMessageTriggerFactory> tf = scfQueryInterface<iMessageTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Me_Text"));
      tf->SetMaskParameter (UITools::GetValue (this, "mask_Me_Combo"));
    }
    else if (type == "operation")
    {
      // @@@ TODO
    }
    else if (type == "propertychange")
    {
      csRef<iPropertyChangeTriggerFactory> tf = scfQueryInterface<iPropertyChangeTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Pc_Text"),
	  UITools::GetValue (this, "tag_Pc_Text"));
      tf->SetPropertyParameter (UITools::GetValue (this, "property_Pc_Text"));
      tf->SetValueParameter (UITools::GetValue (this, "value_Pc_Text"));
      tf->SetOperationParameter (UITools::GetValue (this, "operation_Pc_Choice"));
      wxCheckBox* check = XRCCTRL (*this, "onChange_Pc_Check", wxCheckBox);
      tf->SetOnChangeOnly (check->GetValue ());
    }
    else if (type == "sequencefinish")
    {
      csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Sf_Text"),
	  UITools::GetValue (this, "tag_Sf_Text"));
      tf->SetSequenceParameter (UITools::GetValue (this, "sequence_Sf_Text"));
    }
    else if (type == "timeout")
    {
      csRef<iTimeoutTriggerFactory> tf = scfQueryInterface<iTimeoutTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetTimeoutParameter (UITools::GetValue (this, "timeout_To_Text"));
    }
    else if (type == "trigger")
    {
      csRef<iTriggerTriggerFactory> tf = scfQueryInterface<iTriggerTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Tr_Text"),
	  UITools::GetValue (this, "tag_Tr_Text"));
      wxCheckBox* check = XRCCTRL (*this, "leave_Tr_Check", wxCheckBox);
      tf->EnableLeave (check->GetValue ());
    }
    else if (type == "watch")
    {
      csRef<iWatchTriggerFactory> tf = scfQueryInterface<iWatchTriggerFactory> (triggerResp->GetTriggerFactory ());
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Wa_Text"),
	  UITools::GetValue (this, "tag_Wa_Text"));
      tf->SetTargetEntityParameter (UITools::GetValue (this, "target_Wa_Text"),
	  UITools::GetValue (this, "targettag_Wa_Text"));
      tf->SetChecktimeParameter (UITools::GetValue (this, "checktime_Wa_Text"));
      tf->SetRadiusParameter (UITools::GetValue (this, "radius_Wa_Text"));
      tf->SetOffsetParameter (UITools::GetValue (this, "offsetX_Wa_Text"),
	  UITools::GetValue (this, "offsetY_Wa_Text"),
	  UITools::GetValue (this, "offsetZ_Wa_Text"));
    }
    else
    {
      printf ("Internal error: unknown type '%s'\n", type.GetData ());
    }
  }
  emode->RefreshView ();
}

// -----------------------------------------------------------------------

TriggerPanel::TriggerPanel (wxWindow* parent, UIManager* uiManager,
    EntityMode* emode) :
  uiManager (uiManager), emode (emode)
{
  triggerResp = 0;
  pl = uiManager->GetApp ()->GetAresView ()->GetPL ();
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("TriggerPanel"));
}

TriggerPanel::~TriggerPanel ()
{
}


