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
struct iQuestTriggerResponseFactory;
struct iCelSequenceFactory;
struct iSeqOpFactory;
struct iTriggerFactory;
struct iRewardFactory;
struct iRewardFactoryArray;

//==================================================================================

class RewardSupport : public GridSupport
{
public:
  RewardSupport (const char* name, EntityMode* emode) : GridSupport (name, emode) { }
  virtual ~RewardSupport () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact) = 0;
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact) = 0;
#if 0
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
#endif
};

class RewardSupportDriver : public GridSupport
{
private:
  csHash<csRef<RewardSupport>, csString> editors;

  void RegisterEditor (RewardSupport* editor)
  {
    editors.Put (editor->GetName (), editor);
    editor->DecRef ();
  }
  RewardSupport* GetEditor (const char* name)
  {
    return editors.Get (name, 0);
  }

  wxArrayString rewardtypesArray;

public:
  RewardSupportDriver (const char* name, EntityMode* emode);
  virtual ~RewardSupportDriver () { }

  void Fill (wxPGProperty* responseProp, size_t idx, iRewardFactory* rewardFact);
  RefreshType Update (const csString& field, wxPGProperty* selectedProperty,
      iRewardFactoryArray* rewards, size_t idx);

  void FillRewards (wxPGProperty* responseProp, iRewardFactoryArray* rewards);
#if 0
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
#endif
};

class TriggerSupport : public GridSupport
{
public:
  TriggerSupport (const char* name, EntityMode* emode) : GridSupport (name, emode) { }
  virtual ~TriggerSupport () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact) = 0;
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact) = 0;
#if 0
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
#endif
};

class TriggerSupportDriver : public GridSupport
{
private:
  csHash<csRef<TriggerSupport>, csString> editors;

  void RegisterEditor (TriggerSupport* editor)
  {
    editors.Put (editor->GetName (), editor);
    editor->DecRef ();
  }
  TriggerSupport* GetEditor (const char* name)
  {
    return editors.Get (name, 0);
  }

  wxArrayString trigtypesArray;

public:
  TriggerSupportDriver (const char* name, EntityMode* emode);
  virtual ~TriggerSupportDriver () { }

  void Fill (wxPGProperty* responseProp, size_t idx, iTriggerFactory* triggerFact);
  RefreshType Update (const csString& field, wxPGProperty* selectedProperty,
      iQuestTriggerResponseFactory* response);
#if 0
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
#endif
};

class SequenceSupport : public GridSupport
{
public:
  SequenceSupport (const char* name, EntityMode* emode) : GridSupport (name, emode) { }
  virtual ~SequenceSupport () { }

  virtual void Fill (wxPGProperty* seqProp, iSeqOpFactory* seqopFact) = 0;
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqOpFactory) = 0;
#if 0
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
#endif
};

class SequenceSupportDriver : public GridSupport
{
private:
  csHash<csRef<SequenceSupport>, csString> editors;

  void RegisterEditor (SequenceSupport* editor)
  {
    editors.Put (editor->GetName (), editor);
    editor->DecRef ();
  }
  SequenceSupport* GetEditor (const char* name)
  {
    return editors.Get (name, 0);
  }

  wxArrayString seqoptypesArray;

  void FillSeqOps (wxPGProperty* seqProp, size_t idx, iCelSequenceFactory* state);

public:
  SequenceSupportDriver (const char* name, EntityMode* emode);
  virtual ~SequenceSupportDriver () { }

  void Fill (wxPGProperty* questProp, iQuestFactory* questFact);
  RefreshType Update (const csString& field, wxPGProperty* selectedProperty,
      iCelSequenceFactory* seqFact, size_t index);
#if 0
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
#endif
};

class QuestEditorSupportMain : public GridSupport
{
private:
#if 0
  int idNewChar, idDelChar, idCreatePC, idDelPC;
#endif
  csRef<TriggerSupportDriver> triggerEditor;
  csRef<RewardSupportDriver> rewardEditor;
  csRef<SequenceSupportDriver> sequenceEditor;

  void FillResponses (wxPGProperty* stateProp, size_t idx, iQuestStateFactory* state);
  void FillOnInit (wxPGProperty* stateProp, size_t idx, iQuestStateFactory* state);
  void FillOnExit (wxPGProperty* stateProp, size_t idx, iQuestStateFactory* state);

  RefreshType Update (iQuestFactory* questFact, iQuestStateFactory* stateFact,
      wxPGProperty* selectedProperty, const csString& selectedPropName,
      int responseIndex);

  RefreshType Update (iQuestFactory* questFact, iCelSequenceFactory* seqFact,
      wxPGProperty* selectedProperty, const csString& selectedPropName);

  iQuestStateFactory* GetStateForProperty (wxPGProperty* property,
    csString& selectedPropName, int& responseIndex);
  iCelSequenceFactory* GetSequenceForProperty (wxPGProperty* property,
    csString& selectedPropName);

public:
  QuestEditorSupportMain (EntityMode* emode);
  virtual ~QuestEditorSupportMain () { }

  void Fill (wxPGProperty* templateProp, iQuestFactory* questFact);
  RefreshType Update (wxPGProperty* selectedProperty, iQuestStateFactory*& stateFact,
      iCelSequenceFactory*& sequence);
};

#endif // __aresed_questgrid_h

