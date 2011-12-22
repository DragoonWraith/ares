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

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/listbox.h>
#include <wx/choicebk.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

namespace Ares
{

// --------------------------------------------------------------------------

class ViewValueChangeListener : public ValueChangeListener
{
private:
  View* view;

public:
  ViewValueChangeListener (View* view) : view (view) { }
  virtual ~ViewValueChangeListener () { }
  virtual void ValueChanged (Value* value)
  {
    view->ValueChanged (value);
  }
};

// --------------------------------------------------------------------------

class BufferValueChangeListener : public ValueChangeListener
{
private:
  BufferedValue* bufvalue;

public:
  BufferValueChangeListener (BufferedValue* bufvalue) : bufvalue (bufvalue) { }
  virtual ~BufferValueChangeListener () { }
  virtual void ValueChanged (Value* value)
  {
    bufvalue->ValueChanged ();
  }
};

// --------------------------------------------------------------------------

class MirrorValueChangeListener : public ValueChangeListener
{
private:
  MirrorValue* selvalue;

public:
  MirrorValueChangeListener (MirrorValue* selvalue) : selvalue (selvalue) { }
  virtual ~MirrorValueChangeListener () { }
  virtual void ValueChanged (Value* value)
  {
    selvalue->ValueChanged ();
  }
};

// --------------------------------------------------------------------------

BufferedValue::BufferedValue (Value* originalValue) : originalValue (originalValue)
{
  changeListener.AttachNew (new BufferValueChangeListener (this));
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

void AbstractCompositeValue::ListenToChildren ()
{
  csRef<ChangeListener> listener;
  listener.AttachNew (new ChangeListener (this));
  for (size_t i = 0 ; i < GetChildCount () ; i++)
    GetChild (i)->AddValueChangeListener (listener);
}

// --------------------------------------------------------------------------

void StandardCollectionValue::ListenToChildren ()
{
  csRef<ChangeListener> listener;
  listener.AttachNew (new ChangeListener (this));
  for (size_t i = 0 ; i < children.GetSize () ; i++)
    children[i]->AddValueChangeListener (listener);
}

// --------------------------------------------------------------------------

MirrorValue::MirrorValue (ValueType type) : type (type), idx (0)
{
  changeListener.AttachNew (new MirrorValueChangeListener (this));
  mirroringValue = &nullValue;
}

MirrorValue::~MirrorValue ()
{
  SetMirrorValue (0);
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
  FireValueChanged ();
}

// --------------------------------------------------------------------------

SelectedValue::SelectedValue (wxListCtrl* listCtrl, Value* collectionValue, ValueType type) :
  MirrorValue (type), listCtrl (listCtrl), collectionValue (collectionValue)
{
  listCtrl->Connect (wxEVT_COMMAND_LIST_ITEM_SELECTED,
	  wxCommandEventHandler (SelectedValue :: OnSelectionChange), 0, this);
  listCtrl->Connect (wxEVT_COMMAND_LIST_ITEM_DESELECTED,
	  wxCommandEventHandler (SelectedValue :: OnSelectionChange), 0, this);
  selection = ListCtrlTools::GetFirstSelectedRow (listCtrl);
  UpdateToSelection ();
}

SelectedValue::~SelectedValue ()
{
  listCtrl->Disconnect (wxEVT_COMMAND_LIST_ITEM_SELECTED,
	  wxCommandEventHandler (SelectedValue :: OnSelectionChange), 0, this);
  listCtrl->Disconnect (wxEVT_COMMAND_LIST_ITEM_DESELECTED,
	  wxCommandEventHandler (SelectedValue :: OnSelectionChange), 0, this);
}

void SelectedValue::UpdateToSelection ()
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

void SelectedValue::OnSelectionChange (wxCommandEvent& event)
{
  long idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
  selection = idx;
  UpdateToSelection ();
}

// --------------------------------------------------------------------------

View::View (wxWindow* parent) : parent (parent)
{
  changeListener.AttachNew (new ViewValueChangeListener (this));
}

View::~View ()
{
  ComponentToBinding::GlobalIterator it = bindingsByComponent.GetIterator ();
  while (it.HasNext ())
  {
    csPtrKey<wxWindow> component;
    Binding binding = it.Next (component);
    if (binding.eventType == wxEVT_COMMAND_TEXT_UPDATED)
      component->Disconnect (binding.eventType,
	  wxCommandEventHandler (View :: OnComponentChanged), 0, this);
    else if (binding.eventType == wxEVT_COMMAND_LIST_ITEM_SELECTED)
      component->Disconnect (binding.eventType,
	  wxCommandEventHandler (View :: OnComponentChanged), 0, this);
  }
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
  if (component->IsKindOf (CLASSINFO (wxChoicebook)))
    return Bind (value, wxStaticCast (component, wxChoicebook));
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
  Binding b;
  b.value = value;
  b.component = component;
  b.eventType = eventType;
  bindingsByComponent.Put (component, b);
  if (!bindingsByValue.Contains (value))
    bindingsByValue.Put (value, csArray<Binding> ());
  csArray<Binding> temp;
  bindingsByValue.Get (value, temp).Push (b);
  CS_ASSERT (temp.GetSize () == 0);
  if (eventType != wxEVT_NULL)
    component->Connect (eventType, wxCommandEventHandler (View :: OnComponentChanged), 0, this);
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
  // know the type yet (for example when the value is a SelectedValue and nothing has been
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
  // know the type yet (for example when the value is a SelectedValue and nothing has been
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

void View::OnComponentChanged (wxCommandEvent& event)
{
  wxWindow* component = wxStaticCast (event.GetEventObject (), wxWindow);
  Binding b;
  const Binding& binding = bindingsByComponent.Get (component, b);
  if (!binding.value)
  {
    printf ("OnComponentChanged: Something went wrong! Called without a value!\n");
    return;
  }
  if (component->IsKindOf (CLASSINFO (wxTextCtrl)))
  {
    wxTextCtrl* textCtrl = wxStaticCast (component, wxTextCtrl);
    csString text = (const char*)textCtrl->GetValue ().mb_str (wxConvUTF8);
    StringToValue (text, binding.value);
  }
  else if (component->IsKindOf (CLASSINFO (wxChoicebook)))
  {
    wxChoicebook* choicebook = wxStaticCast (component, wxChoicebook);
    int pageSel = choicebook->GetSelection ();
    if (binding.value->GetType () == VALUE_LONG)
      LongToValue ((long)pageSel, binding.value);
    else
    {
      csString value;
      if (pageSel == wxNOT_FOUND) value = "";
      else
      {
        wxString pageTxt = choicebook->GetPageText (pageSel);
        value = (const char*)pageTxt.mb_str (wxConvUTF8);
      }
      StringToValue (value, binding.value);
    }
  }
  else
  {
    printf ("OnComponentChanged: this type of component not yet supported!\n");
  }
}

void View::ValueChanged (Value* value)
{
  csArray<Binding> b;
  const csArray<Binding>& bindings = bindingsByValue.Get (value, b);
  if (!bindings.GetSize ())
  {
    printf ("ValueChanged: Something went wrong! Called without a value!\n");
    return;
  }
  for (size_t i = 0 ; i < bindings.GetSize () ; i++)
  {
    wxWindow* comp = bindings[i].component;
    if (comp->IsKindOf (CLASSINFO (wxTextCtrl)))
    {
      wxTextCtrl* textCtrl = wxStaticCast (comp, wxTextCtrl);
      csString text = ValueToString (value);
      textCtrl->SetValue (wxString::FromUTF8 (text));
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
      listCtrl->DeleteAllItems ();
      ListHeading lhdef;
      const ListHeading& lh = listToHeading.Get (listCtrl, lhdef);
      long idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
      value->ResetIterator ();
      while (value->HasNext ())
      {
	Value* child = value->NextChild ();
	ListCtrlTools::AddRow (listCtrl, ConstructListRow (lh, child));
      }
      ListCtrlTools::SelectRow (listCtrl, idx, true);
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

// --------------------------------------------------------------------------

} // namespace Ares

