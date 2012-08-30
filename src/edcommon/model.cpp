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

#include "edcommon/model.h"
#include "edcommon/tools.h"
#include "edcommon/uitools.h"
#include "edcommon/listctrltools.h"
#include "edcommon/customcontrol.h"
#include "editor/iuidialog.h"

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

Value* StandardValueIterator::NextChild (csString* name)
{
  if (name && !names.IsEmpty ()) *name = names[idx];
  idx++;
  return children[idx-1];
}

// --------------------------------------------------------------------------

csString Value::Dump (bool verbose)
{
  csString dump;
  switch (GetType ())
  {
    case VALUE_STRING:
      dump.Format ("V(string,'%s'%s)", GetStringValue (), parent ? ",[PAR]": "");
      break;
    case VALUE_STRINGARRAY:
      dump.Format ("V(string[],''%s)", parent ? ",[PAR]": "");
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
      csRef<ValueIterator> it = GetIterator ();
      while (it->HasNext ())
      {
	csString name;
	Value* val = it->NextChild (&name);
	dump.AppendFmt ("\n    %s", (const char*)val->Dump (false));
      }
    }
  }
  return dump;
} 

bool Value::IsChild (Value* value)
{
  csRef<ValueIterator> it = GetIterator ();
  while (it->HasNext ())
  {
    if (value == it->NextChild ()) return true;
  }
  return false;
}

// --------------------------------------------------------------------------

DialogResult AbstractCompositeValue::GetDialogValue ()
{
  DialogResult result;
  csRef<ValueIterator> it = GetIterator ();
  while (it->HasNext ())
  {
    csString name;
    Value* value = it->NextChild (&name);
    result.Put (name, View::ValueToString (value));
  }
  return result;
}

void CompositeValue::AddChildren (ValueType type, ...)
{
  va_list args;
  va_start (args, type);
  AddChildren (type, args);
  va_end (args);
}

void CompositeValue::AddChildren (ValueType type, va_list args)
{
  while (type != VALUE_NONE)
  {
    const char* name = va_arg (args, char*);
    switch (type)
    {
      case VALUE_STRING:
	{
	  const char* value = va_arg (args, char*);
	  AddChild (name, NEWREF(StringValue,new StringValue(value)));
	  break;
	}
      case VALUE_LONG:
	{
	  long value = va_arg (args, long);
	  AddChild (name, NEWREF(LongValue,new LongValue(value)));
	  break;
	}
      case VALUE_FLOAT:
	{
	  float value = va_arg (args, double);
	  AddChild (name, NEWREF(FloatValue,new FloatValue(value)));
	  break;
	}
      case VALUE_BOOL:
	{
	  bool value = va_arg (args, int);
	  AddChild (name, NEWREF(BoolValue,new BoolValue(value)));
	  break;
	}
      case VALUE_STRINGARRAY:
	{
	  const csStringArray* value = va_arg (args, csStringArray*);
	  AddChild (name, NEWREF(StringArrayValue,new StringArrayValue(*value)));
	  break;
	}
      case VALUE_COLLECTION:
      case VALUE_COMPOSITE:
	{
	  Value* value = va_arg (args, Value*);
	  AddChild (name, value);
	  break;
	}
      default:
	break;
    }
    type = (ValueType) va_arg (args, int);
  }
}

// --------------------------------------------------------------------------

CompositeValue* StandardCollectionValue::NewCompositeChild (ValueType type, ...)
{
  va_list args;
  va_start (args, type);
  csRef<CompositeValue> composite = View::CreateComposite (type, args);
  va_end (args);
  children.Push (composite);
  composite->SetParent (this);
  return composite;
}

StringArrayValue* StandardCollectionValue::NewStringArrayChild (ValueType type, ...)
{
  va_list args;
  va_start (args, type);
  csRef<StringArrayValue> stringarray = View::CreateStringArray (type, args);
  va_end (args);
  children.Push (stringarray);
  stringarray->SetParent (this);
  return stringarray;
}

void StandardCollectionValue::RemoveChild (Value* child)
{
  children.Delete (child);
  FireValueChanged ();
}

// --------------------------------------------------------------------------

void FilteredCollectionValue::UpdateFilter ()
{
  filteredChildren.DeleteAll ();
  if (!collection) return;
  csRef<ValueIterator> it = collection->GetIterator ();
  while (it->HasNext ())
  {
    Value* child = it->NextChild ();
    if (Filter (child))
      filteredChildren.Push (child);
  }
}


// --------------------------------------------------------------------------

MirrorValue::MirrorValue (ValueType type) : type (type)
{
  changeListener.AttachNew (new SelChangeListener (this));
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
  csRef<ValueIterator> it = compositeValue->GetIterator ();
  while (it->HasNext ())
  {
    csString name;
    Value* child = it->NextChild (&name);
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
      // @@@ (remove static_cast if children is again an array of MirrorValue
      (static_cast<MirrorValue*> (children[i]))->SetMirrorValue (0);
  }
  mirroringValue = value;
  if (mirroringValue)
  {
    mirroringValue->AddValueChangeListener (changeListener);
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      // @@@ (remove static_cast if children is again an array of MirrorValue
      (static_cast<MirrorValue*> (children[i]))->SetMirrorValue (value->GetChild (i));
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

class SelectedBoolValue : public BoolValue
{
private:
  long* selection;

public:
  SelectedBoolValue (long* s) : selection (s) { }
  virtual ~SelectedBoolValue () { }
  // Make sure this class doesn't cause crashes if the parent list
  // selection value is removed.
  void Invalidate () { selection = 0; }
  virtual void SetBoolValue (bool fl) { }
  virtual bool GetBoolValue ()
  {
    if (!selection) return false;
    return (*selection != -1);
  }
};

ListSelectedValue::ListSelectedValue (wxListCtrl* listCtrl, Value* collectionValue, ValueType type) :
  wxEvtHandler (), MirrorValue (type), listCtrl (listCtrl),
  collectionValue (collectionValue)
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
  if (selectedStateValue) selectedStateValue->Invalidate ();
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
  if (selectedStateValue)
    selectedStateValue->FireValueChanged ();
}

Value* ListSelectedValue::GetSelectedState ()
{
  if (!selectedStateValue)
    selectedStateValue.AttachNew (new SelectedBoolValue (&selection));
  return selectedStateValue;
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
 * Find a tree item corresponding with a given value.
 */
static wxTreeItemId TreeFromValue (wxTreeCtrl* tree, wxTreeItemId parent, Value* collectionValue, Value* value)
{
  wxTreeItemIdValue cookie;
  wxTreeItemId treeChild = tree->GetFirstChild (parent, cookie);
  csRef<ValueIterator> it = collectionValue->GetIterator ();
  while (it->HasNext ())
  {
    Value* child = it->NextChild ();
    if (value == child) return treeChild;
    treeChild = TreeFromValue (tree, treeChild, child, value);
    if (treeChild.IsOk ()) return treeChild;
    treeChild = tree->GetNextChild (parent, cookie);
  }
  return wxTreeItemId();
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
    csRef<ValueIterator> it = value->GetIterator ();
    while (it->HasNext ())
    {
      Value* child = it->NextChild ();
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

bool AbstractNewAction::DoDialog (View* view, wxWindow* component, iUIDialog* dialog,
    bool update)
{
  Value* origValue = 0;
  if (dialog)
  {
    dialog->Clear ();
    if (update)
    {
      origValue = view->GetSelectedValue (component);
      if (!origValue)
        update = false;
      else
      {
	csString d = origValue->Dump (true);
	printf ("%s\n", d.GetData ()); fflush (stdout);
	dialog->SetFieldContents (origValue->GetDialogValue ());
      }
    }
    if (dialog->Show (0) == 0) return false;
  }

  size_t idx = csArrayItemNotFound;
  wxListCtrl* listCtrl = 0;
  //wxTreeCtrl* treeCtrl = 0;
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    listCtrl = wxStaticCast (component, wxListCtrl);
    idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
  }
  else if (component->IsKindOf (CLASSINFO (wxTreeCtrl)))
  {
    //treeCtrl = wxStaticCast (component, wxTreeCtrl);
  }
  DialogResult dialogResult;
  if (dialog) dialogResult = dialog->GetFieldContents ();
  if (update)
  {
    if (!collection->UpdateValue (idx, origValue, dialogResult))
      return false;
    view->SetSelectedValue (component, origValue);
  }
  else
  {
    Value* value = collection->NewValue (idx, view->GetSelectedValue (component),
      dialogResult);
    if (!value) return false;
    view->SetSelectedValue (component, value);
  }
  return true;
}

bool NewChildAction::Do (View* view, wxWindow* component)
{
  return DoDialog (view, component, 0);
}

NewChildDialogAction::NewChildDialogAction (Value* collection, iUIDialog* dialog) :
    AbstractNewAction (collection), dialog (dialog)
{
}

NewChildDialogAction::~NewChildDialogAction ()
{
}

bool NewChildDialogAction::Do (View* view, wxWindow* component)
{
  csRef<iUIDialog> dlg (scfQueryInterface<iUIDialog> (dialog));
  return DoDialog (view, component, dlg);
}

EditChildDialogAction::EditChildDialogAction (Value* collection, iUIDialog* dialog) :
    AbstractNewAction (collection), dialog (dialog)
{
}

EditChildDialogAction::~EditChildDialogAction ()
{
}

bool EditChildDialogAction::Do (View* view, wxWindow* component)
{
  csRef<iUIDialog> dlg (scfQueryInterface<iUIDialog> (dialog));
  return DoDialog (view, component, dlg, true);
}

bool EditChildDialogAction::IsActive (View* view, wxWindow* component)
{
  return view->GetSelectedValue (component) != 0;
}

bool DeleteChildAction::Do (View* view, wxWindow* component)
{
  Value* value = view->GetSelectedValue (component);
  if (!value) return false;	// Nothing to do.
  return collection->DeleteValue (value);
}

bool DeleteChildAction::IsActive (View* view, wxWindow* component)
{
  return view->GetSelectedValue (component) != 0;
}


// --------------------------------------------------------------------------

View::View (wxWindow* parent) : parent (parent), lastContextID (wxID_HIGHEST + 10000), eventHandler (this)
{
  changeListener.AttachNew (new ViewChangeListener (this));
}

void View::DestroyBindings ()
{
  ComponentToBinding::GlobalIterator it = bindingsByComponent.GetIterator ();
  while (it.HasNext ())
  {
    csPtrKey<wxWindow> component;
    Binding* binding = it.Next (component);
    binding->value->RemoveValueChangeListener (changeListener);
    if (binding->eventType == wxEVT_COMMAND_TEXT_UPDATED ||
	binding->eventType == wxEVT_COMMAND_LIST_ITEM_SELECTED ||
	binding->eventType == wxEVT_COMMAND_CHOICE_SELECTED ||
	binding->eventType == wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGED ||
	binding->eventType == wxEVT_COMMAND_CHECKBOX_CLICKED)
      component->Disconnect (binding->eventType,
	  wxCommandEventHandler (EventHandler :: OnComponentChanged), 0, &eventHandler);
  }
}

void View::RemoveBinding (wxWindow* component)
{
  Binding* binding = bindingsByComponent.Get (component, 0);
  if (!binding) return;
  binding->value->RemoveValueChangeListener (changeListener);
  if (binding->eventType == wxEVT_COMMAND_TEXT_UPDATED ||
      binding->eventType == wxEVT_COMMAND_LIST_ITEM_SELECTED ||
      binding->eventType == wxEVT_COMMAND_CHOICE_SELECTED ||
      binding->eventType == wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGED ||
      binding->eventType == wxEVT_COMMAND_CHECKBOX_CLICKED)
    component->Disconnect (binding->eventType,
	wxCommandEventHandler (EventHandler :: OnComponentChanged), 0, &eventHandler);
  bindingsByComponent.Delete (component, binding);
  bindingsByValue.Delete ((Value*)(binding->value), binding);
  bindings.Delete (binding);
}

void View::DestroyActionBindings ()
{
  while (rmbContexts.GetSize () > 0)
  {
    RmbContext lc = rmbContexts.Pop ();
    lc.component->Disconnect (wxEVT_CONTEXT_MENU, wxContextMenuEventHandler (EventHandler :: OnRMB), 0, &eventHandler);
    while (lc.actionDefs.GetSize () > 0)
    {
      ActionDef ad = lc.actionDefs.Pop ();
      lc.component->Disconnect (ad.id, wxEVT_COMMAND_MENU_SELECTED,
	    wxCommandEventHandler (EventHandler :: OnActionExecuted), 0, &eventHandler);
    }
  }
  csHash<csRef<Action>,csPtrKey<wxButton> >::GlobalIterator it = buttonActions.GetIterator ();
  while (it.HasNext ())
  {
    csPtrKey<wxButton> button;
    csRef<Action> action = it.Next (button);
    button->Disconnect (wxEVT_COMMAND_BUTTON_CLICKED,
	    wxCommandEventHandler (EventHandler :: OnActionExecuted), 0, &eventHandler);
  }
  buttonActions.DeleteAll ();
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

bool View::BindEnabled (Value* value, wxWindow* component)
{
  RegisterBinding (value, component, wxEVT_NULL, true);
  ValueChanged (value);
  return true;
}

bool View::BindEnabled (Value* value, const char* compName)
{
  //wxString wxcompName = wxString::FromUTF8 (compName);
  //wxWindow* comp = parent->FindWindow (wxcompName);
  wxWindow* comp = FindComponentByName (parent, compName);
  if (!comp)
  {
    printf ("BindEnabled: Can't find component '%s'!\n", compName);
    return false;
  }
  return BindEnabled (value, comp);
}

bool View::Bind (Value* value, wxWindow* component)
{
  if (component->IsKindOf (CLASSINFO (wxTextCtrl)))
    return Bind (value, wxStaticCast (component, wxTextCtrl));
  if (component->IsKindOf (CLASSINFO (wxChoice)))
    return Bind (value, wxStaticCast (component, wxChoice));
  if (component->IsKindOf (CLASSINFO (wxComboBox)))
    return Bind (value, wxStaticCast (component, wxComboBox));
  if (component->IsKindOf (CLASSINFO (wxCheckBox)))
    return Bind (value, wxStaticCast (component, wxCheckBox));
  if (component->IsKindOf (CLASSINFO (wxPanel)))
    return Bind (value, wxStaticCast (component, wxPanel));
  if (component->IsKindOf (CLASSINFO (wxDialog)))
    return Bind (value, wxStaticCast (component, wxDialog));
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
    return Bind (value, wxStaticCast (component, wxListCtrl));
  if (component->IsKindOf (CLASSINFO (wxTreeCtrl)))
    return Bind (value, wxStaticCast (component, wxTreeCtrl));
  if (component->IsKindOf (CLASSINFO (wxChoicebook)))
    return Bind (value, wxStaticCast (component, wxChoicebook));
  if (component->IsKindOf (CLASSINFO (CustomControl)))
    return Bind (value, wxStaticCast (component, CustomControl));
  csString compName = (const char*)component->GetName ().mb_str (wxConvUTF8);
  printf ("Bind: Unsupported type for component '%s'!\n", compName.GetData ());
  return false;
}

bool View::Bind (Value* value, const char* compName)
{
  //wxString wxcompName = wxString::FromUTF8 (compName);
  //wxWindow* comp = parent->FindWindow (wxcompName);
  wxWindow* comp = FindComponentByName (parent, compName);
  if (!comp)
  {
    printf ("Bind: Can't find component '%s'!\n", compName);
    return false;
  }
  return Bind (value, comp);
}

void View::RegisterBinding (Value* value, wxWindow* component, wxEventType eventType,
    bool changeEnabled)
{
  Binding* b = new Binding ();
  b->value = value;
  b->component = component;
  b->eventType = eventType;
  b->changeEnabled = changeEnabled;
  if (!changeEnabled)
    bindingsByComponent.Put (component, b);
  bindingsByValue.Put (value, b);
  if (eventType != wxEVT_NULL)
    component->Connect (eventType, wxCommandEventHandler (EventHandler :: OnComponentChanged), 0, &eventHandler);
  value->AddValueChangeListener (changeListener);
}

bool View::Bind (Value* value, wxChoice* component)
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
      printf ("Unsupported value type for choice control!\n");
      return false;
  }

  RegisterBinding (value, component, wxEVT_COMMAND_CHOICE_SELECTED);
  ValueChanged (value);
  return true;
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

bool View::Bind (Value* value, wxComboBox* component)
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

bool View::Bind (Value* value, wxCheckBox* component)
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
      printf ("Unsupported value type for checkbox!\n");
      return false;
  }

  RegisterBinding (value, component, wxEVT_COMMAND_CHECKBOX_CLICKED);
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

bool View::BindContainer (Value* value, wxWindow* component)
{
  // We also support VALUE_NONE here because that can be useful in situations where we don't
  // know the type yet (for example when the value is a ListSelectedValue and nothing has been
  // selected).
  if (value->GetType () != VALUE_COMPOSITE && value->GetType () != VALUE_NONE)
  {
    printf ("Unsupported value type for panels! Only VALUE_COMPOSITE is supported.\n");
    return false;
  }

  csString compName = (const char*)component->GetName ().mb_str (wxConvUTF8);

  RegisterBinding (value, component, wxEVT_NULL);

  csRef<ValueIterator> it = value->GetIterator ();
  while (it->HasNext ())
  {
    csString name;
    Value* child = it->NextChild (&name);
    wxWindow* childComp = FindComponentByName (component, name);
    if (!childComp)
    {
      printf ("Warning: no component found for child '%s'!\n", name.GetData ());
    }
    else
    {
      compName = (const char*)childComp->GetName ().mb_str (wxConvUTF8);
      if (!Bind (child, childComp))
	return false;
    }
  }
  ValueChanged (value);
  return true;
}

bool View::Bind (Value* value, wxDialog* component)
{
  return BindContainer (value, component);
}

bool View::Bind (Value* value, wxPanel* component)
{
  return BindContainer (value, component);
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

static bool ValueToBoolStatic (Value* value)
{
  switch (value->GetType ())
  {
    case VALUE_STRING:
      {
	const char* v = value->GetStringValue ();
	return v && *v == 't';
      }
    case VALUE_LONG:
      return bool (value->GetLongValue ());
    case VALUE_BOOL:
      return value->GetBoolValue ();
    case VALUE_FLOAT:
      return fabs (value->GetFloatValue ()) > .000001f;
    case VALUE_STRINGARRAY:
      return value->GetStringArrayValue () && !value->GetStringArrayValue ()->IsEmpty ();
    case VALUE_COLLECTION:
    case VALUE_COMPOSITE:
      {
	csRef<ValueIterator> it = value->GetIterator ();
        return it->HasNext ();
      }
    default:
      return false;
  }
}

bool View::ValueToBool (Value* value)
{
  return ValueToBoolStatic (value);
}

csString View::ValueToString (Value* value)
{
  csString val;
  switch (value->GetType ())
  {
    case VALUE_STRING: return csString (value->GetStringValue ());
    case VALUE_LONG: val.Format ("%ld", value->GetLongValue ()); return val;
    case VALUE_BOOL: return csString (value->GetBoolValue () ? "true" : "false");
    case VALUE_FLOAT: val.Format ("%g", value->GetFloatValue ()); return val;
    case VALUE_STRINGARRAY: return csString("<stringarray>");
    case VALUE_COLLECTION: return csString ("<collection>");
    case VALUE_COMPOSITE: return csString ("<composite>");
    default: return csString ("<?>");
  }
}

void View::LongToValue (long l, Value* value)
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
    case VALUE_STRINGARRAY: return;
    case VALUE_COLLECTION: return;
    case VALUE_COMPOSITE: return;
    default: return;
  }
}

void View::BoolToValue (bool in, Value* value)
{
  switch (value->GetType ())
  {
    case VALUE_STRING:
      value->SetStringValue (in ? "true" : "false");
      return;
    case VALUE_LONG:
      value->SetLongValue (long (in));
      return;
    case VALUE_BOOL:
      value->SetBoolValue (in);
      return;
    case VALUE_FLOAT:
      value->SetFloatValue (float (in));
      return;
    case VALUE_STRINGARRAY: return;
    case VALUE_COLLECTION: return;
    case VALUE_COMPOSITE: return;
    default: return;
  }
}

void View::StringToValue (const char* str, Value* value)
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
    case VALUE_STRINGARRAY: return;
    case VALUE_COLLECTION: return;
    case VALUE_COMPOSITE: return;
    default: return;
  }
}

Value* View::FindChild (Value* collection, const char* str)
{
  csRef<ValueIterator> it = collection->GetIterator ();
  while (it->HasNext ())
  {
    Value* child = it->NextChild ();
    if (ValueToString (child) == str)
      return child;
  }
  return 0;
}

csRef<CompositeValue> View::CreateComposite (ValueType type, va_list args)
{
  csRef<CompositeValue> composite = NEWREF(CompositeValue,new CompositeValue());
  composite->AddChildren (type, args);
  return composite;
}

csRef<CompositeValue> View::CreateComposite (ValueType type, ...)
{
  va_list args;
  va_start (args, type);
  csRef<CompositeValue> value = CreateComposite (type, args);
  va_end (args);
  return value;
}

csRef<StringArrayValue> View::CreateStringArray (ValueType type, va_list args)
{
  csRef<StringArrayValue> stringarray = NEWREF(StringArrayValue,new StringArrayValue());
  csStringArray& array = stringarray->GetArray ();

  while (type != VALUE_NONE)
  {
    switch (type)
    {
      case VALUE_STRING:
	{
	  const char* value = va_arg (args, char*);
	  array.Push (value);
	  break;
	}
      case VALUE_LONG:
	{
	  long value = va_arg (args, long);
	  csString fmt;
	  fmt.Format ("%ld", value);
	  array.Push (fmt);
	  break;
	}
      case VALUE_BOOL:
	{
	  bool value = va_arg (args, int);
	  csString fmt;
	  fmt.Format ("%d", int (value));
	  array.Push (fmt);
	  break;
	}
      case VALUE_FLOAT:
	{
	  float value = va_arg (args, double);
	  csString fmt;
	  fmt.Format ("%g", value);
	  array.Push (fmt);
	  break;
	}
      default:
	array.Push ("<?>");
	break;
    }
    type = (ValueType) va_arg (args, int);
  }
  return stringarray;
}

csRef<StringArrayValue> View::CreateStringArray (ValueType type, ...)
{
  va_list args;
  va_start (args, type);
  csRef<StringArrayValue> value = CreateStringArray (type, args);
  va_end (args);
  return value;
}

csStringArray View::ConstructListRow (const ListHeading& lh, Value* value)
{
  csStringArray row;
  ValueType t = value->GetType ();
  if (t == VALUE_STRINGARRAY)
  {
    const csStringArray* array = value->GetStringArrayValue ();
    if (!array) return row;
    for (size_t i = 0 ; i < lh.heading.GetSize () ; i++)
    {
      csString el = array->Get (lh.indices[i]);
      row.Push (el);
    }
  }
  else if (t == VALUE_COMPOSITE)
  {
    for (size_t i = 0 ; i < lh.names.GetSize () ; i++)
    {
      Value* child = value->GetChildByName (lh.names[i]);
      if (child)
	row.Push (ValueToString (child));
      else
      {
	printf ("Warning: child '%s' is missing!\n", (const char*)lh.names[i]);
	row.Push ("");
      }
    }
  }
  else
  {
    row.Push (ValueToString (value));
  }
  return row;
}

size_t View::FindRmbContext (wxWindow* component)
{
  for (size_t i = 0 ; i < rmbContexts.GetSize () ; i++)
    if (rmbContexts[i].component == component)
      return i;
  return csArrayItemNotFound;
}

void View::OnRMB (wxContextMenuEvent& event)
{
  wxWindow* component = wxStaticCast (event.GetEventObject (), wxWindow);
  size_t idx = FindRmbContext (component);
  if (idx == csArrayItemNotFound)
  {
    // We also try the parent since when the list is empty we apparently get
    // a child of the list instead of the list itself.
    component = component->GetParent ();
    if (component == 0) return;
    idx = FindRmbContext (component);
    if (idx == csArrayItemNotFound) return;
  }

  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    wxListCtrl* listCtrl = wxStaticCast (component, wxListCtrl);
    bool hasItem;
    if (!ListCtrlTools::CheckHitList (listCtrl, hasItem, event.GetPosition ()))
      return;
  }
  else if (component->IsKindOf (CLASSINFO (wxTreeCtrl)))
  {
    wxTreeCtrl* treeCtrl = wxStaticCast (component, wxTreeCtrl);
    if (!treeCtrl->IsShownOnScreen ()) return;
    int flags = 0;
    wxTreeItemId idx = treeCtrl->HitTest (treeCtrl->ScreenToClient (event.GetPosition ()), flags);
    if (!idx.IsOk())
    {
      if (!treeCtrl->GetScreenRect ().Contains (event.GetPosition ()))
        return;
    }
    else treeCtrl->SelectItem (idx);
  }

  const RmbContext& lc = rmbContexts[idx];
  wxMenu contextMenu;
  for (size_t j = 0 ; j < lc.actionDefs.GetSize () ; j++)
  {
    Action* action = lc.actionDefs[j].action;
    wxMenuItem* item = contextMenu.Append (lc.actionDefs[j].id,
	wxString::FromUTF8 (action->GetName ()));
    bool active = action->IsActive (this, component);
    item->Enable (active);
  }
  component->PopupMenu (&contextMenu);
}

void View::OnActionExecuted (wxCommandEvent& event)
{
  int id = event.GetId ();
  // We have to scan all lists here to find the one that has the right id.
  for (size_t i = 0 ; i < rmbContexts.GetSize () ; i++)
  {
    const RmbContext& lc = rmbContexts[i];
    for (size_t j = 0 ; j < lc.actionDefs.GetSize () ; j++)
      if (lc.actionDefs[j].id == id)
      {
	lc.actionDefs[j].action->Do (this, lc.component);
	return;
      }
  }
  // Scan all buttons if we didn't find a list.
  csHash<csRef<Action>,csPtrKey<wxButton> >::GlobalIterator it = buttonActions.GetIterator ();
  while (it.HasNext ())
  {
    csPtrKey<wxButton> button;
    csRef<Action> action = it.Next (button);
    if (button == event.GetEventObject ())
    {
      action->Do (this, button);
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
  else if (component->IsKindOf (CLASSINFO (wxChoice)))
  {
    wxChoice* choiceCtrl = wxStaticCast (component, wxChoice);
    csString text = (const char*)choiceCtrl->GetStringSelection ().mb_str (wxConvUTF8);
    StringToValue (text, binding->value);
  }
  else if (component->IsKindOf (CLASSINFO (wxComboBox)))
  {
    wxComboBox* combo = wxStaticCast (component, wxComboBox);
    csString text = (const char*)combo->GetValue ().mb_str (wxConvUTF8);
    StringToValue (text, binding->value);
  }
  else if (component->IsKindOf (CLASSINFO (wxCheckBox)))
  {
    wxCheckBox* checkBox = wxStaticCast (component, wxCheckBox);
    BoolToValue (checkBox->GetValue (), binding->value);
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
  csRef<ValueIterator> it = value->GetIterator ();
  while (it->HasNext ())
  {
    Value* child = it->NextChild ();
    wxTreeItemId itemId = treeCtrl->AppendItem (parent, wxString::FromUTF8 (child->GetStringValue ()));
    BuildTree (treeCtrl, child, itemId);
  }
}

void View::UpdateTree (wxTreeCtrl* treeCtrl, Value* value, wxTreeItemId& parent)
{
  csRef<ValueIterator> it = value->GetIterator ();
  wxTreeItemIdValue cookie;
  wxTreeItemId itemId = treeCtrl->GetFirstChild (parent, cookie);
  bool addingnew = false;	// Set to true as soon as we're adding new items ourselves.
  while (it->HasNext ())
  {
    Value* child = it->NextChild ();
    wxString newLabel = wxString::FromUTF8 (child->GetStringValue ());
    if ((!addingnew) && itemId.IsOk ())
    {
      wxString currentLabel = treeCtrl->GetItemText (itemId);
      if (currentLabel != newLabel)
      {
        treeCtrl->SetItemText (itemId, newLabel);
      }
      UpdateTree (treeCtrl, child, itemId);
      itemId = treeCtrl->GetNextChild (parent, cookie);
    }
    else
    {
      itemId = treeCtrl->AppendItem (parent, newLabel);
      UpdateTree (treeCtrl, child, itemId);
      addingnew = true;
    }
  }

  if (!addingnew)
  {
    // Might have to remove stuff.
    csArray<wxTreeItemId> toRemove;
    while (itemId.IsOk ())
    {
      toRemove.Push (itemId);
      itemId = treeCtrl->GetNextChild (parent, cookie);
    }
    for (size_t i = 0 ; i < toRemove.GetSize () ; i++)
      treeCtrl->Delete (toRemove[i]);
  }
}

bool View::IsValueBound (Value* value) const
{
  ValueToBinding::ConstIterator it = bindingsByValue.GetIterator (value);
  return it.HasNext ();
}

bool View::CheckIfParentDisabled (wxWindow* window)
{
  window = window->GetParent ();
  while (window)
  {
    if (window == parent) return false;
    if (disabledComponents.In (window)) return true;
    if (window->IsEnabled () == false) return true;
    window = window->GetParent ();
  }
  return false;
}

void View::EnableBoundComponents (wxWindow* comp, bool state)
{
  if (state)
    disabledComponents.Delete (comp);
  else
    disabledComponents.Add (comp);
  bool parentDisabled = CheckIfParentDisabled (comp);
  if (parentDisabled) state = false;
  EnableBoundComponentsInt (comp, state);
}

void View::EnableBoundComponentsInt (wxWindow* comp, bool state)
{
  if (comp->IsKindOf (CLASSINFO (wxPanel)) ||
      comp->IsKindOf (CLASSINFO (wxDialog)))
  {
    if (disabledComponents.In (comp))
      state = false;
    wxWindowList list = comp->GetChildren ();
    wxWindowList::iterator iter;
    for (iter = list.begin (); iter != list.end () ; ++iter)
    {
      wxWindow* child = *iter;
      EnableBoundComponentsInt (child, state);
    }
  }
  else
  {
    Binding* binding = bindingsByComponent.Get (comp, 0);
    if (binding)
    {
      if (!state)
	comp->Enable (state);
      else if (!disabledComponents.In (comp))
	comp->Enable (state);
    }
  }
}

void View::ValueChanged (Value* value)
{
#if DO_DEBUG
  printf ("View::ValueChanged: %s\n", value->Dump ().GetData ());
#endif
  ValueToBinding::Iterator it = bindingsByValue.GetIterator (value);
  if (!it.HasNext ())
  {
    printf ("ValueChanged: Something went wrong! Called without a valid binding!\n");
    CS_ASSERT (false);
    return;
  }
  while (it.HasNext ())
  {
    Binding* b = it.Next ();
    wxWindow* comp = b->component;
    if (b->changeEnabled)
    {
      // Modify disabled/enabled state instead of value.
      bool state = ValueToBool (value);
      EnableBoundComponents (comp, state);
      continue;
    }

    if (!b->processing)
    {
      if (comp->IsKindOf (CLASSINFO (wxTextCtrl)))
      {
	b->processing = true;
	wxTextCtrl* textCtrl = wxStaticCast (comp, wxTextCtrl);
	csString text = ValueToString (value);
	textCtrl->SetValue (wxString::FromUTF8 (text));
	b->processing = false;
      }
      else if (comp->IsKindOf (CLASSINFO (wxChoice)))
      {
	b->processing = true;
	wxChoice* choiceCtrl = wxStaticCast (comp, wxChoice);
	csString text = ValueToString (value);
	choiceCtrl->SetStringSelection (wxString::FromUTF8 (text));
	b->processing = false;
      }
      else if (comp->IsKindOf (CLASSINFO (wxComboBox)))
      {
	b->processing = true;
	wxComboBox* combo = wxStaticCast (comp, wxComboBox);
	csString text = ValueToString (value);
	combo->SetValue (wxString::FromUTF8 (text));
	b->processing = false;
      }
      else if (comp->IsKindOf (CLASSINFO (wxCheckBox)))
      {
	b->processing = true;
	wxCheckBox* checkBox = wxStaticCast (comp, wxCheckBox);
	bool in = ValueToBool (value);
	checkBox->SetValue (in);
	b->processing = false;
      }
      else if (comp->IsKindOf (CLASSINFO (CustomControl)))
      {
	CustomControl* customCtrl = wxStaticCast (comp, CustomControl);
	customCtrl->SyncValue (value);
      }
      else if (comp->IsKindOf (CLASSINFO (wxPanel)) ||
	       comp->IsKindOf (CLASSINFO (wxDialog)))
      {
	// If the value of a composite changes we update the children.
	csRef<ValueIterator> it = value->GetIterator ();
	while (it->HasNext ())
	{
	  Value* child = it->NextChild ();
	  if (IsValueBound (child))
	    ValueChanged (child);
	}
      }
      else if (comp->IsKindOf (CLASSINFO (wxListCtrl)))
      {
//csString compName = (const char*)comp->GetName ().mb_str (wxConvUTF8);
//printf ("ValueChanged for component '%s'\n", compName.GetData ()); fflush (stdout);
	wxListCtrl* listCtrl = wxStaticCast (comp, wxListCtrl);
	long idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
	listCtrl->Freeze ();
	listCtrl->DeleteAllItems ();
	ListHeading lhdef;
	const ListHeading& lh = listToHeading.Get (listCtrl, lhdef);
	csRef<ValueIterator> it = value->GetIterator ();
	while (it->HasNext ())
	{
	  Value* child = it->NextChild ();
	  ListCtrlTools::AddRow (listCtrl, ConstructListRow (lh, child));
	}
	if (idx != -1)
	  ListCtrlTools::SelectRow (listCtrl, idx, true);
	listCtrl->Thaw ();
      }
      else if (comp->IsKindOf (CLASSINFO (wxTreeCtrl)))
      {
	wxTreeCtrl* treeCtrl = wxStaticCast (comp, wxTreeCtrl);
	treeCtrl->Freeze ();
	wxTreeItemId rootId = treeCtrl->GetRootItem ();
	if (rootId.IsOk ())
	{
	  UpdateTree (treeCtrl, value, rootId);
	}
	else
	{
	  treeCtrl->DeleteAllItems ();
	  rootId = treeCtrl->AddRoot (wxString::FromUTF8 (value->GetStringValue ()));
	  BuildTree (treeCtrl, value, rootId);
	}
	treeCtrl->Thaw ();
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

bool View::DefineHeadingIndexed (const char* listName, const char* heading, ...)
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

  va_list args;
  va_start (args, heading);
  bool rc = DefineHeadingIndexed (listCtrl, heading, args);
  va_end (args);
  return rc;
}

bool View::DefineHeadingIndexed (wxListCtrl* listCtrl, const char* heading, ...)
{
  va_list args;
  va_start (args, heading);
  bool rc = DefineHeadingIndexed (listCtrl, heading, args);
  va_end (args);
  return rc;
}

bool View::DefineHeadingIndexed (wxListCtrl* listCtrl, const char* heading, va_list args)
{
  ListHeading lh;
  lh.heading.SplitString (heading, ",");

  for (size_t i = 0 ; i < lh.heading.GetSize () ; i++)
  {
    int index = va_arg (args, int);
    lh.indices.Push (index);
    ListCtrlTools::SetColumn (listCtrl, i, lh.heading[i], 100);
  }

  listToHeading.Put (listCtrl, lh);
  return true;
}

bool View::AddAction (const char* compName, Action* action)
{
  wxString wxcompName = wxString::FromUTF8 (compName);
  wxWindow* comp = parent->FindWindow (wxcompName);
  if (!comp)
  {
    printf ("AddAction: Can't find component '%s'!\n", compName);
    return false;
  }
  return AddAction (comp, action);
}

bool View::AddAction (wxWindow* component, Action* action)
{
  if (component->IsKindOf (CLASSINFO (wxButton)))
    return AddAction (wxStaticCast (component, wxButton), action);
  if (component->IsKindOf (CLASSINFO (wxListCtrl)) || component->IsKindOf (CLASSINFO (wxTreeCtrl)))
    return AddContextAction (component, action);
  printf ("AddAction: Unsupported type for component!\n");
  return false;
}

bool View::AddContextAction (wxWindow* component, Action* action)
{
  component->Connect (lastContextID, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EventHandler :: OnActionExecuted), 0, &eventHandler);

  size_t idx = FindRmbContext (component);
  if (idx == csArrayItemNotFound)
  {
    RmbContext lc;
    lc.component = component;
    idx = rmbContexts.Push (lc);
    component->Connect (wxEVT_CONTEXT_MENU, wxContextMenuEventHandler (EventHandler :: OnRMB), 0, &eventHandler);
  }

  RmbContext& lc = rmbContexts[idx];
  ActionDef ad;
  ad.id = lastContextID;
  ad.action = action;
  lc.actionDefs.Push (ad);
  lastContextID++;
  return true;
}

bool View::AddAction (wxButton* button, Action* action)
{
  button->Connect (wxEVT_COMMAND_BUTTON_CLICKED,
	  wxCommandEventHandler (EventHandler :: OnActionExecuted), 0, &eventHandler);
  buttonActions.Put (button, action);
  wxString wxlabel = wxString::FromUTF8 (action->GetName ());
  button->SetLabel (wxlabel);
  return true;
}

bool View::SetSelectedValue (wxWindow* component, Value* value)
{
  Binding* binding = bindingsByComponent.Get (component, 0);
  if (!binding)
  {
    printf ("SetSelectedValue: Component is not bound to a value!\n");
    return false;
  }
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    wxListCtrl* listCtrl = wxStaticCast (component, wxListCtrl);
    csRef<ValueIterator> it = binding->value->GetIterator ();
    bool found = false;
    size_t idx = 0;
    while (it->HasNext ())
    {
      Value* child = it->NextChild ();
      if (child == value) { found = true; break; }
      idx++;
    }
    if (found)
    {
      ListCtrlTools::SelectRow (listCtrl, idx, true);
      return true;
    }
    return false;
  }
  if (component->IsKindOf (CLASSINFO (wxTreeCtrl)))
  {
    wxTreeCtrl* treeCtrl = wxStaticCast (component, wxTreeCtrl);
    wxTreeItemId child = TreeFromValue (treeCtrl, treeCtrl->GetRootItem (),
	binding->value, value);
    if (child.IsOk ())
    {
      treeCtrl->SelectItem (child);
      return true;
    }
    return false;
  }
  printf ("SetSelectedValue: Unsupported type for component!\n");
  return false;
}

Value* View::GetValue (wxWindow* component)
{
  Binding* binding = bindingsByComponent.Get (component, 0);
  if (!binding) return 0;
  return binding->value;
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

csArray<Value*> View::GetSelectedValues (wxWindow* component)
{
  Binding* binding = bindingsByComponent.Get (component, 0);
  if (!binding)
  {
    printf ("GetSelectedValue: Component is not bound to a value!\n");
    return 0;
  }
  csArray<Value*> values;
  if (component->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    wxListCtrl* listCtrl = wxStaticCast (component, wxListCtrl);
    csArray<long> indices = ListCtrlTools::GetSelectedRowIndices (listCtrl);
    for (size_t i = 0 ; i < indices.GetSize () ; i++)
      values.Push (binding->value->GetChild (size_t (indices[i])));
    return values;
  }
  if (component->IsKindOf (CLASSINFO (wxTreeCtrl)))
  {
    wxTreeCtrl* treeCtrl = wxStaticCast (component, wxTreeCtrl);
    wxTreeItemId selection = treeCtrl->GetSelection ();
    // @@@ Only support first selection for now.
    if (selection.IsOk ())
      values.Push (ValueFromTree (treeCtrl, selection, binding->value));
    return values;
  }
  printf ("GetSelectedValue: Unsupported type for component!\n");
  return 0;
}

class SignalChangeListener : public ValueChangeListener
{
private:
  csRef<Value> source;
  csRef<Value> dest;
  bool dochildren;

public:
  SignalChangeListener (Value* source, Value* dest, bool dochildren) :
    source (source), dest (dest), dochildren (dochildren) { }
  virtual ~SignalChangeListener () { }
  virtual void ValueChanged (Value* value)
  {
    // printf ("SIGNAL: from %s to %s\n", source->Dump ().GetData (), dest->Dump ().GetData ());
    dest->FireValueChanged ();
    if (dochildren)
    {
      csRef<ValueIterator> it = dest->GetIterator ();
      while (it->HasNext ())
      {
	Value* child = it->NextChild ();
	// printf ("  FIRE: to %s\n", child->Dump ().GetData ());
	child->FireValueChanged ();
      }
    }
  }
};

void View::Signal (Value* source, Value* dest, bool dochildren)
{
  csRef<SignalChangeListener> listener;
  listener.AttachNew (new SignalChangeListener (source, dest, dochildren));
  source->AddValueChangeListener (listener);
}

class NotValue : public BoolValue
{
private:
  csRef<Value> value;

public:
  NotValue (Value* value) : value (value) { }
  virtual ~NotValue () { }
  virtual void SetBoolValue (bool fl) { }
  virtual bool GetBoolValue ()
  {
    return !ValueToBoolStatic (value);
  }
};

csRef<Value> View::Not (Value* value)
{
  csRef<Value> v;
  v.AttachNew (new NotValue (value));
  csRef<StandardChangeListener> changeListener;
  changeListener.AttachNew (new StandardChangeListener (v));
  value->AddValueChangeListener (changeListener);
  return v;
}

class AndValue : public BoolValue
{
private:
  csRef<Value> value1, value2;

public:
  AndValue (Value* value1, Value* value2) : value1 (value1), value2 (value2) { }
  virtual ~AndValue () { }
  virtual void SetBoolValue (bool fl) { }
  virtual bool GetBoolValue ()
  {
    return ValueToBoolStatic (value1) && ValueToBoolStatic (value2);
  }
};

csRef<Value> View::And (Value* value1, Value* value2)
{
  csRef<Value> v;
  v.AttachNew (new AndValue (value1, value2));
  csRef<StandardChangeListener> changeListener;
  changeListener.AttachNew (new StandardChangeListener (v));
  value1->AddValueChangeListener (changeListener);
  value2->AddValueChangeListener (changeListener);
  return v;
}

class OrValue : public BoolValue
{
private:
  csRef<Value> value1, value2;

public:
  OrValue (Value* value1, Value* value2) : value1 (value1), value2 (value2) { }
  virtual ~OrValue () { }
  virtual void SetBoolValue (bool fl) { }
  virtual bool GetBoolValue ()
  {
    return ValueToBoolStatic (value1) || ValueToBoolStatic (value2);
  }
};

csRef<Value> View::Or (Value* value1, Value* value2)
{
  csRef<Value> v;
  v.AttachNew (new OrValue (value1, value2));
  csRef<StandardChangeListener> changeListener;
  changeListener.AttachNew (new StandardChangeListener (v));
  value1->AddValueChangeListener (changeListener);
  value2->AddValueChangeListener (changeListener);
  return v;
}

// --------------------------------------------------------------------------

} // namespace Ares

