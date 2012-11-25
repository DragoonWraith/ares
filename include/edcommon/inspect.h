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

#ifndef __appares_inspect_h
#define __appares_inspect_h

#include <crystalspace.h>
#include "aresextern.h"
#include "physicallayer/datatype.h"
#include "celtool/stdparams.h"

struct iCelPropertyClassTemplate;
struct iRewardFactoryArray;
struct iTriggerFactory;
struct iCelPlLayer;
struct iParameter;
struct iParameterManager;
struct iObjectComment;
struct iDynamicObject;
struct iPcDynamicWorld;
struct iQuestManager;
struct iEngine;
struct iQuestFactory;
struct iCelParameterIterator;
struct iCelEntityTemplate;
struct iRewardFactory;
struct iSeqOpFactory;
struct iTriggerFactory;
struct iRewardFactoryArray;

typedef csHash<csRef<iParameter>,csStringID> ParHash;

/**
 * What kind of parameter it is (celDataType is the actual type but this
 * is the actual interpretation).
 */
enum celParameterType
{
  PAR_NONE,
  PAR_CONFLICT,		// Used in case there is a conflicting parameter usage.
  PAR_ENTITY,
  PAR_TEMPLATE,
  PAR_TAG,
  PAR_PC,
  PAR_MESSAGE,
  PAR_SECTOR,
  PAR_NODE,
  PAR_PROPERTY,
  PAR_SEQUENCE,
  PAR_CSSEQUENCE,
  PAR_STATE,
  PAR_CLASS,
  PAR_VALUE,		// Just a value, no specific type.
};

/**
 * This class is used to specify the domain for a parameter
 * for GetQuestParameters() and GetTemplateParameters().
 */
struct ARES_EDCOMMON_EXPORT ParameterDomain
{
  celDataType type;
  celParameterType parType;

  ParameterDomain () : type (CEL_DATA_NONE), parType (PAR_NONE) { }
  ParameterDomain (celDataType type, celParameterType parType) :
    type (type), parType (parType) { }
};

class ARES_EDCOMMON_EXPORT InspectTools
{
private:
  static void CollectParParameters (iCelPlLayer* pl, iCelParameterIterator* it,
      csHash<ParameterDomain,csStringID>& paramTypes,
      csHash<ParameterDomain,csStringID>* questParamTypes = 0);
  static void CollectPCParameters (iCelPlLayer* pl, iQuestManager* questManager,
      iCelPropertyClassTemplate* pctpl,
      csHash<ParameterDomain,csStringID>& paramTypes);
  static void CollectTemplateParameters (iCelPlLayer* pl, iQuestManager* questManager,
      iCelEntityTemplate* tpl,
      csHash<ParameterDomain,csStringID>& paramTypes);
  static void CollectTriggerParameters (iCelPlLayer* pl, iTriggerFactory* trigger,
      csHash<ParameterDomain,csStringID>& params);
  static void CollectRewardParameters (iCelPlLayer* pl, iRewardFactory* reward,
      csHash<ParameterDomain,csStringID>& params);
  static void CollectRewardParameters (iCelPlLayer* pl, iRewardFactoryArray* rewards,
      csHash<ParameterDomain,csStringID>& params);
  static void CollectSeqopParameters (iCelPlLayer* pl, iSeqOpFactory* seqopFact,
      csHash<ParameterDomain,csStringID>& params);

public:
  static celData GetPropertyValue (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid = 0);
  static csString GetPropertyValueString (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid = 0);
  static bool GetPropertyValueBool (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid = 0);
  static long GetPropertyValueLong (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid = 0);
  static float GetPropertyValueFloat (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* propName, bool* valid = 0);

  /**
   * Find the index of the action for which a given parameter has a certain
   * value.
   */
  static size_t FindActionWithParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName,
      const char* parName, const char* parValue);

  /**
   * Find an action and return the value for a given parameter.
   * value.
   */
  static csString FindActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName,
      const char* parName);

  /**
   * Find an action and return the parameter iterator.
   */
  static csRef<iCelParameterIterator> FindAction (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName);

  /**
   * Delete a given parameter from an action.
   * 'idx' is the index of the action.
   * Returns true on success.
   */
  static bool DeleteActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, size_t idx, const char* parName);

  /**
   * Delete a given parameter from an action.
   * Returns true on success.
   */
  static bool DeleteActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName);

  /**
   * Add a parameter to an action.
   * 'idx' is the index of the action.
   */
  static void AddActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, size_t idx,
      const char* parName, iParameter* parameter);

  /**
   * Add a parameter to an action.
   */
  static void AddActionParameter (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName,
      const char* parName, iParameter* parameter);

  /**
   * Add an action with parameters.
   * Following the actionName is a list of celDataType/name/value tuples ended
   * by the value CEL_DATA_NONE.
   */
  static void AddAction (iCelPlLayer* pl, iParameterManager* pm,
      iCelPropertyClassTemplate* pctpl, const char* actionName, ...);

  /**
   * Get the value of a parameter for an action.
   */
  static iParameter* GetActionParameterValue (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, size_t idx, const char* parName);
  static iParameter* GetActionParameterValue (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName);
  static csString GetActionParameterValueString (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid = 0);
  static bool GetActionParameterValueBool (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid = 0);
  static long GetActionParameterValueLong (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid = 0);
  static float GetActionParameterValueFloat (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid = 0);
  static csVector3 GetActionParameterValueVector3 (iCelPlLayer* pl,
      iCelPropertyClassTemplate* pctpl, const char* actionName, const char* parName,
      bool* valid = 0);

  static const char* TypeToString (celDataType type)
  {
    switch (type)
    {
      case CEL_DATA_NONE: return "none";
      case CEL_DATA_BOOL: return "bool";
      case CEL_DATA_LONG: return "long";
      case CEL_DATA_FLOAT: return "float";
      case CEL_DATA_VECTOR2: return "vector2";
      case CEL_DATA_VECTOR3: return "vector3";
      case CEL_DATA_STRING: return "string";
      case CEL_DATA_COLOR: return "color";
      default: return "?";
    }
  }

  static celDataType StringToType (const csString& type)
  {
    if (type == "bool") return CEL_DATA_BOOL;
    if (type == "long") return CEL_DATA_LONG;
    if (type == "float") return CEL_DATA_FLOAT;
    if (type == "string") return CEL_DATA_STRING;
    if (type == "vector2") return CEL_DATA_VECTOR2;
    if (type == "vector3") return CEL_DATA_VECTOR3;
    if (type == "color") return CEL_DATA_COLOR;
    return CEL_DATA_NONE;
  }

  /**
   * Return the parameters that a dynamic object provides for its template.
   */
  static csHash<ParameterDomain,csStringID> GetObjectParameters (iDynamicObject* dynobj);

  /**
   * Return the parameters that a template needs.
   */
  static csHash<ParameterDomain,csStringID> GetTemplateParameters (
      iCelPlLayer* pl, iQuestManager* questManager, iCelEntityTemplate* tpl);

  /**
   * Return the parameters that a quest needs.
   */
  static csHash<ParameterDomain,csStringID> GetQuestParameters (
      iCelPlLayer* pl, iQuestFactory* quest);
};

class ARES_EDCOMMON_EXPORT ResourceCounter
{
private:
  iObjectRegistry* object_reg;
  iPcDynamicWorld* dynworld;
  csRef<iCelPlLayer> pl;
  csRef<iQuestManager> questMgr;
  csRef<iEngine> engine;

  iObject* filter;

  csHash<int,csString> templateCounter;
  csHash<int,csString> questCounter;
  csHash<int,csString> lightCounter;

  void CountTemplatesInPC (
      iCelPropertyClassTemplate* pctpl,
      const char* nameField,
      const char* nameAction,
      const char* tplName);
  void CountResourcesInRewards (iRewardFactoryArray* rewards, iQuestFactory* questFact);
  void CountResourcesInTrigger (iTriggerFactory* trigger, iQuestFactory* questFact);

  bool inc (csHash<int,csString>& counter, iObject* object, const char* name);

public:
  ResourceCounter (iObjectRegistry* object_reg, iPcDynamicWorld* dynworld);

  /**
   * Set a filter to only look at the given resource. Note that in this
   * case the counter will be verbose and report every usage it finds to the
   * reporter.
   */
  void SetFilter (iObject* filter)
  {
    ResourceCounter::filter = filter;
  }

  const csHash<int,csString>& GetTemplateCounter () const { return templateCounter; }
  const csHash<int,csString>& GetQuestCounter () const { return questCounter; }
  const csHash<int,csString>& GetLightCounter () { return lightCounter; }

  void CountResourcesInFactories ();
  void CountResourcesInObjects ();
  void CountResourcesInTemplates ();
  void CountResourcesInQuests ();
  void CountResources ()
  {
    CountResourcesInFactories ();
    CountResourcesInObjects ();
    CountResourcesInTemplates ();
    CountResourcesInQuests ();
  }
};

#endif // __appares_inspect_h

