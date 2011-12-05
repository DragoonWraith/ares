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
#include "rowmodel.h"
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
  if (forcedDialog && ownForcedDialog)
    delete forcedDialog;
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

void TreeCtrlView::Update ()
{
  model->StartUpdate ();
#if 0
  for (int r = 0 ; r < tree->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (tree, r);
    if (!model->UpdateRow (row))
    {
      Refresh ();
      break;
    }
  }
#endif
  model->FinishUpdate ();
}

csStringArray TreeCtrlView::DialogEditRow (const csStringArray& origRow)
{
  UIDialog* dialog = forcedDialog ? forcedDialog : model->GetEditorDialog ();
  dialog->Clear ();
  if (origRow.GetSize () >= columns.GetSize ())
    for (size_t i = 0 ; i < columns.GetSize () ; i++)
      dialog->SetText (columns[i], origRow[i]);
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

void TreeCtrlView::OnAdd (wxCommandEvent& event)
{
  csStringArray empty;
  csStringArray row = DoDialog (empty);
  if (row.GetSize () > 0)
  {
    //ListCtrlTools::AddRow (tree, row);
    Update ();
  }
}

void TreeCtrlView::OnEdit (wxCommandEvent& event)
{
#if 0
  long idx = ListCtrlTools::GetFirstSelectedRow (tree);
  if (idx == -1) return;
  csStringArray oldRow = ListCtrlTools::ReadRow (tree, idx);
  csStringArray row = DoDialog (oldRow);
  if (row.GetSize () > 0)
  {
    ListCtrlTools::ReplaceRow (tree, idx, row);
    Update ();
  }
#endif
}

void TreeCtrlView::OnDelete (wxCommandEvent& event)
{
#if 0
  long idx = ListCtrlTools::GetFirstSelectedRow (tree);
  if (idx == -1) return;
  tree->DeleteItem (idx);
  for (size_t i = 0 ; i < columns.GetSize () ; i++)
    tree->SetColumnWidth (i, wxLIST_AUTOSIZE_USEHEADER);
  Update ();
#endif
}

void TreeCtrlView::OnContextMenu (wxContextMenuEvent& event)
{
#if 0
  bool hasItem;
  if (ListCtrlTools::CheckHitList (tree, hasItem, event.GetPosition ()))
  {
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
#endif
}

//-----------------------------------------------------------------------------

