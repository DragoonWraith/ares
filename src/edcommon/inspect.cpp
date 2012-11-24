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


static csStringID Par2 (iCelPlLayer* pl, const char* par)
{
  if (!par || !*par) return csInvalidStringID;
  if (*par != '$' && *par != '@') return csInvalidStringID;
  if (strcmp (par+1, "this") == 0) return csInvalidStringID;
  return pl->FetchStringID (par+1);
}

void InspectTools::CollectParParameters (iCelPlLayer* pl, iCelParameterIterator* it,
    csHash<celDataType,csStringID>& paramTypes)
{
  while (it->HasNext ())
  {
    csStringID id;
    iParameter* par = it->Next (id);
    csString expr = par->GetOriginalExpression ();
    csStringID parID = Par2 (pl, expr);
    if (parID != csInvalidStringID)
  // @@@ Check for conflicting types
      paramTypes.PutUnique (parID, par->GetPossibleType ());
  }
}

void InspectTools::CollectPCParameters (iCelPlLayer* pl, iCelPropertyClassTemplate* pctpl,
    csHash<celDataType,csStringID>& paramTypes)
{
  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> it = pctpl->GetProperty (i, id, data);
    if (it)
      CollectParParameters (pl, it, paramTypes);
  }
}

void InspectTools::CollectTemplateParameters (iCelPlLayer* pl, iCelEntityTemplate* tpl,
    csHash<celDataType,csStringID>& paramTypes)
{
  // @@@ Support defaults.

  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
    CollectPCParameters (pl, tpl->GetPropertyClassTemplate (i), paramTypes);

  for (size_t i = 0 ; i < tpl->GetMessageCount () ; i++)
  {
    csStringID id;
    csRef<iCelParameterIterator> it = tpl->GetMessage (i, id);
    CollectParParameters (pl, it, paramTypes);
  }

  csRef<iCelEntityTemplateIterator> it = tpl->GetParents ();
  while (it->HasNext ())
  {
    iCelEntityTemplate* parent = it->Next ();
    CollectTemplateParameters (pl, parent, paramTypes);
  }
}

csHash<celDataType,csStringID> InspectTools::GetTemplateParameters (iCelPlLayer* pl,
    iCelEntityTemplate* tpl)
{
  csHash<celDataType,csStringID> paramTypes;
  CollectTemplateParameters (pl, tpl, paramTypes);
  return paramTypes;
}

csHash<celDataType,csStringID> InspectTools::GetObjectParameters (iDynamicObject* dynobj)
{
  csHash<celDataType,csStringID> paramTypes;
  iCelParameterBlock* params = dynobj->GetEntityParameters ();
  if (params)
  {
    for (size_t i = 0 ; i < params->GetParameterCount () ; i++)
    {
      celDataType type;
      csStringID parID = params->GetParameterDef (i, type);
      paramTypes.Put (parID, type);
    }
  }
  return paramTypes;
}

static void Par (iCelPlLayer* pl, csHash<celDataType,csStringID>& params, const char* par,
    celDataType type)
{
  if (!par || !*par) return;
  if (*par != '$') return;
  if (strcmp (par+1, "this") == 0) return;
  // @@@ Check for conflicting types
  params.PutUnique (pl->FetchStringID (par+1), type);
}

void InspectTools::CollectSeqopParameters (iCelPlLayer* pl, iSeqOpFactory* seqopFact,
    csHash<celDataType,csStringID>& params)
{
  csString name;
  if (seqopFact) name = seqopFact->GetSeqOpType ()->GetName ();
  else name = "delay";
  if (name.StartsWith ("cel.seqops."))
    name = name.Slice (11);
  if (name == "delay")
  {
  }
  else if (name == "debugprint")
  {
    csRef<iDebugPrintSeqOpFactory> tf = scfQueryInterface<iDebugPrintSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetMessage (), CEL_DATA_STRING);
  }
  else if (name == "ambientmesh")
  {
    csRef<iAmbientMeshSeqOpFactory> tf = scfQueryInterface<iAmbientMeshSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetRelColorRed (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetRelColorGreen (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetRelColorBlue (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetAbsColorRed (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetAbsColorGreen (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetAbsColorBlue (), CEL_DATA_FLOAT);
  }
  else if (name == "light")
  {
    csRef<iLightSeqOpFactory> tf = scfQueryInterface<iLightSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetRelColorRed (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetRelColorGreen (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetRelColorBlue (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetAbsColorRed (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetAbsColorGreen (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetAbsColorBlue (), CEL_DATA_FLOAT);
  }
  else if (name == "movepath")
  {
    csRef<iMovePathSeqOpFactory> tf = scfQueryInterface<iMovePathSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    for (size_t i = 0 ; i < tf->GetPathCount () ; i++)
    {
      Par (pl, params, tf->GetPathSector (i), CEL_DATA_STRING);
      Par (pl, params, tf->GetPathNode (i), CEL_DATA_STRING);
      Par (pl, params, tf->GetPathTime (i), CEL_DATA_STRING);
    }
  }
  else if (name == "property")
  {
    csRef<iPropertySeqOpFactory> tf = scfQueryInterface<iPropertySeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetPC (), CEL_DATA_STRING);
    Par (pl, params, tf->GetPCTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetProperty (), CEL_DATA_STRING);
    Par (pl, params, tf->GetFloat (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetLong (), CEL_DATA_LONG);
    Par (pl, params, tf->GetVectorX (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetVectorY (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetVectorZ (), CEL_DATA_FLOAT);
  }
  else if (name == "transform")
  {
    csRef<iTransformSeqOpFactory> tf = scfQueryInterface<iTransformSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetVectorX (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetVectorY (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetVectorZ (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetRotationAngle (), CEL_DATA_FLOAT);
  }
  else
  {
    printf ("Sanity checker: unsupported sequence operation type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void InspectTools::CollectTriggerParameters (iCelPlLayer* pl, iTriggerFactory* trigger,
    csHash<celDataType,csStringID>& params)
{
  csString name = trigger->GetTriggerType ()->GetName ();
  if (name.StartsWith ("cel.triggers."))
    name = name.GetData ()+13;
  if (name == "entersector" || name == "meshentersector")
  {
    csRef<iEnterSectorTriggerFactory> tf = scfQueryInterface<iEnterSectorTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetSector (), CEL_DATA_STRING);
  }
  else if (name == "inventory")
  {
    csRef<iInventoryTriggerFactory> tf = scfQueryInterface<iInventoryTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetChildEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetChildTemplate (), CEL_DATA_STRING);
  }
  else if (name == "meshselect")
  {
    csRef<iMeshSelectTriggerFactory> tf = scfQueryInterface<iMeshSelectTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
  }
  else if (name == "message")
  {
    csRef<iMessageTriggerFactory> tf = scfQueryInterface<iMessageTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
  }
  else if (name == "propertychange")
  {
    csRef<iPropertyChangeTriggerFactory> tf = scfQueryInterface<iPropertyChangeTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetProperty (), CEL_DATA_STRING);
    Par (pl, params, tf->GetValue (), CEL_DATA_UNKNOWN);
  }
  else if (name == "sequencefinish")
  {
    csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetSequence (), CEL_DATA_STRING);
  }
  else if (name == "timeout")
  {
    csRef<iTimeoutTriggerFactory> tf = scfQueryInterface<iTimeoutTriggerFactory> (trigger);
    Par (pl, params, tf->GetTimeout (), CEL_DATA_LONG);
  }
  else if (name == "trigger")
  {
    csRef<iTriggerTriggerFactory> tf = scfQueryInterface<iTriggerTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
  }
  else if (name == "watch")
  {
    csRef<iWatchTriggerFactory> tf = scfQueryInterface<iWatchTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTargetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTargetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetChecktime (), CEL_DATA_LONG);
    Par (pl, params, tf->GetRadius (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetOffsetX (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetOffsetY (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetOffsetZ (), CEL_DATA_FLOAT);
  }
  else
  {
    printf ("Sanity checker: unsupported trigger type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void InspectTools::CollectRewardParameters (iCelPlLayer* pl, iRewardFactory* reward,
    csHash<celDataType,csStringID>& params)
{
  csString name = reward->GetRewardType ()->GetName ();
  if (name.StartsWith ("cel.rewards."))
    name = name.GetData ()+12;
  if (name == "newstate")
  {
    csRef<iNewStateQuestRewardFactory> tf = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    Par (pl, params, tf->GetStateParameter (), CEL_DATA_STRING);
    Par (pl, params, tf->GetEntityParameter (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTagParameter (), CEL_DATA_STRING);
    Par (pl, params, tf->GetClassParameter (), CEL_DATA_STRING);
  }
  else if (name == "action")
  {
    csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (reward);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetClass (), CEL_DATA_STRING);
    Par (pl, params, tf->GetID (), CEL_DATA_STRING);
    Par (pl, params, tf->GetPropertyClass (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
  }
  else if (name == "changeproperty")
  {
    csRef<iChangePropertyRewardFactory> tf = scfQueryInterface<iChangePropertyRewardFactory> (reward);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetClass (), CEL_DATA_STRING);
    Par (pl, params, tf->GetPC (), CEL_DATA_STRING);
    Par (pl, params, tf->GetPCTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetProperty (), CEL_DATA_STRING);
    Par (pl, params, tf->GetString (), CEL_DATA_STRING);
    Par (pl, params, tf->GetLong (), CEL_DATA_LONG);
    Par (pl, params, tf->GetFloat (), CEL_DATA_FLOAT);
    Par (pl, params, tf->GetBool (), CEL_DATA_BOOL);
    Par (pl, params, tf->GetDiff (), CEL_DATA_UNKNOWN);
  }
  else if (name == "createentity")
  {
    csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (reward);
    Par (pl, params, tf->GetEntityTemplate (), CEL_DATA_STRING);
    Par (pl, params, tf->GetName (), CEL_DATA_STRING);
  }
  else if (name == "destroyentity")
  {
    csRef<iDestroyEntityRewardFactory> tf = scfQueryInterface<iDestroyEntityRewardFactory> (reward);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetClass (), CEL_DATA_STRING);
  }
  else if (name == "debugprint")
  {
    csRef<iDebugPrintRewardFactory> tf = scfQueryInterface<iDebugPrintRewardFactory> (reward);
    Par (pl, params, tf->GetMessage (), CEL_DATA_STRING);
  }
  else if (name == "inventory")
  {
    csRef<iInventoryRewardFactory> tf = scfQueryInterface<iInventoryRewardFactory> (reward);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetChildEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetChildTag (), CEL_DATA_STRING);
  }
  else if (name == "message")
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (reward);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetClass (), CEL_DATA_STRING);
    Par (pl, params, tf->GetID (), CEL_DATA_STRING);
    for (size_t i = 0 ; i < tf->GetParameterCount () ; i++)
      Par (pl, params, tf->GetParameterValue (i), tf->GetParameterType (i));
  }
  else if (name == "cssequence")
  {
    csRef<iCsSequenceRewardFactory> tf = scfQueryInterface<iCsSequenceRewardFactory> (reward);
    Par (pl, params, tf->GetSequence (), CEL_DATA_STRING);
    Par (pl, params, tf->GetDelay (), CEL_DATA_LONG);
  }
  else if (name == "sequence")
  {
    csRef<iSequenceRewardFactory> tf = scfQueryInterface<iSequenceRewardFactory> (reward);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetClass (), CEL_DATA_STRING);
    Par (pl, params, tf->GetSequence (), CEL_DATA_STRING);
    Par (pl, params, tf->GetDelay (), CEL_DATA_LONG);
  }
  else if (name == "sequencefinish")
  {
    csRef<iSequenceFinishRewardFactory> tf = scfQueryInterface<iSequenceFinishRewardFactory> (reward);
    Par (pl, params, tf->GetEntity (), CEL_DATA_STRING);
    Par (pl, params, tf->GetTag (), CEL_DATA_STRING);
    Par (pl, params, tf->GetClass (), CEL_DATA_STRING);
    Par (pl, params, tf->GetSequence (), CEL_DATA_STRING);
  }
  else
  {
    printf ("Sanity checker: unsupported reward type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void InspectTools::CollectRewardParameters (iCelPlLayer* pl, iRewardFactoryArray* rewards,
    csHash<celDataType,csStringID>& params)
{
  for (size_t i = 0 ; i < rewards->GetSize () ; i++)
  {
    iRewardFactory* reward = rewards->Get (i);
    CollectRewardParameters (pl, reward, params);
  }
}

csHash<celDataType,csStringID> InspectTools::GetQuestParameters (iCelPlLayer* pl, iQuestFactory* quest)
{
  csHash<celDataType,csStringID> params;
  csRef<iQuestStateFactoryIterator> stateIt = quest->GetStates ();
  while (stateIt->HasNext ())
  {
    iQuestStateFactory* state = stateIt->Next ();
    csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
    for (size_t i = 0 ; i < responses->GetSize () ; i++)
    {
      iQuestTriggerResponseFactory* response = responses->Get (i);
      CollectRewardParameters (pl, response->GetRewardFactories (), params);
      iTriggerFactory* trigger = response->GetTriggerFactory ();
      CollectTriggerParameters (pl, trigger, params);
    }
    CollectRewardParameters (pl, state->GetInitRewardFactories (), params);
    CollectRewardParameters (pl, state->GetExitRewardFactories (), params);
  }

  csRef<iCelSequenceFactoryIterator> seqIt = quest->GetSequences ();
  while (seqIt->HasNext ())
  {
    iCelSequenceFactory* seq = seqIt->Next ();
    for (size_t i = 0 ; i < seq->GetSeqOpFactoryCount () ; i++)
    {
      iSeqOpFactory* seqopFact = seq->GetSeqOpFactory (i);
      CollectSeqopParameters (pl, seqopFact, params);
      Par (pl, params, seq->GetSeqOpFactoryDuration (i), CEL_DATA_LONG);
    }
  }
  return params;
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

