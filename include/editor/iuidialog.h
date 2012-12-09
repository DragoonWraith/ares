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

#ifndef __iuidialog_h__
#define __iuidialog_h__

#include "csutil/scf.h"
#include <wx/wx.h>
#include "edcommon/model.h"
#include "edcommon/smartpicker.h"

class wxCommandEvent;
class wxWindow;
class wxListCtrl;
class RowModel;

typedef csHash<csString,csString> DialogResult;
typedef csHash<csRef<Ares::Value>,csString> DialogValues;

struct iUIDialog;

struct iUIDialogCallback : public virtual iBase
{
  SCF_INTERFACE(iUIDialogCallback,0,0,1);
  virtual void ButtonPressed (iUIDialog* dialog, const char* button) = 0;
};


/**
 * A dialog.
 */
struct iUIDialog : public virtual iBase
{
  SCF_INTERFACE(iUIDialog,0,0,1);

  /// No automatic 'okcancel' row.
  virtual void DisableAutomaticOkCancel () = 0;

  /// Add a new row in this dialog.
  virtual void AddRow (int proportion = 0) = 0;

  /// Add a label in the current row.
  virtual void AddLabel (const char* str) = 0;

  /**
   * Add a single line text control in the current row.
   * if 'enterIsOk' is true then pressing enter in this text control will confirm the dialog.
   */
  virtual void AddText (const char* name, bool enterIsOk = false) = 0;

  /**
   * Add a typed text control.
   */
  virtual void AddTypedText (SmartPickerType type, const char* name) = 0;

  /// Add a multi line text control in the current row.
  virtual void AddMultiText (const char* name) = 0;
  /// Add a button in the current row.
  virtual void AddButton (const char* str) = 0;
  /// Add a combobox control in the current row with the given choices (end with 0).
  virtual void AddCombo (const char* name, ...) = 0;
  virtual void AddCombo (const char* name, const csStringArray& choiceArray) = 0;
  /// Add a choice control in the current row with the given choices (end with 0).
  virtual void AddChoice (const char* name, ...) = 0;
  virtual void AddChoice (const char* name, const csStringArray& choiceArray) = 0;
  /// Add a horizontal spacer in the current row.
  virtual void AddSpace () = 0;
  /**
   * Add a list. The given column index is used as the 'value'
   * from the model. The value should be a collection with composites
   * as children and the requested value out of the composite should
   * be a string.
   * If 'multi' is true then this is a multiselection list and the value
   * returned can hold multiple values.
   */
  virtual void AddList (const char* name, Ares::Value* collectionValue,
      size_t valueColumn, bool multi, int height, const char* heading, const char* names) = 0;
  virtual void AddListIndexed (const char* name, Ares::Value* collectionValue,
      size_t valueColumn, bool multi, int height, const char* heading, ...) = 0;
  virtual void AddListIndexed (const char* name, Ares::Value* collectionValue,
      size_t valueColumn, bool multi, int height, const char* heading, va_list args) = 0;

  // Clear all input fields to empty or default values.
  virtual void Clear () = 0;

  /**
   * Set the value of a given control. This will do the right thing depending
   * on the type of the control.
   */
  virtual void SetValue (const char* name, const char* value) = 0;

  /// Set the value of the given text control.
  virtual void SetText (const char* name, const char* value) = 0;
  /// Set the value of the given choice.
  virtual void SetChoice (const char* name, const char* value) = 0;
  /// Set the value of the given combo.
  virtual void SetCombo (const char* name, const char* value) = 0;
  /// Set the vlaue of the given list.
  virtual void SetList (const char* name, const char* value) = 0;

  /**
   * Show the dialog in a modal manner.
   * Returns 1 if Ok was pressed and 0 otherwise.
   */
  virtual int Show (iUIDialogCallback* cb) = 0;

  /**
   * Show the dialog in a non-modal manner.
   */
  virtual void ShowNonModal (iUIDialogCallback* cb) = 0;

  /**
   * Close the dialog (only if it was opened non-modal).
   */
  virtual void Close () = 0;

  /**
   * When any button is pressed (including Ok and Cancel) this will return
   * the contents of all text controls and choices.
   */
  virtual const DialogResult& GetFieldContents () const = 0;

  /**
   * When any button is pressed (including Ok and Cancel) this will return
   * the contents of all values.
   */
  virtual const DialogValues& GetFieldValues () const = 0;

  /**
   * Fill this dialog with the DialogResult contents.
   */
  virtual void SetFieldContents (const DialogResult& result) = 0;
};

#endif // __iuidialog_h__

