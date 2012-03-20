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

#ifndef __appares_treeview_h
#define __appares_treeview_h

#include <csutil/stringarray.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

#include "edcommon/rowmodel.h"

struct iUIDialog;


/**
 * A view based on a tree control on top of a RowModel.
 */
class ARES_EXPORT TreeCtrlView : public wxEvtHandler
{
private:
  wxTreeCtrl* tree;
  csRef<RowModel> model;
  csRef<EditorModel> editorModel;
  csStringArray columns;
  csRef<iUIDialog> forcedDialog;
  csString rootName;

  void UnbindModel ();
  void BindModel (RowModel* model);

  void UpdateEditor ();
  void OnSelectionChange (wxTreeEvent& event);

  void OnContextMenu (wxContextMenuEvent& event);
  void OnAdd (wxCommandEvent& event);
  void OnEdit (wxCommandEvent& event);
  void OnDelete (wxCommandEvent& event);

  /// This is used in case the model has a dialog to use.
  csStringArray DialogEditRow (const csStringArray& origRow);

  /// Get the right dialog.
  csStringArray DoDialog (const csStringArray& origRow);

public:
  TreeCtrlView (wxTreeCtrl* tree) : tree (tree)  { }
  TreeCtrlView (wxTreeCtrl* tree, RowModel* model) : tree (tree)
  {
    BindModel (model);
  }
  ~TreeCtrlView ();

  /**
   * Set the row model to use for this view. Automatically refresh
   * the tree from this model. A view needs a model to operate. This is
   * not optional.
   */
  void SetModel (RowModel* model) { BindModel (model); Refresh (); }

  /**
   * Set the editor model to use for this view. This is optional.
   * If an editor model is present then selecting rows in the tree
   * will automatically update this editor.
   */
  void SetEditorModel (EditorModel* model);

  /**
   * Set the name of the root of the tree.
   */
  void SetRootName (const char* rootName) { TreeCtrlView::rootName = rootName; }

  /**
   * Manually force an editor dialog to use.
   */
  void SetEditorDialog (iUIDialog* dialog);

  /**
   * Refresh the tree from the data in the model.
   */
  void Refresh ();

  /**
   * Get the current selected row (or empty row in case nothing is
   * selected).
   */
  csStringArray GetSelectedRow ();
};

#endif // __appares_treeview_h

