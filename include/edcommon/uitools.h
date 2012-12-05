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

#ifndef __appares_uitools_h
#define __appares_uitools_h

#include <csutil/stringarray.h>
#include "aresextern.h"

class wxWindow;

/**
 * Various tools for UI.
 */
class ARES_EDCOMMON_EXPORT UITools
{
public:
  /**
   * Clear a single control by name.
   */
  static void ClearControl (wxWindow* parent, const char* name);

  /**
   * Clear all the controls by name in this parent.
   * Use (const char*)0 to end.
   */
  static void ClearControls (wxWindow* parent, ...);

  /**
   * Clear items on a wxControlWithItems.
   */
  static void ClearChoices (wxWindow* parent, const char* name);

  /**
   * Add choices to a wxControlWithItems.
   */
  static void AddChoices (wxWindow* parent, const char* name, ...);

  /**
   * Get the value of a specific control as a string.
   */
  static csString GetValue (wxWindow* component);

  /**
   * Get the value of a specific control as a string.
   */
  static csString GetValue (wxWindow* parent, const char* name);

  /**
   * Set the value of a specific control (only for controls supporting
   * text values like text fields, buttons, and labels).
   * Return false if the component type is not recognized.
   */
  static bool SetValue (wxWindow* component, const char* value);

  /**
   * Set the value of a specific control (only for controls supporting
   * text values like text fields, buttons, and labels).
   */
  static void SetValue (wxWindow* parent, const char* name, const char* value);

  /**
   * Set the value of a specific control (only for controls supporting
   * text values like text fields, buttons, and labels).
   */
  static void SetValue (wxWindow* parent, const char* name, float value);

  /**
   * Set the value of a specific control (only for controls supporting
   * text values like text fields, buttons, and labels).
   */
  static void SetValue (wxWindow* parent, const char* name, int value);

  /**
   * Switch a wxNotebook or wxChoicebook to the specific page. Returns false
   * on failure (page could not be found or component not of the right type).
   */
  static bool SwitchPage (wxWindow* parent, const char* name, const char* pageName);
};

#endif // __appares_uitools_h

