/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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

#include "edcommon/sanity.h"
#include "edcommon/inspect.h"
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "propclass/dynworld.h"

//----------------------------------------------------------------------

static bool IsEmpty (const char* par)
{
  if (par == 0) return true;
  if (*par == 0) return true;
  return false;
}

static bool IsConstant (const char* par)
{
  if (IsEmpty (par)) return false;
  if (*par == '$' || *par == '@' || *par == '=') return false;
  return true;
}

static bool IsConstantOrEmpty (const char* par)
{
  if (IsEmpty (par)) return true;
  if (*par == '$' || *par == '@' || *par == '=') return false;
  return true;
}

static bool Equals (const char* p1, const char* p2)
{
  if (IsEmpty (p1)) return IsEmpty (p2);
  if (IsEmpty (p2)) return false;
  return strcmp (p1, p2) == 0;
}

//----------------------------------------------------------------------

SanityResult& SanityResult::Object (iDynamicObject* object)
{
  SanityResult::object = object;
  return *this;
}

SanityResult& SanityResult::Resource (iObject* object)
{
  resource = object;
  return *this;
}

SanityResult& SanityResult::Name (const char* fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  name.FormatV (fmt, args);
  va_end (args);
  return *this;
}

SanityResult& SanityResult::Message (const char* fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  message.FormatV (fmt, args);
  va_end (args);
  return *this;
}

SanityResult& SanityResult::MessageV (const char* fmt, va_list args)
{
  message.FormatV (fmt, args);
  return *this;
}

//----------------------------------------------------------------------

SanityChecker::SanityChecker (iObjectRegistry* object_reg, iPcDynamicWorld* dynworld) :
  dynworld (dynworld)
{
  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  engine = csQueryRegistry<iEngine> (object_reg);
  questManager = csQueryRegistry<iQuestManager> (object_reg);
}

SanityChecker::~SanityChecker ()
{
}

void SanityChecker::ClearResults ()
{
  results.Empty ();
}

void SanityChecker::PushResult (iQuestFactory* quest, const char* msg, ...)
{
  va_list args;
  va_start (args, msg);
  results.Push (SanityResult ().
	Resource (quest->QueryObject ()).
	Name ("Quest %s", quest->GetName ()).
	MessageV (msg, args));
  va_end (args);
}

void SanityChecker::PushResult (iDynamicFactory* fact, const char* msg, ...)
{
  va_list args;
  va_start (args, msg);
  results.Push (SanityResult ().
	Resource (fact->QueryObject ()).
	Name ("Fact %s", fact->GetName ()).
	MessageV (msg, args));
  va_end (args);
}

void SanityChecker::PushResult (iDynamicObject* obj, const char* msg, ...)
{
  va_list args;
  va_start (args, msg);
  if (obj->GetEntityName ())
    results.Push (SanityResult ().
	Object (obj).
	Name ("Obj %s(%s)", obj->GetEntityName (), obj->GetFactory ()->GetName ()).
	MessageV (msg, args));
  else
    results.Push (SanityResult ().
	Object (obj).
	Name ("Obj %d(%s)", obj->GetID (), obj->GetFactory ()->GetName ()).
	MessageV (msg, args));
  va_end (args);
}

void SanityChecker::PushResult (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl,
    const char* msg, ...)
{
  va_list args;
  va_start (args, msg);
  if (pctpl->GetTag () && *pctpl->GetTag ())
    results.Push (SanityResult ().
	Resource (tpl->QueryObject ()).
	Name ("Tpl %s/%s(%s)", tpl->GetName (), pctpl->GetName (), pctpl->GetTag ()).
	MessageV (msg, args));
  else
    results.Push (SanityResult ().
	Resource (tpl->QueryObject ()).
	Name ("Tpl %s/%s", tpl->GetName (), pctpl->GetName ()).
	MessageV (msg, args));
  va_end (args);
}

void SanityChecker::CheckQuestPC (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl)
{
  csRef<iCelParameterIterator> newquestParams = InspectTools::FindAction (pl, pctpl, "NewQuest");
  if (!newquestParams)
  {
    PushResult (tpl, pctpl, "NewQuest action is missing!");
    return;
  }
  csStringID nameID = pl->FetchStringID ("name");

  bool questNameGiven = false;
  iQuestFactory* quest = 0;

  csSet<csStringID> given;
  while (newquestParams->HasNext ())
  {
    csStringID parid;
    iParameter* par = newquestParams->Next (parid);
    if (parid == nameID)
    {
      questNameGiven = true;
      csString value = par->GetOriginalExpression ();
      if (value.IsEmpty ())
      {
        PushResult (tpl, pctpl, "Questname is empty!");
        return;
      }
      const char first = value.GetData ()[0];
      if (first != '=' && first != '$' && first != '@' && first != '?')
      {
	quest = questManager->GetQuestFactory (value);
	if (!quest)
	{
          PushResult (tpl, pctpl, "Cannot find quest '%s'!", value.GetData ());
          return;
	}
      }
    }
    else
    {
      given.Add (parid);
    }
  }
  if (!questNameGiven)
  {
    PushResult (tpl, pctpl, "Quest name parameter is not given!");
    return;
  }
  if (quest)
  {
    csSet<csStringID> wanted = InspectTools::GetQuestParameters (pl, quest);
    csSet<csStringID> missing = Subtract (wanted, given);
    {
      csSet<csStringID>::GlobalIterator it = missing.GetIterator ();
      while (it.HasNext ())
      {
        csStringID id = it.Next ();
        csString parName = pl->FetchString (id);
        PushResult (tpl, pctpl, "Quest parameter '%s' is missing!", parName.GetData ());
      }
    }
    csSet<csStringID> toomuch = Subtract (given, wanted);
    {
      csSet<csStringID>::GlobalIterator it = toomuch.GetIterator ();
      while (it.HasNext ())
      {
        csStringID id = it.Next ();
        csString parName = pl->FetchString (id);
        PushResult (tpl, pctpl, "Quest parameter '%s' is not needed!", parName.GetData ());
      }
    }
  }
}

void SanityChecker::Check (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl)
{
  csString name = pctpl->GetName ();
  if (name == "pclogic.quest")
    CheckQuestPC (tpl, pctpl);
}

void SanityChecker::Check (iCelEntityTemplate* tpl)
{
  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
    Check (tpl, pctpl);
  }
}

void SanityChecker::CheckTemplates ()
{
  csRef<iCelEntityTemplateIterator> it = pl->GetEntityTemplates ();
  while (it->HasNext ())
  {
    iCelEntityTemplate* tpl = it->Next ();
    Check (tpl);
  }
}

void SanityChecker::Check (iDynamicFactory* dynfact)
{
  const char* deftpl = dynfact->GetDefaultEntityTemplate ();
  if (deftpl)
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (deftpl);
    if (!tpl)
      PushResult (dynfact, "Can't find template '%s'!", deftpl);
  }
}

void SanityChecker::CheckParameterTypes (iDynamicObject* dynobj,
    const csHash<celDataType,csStringID>& givenTypes,
    const csHash<celDataType,csStringID>& wantedTypes)
{
  csHash<celDataType,csStringID>::ConstGlobalIterator it = givenTypes.GetIterator ();
  while (it.HasNext ())
  {
    csStringID givenPar;
    celDataType givenType = it.Next (givenPar);
    if (wantedTypes.Contains (givenPar))
    {
      celDataType wantedType = wantedTypes.Get (givenPar, CEL_DATA_NONE);
      if (wantedType != CEL_DATA_NONE)
      {
	if (givenType != wantedType)
	{
	  csString givenTypeS = celParameterTools::GetTypeName (givenType);
	  csString wantedTypeS = celParameterTools::GetTypeName (wantedType);
	  csString parName = pl->FetchString (givenPar);
	  PushResult (dynobj, "Template parameter '%s' has wrong type! Wanted %s but got %s instead.",
	      parName.GetData (), wantedTypeS.GetData (), givenTypeS.GetData ());
	}
      }
    }
    else
    {
      csString parName = pl->FetchString (givenPar);
      PushResult (dynobj, "Template parameter '%s' is not needed!", parName.GetData ());
    }
  }
  it = wantedTypes.GetIterator ();
  while (it.HasNext ())
  {
    csStringID wantedPar;
    it.Next (wantedPar);
    if (!givenTypes.Contains (wantedPar))
    {
      csString parName = pl->FetchString (wantedPar);
      PushResult (dynobj, "Template parameter '%s' is missing!", parName.GetData ());
    }
  }
}

void SanityChecker::Check (iDynamicObject* dynobj)
{
  iCelEntityTemplate* tpl = FindTemplateForObject (dynobj);
  if (dynobj->GetEntityName () && !tpl)
    PushResult (dynobj, "Object '%s' has no matching template!", dynobj->GetEntityName ());

  if (tpl)
  {
    csHash<celDataType,csStringID> wantedTypes = InspectTools::GetTemplateParameters (pl, tpl);
    csHash<celDataType,csStringID> givenTypes = InspectTools::GetObjectParameters (dynobj);

    CheckParameterTypes (dynobj, givenTypes, wantedTypes);
  }
}

void SanityChecker::Check (iDynamicCell* cell)
{
  for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
    Check (cell->GetObject (i));
}

void SanityChecker::CheckObjects ()
{
  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
    Check (dynworld->GetFactory (i));

  csRef<iDynamicCellIterator> it = dynworld->GetCells ();
  while (it->HasNext ())
    Check (it->NextCell ());
}

iCelPropertyClassTemplate* SanityChecker::FindPCTemplate (iCelEntityTemplate* tpl,
    const char* name, const char* tag)
{
  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
    csString pcname = pctpl->GetName ();
    if (pcname == name && Equals (tag, pctpl->GetTag ()))
      return pctpl;
  }
  csRef<iCelEntityTemplateIterator> it = tpl->GetParents ();
  while (it->HasNext ())
  {
    iCelEntityTemplate* parent = it->Next ();
    iCelPropertyClassTemplate* pctpl = FindPCTemplate (parent, name, tag);
    if (pctpl) return pctpl;
  }
  return 0;
}

iQuestFactory* SanityChecker::FindQuest (iCelEntityTemplate* tpl, const char* tag)
{
  iCelPropertyClassTemplate* pctpl = FindPCTemplate (tpl, "pclogic.quest", tag);
  if (pctpl)
  {
    csString questName = InspectTools::FindActionParameter (pl, pctpl, "NewQuest", "name");
    if (questName.IsEmpty ()) return 0;
    return questManager->GetQuestFactory (questName);
  }
  return 0;
}

iDynamicObject* SanityChecker::FindEntity (const char* par)
{
  csRef<iDynamicCellIterator> it = dynworld->GetCells ();
  while (it->HasNext ())
  {
    iDynamicCell* cell = it->NextCell ();
    for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
    {
      iDynamicObject* object = cell->GetObject (i);
      const char* entityName = object->GetEntityName ();
      if (entityName && strcmp (entityName, par) == 0)
	return object;
    }
  }
  return 0;
}

iDynamicObject* SanityChecker::CheckExistingEntityAndReport (iQuestFactory* quest,
    const char* parent, const char* par)
{
  if (!IsConstant (par)) return 0;
  iDynamicObject* object = FindEntity (par);
  if (!object)
    if (strcmp (par, "World") != 0)
      PushResult (quest, "Cannot find entity '%s' in '%s'!", par, parent);
  return object;
}

void SanityChecker::CheckExistingEntityTemplateAndReport (iQuestFactory* quest, const char* parent,
    const char* par)
{
  if (!IsConstant (par)) return;
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (par);
  if (!tpl)
    PushResult (quest, "Cannot find template '%s' in '%s'!", par, parent);
}

iCelEntityTemplate* SanityChecker::FindTemplateForObject (iDynamicObject* object)
{
  iCelEntityTemplate* tpl = object->GetEntityTemplate ();
  if (tpl) return tpl;
  const char* deftpl = object->GetFactory ()->GetDefaultEntityTemplate ();
  if (deftpl) return pl->FindEntityTemplate (deftpl);
  return pl->FindEntityTemplate (object->GetFactory ()->GetName ());
}

void SanityChecker::Check (iQuestFactory* quest, iRewardFactory* reward)
{
  csString name = reward->GetRewardType ()->GetName ();
  if (name.StartsWith ("cel.rewards."))
    name = name.GetData ()+12;
  if (name == "newstate")
  {
    csRef<iNewStateQuestRewardFactory> tf = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    const char* entityPar = tf->GetEntityParameter ();
    const char* statePar = tf->GetStateParameter ();
    const char* tagPar = tf->GetTagParameter ();
    CheckExistingEntityAndReport (quest, name, entityPar);
    if (IsEmpty (entityPar) || Equals ("$this", entityPar))
    {
      if (IsConstant (statePar))
	if (!quest->GetState (statePar))
	  PushResult (quest, "Cannot find state '%s' in 'newstate' for this quest!", statePar);
    }
    else if (IsConstant (entityPar) && IsConstantOrEmpty (tagPar))
    {
      iDynamicObject* object = FindEntity (entityPar);
      if (object)
      {
	iCelEntityTemplate* tpl = FindTemplateForObject (object);
	if (tpl)
	{
	  iQuestFactory* destQuest = FindQuest (tpl, tagPar);
	  if (!destQuest)
	    PushResult (quest, "Cannot find a quest in entity '%s' for 'newstate'!", entityPar);
	  else if (IsConstant (statePar))
	    if (!destQuest->GetState (statePar))
	      PushResult (quest, "Cannot find state '%s' in quest at entity '%s' for 'newstate'!",
		statePar, entityPar);
	}
      }
    }
    //Par (pl, params, tf->GetClassParameter ());
  }
  else if (name == "action")
  {
    csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (reward);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetClass ());
    //Par (pl, params, tf->GetID ());
    //Par (pl, params, tf->GetPropertyClass ());
    //Par (pl, params, tf->GetTag ());
  }
  else if (name == "changeproperty")
  {
    csRef<iChangePropertyRewardFactory> tf = scfQueryInterface<iChangePropertyRewardFactory> (reward);
    const char* entityPar = tf->GetEntity ();
    const char* pcPar = tf->GetPC ();
    const char* tagPar = tf->GetPCTag ();
    iDynamicObject* object = CheckExistingEntityAndReport (quest, name, entityPar);
    if (object) CheckObjectForPC (object, quest, entityPar, pcPar, tagPar, name);

    //Par (pl, params, tf->GetClass ());
    //Par (pl, params, tf->GetProperty ());
    //Par (pl, params, tf->GetString ());
    //Par (pl, params, tf->GetLong ());
    //Par (pl, params, tf->GetFloat ());
    //Par (pl, params, tf->GetBool ());
    //Par (pl, params, tf->GetDiff ());
  }
  else if (name == "createentity")
  {
    csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (reward);
    CheckExistingEntityTemplateAndReport (quest, name, tf->GetEntityTemplate ());
    //Par (pl, params, tf->GetName ());
  }
  else if (name == "destroyentity")
  {
    csRef<iDestroyEntityRewardFactory> tf = scfQueryInterface<iDestroyEntityRewardFactory> (reward);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetClass ());
  }
  else if (name == "debugprint")
  {
    csRef<iDebugPrintRewardFactory> tf = scfQueryInterface<iDebugPrintRewardFactory> (reward);
    //Par (pl, params, tf->GetMessage ());
  }
  else if (name == "inventory")
  {
    csRef<iInventoryRewardFactory> tf = scfQueryInterface<iInventoryRewardFactory> (reward);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    CheckExistingEntityAndReport (quest, name, tf->GetChildEntity ());
    //Par (pl, params, tf->GetTag ());
    //Par (pl, params, tf->GetChildTag ());
  }
  else if (name == "message")
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (reward);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetClass ());
    //Par (pl, params, tf->GetID ());
    //for (size_t i = 0 ; i < tf->GetParameterCount () ; i++)
      //Par (pl, params, tf->GetParameterValue (i));
  }
  else if (name == "cssequence")
  {
    csRef<iCsSequenceRewardFactory> tf = scfQueryInterface<iCsSequenceRewardFactory> (reward);
    //Par (pl, params, tf->GetSequence ());
    //Par (pl, params, tf->GetDelay ());
  }
  else if (name == "sequence")
  {
    csRef<iSequenceRewardFactory> tf = scfQueryInterface<iSequenceRewardFactory> (reward);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetTag ());
    //Par (pl, params, tf->GetClass ());
    //Par (pl, params, tf->GetSequence ());
    //Par (pl, params, tf->GetDelay ());
  }
  else if (name == "sequencefinish")
  {
    csRef<iSequenceFinishRewardFactory> tf = scfQueryInterface<iSequenceFinishRewardFactory> (reward);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetTag ());
    //Par (pl, params, tf->GetClass ());
    //Par (pl, params, tf->GetSequence ());
  }
  else
  {
    printf ("Sanity checker: unsupported reward type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void SanityChecker::Check (iQuestFactory* quest, iRewardFactoryArray* rewards)
{
  for (size_t i = 0 ; i < rewards->GetSize () ; i++)
  {
    iRewardFactory* reward = rewards->Get (i);
    Check (quest, reward);
  }
}

void SanityChecker::CheckObjectForPC (iDynamicObject* object, iQuestFactory* quest,
    const char* entityPar, const char* pcPar, const char* tagPar, const char* name)
{
  if (!IsConstant (pcPar)) return;
  if (!IsConstantOrEmpty (tagPar)) return;
  iCelEntityTemplate* tpl = FindTemplateForObject (object);
  if (tpl)
    if (!FindPCTemplate (tpl, pcPar, tagPar))
      PushResult (quest, "Cannot find %s in entity '%s' for '%s'!", pcPar, entityPar, name);
}

void SanityChecker::Check (iQuestFactory* quest, iTriggerFactory* trigger)
{
  csString name = trigger->GetTriggerType ()->GetName ();
  if (name.StartsWith ("cel.triggers."))
    name = name.GetData ()+13;
  if (name == "entersector" || name == "meshentersector")
  {
    csRef<iEnterSectorTriggerFactory> tf = scfQueryInterface<iEnterSectorTriggerFactory> (trigger);
    const char* entityPar = tf->GetEntity ();
    const char* tagPar = tf->GetTag ();
    //Par (pl, params, tf->GetSector ());
    iDynamicObject* object = CheckExistingEntityAndReport (quest, name, entityPar);
    if (object) CheckObjectForPC (object, quest, entityPar, "pcobject.mesh", tagPar, name);
  }
  else if (name == "inventory")
  {
    csRef<iInventoryTriggerFactory> tf = scfQueryInterface<iInventoryTriggerFactory> (trigger);
    const char* entityPar = tf->GetEntity ();
    const char* tagPar = tf->GetTag ();
    iDynamicObject* object = CheckExistingEntityAndReport (quest, name, entityPar);
    CheckExistingEntityAndReport (quest, name, tf->GetChildEntity ());
    CheckExistingEntityTemplateAndReport (quest, name, tf->GetChildTemplate ());
    if (object) CheckObjectForPC (object, quest, entityPar, "pctools.inventory", tagPar, name);
  }
  else if (name == "meshselect")
  {
    csRef<iMeshSelectTriggerFactory> tf = scfQueryInterface<iMeshSelectTriggerFactory> (trigger);
    const char* entityPar = tf->GetEntity ();
    const char* tagPar = tf->GetTag ();
    iDynamicObject* object = CheckExistingEntityAndReport (quest, name, entityPar);
    if (object) CheckObjectForPC (object, quest, entityPar, "pcobject.mesh", tagPar, name);
  }
  else if (name == "message")
  {
    csRef<iMessageTriggerFactory> tf = scfQueryInterface<iMessageTriggerFactory> (trigger);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
  }
  else if (name == "propertychange")
  {
    csRef<iPropertyChangeTriggerFactory> tf = scfQueryInterface<iPropertyChangeTriggerFactory> (trigger);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    // @@@ Check if the property class exists
    //Par (pl, params, tf->GetTag ());
    //Par (pl, params, tf->GetProperty ());
    //Par (pl, params, tf->GetValue ());
  }
  else if (name == "sequencefinish")
  {
    csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (trigger);
    const char* entityPar = tf->GetEntity ();
    const char* tagPar = tf->GetTag ();
    iDynamicObject* object = CheckExistingEntityAndReport (quest, name, entityPar);
    if (object) CheckObjectForPC (object, quest, entityPar, "pclogic.quest", tagPar, name);
    //Par (pl, params, tf->GetSequence ());
  }
  else if (name == "timeout")
  {
    csRef<iTimeoutTriggerFactory> tf = scfQueryInterface<iTimeoutTriggerFactory> (trigger);
    //Par (pl, params, tf->GetTimeout ());
  }
  else if (name == "trigger")
  {
    csRef<iTriggerTriggerFactory> tf = scfQueryInterface<iTriggerTriggerFactory> (trigger);
    const char* entityPar = tf->GetEntity ();
    const char* tagPar = tf->GetTag ();
    iDynamicObject* object = CheckExistingEntityAndReport (quest, name, entityPar);
    if (object) CheckObjectForPC (object, quest, entityPar, "pclogic.trigger", tagPar, name);
  }
  else if (name == "watch")
  {
    csRef<iWatchTriggerFactory> tf = scfQueryInterface<iWatchTriggerFactory> (trigger);
    const char* entityPar = tf->GetEntity ();
    const char* tagPar = tf->GetTag ();
    iDynamicObject* object = CheckExistingEntityAndReport (quest, name, entityPar);
    if (object) CheckObjectForPC (object, quest, entityPar, "pcobject.mesh", tagPar, name);

    const char* targetEntityPar = tf->GetTargetEntity ();
    const char* targetTagPar = tf->GetTargetTag ();
    iDynamicObject* targetObject = CheckExistingEntityAndReport (quest, name, targetEntityPar);
    if (targetObject) CheckObjectForPC (targetObject, quest, targetEntityPar, "pcobject.mesh",
	targetTagPar, name);
    //Par (pl, params, tf->GetChecktime ());
    //Par (pl, params, tf->GetRadius ());
    //Par (pl, params, tf->GetOffsetX ());
    //Par (pl, params, tf->GetOffsetY ());
    //Par (pl, params, tf->GetOffsetZ ());
  }
  else
  {
    printf ("Sanity checker: unsupported trigger type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void SanityChecker::Check (iQuestFactory* quest, iSeqOpFactory* seqopFact)
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
    //Par (pl, params, tf->GetMessage ());
  }
  else if (name == "ambientmesh")
  {
    csRef<iAmbientMeshSeqOpFactory> tf = scfQueryInterface<iAmbientMeshSeqOpFactory> (seqopFact);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetTag ());
    //Par (pl, params, tf->GetRelColorRed ());
    //Par (pl, params, tf->GetRelColorGreen ());
    //Par (pl, params, tf->GetRelColorBlue ());
    //Par (pl, params, tf->GetAbsColorRed ());
    //Par (pl, params, tf->GetAbsColorGreen ());
    //Par (pl, params, tf->GetAbsColorBlue ());
  }
  else if (name == "light")
  {
    csRef<iLightSeqOpFactory> tf = scfQueryInterface<iLightSeqOpFactory> (seqopFact);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetTag ());
    //Par (pl, params, tf->GetRelColorRed ());
    //Par (pl, params, tf->GetRelColorGreen ());
    //Par (pl, params, tf->GetRelColorBlue ());
    //Par (pl, params, tf->GetAbsColorRed ());
    //Par (pl, params, tf->GetAbsColorGreen ());
    //Par (pl, params, tf->GetAbsColorBlue ());
  }
  else if (name == "movepath")
  {
    csRef<iMovePathSeqOpFactory> tf = scfQueryInterface<iMovePathSeqOpFactory> (seqopFact);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetTag ());
    //for (size_t i = 0 ; i < tf->GetPathCount () ; i++)
    //{
      //Par (pl, params, tf->GetPathSector (i));
      //Par (pl, params, tf->GetPathNode (i));
      //Par (pl, params, tf->GetPathTime (i));
    //}
  }
  else if (name == "property")
  {
    csRef<iPropertySeqOpFactory> tf = scfQueryInterface<iPropertySeqOpFactory> (seqopFact);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetPC ());
    //Par (pl, params, tf->GetPCTag ());
    //Par (pl, params, tf->GetProperty ());
    //Par (pl, params, tf->GetFloat ());
    //Par (pl, params, tf->GetLong ());
    //Par (pl, params, tf->GetVectorX ());
    //Par (pl, params, tf->GetVectorY ());
    //Par (pl, params, tf->GetVectorZ ());
  }
  else if (name == "transform")
  {
    csRef<iTransformSeqOpFactory> tf = scfQueryInterface<iTransformSeqOpFactory> (seqopFact);
    CheckExistingEntityAndReport (quest, name, tf->GetEntity ());
    //Par (pl, params, tf->GetTag ());
    //Par (pl, params, tf->GetVectorX ());
    //Par (pl, params, tf->GetVectorY ());
    //Par (pl, params, tf->GetVectorZ ());
    //Par (pl, params, tf->GetRotationAngle ());
  }
  else
  {
    printf ("Sanity checker: unsupported sequence operation type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void SanityChecker::Check (iQuestFactory* quest)
{
  csRef<iQuestStateFactoryIterator> stateIt = quest->GetStates ();
  while (stateIt->HasNext ())
  {
    iQuestStateFactory* state = stateIt->Next ();
    csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
    for (size_t i = 0 ; i < responses->GetSize () ; i++)
    {
      iQuestTriggerResponseFactory* response = responses->Get (i);
      Check (quest, response->GetRewardFactories ());
      iTriggerFactory* trigger = response->GetTriggerFactory ();
      Check (quest, trigger);
    }
    Check (quest, state->GetInitRewardFactories ());
    Check (quest, state->GetExitRewardFactories ());
  }

  csRef<iCelSequenceFactoryIterator> seqIt = quest->GetSequences ();
  while (seqIt->HasNext ())
  {
    iCelSequenceFactory* seq = seqIt->Next ();
    for (size_t i = 0 ; i < seq->GetSeqOpFactoryCount () ; i++)
    {
      iSeqOpFactory* seqopFact = seq->GetSeqOpFactory (i);
      Check (quest, seqopFact);
    }
  }
}

void SanityChecker::CheckQuests ()
{
  csRef<iQuestFactoryIterator> it = questManager->GetQuestFactories ();
  while (it->HasNext ())
    Check (it->Next ());
}

void SanityChecker::CheckAll ()
{
  ClearResults ();
  CheckTemplates ();
  CheckObjects ();
  CheckQuests ();
}

