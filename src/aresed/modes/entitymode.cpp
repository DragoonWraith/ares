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
  view->SetColors (mgr->FindMarkerColor ("white"), mgr->FindMarkerColor ("blue"),
      mgr->FindMarkerColor ("yellow"));
  view->Clear ();

  view->SetVisible (false);
}

EntityMode::~EntityMode ()
{
  aresed3d->GetMarkerManager ()->DestroyGraphView (view);
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

  view->CreateNode (templateName);
  view->ForcePosition (templateName,
      csVector2 (aresed3d->GetG2D ()->GetWidth ()/2,
	aresed3d->GetG2D ()->GetHeight ()/2));

  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
    view->CreateNode (pctpl->GetName ());
    view->LinkNode (templateName, pctpl->GetName ());
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

