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

SanityChecker::SanityChecker (iObjectRegistry* object_reg)
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

void SanityChecker::PushResult (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl,
    const char* msg, ...)
{
  va_list args;
  va_start (args, msg);
  results.Push (SanityResult ().
	Resource (tpl->QueryObject ()).
	Name ("%s/%s(%s)", tpl->GetName (), pctpl->GetName (), pctpl->GetTag ()).
	MessageV (msg, args));
  va_end (args);
}

static void Par (iCelPlLayer* pl, csSet<csStringID>& params, const char* par)
{
  if (!par || !*par) return;
  if (*par != '$') return;
  if (strcmp (par+1, "this") == 0) return;
  params.Add (pl->FetchStringID (par+1));
}

void SanityChecker::CollectSeqopParameters (iSeqOpFactory* seqopFact, csSet<csStringID>& params)
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
    Par (pl, params, tf->GetMessage ());
  }
  else if (name == "ambientmesh")
  {
    csRef<iAmbientMeshSeqOpFactory> tf = scfQueryInterface<iAmbientMeshSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetRelColorRed ());
    Par (pl, params, tf->GetRelColorGreen ());
    Par (pl, params, tf->GetRelColorBlue ());
    Par (pl, params, tf->GetAbsColorRed ());
    Par (pl, params, tf->GetAbsColorGreen ());
    Par (pl, params, tf->GetAbsColorBlue ());
  }
  else if (name == "light")
  {
    csRef<iLightSeqOpFactory> tf = scfQueryInterface<iLightSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetRelColorRed ());
    Par (pl, params, tf->GetRelColorGreen ());
    Par (pl, params, tf->GetRelColorBlue ());
    Par (pl, params, tf->GetAbsColorRed ());
    Par (pl, params, tf->GetAbsColorGreen ());
    Par (pl, params, tf->GetAbsColorBlue ());
  }
  else if (name == "movepath")
  {
    csRef<iMovePathSeqOpFactory> tf = scfQueryInterface<iMovePathSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    for (size_t i = 0 ; i < tf->GetPathCount () ; i++)
    {
      Par (pl, params, tf->GetPathSector (i));
      Par (pl, params, tf->GetPathNode (i));
      Par (pl, params, tf->GetPathTime (i));
    }
  }
  else if (name == "property")
  {
    csRef<iPropertySeqOpFactory> tf = scfQueryInterface<iPropertySeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetPC ());
    Par (pl, params, tf->GetPCTag ());
    Par (pl, params, tf->GetProperty ());
    Par (pl, params, tf->GetFloat ());
    Par (pl, params, tf->GetLong ());
    Par (pl, params, tf->GetVectorX ());
    Par (pl, params, tf->GetVectorY ());
    Par (pl, params, tf->GetVectorZ ());
  }
  else if (name == "transform")
  {
    csRef<iTransformSeqOpFactory> tf = scfQueryInterface<iTransformSeqOpFactory> (seqopFact);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetVectorX ());
    Par (pl, params, tf->GetVectorY ());
    Par (pl, params, tf->GetVectorZ ());
    Par (pl, params, tf->GetRotationAngle ());
  }
  else
  {
    printf ("Sanity checker: unsupported sequence operation type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void SanityChecker::CollectTriggerParameters (iTriggerFactory* trigger, csSet<csStringID>& params)
{
  csString name = trigger->GetTriggerType ()->GetName ();
  if (name.StartsWith ("cel.triggers."))
    name = name.GetData ()+13;
  if (name == "entersector" || name == "meshentersector")
  {
    csRef<iEnterSectorTriggerFactory> tf = scfQueryInterface<iEnterSectorTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetSector ());
  }
  else if (name == "inventory")
  {
    csRef<iInventoryTriggerFactory> tf = scfQueryInterface<iInventoryTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetChildEntity ());
    Par (pl, params, tf->GetChildTemplate ());
  }
  else if (name == "meshselect")
  {
    csRef<iMeshSelectTriggerFactory> tf = scfQueryInterface<iMeshSelectTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
  }
  else if (name == "message")
  {
    csRef<iMessageTriggerFactory> tf = scfQueryInterface<iMessageTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity ());
  }
  else if (name == "propertychange")
  {
    csRef<iPropertyChangeTriggerFactory> tf = scfQueryInterface<iPropertyChangeTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetProperty ());
    Par (pl, params, tf->GetValue ());
  }
  else if (name == "sequencefinish")
  {
    csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetSequence ());
  }
  else if (name == "timeout")
  {
    csRef<iTimeoutTriggerFactory> tf = scfQueryInterface<iTimeoutTriggerFactory> (trigger);
    Par (pl, params, tf->GetTimeout ());
  }
  else if (name == "trigger")
  {
    csRef<iTriggerTriggerFactory> tf = scfQueryInterface<iTriggerTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
  }
  else if (name == "watch")
  {
    csRef<iWatchTriggerFactory> tf = scfQueryInterface<iWatchTriggerFactory> (trigger);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetTargetEntity ());
    Par (pl, params, tf->GetTargetTag ());
    Par (pl, params, tf->GetChecktime ());
    Par (pl, params, tf->GetRadius ());
    Par (pl, params, tf->GetOffsetX ());
    Par (pl, params, tf->GetOffsetY ());
    Par (pl, params, tf->GetOffsetZ ());
  }
  else
  {
    printf ("Sanity checker: unsupported trigger type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void SanityChecker::CollectRewardParameters (iRewardFactory* reward, csSet<csStringID>& params)
{
  csString name = reward->GetRewardType ()->GetName ();
  if (name.StartsWith ("cel.rewards."))
    name = name.GetData ()+12;
  if (name == "newstate")
  {
    csRef<iNewStateQuestRewardFactory> tf = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    Par (pl, params, tf->GetStateParameter ());
    Par (pl, params, tf->GetEntityParameter ());
    Par (pl, params, tf->GetTagParameter ());
    Par (pl, params, tf->GetClassParameter ());
  }
  else if (name == "action")
  {
    csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (reward);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetClass ());
    Par (pl, params, tf->GetID ());
    Par (pl, params, tf->GetPropertyClass ());
    Par (pl, params, tf->GetTag ());
  }
  else if (name == "changeproperty")
  {
    csRef<iChangePropertyRewardFactory> tf = scfQueryInterface<iChangePropertyRewardFactory> (reward);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetClass ());
    Par (pl, params, tf->GetPC ());
    Par (pl, params, tf->GetPCTag ());
    Par (pl, params, tf->GetProperty ());
    Par (pl, params, tf->GetString ());
    Par (pl, params, tf->GetLong ());
    Par (pl, params, tf->GetFloat ());
    Par (pl, params, tf->GetBool ());
    Par (pl, params, tf->GetDiff ());
  }
  else if (name == "createentity")
  {
    csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (reward);
    Par (pl, params, tf->GetEntityTemplate ());
    Par (pl, params, tf->GetName ());
  }
  else if (name == "destroyentity")
  {
    csRef<iDestroyEntityRewardFactory> tf = scfQueryInterface<iDestroyEntityRewardFactory> (reward);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetClass ());
  }
  else if (name == "debugprint")
  {
    csRef<iDebugPrintRewardFactory> tf = scfQueryInterface<iDebugPrintRewardFactory> (reward);
    Par (pl, params, tf->GetMessage ());
  }
  else if (name == "inventory")
  {
    csRef<iInventoryRewardFactory> tf = scfQueryInterface<iInventoryRewardFactory> (reward);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetChildEntity ());
    Par (pl, params, tf->GetChildTag ());
  }
  else if (name == "message")
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (reward);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetClass ());
  }
  else if (name == "cssequence")
  {
    csRef<iCsSequenceRewardFactory> tf = scfQueryInterface<iCsSequenceRewardFactory> (reward);
    Par (pl, params, tf->GetSequence ());
    Par (pl, params, tf->GetDelay ());
  }
  else if (name == "sequence")
  {
    csRef<iSequenceRewardFactory> tf = scfQueryInterface<iSequenceRewardFactory> (reward);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetClass ());
    Par (pl, params, tf->GetSequence ());
    Par (pl, params, tf->GetDelay ());
  }
  else if (name == "sequencefinish")
  {
    csRef<iSequenceFinishRewardFactory> tf = scfQueryInterface<iSequenceFinishRewardFactory> (reward);
    Par (pl, params, tf->GetEntity ());
    Par (pl, params, tf->GetTag ());
    Par (pl, params, tf->GetClass ());
    Par (pl, params, tf->GetSequence ());
  }
  else
  {
    printf ("Sanity checker: unsupported reward type '%s'!\n", name.GetData ());
    fflush (stdout);
  }
}

void SanityChecker::CollectRewardParameters (iRewardFactoryArray* rewards, csSet<csStringID>& params)
{
  for (size_t i = 0 ; i < rewards->GetSize () ; i++)
  {
    iRewardFactory* reward = rewards->Get (i);
    CollectRewardParameters (reward, params);
  }
}

csSet<csStringID> SanityChecker::CollectQuestParameters (iQuestFactory* quest)
{
  csSet<csStringID> params;
  csRef<iQuestStateFactoryIterator> stateIt = quest->GetStates ();
  while (stateIt->HasNext ())
  {
    iQuestStateFactory* state = stateIt->Next ();
    csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
    for (size_t i = 0 ; i < responses->GetSize () ; i++)
    {
      iQuestTriggerResponseFactory* response = responses->Get (i);
      CollectRewardParameters (response->GetRewardFactories (), params);
      iTriggerFactory* trigger = response->GetTriggerFactory ();
      CollectTriggerParameters (trigger, params);
    }
    CollectRewardParameters (state->GetInitRewardFactories (), params);
    CollectRewardParameters (state->GetExitRewardFactories (), params);
  }

  csRef<iCelSequenceFactoryIterator> seqIt = quest->GetSequences ();
  while (seqIt->HasNext ())
  {
    iCelSequenceFactory* seq = seqIt->Next ();
    for (size_t i = 0 ; i < seq->GetSeqOpFactoryCount () ; i++)
    {
      iSeqOpFactory* seqopFact = seq->GetSeqOpFactory (i);
      CollectSeqopParameters (seqopFact, params);
      Par (pl, params, seq->GetSeqOpFactoryDuration (i));
    }
  }
  return params;
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

  csSet<csStringID> givenParameters;
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
      givenParameters.Add (parid);
    }
  }
  if (!questNameGiven)
  {
    PushResult (tpl, pctpl, "Quest name parameter is not given!");
    return;
  }
  if (quest)
  {
    csSet<csStringID> questParams = CollectQuestParameters (quest);
    csSet<csStringID> missing = Subtract (questParams, givenParameters);
    csSet<csStringID>::GlobalIterator it = missing.GetIterator ();
    while (it.HasNext ())
    {
      csStringID id = it.Next ();
      csString parName = pl->FetchString (id);
      PushResult (tpl, pctpl, "Quest parameter '%s' is missing!", parName.GetData ());
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

void SanityChecker::CheckAll ()
{
  CheckTemplates ();
}

