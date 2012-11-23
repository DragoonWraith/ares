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

#include "edcommon/inspect.h"
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "propclass/dynworld.h"

celData InspectTools::GetPropertyValue (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid)
{
  csStringID propID = pl->FetchStringID (propName);
  size_t idx = pctpl->FindProperty (propID);
  if (valid) *valid = idx != csArrayItemNotFound;
  celData data;
  if (idx == csArrayItemNotFound) return data;

  pctpl->GetProperty (idx, propID, data);
  return data;
}

csString InspectTools::GetPropertyValueString (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid)
{
  celData data = GetPropertyValue (pl, pctpl, propName, valid);
  csString value;
  celParameterTools::ToString (data, value);
  return value;
}

bool InspectTools::GetPropertyValueBool (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid)
{
  celData data = GetPropertyValue (pl, pctpl, propName, valid);
  bool value;
  celParameterTools::ToBool (data, value);
  return value;
}

long InspectTools::GetPropertyValueLong (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid)
{
  celData data = GetPropertyValue (pl, pctpl, propName, valid);
  long value;
  celParameterTools::ToLong (data, value);
  return value;
}

float InspectTools::GetPropertyValueFloat (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid)
{
  celData data = GetPropertyValue (pl, pctpl, propName, valid);
  float value;
  celParameterTools::ToFloat (data, value);
  return value;
}

iParameter* InspectTools::GetActionParameterValue (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, size_t idx, const char* parName)
{
  csStringID actionID;
  celData data;
  csRef<iCelParameterIterator> parit = pctpl->GetProperty (idx, actionID, data);
  csStringID parID = pl->FetchStringID (parName);
  while (parit->HasNext ())
  {
    csStringID parid;
    iParameter* par = parit->Next (parid);
    // @@@ We don't support expression parameters here. 'params'
    // for creating entities is missing.
    if (parid == parID) return par;
  }
  return 0;
}

iParameter* InspectTools::GetActionParameterValue (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName)
{
  csStringID actionID = pl->FetchStringID (actionName);
  size_t idx = pctpl->FindProperty (actionID);
  if (idx == csArrayItemNotFound) return 0;
  return GetActionParameterValue (pl, pctpl, idx, parName);
}

csString InspectTools::GetActionParameterValueString (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid)
{
  iParameter* par = GetActionParameterValue (pl, pctpl, actionName, parName);
  if (valid) *valid = par != 0;
  if (par) return par->Get (0);
  return "";
}

bool InspectTools::GetActionParameterValueBool (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid)
{
  iParameter* par = GetActionParameterValue (pl, pctpl, actionName, parName);
  if (valid) *valid = par != 0;
  if (par) return par->GetBool (0);
  return false;
}

long InspectTools::GetActionParameterValueLong (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid)
{
  iParameter* par = GetActionParameterValue (pl, pctpl, actionName, parName);
  if (valid) *valid = par != 0;
  if (par) return par->GetLong (0);
  return 0;
}

float InspectTools::GetActionParameterValueFloat (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid)
{
  iParameter* par = GetActionParameterValue (pl, pctpl, actionName, parName);
  if (valid) *valid = par != 0;
  if (par) return par->GetFloat (0);
  return 0.0f;
}

csVector3 InspectTools::GetActionParameterValueVector3 (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid)
{
  iParameter* par = GetActionParameterValue (pl, pctpl, actionName, parName);
  if (valid) *valid = par != 0;
  if (par)
  {
    const celData* data = par->GetData (0);
    csVector3 v;
    celParameterTools::ToVector3 (*data, v);
    return v;
  }
  return csVector3 (0.0f);
}

bool InspectTools::DeleteActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, size_t idx, const char* parName)
{
  csStringID parID = pl->FetchStringID (parName);
  csStringID id;
  celData data;
  csRef<iCelParameterIterator> it = pctpl->GetProperty (idx, id, data);
  csHash<csRef<iParameter>,csStringID> newParams;
  bool removed = false;
  while (it->HasNext ())
  {
    csStringID parid;
    iParameter* par = it->Next (parid);
    if (parid != parID)
      newParams.Put (parid, par);
    else
      removed = true;
  }
  pctpl->ReplaceActionParameters (idx, newParams);
  return removed;
}

bool InspectTools::DeleteActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName)
{
  csStringID actionID = pl->FetchStringID (actionName);
  size_t idx = pctpl->FindProperty (actionID);
  if (idx == csArrayItemNotFound) return false;
  return DeleteActionParameter (pl, pctpl, idx, parName);
}

void InspectTools::AddActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, size_t idx,
      const char* parName, iParameter* parameter)
{
  csStringID parID = pl->FetchStringID (parName);
  csStringID id;
  celData data;
  csRef<iCelParameterIterator> it = pctpl->GetProperty (idx, id, data);
  csHash<csRef<iParameter>,csStringID> newParams;
  while (it->HasNext ())
  {
    csStringID parid;
    iParameter* par = it->Next (parid);
    newParams.Put (parid, par);
  }
  newParams.Put (parID, parameter);
  pctpl->ReplaceActionParameters (idx, newParams);
}

void InspectTools::AddActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName,
      const char* parName, iParameter* parameter)
{
  csStringID actionID = pl->FetchStringID (actionName);
  size_t idx = pctpl->FindProperty (actionID);
  if (idx == csArrayItemNotFound)
  {
    csHash<csRef<iParameter>,csStringID> newParams;
    pctpl->PerformAction (actionID, newParams);
    idx = pctpl->FindProperty (actionID);
  }
  AddActionParameter (pl, pctpl, idx, parName, parameter);
}

void InspectTools::AddAction (iCelPlLayer* pl, iParameterManager* pm,
      iCelPropertyClassTemplate* pctpl, const char* actionName, ...)
{
  ParHash params;
  va_list args;
  va_start (args, actionName);
  celDataType type = (celDataType)va_arg (args, int);
  while (type != CEL_DATA_NONE)
  {
    const char* name = va_arg (args, char*);
    const char* value = va_arg (args, char*);
    // @@@ Error checking?
    csRef<iParameter> par = pm->GetParameter (value, type);
    params.Put (pl->FetchStringID (name), par);
    type = (celDataType)va_arg (args, int);
  }

  va_end (args);
  pctpl->PerformAction (pl->FetchStringID (actionName), params);
}

size_t InspectTools::FindActionWithParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName,
      const char* parName, const char* parValue)
{
  csStringID actionID = pl->FetchStringID (actionName);
  csStringID parID = pl->FetchStringID (parName);
  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> it = pctpl->GetProperty (i, id, data);
    if (id == actionID)
    {
      while (it->HasNext ())
      {
        csStringID parid;
        iParameter* par = it->Next (parid);
	if (parid == parID)
	{
	  csString value = par->GetOriginalExpression ();
	  if (value == parValue) return i;
	}
      }
    }
  }
  return csArrayItemNotFound;
}

csRef<iCelParameterIterator> InspectTools::FindAction (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName)
{
  csStringID actionID = pl->FetchStringID (actionName);
  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> it = pctpl->GetProperty (i, id, data);
    if (id == actionID)
      return it;
  }
  return 0;
}

csString InspectTools::FindActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName,
      const char* parName)
{
  csRef<iCelParameterIterator> params = FindAction (pl, pctpl, actionName);
  if (!params) return "";
  csStringID nameID = pl->FetchStringID (parName);
  while (params->HasNext ())
  {
    csStringID parid;
    iParameter* par = params->Next (parid);
    if (parid == nameID)
      return par->GetOriginalExpression ();
  }
  return "";
}


csArray<celParSpec> InspectTools::GetParameterSuggestions (iCelPlLayer* pl, iObjectComment* comment)
{
  csArray<celParSpec> suggestions;
  using namespace CS::Utility;
  StringArray<> lines (comment->GetComment ()->GetData (), "\n",
      StringArray<>::delimIgnore);
  for (size_t i = 0 ; i < lines.GetSize () ; i++)
  {
    csString line = lines[i];
    if (line.StartsWith ("@param "))
    {
      StringArray<> tokens (line, " ", StringArray<>::delimIgnore);
      StringArray<> parspecs (tokens[1], ",");
      csStringID id = pl->FetchStringID (parspecs[0]);
      celDataType type = CEL_DATA_STRING;
      csString value;
      if (parspecs.GetSize () > 1)
        type = celParameterTools::GetType (parspecs[1]);
      if (parspecs.GetSize () > 2)
        value = parspecs[2];
      suggestions.Push (celParSpec (type, id, value));
    }
  }
  return suggestions;
}

//---------------------------------------------------------------------------------------

ResourceCounter::ResourceCounter (iObjectRegistry* object_reg, iPcDynamicWorld* dynworld) :
  object_reg (object_reg), dynworld (dynworld), filter (0)
{
  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  questMgr = csQueryRegistry<iQuestManager> (object_reg);
  engine = csQueryRegistry<iEngine> (object_reg);
}

bool ResourceCounter::inc (csHash<int,csString>& counter, iObject* object, const char* name)
{
  if (name)
  {
    counter.PutUnique (name, counter.Get (name, 0)+1);
    if (filter && filter == object) return true;
  }
  return false;
}

void ResourceCounter::CountResourcesInFactories ()
{
  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
  {
    iDynamicFactory* fact = dynworld->GetFactory (i);
    iObject* tplObject = 0;
    if (filter)
    {
      iCelEntityTemplate* tpl;
      if (fact->GetDefaultEntityTemplate ())
	tpl = pl->FindEntityTemplate (fact->GetDefaultEntityTemplate ());
      else
	tpl = pl->FindEntityTemplate (fact->QueryObject ()->GetName ());
      if (tpl)
	tplObject = tpl->QueryObject ();
    }

    bool report;
    if (fact->GetDefaultEntityTemplate ())
      report = inc (templateCounter, tplObject, fact->GetDefaultEntityTemplate ());
    else
      report = inc (templateCounter, tplObject, fact->QueryObject ()->GetName ());
    if (report)
      csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	    "    Used in factory '%s'", fact->QueryObject ()->GetName ());

    if (fact->IsLightFactory ())
    {
      iObject* tplLight = 0;
      if (filter)
      {
	iLightFactory* lf = engine->FindLightFactory (fact->QueryObject ()->GetName ());
	if (lf)
	  tplLight = lf->QueryObject ();
      }
      if (inc (lightCounter, tplLight, fact->QueryObject ()->GetName ()))
        csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	    "    Used in factory '%s'", fact->QueryObject ()->GetName ());
    }
  }
}

void ResourceCounter::CountResourcesInObjects ()
{
  int cntUnnamed = 0;
  csRef<iDynamicCellIterator> it = dynworld->GetCells ();
  while (it->HasNext ())
  {
    iDynamicCell* cell = it->NextCell ();
    for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
    {
      iDynamicObject* dynobj = cell->GetObject (i);
      iCelEntityTemplate* tpl = dynobj->GetEntityTemplate ();
      if (tpl)
	if (inc (templateCounter, tpl->QueryObject (), tpl->QueryObject ()->GetName ()))
	{
	  if (dynobj->GetEntityName ())
            csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	      "    Used in object '%s'", dynobj->GetEntityName ());
	  else
	    cntUnnamed++;
	}
    }
  }
  if (cntUnnamed)
    csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	      "    Used in %d unnamed objects", cntUnnamed);
}

void ResourceCounter::CountTemplatesInPC (
      iCelPropertyClassTemplate* pctpl,
      const char* nameField,
      const char* nameAction,
      const char* tplName)
{
  csStringID nameID = pl->FetchStringID (nameField);
  for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
    csString name = pl->FetchString (id);
    if (name == nameAction)
    {
      while (params->HasNext ())
      {
	csStringID parid;
	iParameter* par = params->Next (parid);
	if (parid == nameID)
	{
	  csString parName = par->GetOriginalExpression ();
	  iObject* tplObject = 0;
	  if (filter)
	  {
	    iCelEntityTemplate* tpl = pl->FindEntityTemplate (parName);
	    if (tpl)
	      tplObject = tpl->QueryObject ();
	  }

	  if (inc (templateCounter, tplObject, parName))
	    csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	      "    Used in template '%s' (property class '%s')", tplName,
	      pctpl->GetName ());
	}
      }
    }
  }
}

void ResourceCounter::CountResourcesInTemplates ()
{
  csRef<iCelEntityTemplateIterator> it = pl->GetEntityTemplates ();
  while (it->HasNext ())
  {
    iCelEntityTemplate* tpl = it->Next ();
    for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
    {
      iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
      csString name = pctpl->GetName ();
      if (name == "pclogic.quest")
      {
	csString questName = InspectTools::GetActionParameterValueString (pl, pctpl, "NewQuest", "name");
	iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
	if (inc (questCounter, questFact ? questFact->QueryObject () : 0, questName))
	  csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	      "    Used in template '%s'", tpl->QueryObject ()->GetName ());
      }
      else if (name == "pclogic.spawn")
      {
	CountTemplatesInPC (pctpl, "template", "AddEntityTemplateType", tpl->QueryObject ()->GetName ());
      }
      else if (name == "pctools.inventory")
      {
	CountTemplatesInPC (pctpl, "name", "AddTemplate", tpl->QueryObject ()->GetName ());
      }
    }
    csRef<iCelEntityTemplateIterator> parentIt = tpl->GetParents ();
    while (parentIt->HasNext ())
    {
      csString parentName = parentIt->Next ()->GetName ();
      iCelEntityTemplate* parent = pl->FindEntityTemplate (parentName);
      if (inc (templateCounter, parent ? parent->QueryObject () : 0, parentName))
	csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	      "    Used (as parent) in template '%s'", tpl->QueryObject ()->GetName ());
    }
  }
}

void ResourceCounter::CountResourcesInRewards (iRewardFactoryArray* rewards,
    iQuestFactory* questFact)
{
  for (size_t i = 0 ; i < rewards->GetSize () ; i++)
  {
    iRewardFactory* reward = rewards->Get (i);
    csString name = reward->GetRewardType ()->GetName ();
    if (name.StartsWith ("cel.rewards."))
      name = name.GetData ()+12;
    if (name == "createentity")
    {
      csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (reward);
      csString tplName = tf->GetEntityTemplate ();
      iCelEntityTemplate* tpl = pl->FindEntityTemplate (tplName);
      if (inc (templateCounter, tpl ? tpl->QueryObject () : 0, tplName))
	csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	      "    Used in quest '%s' (reward '%s')", questFact->QueryObject ()->GetName (),
	      name.GetData ());
    }
  }
}

void ResourceCounter::CountResourcesInTrigger (iTriggerFactory* trigger,
    iQuestFactory* questFact)
{
  csString name = trigger->GetTriggerType ()->GetName ();
  if (name.StartsWith ("cel.triggers."))
    name = name.GetData ()+13;
  if (name == "inventory")
  {
    csRef<iInventoryTriggerFactory> tf = scfQueryInterface<iInventoryTriggerFactory> (trigger);
      csString tplName = tf->GetChildTemplate ();
      iCelEntityTemplate* tpl = pl->FindEntityTemplate (tplName);
    if (inc (templateCounter, tpl ? tpl->QueryObject () : 0, tplName))
	csReport (object_reg, CS_REPORTER_SEVERITY_NOTIFY, "ares.usage",
	      "    Used in quest '%s' (trigger '%s')", questFact->QueryObject ()->GetName (),
	      name.GetData ());
  }
}

void ResourceCounter::CountResourcesInQuests ()
{
  if (questMgr)
  {
    csRef<iQuestFactoryIterator> it = questMgr->GetQuestFactories ();
    while (it->HasNext ())
    {
      iQuestFactory* fact = it->Next ();
      csRef<iQuestStateFactoryIterator> stateIt = fact->GetStates ();
      while (stateIt->HasNext ())
      {
	iQuestStateFactory* state = stateIt->Next ();
	csRef<iQuestTriggerResponseFactoryArray> triggerResponses = state->GetTriggerResponseFactories ();
	for (size_t i = 0 ; i < triggerResponses->GetSize () ; i++)
	{
	  iQuestTriggerResponseFactory* triggerResponse = triggerResponses->Get (i);
	  iTriggerFactory* trigger = triggerResponse->GetTriggerFactory ();
	  CountResourcesInTrigger (trigger, fact);
	  csRef<iRewardFactoryArray> rewards = triggerResponse->GetRewardFactories ();
	  CountResourcesInRewards (rewards, fact);
	}

	csRef<iRewardFactoryArray> initRewards = state->GetInitRewardFactories ();
	CountResourcesInRewards (initRewards, fact);
	csRef<iRewardFactoryArray> exitRewards = state->GetExitRewardFactories ();
	CountResourcesInRewards (exitRewards, fact);
      }
    }
  }
}

