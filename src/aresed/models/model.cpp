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
    bufvalue->ValueChanged (value);
  }
};

// --------------------------------------------------------------------------

BufferedValue::BufferedValue (wxListCtrl* listCtrl, Value* collectionValue) :
     listCtrl (listCtrl), collectionValue (collectionValue)
{
  listCtrl->Connect (wxEVT_COMMAND_LIST_ITEM_SELECTED,
	  wxCommandEventHandler (BufferedValue :: OnSelectionChange), 0, this);
  selection = ListCtrlTools::GetFirstSelectedRow (listCtrl);
  currentValue = 0;
  changeListener.AttachNew (new BufferValueChangeListener (this));
  UpdateToSelection ();
}

BufferedValue::~BufferedValue ()
{
  listCtrl->Disconnect (wxEVT_COMMAND_LIST_ITEM_SELECTED,
	  wxCommandEventHandler (BufferedValue :: OnSelectionChange), 0, this);
}

void BufferedValue::UpdateToSelection ()
{
  if (currentValue)
    currentValue->RemoveValueChangeListener (changeListener);
  if (selection == -1)
    currentValue = 0;
  else
  {
    currentValue = collectionValue->GetChild (size_t (selection));
    currentValue->AddValueChangeListener (changeListener);
  }
}

void BufferedValue::ValueChanged (Value* value)
{
  CS_ASSERT (value == currentValue);
  FireValueChanged ();
}

void BufferedValue::OnSelectionChange (wxCommandEvent& event)
{
  long idx = ListCtrlTools::GetFirstSelectedRow (listCtrl);
  if (idx != selection)
  {
    selection = idx;
    UpdateToSelection ();
    FireValueChanged ();
  }
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

bool View::Bind (Value* value, wxTextCtrl* component)
{
  switch (value->GetType ())
  {
    case VALUE_STRING:
    case VALUE_LONG:
    case VALUE_BOOL:
    case VALUE_FLOAT:
      break;
    default:
      printf ("Unsupported value type for text control!\n");
      return false;
  }

  Binding b;
  b.value = value;
  b.component = wxStaticCast (component, wxWindow);
  b.eventType = wxEVT_COMMAND_TEXT_UPDATED;
  bindingsByComponent.Put (component, b);
  bindingsByValue.Put (value, b);
  component->Connect (wxEVT_COMMAND_TEXT_UPDATED,
      wxCommandEventHandler (View :: OnComponentChanged), 0, this);
  value->AddValueChangeListener (changeListener);
  ValueChanged (value);
  return true;
}

bool View::Bind (Value* value, wxPanel* component)
{
  if (value->GetType () != VALUE_COMPOSITE)
  {
    printf ("Unsupported value type for panels! Only VALUE_COMPOSITE is supported.\n");
    return false;
  }

  Binding b;
  b.value = value;
  b.component = wxStaticCast (component, wxWindow);
  bindingsByComponent.Put (component, b);
  bindingsByValue.Put (value, b);

  value->ResetIterator ();
  while (value->HasChildren ())
  {
    Value* child = value->NextChild ();
    csString name = child->GetName ();
    wxWindow* childComp = FindComponentByName (parent, name);
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
  value->AddValueChangeListener (changeListener);
  ValueChanged (value);
  return true;
}

bool View::Bind (Value* value, wxListCtrl* component)
{
  if (value->GetType () != VALUE_COLLECTION)
  {
    printf ("Unsupported value type for lists! Only VALUE_COLLECTION is supported.\n");
    return false;
  }

  Binding b;
  b.value = value;
  b.component = wxStaticCast (component, wxWindow);
  bindingsByComponent.Put (component, b);
  bindingsByValue.Put (value, b);
  value->AddValueChangeListener (changeListener);
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
  else
  {
    printf ("OnComponentChanged: this type of component not yet supported!\n");
  }
}

void View::ValueChanged (Value* value)
{
  Binding b;
  const Binding& binding = bindingsByValue.Get (value, b);
  if (!binding.value)
  {
    printf ("ValueChanged: Something went wrong! Called without a value!\n");
    return;
  }
  if (binding.component->IsKindOf (CLASSINFO (wxTextCtrl)))
  {
    wxTextCtrl* textCtrl = wxStaticCast (binding.component, wxTextCtrl);
    csString text = ValueToString (value);
    textCtrl->SetValue (wxString::FromUTF8 (text));
  }
  else if (binding.component->IsKindOf (CLASSINFO (wxPanel)))
  {
    // @@@ TODO: what if a composite value changes? Can we update
    // the bindings to the panel?
  }
  else if (binding.component->IsKindOf (CLASSINFO (wxListCtrl)))
  {
    wxListCtrl* listCtrl = wxStaticCast (binding.component, wxListCtrl);
    listCtrl->DeleteAllItems ();
    ListHeading lhdef;
    const ListHeading& lh = listToHeading.Get (listCtrl, lhdef);
    value->ResetIterator ();
    while (value->HasChildren ())
    {
      Value* child = value->NextChild ();
      ListCtrlTools::AddRow (listCtrl, ConstructListRow (lh, child));
    }
  }
  else
  {
    printf ("ValueChanged: this type of component not yet supported!\n");
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

