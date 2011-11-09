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

  testMarker = aresed3d->GetMarkerManager ()->CreateMarker ();
  iMarkerColor* white = aresed3d->GetMarkerManager ()->FindMarkerColor ("white");
  testMarker->RoundedBox2D (MARKER_2D, csVector3 (50, 50, 0),
      csVector3 (150, 75, 0), 10, white);
  testMarker->Text (MARKER_2D, csVector3 (100, 62, 0), "Hello!", white, true);
  testMarker->SetSelectionLevel (1);
  testMarker->SetVisible (false);
}

EntityMode::~EntityMode ()
{
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
  testMarker->SetVisible (true);
}

void EntityMode::Stop ()
{
  testMarker->SetVisible (false);
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

