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

#ifndef __uimanager_h
#define __uimanager_h

#include <wx/wx.h>
#include "../models/model.h"

class AppAresEditWX;
class FileReq;
class NewProjectDialog;
class CellDialog;
class UIDialog;
class DynfactDialog;
class RowModel;
class SimpleListCtrlView;

class wxBoxSizer;
class wxTextCtrl;
class wxChoice;
class wxButton;
class wxCommandEvent;
class wxWindow;
class wxListCtrl;

typedef csHash<csString,csString> DialogResult;

struct UIDialogCallback : public csRefCount
{
  virtual void ButtonPressed (UIDialog* dialog, const char* button) = 0;
};

struct ListInfo
{
  wxListCtrl* list;
  size_t col;
  csRef<RowModel> model;
  SimpleListCtrlView* view;
};

struct ValueListInfo
{
  wxListCtrl* list;
  size_t col;
  csRef<Ares::Value> collectionValue;
};

class UIDialog : public wxDialog, public Ares::View
{
private:
  wxPanel* mainPanel;
  wxBoxSizer* sizer;
  wxBoxSizer* lastRowSizer;
  csHash<wxTextCtrl*,csString> textFields;
  csHash<wxChoice*,csString> choiceFields;

  csHash<ListInfo,csString> listFields;
  csHash<ValueListInfo,csString> valueListFields;

  csArray<wxButton*> buttons;
  csRef<UIDialogCallback> callback;

  DialogResult fieldContents;

  bool okCancelAdded;
  void AddOkCancel ();

  virtual void OnButtonClicked (wxCommandEvent& event);

public:
  UIDialog (wxWindow* parent, const char* title, int width = -1, int height = -1);
  virtual ~UIDialog ();

  /// Add a new row in this dialog.
  void AddRow (int proportion = 0);

  /// Add a label in the current row.
  void AddLabel (const char* str);
  /// Add a single line text control in the current row.
  void AddText (const char* name);
  /// Add a multi line text control in the current row.
  void AddMultiText (const char* name);
  /// Add a button in the current row.
  void AddButton (const char* str);
  /// Add a choice control in the current row with the given choices (end with 0).
  void AddChoice (const char* name, ...);
  /// Add a horizontal spacer in the current row.
  void AddSpace ();
  /**
   * Add a list. The given column index is used as the 'value'
   * from the model.
   */
  void AddList (const char* name, RowModel* model, size_t valueColumn);
  /**
   * Add a list. The given column index is used as the 'value'
   * from the model. The value should be a collection with composites
   * as children and the requested value out of the composite should
   * be a string.
   */
  void AddList (const char* name, Ares::Value* collectionValue, size_t valueColumn,
      const char* heading, const char* names);

  // Clear all input fields to empty or default values.
  void Clear ();

  /**
   * Set the value of a given control. This will do the right thing depending
   * on the type of the control.
   */
  void SetValue (const char* name, const char* value);

  /// Set the value of the given text control.
  void SetText (const char* name, const char* value);
  /// Set the value of the given choice.
  void SetChoice (const char* name, const char* value);
  /// Set the vlaue of the given list.
  void SetList (const char* name, const char* value);

  /**
   * Show the dialog in a modal manner.
   * Returns 1 if Ok was pressed and 0 otherwise.
   */
  int Show (UIDialogCallback* cb);

  /**
   * When any button is pressed (including Ok and Cancel) this will return
   * the contents of all text controls and choices.
   */
  const DialogResult& GetFieldContents () const { return fieldContents; }
};


class UIManager
{
private:
  AppAresEditWX* app;
  wxWindow* parent;

  FileReq* filereqDialog;
  NewProjectDialog* newprojectDialog;
  CellDialog* cellDialog;
  csRef<DynfactDialog> dynfactDialog;

  int contextMenuID;

public:
  UIManager (AppAresEditWX* app, wxWindow* parent);
  ~UIManager ();

  AppAresEditWX* GetApp () const { return app; }

  void Message (const char* description, ...);
  void Error (const char* description, ...);
  bool Ask (const char* description, ...);
  csString AskDialog (const char* description, const char* label);

  FileReq* GetFileReqDialog () const { return filereqDialog; }
  NewProjectDialog* GetNewProjectDialog () const { return newprojectDialog; }
  CellDialog* GetCellDialog () const { return cellDialog; }
  DynfactDialog* GetDynfactDialog () const { return dynfactDialog; }

  /// Create a dynamically buildable dialog.
  UIDialog* CreateDialog (const char* title);

  int AllocContextMenuID () { contextMenuID++; return contextMenuID; }
};

#endif // __uimanager_h

