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

#include "model.h"
#include "../tools/tools.h"
#include "../ui/uitools.h"
#include "../ui/listctrltools.h"
#include "../ui/customcontrol.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/listbox.h>
#include <wx/choicebk.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

namespace Ares
{

// --------------------------------------------------------------------------

csString Value::Dump (bool verbose)
{
  csString dump;
  switch (GetType ())
  {
    case VALUE_STRING:
      dump.Format ("V(string,'%s'%s)", GetStringValue (), parent ? ",[PAR]": "");
      break;
    case VALUE_LONG:
      dump.Format ("V(long,'%ld'%s)", GetLongValue (), parent ? ",[PAR]": "");
      break;
    case VALUE_BOOL:
      dump.Format ("V(long,'%d'%s)", GetBoolValue (), parent ? ",[PAR]": "");
      break;
    case VALUE_FLOAT:
      dump.Format ("V(float,'%g'%s)", GetFloatValue (), parent ? ",[PAR]": "");
      break;
    case VALUE_COLLECTION:
      dump.Format ("V(collection%s)", parent ? ",[PAR]": "");
      break;
    case VALUE_COMPOSITE:
      dump.Format ("V(composite%s)", parent ? ",[PAR]": "");
      break;
    case VALUE_NONE:
      dump.Format ("V(none%s)", parent ? ",[PAR]": "");
      break;
    default:
      dump.Format ("V(?%s)", parent ? ",[PAR]": "");
      break;
  }
  if (verbose)
  {
    if (GetType () == VALUE_COLLECTION || GetType () == VALUE_COMPOSITE)
    {
      ResetIterator ();
      while (HasNext ())
      {
	csString name;
	Value* val = NextChild (&name);
	dump.AppendFmt ("\n    %s", (const char*)val->Dump (false));
      }
    }
  }
  return dump;
} 

// --------------------------------------------------------------------------

BufferedValue::BufferedValue (Value* originalValue) : originalValue (originalValue)
{
  changeListener.AttachNew (new ChangeListener (this));
  originalValue->AddValueChangeListener (changeListener);
}

BufferedValue::~BufferedValue ()
{
}

csRef<BufferedValue> BufferedValue::CreateBufferedValue (Value* originalValue)
{
  csRef<BufferedValue> bufferedValue;
  switch (originalValue->GetType ())
  {
    case VALUE_STRING:
      bufferedValue.AttachNew (new StringBufferedValue (originalValue));
      return bufferedValue;
    case VALUE_COMPOSITE:
      bufferedValue.AttachNew (new CompositeBufferedValue (originalValue));
      return bufferedValue;
    case VALUE_COLLECTION:
      bufferedValue.AttachNew (new CollectionBufferedValue (originalValue));
      return bufferedValue;
    default:
      printf ("Could not create buffered value for this type!\n");
      return 0;
  }
}

void CompositeBufferedValue::ValueChanged ()
{
#if DO_DEBUG
  printf ("ValueChanged: %s\n", Dump ().GetData ());
#endif
  buffer.DeleteAll ();
  names.DeleteAll ();
  originalValue->ResetIterator ();
  while (originalValue->HasNext ())
  {
    csString name;
    Value* value = originalValue->NextChild (&name);
    buffer.Push (BufferedValue::CreateBufferedValue (value));
    names.Push (name);
  }
  dirty = false;
}

void CompositeBufferedValue::Apply ()
{
  if (!dirty) return;
  dirty = false;
  ResetIterator ();
  while (HasNext ())
  {
    BufferedValue* bufferedValue = static_cast<BufferedValue*> (NextChild ());
    bufferedValue->Apply ();
  }
}

Value* CompositeBufferedValue::GetChild (const char* name)
{
  csString sname = name;
  for (size_t i = 0 ; i < buffer.GetSize () ; i++)
  {
    if (sname == names[i]) return buffer[i];
  }
  return 0;
}

void CollectionBufferedValue::ValueChanged ()
{
#if DO_DEBUG
  printf ("ValueChanged: %s\n", Dump ().GetData ());
#endif
  buffer.DeleteAll ();
  originalToBuffered.DeleteAll ();
  bufferedToOriginal.DeleteAll ();
  newvalues.DeleteAll ();
  deletedvalues.DeleteAll ();

  originalValue->ResetIterator ();
  while (originalValue->HasNext ())
  {
    Value* value = originalValue->NextChild ();
    csRef<BufferedValue> bufferedValue = BufferedValue::CreateBufferedValue (value);
    originalToBuffered.Put (value, (BufferedValue*)bufferedValue);
    bufferedToOriginal.Put ((BufferedValue*)bufferedValue, value);
    buffer.Push (bufferedValue);
  }
  dirty = false;
}

void CollectionBufferedValue::Apply ()
{
  if (!dirty) return;
  dirty = false;
  ResetIterator ();
  while (HasNext ())
  {
    BufferedValue* bufferedValue = static_cast<BufferedValue*> (NextChild ());
    bufferedValue->Apply ();
  }
  for (size_t i = 0 ; i < newvalues.GetSize () ; i++)
    originalValue->AddValue (newvalues[i]);
  newvalues.DeleteAll ();
  for (size_t i = 0 ; i < deletedvalues.GetSize () ; i++)
    originalValue->DeleteValue (deletedvalues[i]);
  deletedvalues.DeleteAll ();
}

bool CollectionBufferedValue::DeleteValue (Value* child)
{
  if (newvalues.Find (child))
    newvalues.Delete (child);
  else
    deletedvalues.Push (child);
  dirty = newvalues.GetSize () > 0 || deletedvalues.GetSize () > 0;
  BufferedValue* bufferedValue = originalToBuffered.Get (child, 0);
  if (bufferedValue)
    buffer.Delete (bufferedValue);

  return true;
}

bool CollectionBufferedValue::AddValue (Value* child)
{
  csRef<BufferedValue> bufferedValue;
  if (deletedvalues.Find (child))
    deletedvalues.Delete (child);
  else
  {
    newvalues.Push (child);
    bufferedValue = BufferedValue::CreateBufferedValue (child);
    originalToBuffered.Put (child, (BufferedValue*)bufferedValue);
  }
  dirty = newvalues.GetSize () > 0 || deletedvalues.GetSize () > 0;
  bufferedValue = originalToBuffered.Get (child, 0);
  if (bufferedValue)
    buffer.Push (bufferedValue);
  return true;
}

// --------------------------------------------------------------------------

MirrorValue::MirrorValue (ValueType type) : type (type), idx (0)
{
  changeListener.AttachNew (new ChangeListener (this));
  mirroringValue = &nullValue;
}

MirrorValue::~MirrorValue ()
{
  SetMirrorValue (0);
  DeleteAll ();
}

void MirrorValue::SetupComposite (Value* compositeValue)
{
  CS_ASSERT (compositeValue->GetType () == VALUE_COMPOSITE);
  DeleteAll ();
  compositeValue->ResetIterator ();
  while (compositeValue->HasNext ())
  {
    csString name;
    Value* child = compositeValue->NextChild (&name);
    csRef<MirrorValue> mv;
    mv.AttachNew (new MirrorValue (child->GetType ()));
    AddChild (name, mv);
  }
}

void MirrorValue::SetMirrorValue (Value* value)
{
  if (mirroringValue == value) return;

  if (mirroringValue && mirroringValue != &nullValue)
  {
    mirroringValue->RemoveValueChangeListener (changeListener);
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      children[i]->SetMirrorValue (0);
  }
  mirroringValue = value;
  if (mirroringValue)
  {
    mirroringValue->AddValueChangeListener (changeListener);
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      children[i]->SetMirrorValue (value->GetChild (i));
  }
  else
    mirroringValue = &nullValue;
}

void MirrorValue::ValueChanged ()
{
#if DO_DEBUG
  printf ("ValueChanged: %s\n", Dump ().GetData ());
#endif
  FireValueChanged ();
}

// --------------------------------------------------------------------------

ListSelectedValue::ListSelectedValue (wxListCtrl* listCtrl, Value* collectionValue, ValueType type) :
  MirrorValue (type), listCtrl (listCtrl), collectionValue (collectionValue)
{
  listCtrl->Connect (wxEVT_COMMAND_LIST_ITEM_SELECTED,
	  wxCommandEventHandler (ListSelectedValue :: OnSelectionChange), 0, this);
  listCtrl->Connect (wxEVT_COMMAND_LIST_ITEM_DESELECTED,
	  wxCommandEventHandler (ListSelectedValue :: OnSelectionChange), 0, this);
  selection = ListCtrlTools::GetFirstSelectedRow (listCtrl);
  UpdateToSelection ();
}

ListSelectedValue::~ListSelectedValue ()
{
  listCtrl->Disconnect (wxEVT_COMMAND_LIST_ITEM_SELECTED,
	  wxCommandEventHandler (ListSelectedValue :: OnSelectionChange), 0, this);
  listCtrl->Disconnect (wxEVT_COMMAND_LIST_ITEM_DESELECTED,
	  wxCommandEventHandler (ListSelectedValue :: OnSelectionChange), 0, this);
}

void ListSelectedValue::UpdateToSelection ()
{
  Value* value = 0;
  if (selection != -1)
    value = collectionValue->GetChild (size_t (selection));
  if (value != GetMirrorValue ())
  {
    SetMirrorValue (value);
    FireValueChanged ();
  }
}

void ListSelectedValue::OnSelectionChange (wxCommandEvent& event)
{
  long idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
  selection = idx;
#if DO_DEBUG
  printf ("ListSelectedValue::OnSelectionChange: %s\n", Dump ().GetData ());
#endif
  UpdateToSelection ();
}

// --------------------------------------------------------------------------

TreeSelectedValue::TreeSelectedValue (wxTreeCtrl* treeCtrl, Value* collectionValue, ValueType type) :
  MirrorValue (type), treeCtrl (treeCtrl), collectionValue (collectionValue)
{
  treeCtrl->Connect (wxEVT_COMMAND_TREE_SEL_CHANGED,
	  wxCommandEventHandler (TreeSelectedValue :: OnSelectionChange), 0, this);
  selection = treeCtrl->GetSelection ();
  UpdateToSelection ();
}

TreeSelectedValue::~TreeSelectedValue ()
{
  treeCtrl->Disconnect (wxEVT_COMMAND_TREE_SEL_CHANGED,
	  wxCommandEventHandler (TreeSelectedValue :: OnSelectionChange), 0, this);
}

/**
 * Get a value corresponding with a given tree item.
 */
static Value* ValueFromTree (wxTreeCtrl* tree, wxTreeItemId item, Value* collectionValue)
{
  wxTreeItemId parent = tree->GetItemParent (item);
  if (parent.IsOk ())
  {
    Value* value = ValueFromTree (tree, parent, collectionValue);
    if (!value) return 0;	// Can this happen?
    csString name = (const char*)tree->GetItemText (item).mb_str (wxConvUTF8);
    value->ResetIterator ();
    while (value->HasNext ())
    {
      Value* child = value->NextChild ();
      if (name == child->GetStringValue ())
	return child;
    }
    return 0;	// Can this happen?
  }
  else
  {
    return collectionValue;
  }
}

void TreeSelectedValue::UpdateToSelection ()
{
  Value* value = 0;
  if (selection.IsOk ())
    value = ValueFromTree (treeCtrl, selection, collectionValue);
  if (value != GetMirrorValue ())
  {
    SetMirrorValue (value);
    FireValueChanged ();
  }
}

void TreeSelectedValue::OnSelectionChange (wxCommandEvent& event)
{
  selection = treeCtrl->GetSelection ();
#if DO_DEBUG
  printf ("TreeSelectedValue::OnSelectionChange: %s\n", Dump ().GetData ());
#endif
  UpdateToSelection ();
}

// --------------------------------------------------------------------------

bool NewChildAction::Do (View* view, wxWindow* component)
{
  size_t idx = csArrayItemNotFound;
  wxListCtrl* listCtrl = 0;
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    listCtrl = wxStaticCast (component, wxListCtrl);
    idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
  }
  Value* value = collection->NewValue (idx);
  if (!value) return false;

  if (listCtrl)
  {
    collection->ResetIterator ();
    bool found = false;
    idx = 0;
    while (collection->HasNext ())
    {
      Value* child = collection->NextChild ();
      if (child == value) { found = true; break; }
      idx++;
    }
    if (found)
      ListCtrlTools::SelectRow (listCtrl, idx, true);
  }
  return true;
}

bool DeleteChildAction::Do (View* view, wxWindow* component)
{
  Value* value = view->GetSelectedValue (component);
  if (!value) return false;	// Nothing to do.
  return collection->DeleteValue (value);
}

// --------------------------------------------------------------------------

View::View (wxWindow* parent) : parent (parent), lastContextID (wxID_HIGHEST + 10000), eventHandler (this)
{
  changeListener.AttachNew (new ChangeListener (this));
}

void View::DestroyBindings ()
{
  ComponentToBinding::GlobalIterator it = bindingsByComponent.GetIterator ();
  while (it.HasNext ())
  {
    csPtrKey<wxWindow> component;
    Binding* binding = it.Next (component);
    if (binding->eventType == wxEVT_COMMAND_TEXT_UPDATED)
      component->Disconnect (binding->eventType,
	  wxCommandEventHandler (EventHandler :: OnComponentChanged), 0, &eventHandler);
    else if (binding->eventType == wxEVT_COMMAND_LIST_ITEM_SELECTED)
      component->Disconnect (binding->eventType,
	  wxCommandEventHandler (EventHandler :: OnComponentChanged), 0, &eventHandler);
  }
}

void View::DestroyActionBindings ()
{
  while (listContexts.GetSize () > 0)
  {
    ListContext lc = listContexts.Pop ();
    lc.listCtrl->Disconnect (wxEVT_CONTEXT_MENU, wxContextMenuEventHandler (EventHandler :: OnRMB), 0, &eventHandler);
    while (lc.actionDefs.GetSize () > 0)
    {
      ActionDef ad = lc.actionDefs.Pop ();
      lc.listCtrl->Disconnect (ad.id, wxEVT_COMMAND_MENU_SELECTED,
	    wxCommandEventHandler (EventHandler :: OnActionExecuted), 0, &eventHandler);
    }
  }
}

View::~View ()
{
  DestroyBindings ();
  DestroyActionBindings ();
}

wxWindow* View::FindComponentByName (wxWindow* container, const char* name)
{
  wxWindowList list = container->GetChildren ();
  wxWindowList::iterator iter;
  for (iter = list.begin (); iter != list.end () ; ++iter)
  {
    wxWindow* child = *iter;
    csString childName = (const char*)child->GetName ().mb_str (wxConvUTF8);
    if (childName == name) return child;
    size_t i = childName.FindFirst ('_');
    if (i != (size_t)-1)
    {
      childName = childName.Slice (0, i);
      if (childName == name) return child;
    }
    wxWindow* found = FindComponentByName (child, name);
    if (found) return found;
  }
  return 0;
}

bool View::Bind (Value* value, wxWindow* component)
{
  if (component->IsKindOf (CLASSINFO (wxTextCtrl)))
    return Bind (value, wxStaticCast (component, wxTextCtrl));
  if (component->IsKindOf (CLASSINFO (wxPanel)))
    return Bind (value, wxStaticCast (component, wxPanel));
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
    return Bind (value, wxStaticCast (component, wxListCtrl));
  if (component->IsKindOf (CLASSINFO (wxTreeCtrl)))
    return Bind (value, wxStaticCast (component, wxTreeCtrl));
  if (component->IsKindOf (CLASSINFO (wxChoicebook)))
    return Bind (value, wxStaticCast (component, wxChoicebook));
  if (component->IsKindOf (CLASSINFO (CustomControl)))
    return Bind (value, wxStaticCast (component, CustomControl));
  printf ("Bind: Unsupported type for component!\n");
  return false;
}

bool View::Bind (Value* value, const char* compName)
{
  wxString wxcompName = wxString::FromUTF8 (compName);
  wxWindow* comp = parent->FindWindow (wxcompName);
  if (!comp)
  {
    printf ("Bind: Can't find component '%s'!\n", compName);
    return false;
  }
  return Bind (value, comp);
}

void View::RegisterBinding (Value* value, wxWindow* component, wxEventType eventType)
{
  Binding* b = new Binding ();
  b->value = value;
  b->component = component;
  b->eventType = eventType;
  bindingsByComponent.Put (component, b);
  if (!bindingsByValue.Contains (value))
    bindingsByValue.Put (value, csArray<Binding*> ());
  csArray<Binding*> temp;
  bindingsByValue.Get (value, temp).Push (b);
  CS_ASSERT (temp.GetSize () == 0);
  if (eventType != wxEVT_NULL)
    component->Connect (eventType, wxCommandEventHandler (EventHandler :: OnComponentChanged), 0, &eventHandler);
  value->AddValueChangeListener (changeListener);
}

bool View::Bind (Value* value, wxTextCtrl* component)
{
  switch (value->GetType ())
  {
    case VALUE_STRING:
    case VALUE_LONG:
    case VALUE_BOOL:
    case VALUE_FLOAT:
    case VALUE_NONE:	// Supported too in case the type is as of yet unknown.
      break;
    default:
      printf ("Unsupported value type for text control!\n");
      return false;
  }

  RegisterBinding (value, component, wxEVT_COMMAND_TEXT_UPDATED);
  ValueChanged (value);
  return true;
}

bool View::Bind (Value* value, CustomControl* component)
{
  RegisterBinding (value, component, wxEVT_NULL);
  ValueChanged (value);
  return true;
}

bool View::Bind (Value* value, wxChoicebook* component)
{
  switch (value->GetType ())
  {
    case VALUE_STRING:
    case VALUE_LONG:
    case VALUE_NONE:	// Supported too in case the type is as of yet unknown.
      break;
    default:
      printf ("Unsupported value type for text control!\n");
      return false;
  }

  RegisterBinding (value, component, wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGED);
  ValueChanged (value);
  return true;
}

bool View::Bind (Value* value, wxPanel* component)
{
  // We also support VALUE_NONE here because that can be useful in situations where we don't
  // know the type yet (for example when the value is a ListSelectedValue and nothing has been
  // selected).
  if (value->GetType () != VALUE_COMPOSITE && value->GetType () != VALUE_NONE)
  {
    printf ("Unsupported value type for panels! Only VALUE_COMPOSITE is supported.\n");
    return false;
  }

  RegisterBinding (value, component, wxEVT_NULL);

  value->ResetIterator ();
  while (value->HasNext ())
  {
    csString name;
    Value* child = value->NextChild (&name);
    wxWindow* childComp = FindComponentByName (component, name);
    if (!childComp)
    {
      printf ("Warning: no component found for child '%s'!\n", name.GetData ());
    }
    else
    {
      if (!Bind (child, childComp))
	return false;
    }
  }
  ValueChanged (value);
  return true;
}

bool View::Bind (Value* value, wxListCtrl* component)
{
  // We also support VALUE_NONE here because that can be useful in situations where we don't
  // know the type yet (for example when the value is a ListSelectedValue and nothing has been
  // selected).
  if (value->GetType () != VALUE_COLLECTION && value->GetType () != VALUE_NONE)
  {
    printf ("Unsupported value type for lists! Only VALUE_COLLECTION is supported.\n");
    return false;
  }

  RegisterBinding (value, component, wxEVT_NULL);
  ValueChanged (value);
  return true;
}

bool View::Bind (Value* value, wxTreeCtrl* component)
{
  // We also support VALUE_NONE here because that can be useful in situations where we don't
  // know the type yet (for example when the value is a ListSelectedValue and nothing has been
  // selected).
  if (value->GetType () != VALUE_COLLECTION && value->GetType () != VALUE_NONE)
  {
    printf ("Unsupported value type for trees! Only VALUE_COLLECTION is supported.\n");
    return false;
  }

  RegisterBinding (value, component, wxEVT_NULL);
  ValueChanged (value);
  return true;
}

static csString ValueToString (Value* value)
{
  csString val;
  switch (value->GetType ())
  {
    case VALUE_STRING: return csString (value->GetStringValue ());
    case VALUE_LONG: val.Format ("%ld", value->GetLongValue ()); return val;
    case VALUE_BOOL: return csString (value->GetBoolValue () ? "true" : "false");
    case VALUE_FLOAT: val.Format ("%g", value->GetFloatValue ()); return val;
    case VALUE_COLLECTION: return csString ("<collection>");
    case VALUE_COMPOSITE: return csString ("<composite>");
    default: return csString ("<?>");
  }
}

static void LongToValue (long l, Value* value)
{
  csString str;
  switch (value->GetType ())
  {
    case VALUE_STRING:
      str.Format ("%ld", l);
      value->SetStringValue (str);
      return;
    case VALUE_LONG:
      value->SetLongValue (l);
      return;
    case VALUE_BOOL:
      value->SetBoolValue (bool (l));
      return;
    case VALUE_FLOAT:
      value->SetFloatValue (float (l));
      return;
    case VALUE_COLLECTION: return;
    case VALUE_COMPOSITE: return;
    default: return;
  }
}

static void StringToValue (const char* str, Value* value)
{
  switch (value->GetType ())
  {
    case VALUE_STRING:
      value->SetStringValue (str);
      return;
    case VALUE_LONG:
    {
      long l;
      csScanStr (str, "%d", &l);
      value->SetLongValue (l);
      return;
    }
    case VALUE_BOOL:
    {
      bool l;
      csScanStr (str, "%b", &l);
      value->SetBoolValue (l);
      return;
    }
    case VALUE_FLOAT:
    {
      float l;
      csScanStr (str, "%f", &l);
      value->SetFloatValue (l);
      return;
    }
    case VALUE_COLLECTION: return;
    case VALUE_COMPOSITE: return;
    default: return;
  }
}

csStringArray View::ConstructListRow (const ListHeading& lh, Value* value)
{
  csStringArray row;
  if (value->GetType () != VALUE_COMPOSITE)
  {
    row.Push (ValueToString (value));
    return row;
  }
  for (size_t i = 0 ; i < lh.names.GetSize () ; i++)
  {
    Value* child = value->GetChild (lh.names[i]);
    if (child)
      row.Push (ValueToString (child));
    else
    {
      printf ("Warning: child '%s' is missing!\n", (const char*)lh.names[i]);
      row.Push ("");
    }
  }
  return row;
}

size_t View::FindListContext (wxListCtrl* listCtrl)
{
  for (size_t i = 0 ; i < listContexts.GetSize () ; i++)
    if (listContexts[i].listCtrl == listCtrl)
      return i;
  return csArrayItemNotFound;
}

void View::OnRMB (wxContextMenuEvent& event)
{
  wxWindow* component = wxStaticCast (event.GetEventObject (), wxWindow);
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    wxListCtrl* listCtrl = wxStaticCast (component, wxListCtrl);
    size_t idx = FindListContext (listCtrl);
    if (idx == csArrayItemNotFound) return;
    bool hasItem;
    if (ListCtrlTools::CheckHitList (listCtrl, hasItem, event.GetPosition ()))
    {
      const ListContext& lc = listContexts[idx];
      wxMenu contextMenu;
      for (size_t j = 0 ; j < lc.actionDefs.GetSize () ; j++)
	contextMenu.Append(lc.actionDefs[j].id, wxString::FromUTF8 (lc.actionDefs[j].action->GetName ()));
      listCtrl->PopupMenu (&contextMenu);
    }
  }
}

void View::OnActionExecuted (wxCommandEvent& event)
{
  int id = event.GetId ();
  // We have to scan all lists here to find the one that has the right id.
  for (size_t i = 0 ; i < listContexts.GetSize () ; i++)
  {
    const ListContext& lc = listContexts[i];
    for (size_t j = 0 ; j < lc.actionDefs.GetSize () ; j++)
      if (lc.actionDefs[j].id == id)
      {
	lc.actionDefs[j].action->Do (this, lc.listCtrl);
	return;
      }
  }
}

void View::OnComponentChanged (wxCommandEvent& event)
{
  wxWindow* component = wxStaticCast (event.GetEventObject (), wxWindow);
  Binding* binding = bindingsByComponent.Get (component, 0);
  if (!binding)
  {
    printf ("OnComponentChanged: Something went wrong! Called without a value!\n");
    return;
  }

  if (binding->processing) return;
  binding->processing = true;

#if DO_DEBUG
  printf ("View::OnComponentChanged: %s\n", binding->value->Dump ().GetData ());
#endif

  if (component->IsKindOf (CLASSINFO (wxTextCtrl)))
  {
    wxTextCtrl* textCtrl = wxStaticCast (component, wxTextCtrl);
    csString text = (const char*)textCtrl->GetValue ().mb_str (wxConvUTF8);
    StringToValue (text, binding->value);
  }
  else if (component->IsKindOf (CLASSINFO (wxChoicebook)))
  {
    wxChoicebook* choicebook = wxStaticCast (component, wxChoicebook);
    int pageSel = choicebook->GetSelection ();
    if (binding->value->GetType () == VALUE_LONG)
      LongToValue ((long)pageSel, binding->value);
    else
    {
      csString value;
      if (pageSel == wxNOT_FOUND) value = "";
      else
      {
        wxString pageTxt = choicebook->GetPageText (pageSel);
        value = (const char*)pageTxt.mb_str (wxConvUTF8);
      }
      StringToValue (value, binding->value);
    }
  }
  else
  {
    printf ("OnComponentChanged: this type of component not yet supported!\n");
  }
  binding->processing = false;
}

void View::BuildTree (wxTreeCtrl* treeCtrl, Value* value, wxTreeItemId& parent)
{
  value->ResetIterator ();
  while (value->HasNext ())
  {
    Value* child = value->NextChild ();
    wxTreeItemId itemId = treeCtrl->AppendItem (parent, wxString::FromUTF8 (child->GetStringValue ()));
    BuildTree (treeCtrl, child, itemId);
  }
}

void View::ValueChanged (Value* value)
{
#if DO_DEBUG
  printf ("View::ValueChanged: %s\n", value->Dump ().GetData ());
#endif
  csArray<Binding*> b;
  csArray<Binding*>& bindings = bindingsByValue.Get (value, b);
  if (!bindings.GetSize ())
  {
    printf ("ValueChanged: Something went wrong! Called without a value!\n");
    //CS_ASSERT (false);
    return;
  }
  for (size_t i = 0 ; i < bindings.GetSize () ; i++)
    if (!bindings[i]->processing)
    {
      wxWindow* comp = bindings[i]->component;
      if (comp->IsKindOf (CLASSINFO (wxTextCtrl)))
      {
	bindings[i]->processing = true;
	wxTextCtrl* textCtrl = wxStaticCast (comp, wxTextCtrl);
	csString text = ValueToString (value);
	textCtrl->SetValue (wxString::FromUTF8 (text));
	bindings[i]->processing = false;
      }
      else if (comp->IsKindOf (CLASSINFO (CustomControl)))
      {
	CustomControl* customCtrl = wxStaticCast (comp, CustomControl);
	customCtrl->SyncValue (value);
      }
      else if (comp->IsKindOf (CLASSINFO (wxPanel)))
      {
	// If the value of a composite changes we update the children.
	value->ResetIterator ();
	while (value->HasNext ())
	{
	  Value* child = value->NextChild ();
	  ValueChanged (child);
	}
      }
      else if (comp->IsKindOf (CLASSINFO (wxListCtrl)))
      {
	wxListCtrl* listCtrl = wxStaticCast (comp, wxListCtrl);
	long idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
	listCtrl->DeleteAllItems ();
	ListHeading lhdef;
	const ListHeading& lh = listToHeading.Get (listCtrl, lhdef);
	value->ResetIterator ();
	while (value->HasNext ())
	{
	  Value* child = value->NextChild ();
	  ListCtrlTools::AddRow (listCtrl, ConstructListRow (lh, child));
	}
	if (idx != -1)
	  ListCtrlTools::SelectRow (listCtrl, idx, true);
      }
      else if (comp->IsKindOf (CLASSINFO (wxTreeCtrl)))
      {
	wxTreeCtrl* treeCtrl = wxStaticCast (comp, wxTreeCtrl);
	treeCtrl->DeleteAllItems ();
	wxTreeItemId rootId = treeCtrl->AddRoot (wxString::FromUTF8 (value->GetStringValue ()));
	BuildTree (treeCtrl, value, rootId);
      }
      else if (comp->IsKindOf (CLASSINFO (wxChoicebook)))
      {
	wxChoicebook* choicebook = wxStaticCast (comp, wxChoicebook);
	if (value->GetType () == VALUE_LONG)
	  choicebook->ChangeSelection (value->GetLongValue ());
	else
	{
	  csString text = ValueToString (value);
	  wxString wxtext = wxString::FromUTF8 (text);
	  for (size_t i = 0 ; i < choicebook->GetPageCount () ; i++)
	  {
	    wxString wxp = choicebook->GetPageText (i);
	    if (wxp == wxtext)
	    {
	      choicebook->ChangeSelection (i);
	      return;
	    }
	  }
	  // If we come here we set to the first page.
	  choicebook->SetSelection (0);
	}
      }
      else
      {
	printf ("ValueChanged: this type of component not yet supported!\n");
      }
    }
}

bool View::DefineHeading (const char* listName, const char* heading,
      const char* names)
{
  wxString wxlistName = wxString::FromUTF8 (listName);
  wxWindow* comp = parent->FindWindow (wxlistName);
  if (!comp)
  {
    printf ("DefineHeading: Can't find component '%s'!\n", listName);
    return false;
  }
  if (!comp->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    printf ("DefineHeading: Component '%s' is not a list control!\n", listName);
    return false;
  }
  wxListCtrl* listCtrl = wxStaticCast (comp, wxListCtrl);
  return DefineHeading (listCtrl, heading, names);
}

bool View::DefineHeading (wxListCtrl* listCtrl, const char* heading,
      const char* names)
{
  ListHeading lh;
  lh.heading.SplitString (heading, ",");
  lh.names.SplitString (names, ",");
  for (size_t i = 0 ; i < lh.heading.GetSize () ; i++)
    ListCtrlTools::SetColumn (listCtrl, i, lh.heading[i], 100);
  listToHeading.Put (listCtrl, lh);
  return true;
}

bool View::AddAction (wxWindow* component, Action* action)
{
  if (component->IsKindOf (CLASSINFO (wxButton)))
    return AddAction (wxStaticCast (component, wxButton), action);
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
    return AddAction (wxStaticCast (component, wxListCtrl), action);
  if (component->IsKindOf (CLASSINFO (wxTreeCtrl)))
    return AddAction (wxStaticCast (component, wxTreeCtrl), action);
  printf ("AddAction: Unsupported type for component!\n");
  return false;
}

bool View::AddAction (wxListCtrl* listCtrl, Action* action)
{
  listCtrl->Connect (lastContextID, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EventHandler :: OnActionExecuted), 0, &eventHandler);

  size_t idx = FindListContext (listCtrl);
  if (idx == csArrayItemNotFound)
  {
    ListContext lc;
    lc.listCtrl = listCtrl;
    idx = listContexts.Push (lc);
    listCtrl->Connect (wxEVT_CONTEXT_MENU, wxContextMenuEventHandler (EventHandler :: OnRMB), 0, &eventHandler);
  }

  ListContext& lc = listContexts[idx];
  ActionDef ad;
  ad.id = lastContextID;
  ad.action = action;
  lc.actionDefs.Push (ad);
  lastContextID++;
  return true;
}

bool View::AddAction (wxTreeCtrl* treeCtrl, Action* action)
{
  printf ("AddAction: tree controls not implemented yet!\n");
  return true;
}

bool View::AddAction (wxButton* button, Action* action)
{
  printf ("AddAction: buttons not implemented yet!\n");
  return true;
}

Value* View::GetSelectedValue (wxWindow* component)
{
  Binding* binding = bindingsByComponent.Get (component, 0);
  if (!binding)
  {
    printf ("GetSelectedValue: Component is not bound to a value!\n");
    return 0;
  }
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    wxListCtrl* listCtrl = wxStaticCast (component, wxListCtrl);
    long idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
    if (idx < 0) return 0;
    return binding->value->GetChild (size_t (idx));
  }
  if (component->IsKindOf (CLASSINFO (wxTreeCtrl)))
  {
    wxTreeCtrl* treeCtrl = wxStaticCast (component, wxTreeCtrl);
    wxTreeItemId selection = treeCtrl->GetSelection ();
    if (selection.IsOk ())
      return ValueFromTree (treeCtrl, selection, binding->value);
    else
      return 0;
  }
  printf ("GetSelectedValue: Unsupported type for component!\n");
  return 0;
}

class SignalChangeListener : public ValueChangeListener
{
private:
  csRef<Value> dest;

public:
  SignalChangeListener (Value* dest) : dest (dest) { }
  virtual ~SignalChangeListener () { }
  virtual void ValueChanged (Value* value)
  {
    dest->FireValueChanged ();
  }
};

void View::Signal (Value* source, Value* dest)
{
  csRef<SignalChangeListener> listener;
  listener.AttachNew (new SignalChangeListener (dest));
  source->AddValueChangeListener (listener);
}

// --------------------------------------------------------------------------

} // namespace Ares

