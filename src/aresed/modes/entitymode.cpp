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
END_EVENT_TABLE()

//---------------------------------------------------------------------------

EntityMode::EntityMode (wxWindow* parent, AresEdit3DView* aresed3d)
  : EditingMode (aresed3d, "Entity")
{
  panel = new Panel (parent, this);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("EntityModePanel"));

  //iMarkerColor* white = aresed3d->GetMarkerManager ()->FindMarkerColor ("white");

  view = aresed3d->GetMarkerManager ()->CreateGraphView ();
  view->CreateNode ("Node 1");
  view->CreateNode ("Node 2");
  view->CreateNode ("Node 3");
  view->CreateNode ("Node 4");
  view->CreateNode ("Node 5");
  view->CreateNode ("Node 6");
  view->CreateNode ("Node 7");
  view->LinkNode ("Link 1", "Node 1", "Node 2");
  view->LinkNode ("Link 2", "Node 1", "Node 3");
  view->LinkNode ("Link 3", "Node 1", "Node 4");
  view->LinkNode ("Link 4", "Node 1", "Node 6");
  view->LinkNode ("SLink 1", "Node 5", "Node 6");
  view->LinkNode ("SLink 2", "Node 7", "Node 6");
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

