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

#include <crystalspace.h>

#include "treeview.h"
#include "../models/rowmodel.h"
#include "uimanager.h"

//-----------------------------------------------------------------------------

enum
{
  ID_Add = wxID_HIGHEST + 10000,
  ID_Edit,
  ID_Delete,
};

void TreeCtrlView::UnbindModel ()
{
  if (!model) return;
  tree->Disconnect (wxEVT_CONTEXT_MENU, wxContextMenuEventHandler (TreeCtrlView :: OnContextMenu),
      0, static_cast<wxEvtHandler*> (this));
  tree->Disconnect (ID_Add, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (TreeCtrlView :: OnAdd), 0, this);
  tree->Disconnect (ID_Delete, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (TreeCtrlView :: OnDelete), 0, this);
  tree->Disconnect (ID_Edit, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (TreeCtrlView :: OnEdit), 0, this);
  model = 0;
}

void TreeCtrlView::BindModel (RowModel* model)
{
  if (model == TreeCtrlView::model) return;
  UnbindModel ();
  TreeCtrlView::model = model;

  tree->Connect (wxEVT_CONTEXT_MENU, wxContextMenuEventHandler (TreeCtrlView :: OnContextMenu), 0, this);
  tree->Connect (ID_Add, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (TreeCtrlView :: OnAdd), 0, this);
  tree->Connect (ID_Delete, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (TreeCtrlView :: OnDelete), 0, this);
  tree->Connect (ID_Edit, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (TreeCtrlView :: OnEdit), 0, this);

  const char* columnsString = model->GetColumns ();
  columns.DeleteAll ();
  columns.SplitString (columnsString, ",");
}

TreeCtrlView::~TreeCtrlView ()
{
  UnbindModel ();
  SetEditorModel (0);
  if (forcedDialog && ownForcedDialog)
    delete forcedDialog;
}

void TreeCtrlView::UpdateEditor ()
{
  if (!editorModel) return;
  editorModel->Update (GetSelectedRow ());
}

void TreeCtrlView::OnSelectionChange (wxTreeEvent& event)
{
  UpdateEditor ();
}

void TreeCtrlView::SetEditorModel (EditorModel* model)
{
  if (editorModel)
  {
    tree->Disconnect (wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler (
	  TreeCtrlView :: OnSelectionChange), 0, this);
  }
  editorModel = model;
  if (editorModel)
  {
    tree->Connect (wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler (
	  TreeCtrlView :: OnSelectionChange), 0, this);
  }
}

struct TreeNode
{
  csString name;
  csHash<TreeNode*,csString> children;
  ~TreeNode ()
  {
    csHash<TreeNode*,csString>::GlobalIterator it = children.GetIterator ();
    while (it.HasNext ())
    {
      TreeNode* node = it.Next ();
      delete node;
    }
  }
  void BuildWxTree (wxTreeCtrl* tree, wxTreeItemId& parent)
  {
    csHash<TreeNode*,csString>::GlobalIterator it = children.GetIterator ();
    while (it.HasNext ())
    {
      TreeNode* node = it.Next ();
      wxTreeItemId itemId = tree->AppendItem (parent, wxString::FromUTF8 (node->name));
      node->BuildWxTree (tree, itemId);
    }
  }
};

void TreeCtrlView::Refresh ()
{
  tree->DeleteAllItems ();
  wxTreeItemId rootId = tree->AddRoot (wxString::FromUTF8 (rootName));
  TreeNode* root = new TreeNode ();
  root->name = rootName;

  model->ResetIterator ();
  while (model->HasRows ())
  {
    csStringArray row = model->NextRow ();
    TreeNode* c = root;
    for (size_t i = 0 ; i < row.GetSize () ; i++)
    {
      if (c->children.Contains (row[i]))
      {
	c = c->children.Get (row[i], 0);
      }
      else
      {
	TreeNode* newNode = new TreeNode ();
	newNode->name = row[i];
	c->children.Put (row[i], newNode);
	c = newNode;
      }
    }
  }
  root->BuildWxTree (tree, rootId);
  delete root;
}

csStringArray TreeCtrlView::DialogEditRow (const csStringArray& origRow)
{
  UIDialog* dialog = forcedDialog ? forcedDialog : model->GetEditorDialog ();
  dialog->Clear ();
  if (origRow.GetSize () >= columns.GetSize ())
    for (size_t i = 0 ; i < columns.GetSize () ; i++)
      dialog->SetValue (columns[i], origRow[i]);
  csStringArray ar;
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    for (size_t i = 0 ; i < columns.GetSize () ; i++)
      ar.Push (fields.Get (columns[i], ""));
  }
  return ar;
}

csStringArray TreeCtrlView::DoDialog (const csStringArray& origRow)
{
  if (forcedDialog || model->GetEditorDialog ())
    return DialogEditRow (origRow);
  return model->EditRow (origRow);
}

void TreeCtrlView::SetEditorDialog (UIDialog* dialog, bool own)
{
  if (forcedDialog && ownForcedDialog && forcedDialog != dialog) delete forcedDialog;
  forcedDialog = dialog;
  ownForcedDialog = own;
}

/**
 * Add a row to a tree. Does nothing if it already exists.
 */
static void AddToTree (wxTreeCtrl* tree, wxTreeItemId& parent,
    const csStringArray& row, size_t rowIdx)
{
  if (rowIdx >= row.GetSize ()) return;
  wxTreeItemIdValue cookie;
  wxTreeItemId child = tree->GetFirstChild (parent, cookie);
  while (child.IsOk ())
  {
    csString name = (const char*)tree->GetItemText (child).mb_str (wxConvUTF8);
    if (name == row[rowIdx])
    {
      AddToTree (tree, child, row, rowIdx+1);
      return;
    }
    child = tree->GetNextChild (parent, cookie);
  }
  child = tree->AppendItem (parent, wxString::FromUTF8 (row[rowIdx]));
  AddToTree (tree, child, row, rowIdx+1);
}

void TreeCtrlView::OnAdd (wxCommandEvent& event)
{
  csStringArray empty;
  csStringArray row = DoDialog (empty);
  if (row.GetSize () > 0)
  {
    wxTreeItemId rootId = tree->GetRootItem ();
    if (model->AddRow (row))
    {
      AddToTree (tree, rootId, row, 0);
      model->FinishUpdate ();
    }
  }
}

/**
 * Construct a row from a given tree item.
 */
static void ConstructRowFromTree (wxTreeCtrl* tree, wxTreeItemId& item,
    csStringArray& row)
{
  wxTreeItemId parent = tree->GetItemParent (item);
  if (parent.IsOk ())
  {
    ConstructRowFromTree (tree, parent, row);
  }
  csString name = (const char*)tree->GetItemText (item).mb_str (wxConvUTF8);
  row.Push (name);
}

void TreeCtrlView::OnEdit (wxCommandEvent& event)
{
  wxTreeItemId sel = tree->GetSelection ();
  if (!sel.IsOk ()) return;

  csStringArray oldRow;
  ConstructRowFromTree (tree, sel, oldRow);

  csStringArray row = DoDialog (oldRow);
  if (row.GetSize () > 0)
  {
    if (model->UpdateRow (oldRow, row))
    {
      wxTreeItemId rootId = tree->GetRootItem ();
      AddToTree (tree, rootId, row, 0);
      model->FinishUpdate ();
    }
  }
}

void TreeCtrlView::OnDelete (wxCommandEvent& event)
{
  wxTreeItemId sel = tree->GetSelection ();
  if (!sel.IsOk ()) return;

  csStringArray oldRow;
  ConstructRowFromTree (tree, sel, oldRow);

  if (model->DeleteRow (oldRow))
  {
    tree->Delete (sel);
    model->FinishUpdate ();
  }
}

void TreeCtrlView::OnContextMenu (wxContextMenuEvent& event)
{
  if (!tree->IsShownOnScreen ()) return;
  int flags = 0;
  long idx = tree->HitTest (tree->ScreenToClient (event.GetPosition ()), flags);
  bool hasItem;
  if (idx == wxNOT_FOUND)
  {
    if (!tree->GetScreenRect ().Contains (event.GetPosition ()))
      return;
    hasItem = false;
  }
  else
  {
    hasItem = true;
  }
  wxMenu contextMenu;
  contextMenu.Append(ID_Add, wxT ("&Add"));
  if (hasItem)
  {
    if (model->IsEditAllowed ())
      contextMenu.Append(ID_Edit, wxT ("&Edit"));
    contextMenu.Append(ID_Delete, wxT ("&Delete"));
  }
  tree->PopupMenu (&contextMenu);
}

csStringArray TreeCtrlView::GetSelectedRow ()
{
  wxTreeItemId sel = tree->GetSelection ();
  if (!sel.IsOk ()) return csStringArray ();

  csStringArray row;
  ConstructRowFromTree (tree, sel, row);
  return row;
}

//-----------------------------------------------------------------------------

