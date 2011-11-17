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

  templateColorFG = 0;
  templateColorBG = 0;
  pcColorFG = 0;
  pcColorBG = 0;
  stateColorFG = 0;
  stateColorBG = 0;
  InitColors ();

  view->SetColors (
      mgr->FindMarkerColor ("white"),
      pcColorFG, pcColorBG,
      mgr->FindMarkerColor ("yellow"));
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
  if (templateColorFG) return;

  templateColorFG = NewColor ("templateColorFG", .5, .5, .5, 1, 1, 1, false);
  templateColorBG = NewColor ("templateColorBG", .1, .2, .2, .2, .3, .3, true);
  pcColorFG = NewColor ("pcColorFG", 0, 0, .5, 0, 0, 1, false);
  pcColorBG = NewColor ("pcColorBG", .1, .2, .2, .2, .3, .3, true);
  stateColorFG = NewColor ("stateColorFG", 0, .5, 0, 0, 1, 0, false);
  stateColorBG = NewColor ("stateColorBG", .1, .2, .2, .2, .3, .3, true);
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

void EntityMode::ShowTemplate (const char* templateName)
{
  view->Clear ();

  iCelPlLayer* pl = aresed3d->GetPlLayer ();
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (templateName);
  if (!tpl) return;

  view->CreateNode (templateName, csVector2 (150, 26),
      0, templateColorFG, templateColorBG);

  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
    csString pcName = pctpl->GetName ();
    csString nodeName;
    if (pctpl->GetTag () == 0)
      nodeName = pcName;
    else
      nodeName.Format ("%s (%s)", pcName.GetData (), pctpl->GetTag ());
    view->CreateNode (nodeName, csVector2 (150, 26), 0, pcColorFG, pcColorBG);
    view->LinkNode (templateName, nodeName);
    if (pcName == "pclogic.quest")
    {
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
	    questName = par->Get (0);
	    break;
	  }
	}
	if (!questName.IsEmpty ())
	{
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
	      csString stateNameKey = nodeName + stateFact->GetName ();
	      view->CreateNode (stateNameKey, csVector2 (100, 26),
		  stateFact->GetName (),
		  stateColorFG, stateColorBG);
	      view->LinkNode (nodeName, stateNameKey);
	    }
	  }
	}
      }
    }
  }
}

void EntityMode::OnTemplateSelect ()
{
  wxListBox* list = XRCCTRL (*panel, "templateList", wxListBox);
  csString templateName = (const char*)list->GetStringSelection ().mb_str(wxConvUTF8);
  ShowTemplate (templateName);
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

