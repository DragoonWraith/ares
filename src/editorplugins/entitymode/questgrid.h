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

#ifndef __aresed_questgrid_h
#define __aresed_questgrid_h

#include "csutil/csstring.h"
#include "edcommon/model.h"
#include "physicallayer/datatype.h"
#include "gridsupport.h"

class EntityMode;
struct iUIManager;
struct iCelEntityTemplateIterator;
struct iParameterManager;
struct iQuestFactory;
struct iQuestStateFactory;

//==================================================================================

#if 0
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
#endif

class QuestEditorSupport : public GridSupport
{
public:
  QuestEditorSupport (const char* name, EntityMode* emode) : GridSupport (name, emode) { }
  virtual ~QuestEditorSupport () { }

  virtual void Fill (wxPGProperty* pcProp, iQuestFactory* questFact) = 0;
#if 0
  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty) = 0;
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
#endif
};

class QuestEditorSupportMain : public QuestEditorSupport
{
private:
#if 0
  int idNewChar, idDelChar, idCreatePC, idDelPC;
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
#endif
  void FillResponses (wxPGProperty* stateProp, iQuestStateFactory* state);
  void FillOnInit (wxPGProperty* stateProp, iQuestStateFactory* state);
  void FillOnExit (wxPGProperty* stateProp, iQuestStateFactory* state);

public:
  QuestEditorSupportMain (EntityMode* emode);
  virtual ~QuestEditorSupportMain () { }

  virtual void Fill (wxPGProperty* templateProp, iQuestFactory* questFact);
};

#endif // __aresed_questgrid_h

