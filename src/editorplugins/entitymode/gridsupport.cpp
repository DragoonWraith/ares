/*
The MIT License

Copyright (c) 2013 by Jorrit Tyberghein

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
#include "editor/i3dview.h"
#include "editor/iapp.h"
#include "editor/iuimanager.h"
#include "gridsupport.h"

#include "entitymode.h"

//---------------------------------------------------------------------------

GridSupport::GridSupport (const char* name, EntityMode* emode) : name (name), emode (emode)
{
  pl = emode->GetPL ();
  pm = emode->GetPM ();
  ui = emode->GetApplication ()->GetUI ();
  detailGrid = emode->GetDetailGrid ();

  typesArray.Add (wxT ("string"));  typesArrayIdx.Add (CEL_DATA_STRING);
  typesArray.Add (wxT ("float"));   typesArrayIdx.Add (CEL_DATA_FLOAT);
  typesArray.Add (wxT ("long"));    typesArrayIdx.Add (CEL_DATA_LONG);
  typesArray.Add (wxT ("bool"));    typesArrayIdx.Add (CEL_DATA_BOOL);
  typesArray.Add (wxT ("vector2")); typesArrayIdx.Add (CEL_DATA_VECTOR2);
  typesArray.Add (wxT ("vector3")); typesArrayIdx.Add (CEL_DATA_VECTOR3);
  typesArray.Add (wxT ("color"));   typesArrayIdx.Add (CEL_DATA_COLOR);
}

csString GridSupport::GetPropertyValueAsString (const csString& property, const char* sub)
{
  wxString wxValue = detailGrid->GetPropertyValueAsString (wxString::FromUTF8 (
	property + "." + sub));
  csString value = (const char*)wxValue.mb_str (wxConvUTF8);
  return value;
}

int GridSupport::GetPropertyValueAsInt (const csString& property, const char* sub)
{
  int value = detailGrid->GetPropertyValueAsInt (wxString::FromUTF8 (
	property + "." + sub));
  return value;
}

wxPGProperty* GridSupport::AppendStringPar (wxPGProperty* parent,
    const char* label, const char* name, const char* value)
{
  return detailGrid->AppendIn (parent,
      new wxStringProperty (wxString::FromUTF8 (label),
	wxString::FromUTF8 (name), wxString::FromUTF8 (value)));
}

wxPGProperty* GridSupport::AppendBoolPar (wxPGProperty* parent,
    const char* label, const char* name, bool value)
{
  return detailGrid->AppendIn (parent,
      new wxBoolProperty (wxString::FromUTF8 (label),
	wxString::FromUTF8 (name), value));
}

wxPGProperty* GridSupport::AppendIntPar (wxPGProperty* parent,
    const char* label, const char* name, int value)
{
  return detailGrid->AppendIn (parent,
      new wxIntProperty (wxString::FromUTF8 (label),
	wxString::FromUTF8 (name), value));
}

wxPGProperty* GridSupport::AppendEnumPar (wxPGProperty* parent,
    const char* label, const char* name, const wxArrayString& labels, const wxArrayInt& values,
    int value)
{
  return detailGrid->AppendIn (parent,
      new wxEnumProperty (wxString::FromUTF8 (label),
	wxString::FromUTF8 (name), labels, values, value));
}

wxPGProperty* GridSupport::AppendEditEnumPar (wxPGProperty* parent,
    const char* label, const char* name, const wxArrayString& labels, const wxArrayInt& values,
    const char* value)
{
  return detailGrid->AppendIn (parent,
      new wxEditEnumProperty (wxString::FromUTF8 (label),
	wxString::FromUTF8 (name), labels, values, wxString::FromUTF8 (value)));
}

void GridSupport::AppendPar (
    wxPGProperty* parent, const char* partype,
    const char* name, celDataType type, const char* value)
{
  csString s;
  s.Format ("%s:%s", partype, name);
  wxPGProperty* parProp = AppendStringPar (parent, partype, s, "<composed>");
  AppendStringPar (parProp, "Name", "Name", name);
  if (type != CEL_DATA_NONE)
    AppendEnumPar (parProp, "Type", "Type", typesArray, typesArrayIdx, type);
  AppendStringPar (parProp, "Value", "Value", value);
  detailGrid->Collapse (parProp);
}

void GridSupport::AppendColorPar (
    wxPGProperty* parent, const char* label,
    const char* red, const char* green, const char* blue)
{
  wxPGProperty* parProp = AppendButtonPar (parent, label, "c:", "<composed>");
  AppendStringPar (parProp, "Red", "Red", red);
  AppendStringPar (parProp, "Green", "Green", green);
  AppendStringPar (parProp, "Blue", "Blue", blue);
  detailGrid->Collapse (parProp);
}

void GridSupport::AppendVectorPar (
    wxPGProperty* parent, const char* label,
    const char* x, const char* y, const char* z)
{
  wxPGProperty* parProp = AppendButtonPar (parent, label, "V:", "<composed>");
  AppendStringPar (parProp, "X", "X", x);
  AppendStringPar (parProp, "Y", "Y", y);
  AppendStringPar (parProp, "Z", "Z", z);
  detailGrid->Collapse (parProp);
}

void GridSupport::AppendVectorPar (
    wxPGProperty* parent, const char* label,
    const char* vec)
{
  AppendButtonPar (parent, label, "v:", vec);
}

void GridSupport::AppendPCPar (wxPGProperty* parent, const char* label, const char* name,
    const char* prefix, const char* entity, const char* tag, const char* clazz)
{
  csString s;
  wxPGProperty* parProp = AppendButtonPar (parent, label, name, "<composed>");
  s.Format ("%s%sEntity", prefix ? prefix : "", prefix ? " " : "");
  AppendButtonPar (parProp, s, "E:", entity);
  s.Format ("%s%sTag", prefix ? prefix : "", prefix ? " " : "");
  AppendStringPar (parProp, "Tag", s, tag);
  s.Format ("%s%sClass", prefix ? prefix : "", prefix ? " " : "");
  AppendStringPar (parProp, "Class", s, clazz);
  detailGrid->Collapse (parProp);
}

wxPGProperty* GridSupport::AppendButtonPar (
    wxPGProperty* parent, const char* partype, const char* type, const char* name)
{
  wxStringProperty* prop = new wxStringProperty (
      wxString::FromUTF8 (partype),
      wxString::FromUTF8 (csString (type) + partype),
      wxString::FromUTF8 (name));
  detailGrid->AppendIn (parent, prop);
  detailGrid->SetPropertyEditor (prop, wxPGEditor_TextCtrlAndButton);
  return prop;
}

int GridSupport::RegisterContextMenu (wxObjectEventFunction handler)
{
  int id = ui->AllocContextMenuID ();
  emode->panel->Connect (id, wxEVT_COMMAND_MENU_SELECTED, handler, 0, emode->panel);
  return id;
}

csString GridSupport::GetPropertyName (wxPGProperty* property)
{
  return (const char*)property->GetName ().mb_str (wxConvUTF8);
}

// ------------------------------------------------------------------------

