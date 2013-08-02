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

#ifndef __aresed_templategrid_h
#define __aresed_templategrid_h

#include "csutil/csstring.h"
#include "edcommon/model.h"
#include "physicallayer/datatype.h"

#include "cseditor/wx/propgrid/propgrid.h"

class EntityMode;
struct iUIManager;
struct iCelEntityTemplateIterator;
struct iParameterManager;

//==================================================================================

// Refresh types are ordered by priority.
// More important refresh types come later.
enum RefreshType
{
  REFRESH_NOCHANGE = 0,		// Nothing has changed, no refresh needed.
  REFRESH_NO,			// There was a change but no refresh needed.
  REFRESH_PC,			// Only PC has to be refreshed.
  REFRESH_TEMPLATE,		// Only template stuff has to be refreshed.
  REFRESH_FULL			// Full refresh is required.
};

class PcEditorSupport : public csRefCount
{
protected:
  iCelPlLayer* pl;
  iUIManager* ui;
  iParameterManager* pm;
  csString name;
  EntityMode* emode;
  wxPropertyGrid* detailGrid;

  wxArrayString typesArray;
  wxArrayInt typesArrayIdx;

  int RegisterContextMenu (wxObjectEventFunction handler);
  void AppendButtonPar (
    wxPGProperty* parent, const char* partype, const char* type, const char* name);
  void AppendPar (
    wxPGProperty* parent, const char* partype,
    const char* name, celDataType type, const char* value);
  wxPGProperty* AppendStringPar (wxPGProperty* parent,
    const char* label, const char* name, const char* value);
  wxPGProperty* AppendBoolPar (wxPGProperty* parent,
    const char* label, const char* name, bool value);
  wxPGProperty* AppendIntPar (wxPGProperty* parent,
    const char* label, const char* name, int value);
  wxPGProperty* AppendEnumPar (wxPGProperty* parent,
    const char* label, const char* name, const wxArrayString& labels, const wxArrayInt& values,
    int value = 0);

  csString GetPropertyValueAsString (const csString& property, const char* sub);
  int GetPropertyValueAsInt (const csString& property, const char* sub);

public:
  PcEditorSupport (const char* name, EntityMode* emode);
  virtual ~PcEditorSupport () { }

  const csString& GetName () { return name; }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl) = 0;
  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty) = 0;
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
};

class PcEditorSupportTemplate : public PcEditorSupport
{
private:
  int idNewChar, idDelChar, idCreatePC, idDelPC;
  wxArrayString pctypesArray;
  csHash<csRef<PcEditorSupport>, csString> editors;

  void RegisterEditor (PcEditorSupport* editor)
  {
    editors.Put (editor->GetName (), editor);
    editor->DecRef ();
  }
  PcEditorSupport* GetEditor (const char* name)
  {
    return editors.Get (name, 0);
  }

  bool ValidateTemplateParentsFromGrid (const wxPropertyGridEvent& event);
  void AppendCharacteristics (wxPGProperty* parentProp, iCelEntityTemplate* tpl);
  void AppendTemplatesPar (wxPGProperty* parentProp, iCelEntityTemplateIterator* it, const char* partype);
  void AppendClassesPar (wxPGProperty* parentProp, csSet<csStringID>::GlobalIterator* it, const char* partype);

  RefreshType UpdateCharacteristicFromGrid (wxPGProperty* property, const csString& propName);
  RefreshType UpdateTemplateClassesFromGrid ();
  RefreshType UpdateTemplateParentsFromGrid ();

  void FillPC (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl);

public:
  PcEditorSupportTemplate (EntityMode* emode);
  virtual ~PcEditorSupportTemplate () { }

  virtual void Fill (wxPGProperty* templateProp, iCelPropertyClassTemplate* pctpl);

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty);

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event);

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu);

  iCelPropertyClassTemplate* GetPCForProperty (wxPGProperty* property, csString& pcPropName,
      csString& selectedPropName);

  // General property deletion function.
  void OnDeleteProperty ();
  void OnDeleteActionParameter ();

  void PcProp_OnNewProperty ();
  void PcProp_OnDelProperty ();
  void PcMsg_OnNewSlot ();
  void PcMsg_OnNewType ();
  void PcSpawn_OnNewTemplate ();
  void PcInv_OnNewTemplate ();
  void PcWire_OnNewOutput ();
  void PcWire_OnNewParameter ();
  void PcQuest_OnNewParameter ();
  void PcQuest_OnSuggestParameters ();
  void OnNewCharacteristic ();
  void OnDeleteCharacteristic ();

};

#endif // __aresed_templategrid_h

