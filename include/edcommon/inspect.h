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

struct iCelPropertyClassTemplate;
struct iCelPlLayer;
struct iParameter;
struct iParameterManager;

typedef csHash<csRef<iParameter>,csStringID> ParHash;

class ARES_EDCOMMON_EXPORT InspectTools
{
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
};

#endif // __appares_inspect_h

