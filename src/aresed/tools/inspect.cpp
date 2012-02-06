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

#include "inspect.h"
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"

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

void InspectTools::DeleteActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, size_t idx, const char* parName)
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
    if (parid != parID)
      newParams.Put (parid, par);
  }
  pctpl->ReplaceActionParameters (idx, newParams);
}

void InspectTools::DeleteActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName)
{
  csStringID actionID = pl->FetchStringID (actionName);
  size_t idx = pctpl->FindProperty (actionID);
  if (idx == csArrayItemNotFound) return;
  DeleteActionParameter (pl, pctpl, idx, parName);
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

