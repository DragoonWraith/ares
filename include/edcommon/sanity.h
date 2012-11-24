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

#ifndef __appares_sanity_h
#define __appares_sanity_h

#include <crystalspace.h>
#include "aresextern.h"

#include "physicallayer/datatype.h"

struct iCelPlLayer;
struct iCelEntityTemplate;
struct iCelPropertyClassTemplate;
struct iQuestFactory;
struct iQuestManager;
struct iRewardFactoryArray;
struct iRewardFactory;
struct iTriggerFactory;
struct iSeqOpFactory;
struct iPcDynamicWorld;
struct iDynamicFactory;
struct iDynamicObject;
struct iDynamicCell;
struct iCelParameterIterator;

struct ARES_EDCOMMON_EXPORT SanityResult
{
public:
  iObject* resource;
  iDynamicObject* object;
  csString name;
  csString message;

  SanityResult () : resource (0), object (0) { }

  SanityResult& Resource (iObject* object);
  SanityResult& Object (iDynamicObject* object);
  SanityResult& Name (const char* fmt, ...);
  SanityResult& Message (const char* fmt, ...);
  SanityResult& MessageV (const char* fmt, va_list args);
};

/**
 * Sanity checker on various kinds of objects.
 */
class ARES_EDCOMMON_EXPORT SanityChecker
{
private:
  csRef<iCelPlLayer> pl;
  csRef<iEngine> engine;
  csRef<iQuestManager> questManager;
  iPcDynamicWorld* dynworld;

  csArray<SanityResult> results;

  // Collect parameters.
  void CollectTriggerParameters (iTriggerFactory* trigger, csSet<csStringID>& params);
  void CollectRewardParameters (iRewardFactory* reward, csSet<csStringID>& params);
  void CollectRewardParameters (iRewardFactoryArray* rewards, csSet<csStringID>& params);
  void CollectSeqopParameters (iSeqOpFactory* seqopFact, csSet<csStringID>& params);
  csSet<csStringID> CollectQuestParameters (iQuestFactory* quest);

  void CollectParParameters (iCelParameterIterator* it, csHash<celDataType,csStringID>& paramTypes);
  void CollectPCParameters (iCelPropertyClassTemplate* pctpl, csHash<celDataType,csStringID>& paramTypes);
  void CollectTemplateParameters (iCelEntityTemplate* tpl, csHash<celDataType,csStringID>& paramTypes);

  void CollectObjectParameters (iDynamicObject* dynobj,
      csHash<celDataType,csStringID>& paramTypes);

  void CheckParameterTypes (iDynamicObject* dynobj,
    const csHash<celDataType,csStringID>& givenTypes,
    const csHash<celDataType,csStringID>& paramTypes);

  // Find stuff.
  iDynamicObject* FindEntity (const char* par);
  iCelPropertyClassTemplate* FindPCTemplate (iCelEntityTemplate* tpl,
      const char* name, const char* tag);
  iQuestFactory* FindQuest (iCelEntityTemplate* tpl, const char* tag);

  iCelEntityTemplate* FindTemplateForObject (iDynamicObject* object);

  // Check stuff.
  void CheckObjectForPC (iDynamicObject* object, iQuestFactory* quest,
    const char* entityPar, const char* pcPar, const char* tagPar, const char* name);
  iDynamicObject* CheckExistingEntityAndReport (iQuestFactory* quest, const char* parent, const char* par);
  void CheckExistingEntityTemplateAndReport (iQuestFactory* quest, const char* parent, const char* par);

  void CheckQuestPC (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl);

  void PushResult (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl,
      const char* msg, ...);
  void PushResult (iDynamicFactory* fact, const char* msg, ...);
  void PushResult (iDynamicObject* obj, const char* msg, ...);
  void PushResult (iQuestFactory* quest, const char* msg, ...);

public:
  SanityChecker (iObjectRegistry* object_reg, iPcDynamicWorld* dynworld);
  ~SanityChecker ();

  void ClearResults ();

  void Check (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl);
  void Check (iCelEntityTemplate* tpl);
  void Check (iDynamicFactory* dynfact);
  void Check (iDynamicObject* dynobj);
  void Check (iDynamicCell* cell);
  void Check (iQuestFactory* quest, iRewardFactory* reward);
  void Check (iQuestFactory* quest, iRewardFactoryArray* rewards);
  void Check (iQuestFactory* quest, iTriggerFactory* trigger);
  void Check (iQuestFactory* quest, iSeqOpFactory* seqop);
  void Check (iQuestFactory* quest);

  void CheckTemplates ();
  void CheckObjects ();
  void CheckQuests ();
  void CheckAll ();

  const csArray<SanityResult>& GetResults () const { return results; }
};

#endif // __appares_sanity_h

