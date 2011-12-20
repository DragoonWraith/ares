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

#ifndef __appares_model_h
#define __appares_model_h

#include <csutil/hash.h>
#include <csutil/stringarray.h>

#include <wx/event.h>

class wxWindow;
class wxTextCtrl;
class wxCheckBox;
class wxListCtrl;
class wxStaticText;
class wxPanel;

namespace Ares
{

class Value;
class BufferValueChangeListener;
class ViewValueChangeListener;
class SelectedValueChangeListener;

/**
 * Listen to changes in a value.
 */
struct ValueChangeListener : public csRefCount
{
  /**
   * Called if the value changes.
   */
  virtual void ValueChanged (Value* value) = 0;
};

/**
 * All possible value types.
 */
enum ValueType
{
  /// No type.
  VALUE_NONE = 0,

  /// The basic types.
  VALUE_STRING,
  VALUE_LONG,
  VALUE_BOOL,
  VALUE_FLOAT,

  /**
   * A collection of values. In this situation the
   * children have no specific name and can only
   * be accessed by iterating over them.
   */
  VALUE_COLLECTION,

  /**
   * A collection of values. In this situation the
   * children can also be accessed by name and
   * it is not possible to delete or add children.
   */
  VALUE_COMPOSITE,
};


/**
 * An interface representing a domain value.
 */
class Value : public csRefCount
{
protected:
  csRefArray<ValueChangeListener> listeners;

  void FireValueChanged ()
  {
    for (size_t i = 0 ; i < listeners.GetSize () ; i++)
      listeners[i]->ValueChanged (this);
  }

public:
  /**
   * Get the type of the value.
   */
  virtual ValueType GetType () const { return VALUE_STRING; }

  /**
   * Get the value if the type is VALUE_STRING.
   * Returns 0 if the value does not represent a string.
   */
  virtual const char* GetStringValue () { return 0; }

  /**
   * Set the value as a string. Does nothing if the value does
   * not represent a string.
   */
  virtual void SetStringValue (const char* str) { }

  /**
   * Get the value if the type is VALUE_LONG.
   * Returns 0 if the value does not represent a long.
   */
  virtual long GetLongValue () { return 0; }

  /**
   * Set the value as a long. Does nothing if the value does
   * not represent a long.
   */
  virtual void SetLongValue (long v) { }

  /**
   * Get the value if the type is VALUE_BOOL.
   * Returns false if the value does not represent a bool.
   */
  virtual bool GetBoolValue () { return false; }

  /**
   * Set the value as a bool. Does nothing if the value does
   * not represent a bool.
   */
  virtual void SetBoolValue (bool v) { }

  /**
   * Get the value if the type is VALUE_FLOAT.
   * Returns 0.0f if the value does not represent a float.
   */
  virtual float GetFloatValue () { return 0.0f; }

  /**
   * Set the value as a float. Does nothing if the value does
   * not represent a float.
   */
  virtual void SetFloatValue (float v) { }

  /**
   * Add a listener for value changes. Note that a value does not
   * have to support this.
   */
  virtual void AddValueChangeListener (ValueChangeListener* listener)
  {
    listeners.Push (listener);
  }
  
  /**
   * Remove a listener.
   */
  virtual void RemoveValueChangeListener (ValueChangeListener* listener)
  {
    listeners.Delete (listener);
  }

  // -----------------------------------------------------------------------------

  /**
   * If the type of this value is VALUE_COLLECTION or VALUE_COMPOSITE,
   * then you can get the children using ResetIterator(), HasNext(),
   * and NextChild().
   */
  virtual void ResetIterator () { }

  /**
   * Check if there are still children to process.
   */
  virtual bool HasNext () { return false; }

  /**
   * Get the next child. If the optional 'name' parameter
   * is given then it will be filled with the name of the component (if
   * it has a name). Only components of type VALUE_COMPOSITE support names
   * for their children.
   */
  virtual Value* NextChild (csString* name = 0) { return 0; }

  /**
   * If the type of this value is VALUE_COMPOSITE or VALUE_COLLECTION then you
   * can sometimes get a child by index here.
   */
  virtual Value* GetChild (size_t idx) { return 0; }

  /**
   * If the type of this value is VALUE_COMPOSITE then you can get
   * a child by name here.
   */
  virtual Value* GetChild (const char* name) { return 0; }

  // -----------------------------------------------------------------------------

  /**
   * Delete a child value from this value. Returns false if this value could not
   * be deleted for some reason. This only works for VALUE_COLLECTION.
   */
  virtual bool DeleteValue (Value* child) { return false; }

  /**
   * Add a child value to this value. Returns false if this value could
   * not be added for some reason. This only works for VALUE_COLLECTION.
   */
  virtual bool AddValue (Value* child) { return false; }

  /**
   * Update a child. Returns false if this child could not be updated.
   * The default implementation just calls DeleteChild() first and then
   * AddChild() with the new child. This only works for VALUE_COLLECTION.
   */
  virtual bool UpdateValue (Value* oldChild, Value* child)
  {
    if (!DeleteValue (oldChild)) return false;
    if (!AddValue (child))
    {
      AddValue (oldChild);
      return false;
    }
    return true;
  }
};

/**
 * A constant null value.
 */
class NullValue : public Value
{
public:
  virtual ValueType GetType () const { return VALUE_NONE; }
};

/**
 * An abstract composite value which supports iteration and
 * selection of the children by name. This is an abstract
 * class that has to be subclassed in order to provide the
 * name and value of every child by index.
 * Subclasses must also override the standard Value::GetChild(idx).
 */
class AbstractCompositeValue : public Value
{
private:
  size_t idx;

  using Value::GetChild;

protected:
  /// Override this function to indicate the number of children.
  virtual size_t GetChildCount () = 0;
  /// Override this function to return the right name for a given child.
  virtual const char* GetName (size_t idx) = 0;

public:
  AbstractCompositeValue () { }
  virtual ~AbstractCompositeValue () { }

  virtual ValueType GetType () const { return VALUE_COMPOSITE; }

  virtual void ResetIterator () { idx = 0; }
  virtual bool HasNext () { return idx < GetChildCount (); }
  virtual Value* NextChild (csString* name = 0)
  {
    if (name) *name = GetName (idx);
    idx++;
    return GetChild (idx-1);
  }
  virtual Value* GetChild (const char* name)
  {
    csString sname = name;
    for (size_t i = 0 ; i < GetChildCount () ; i++)
      if (sname == GetName (i)) return GetChild (i);
    return 0;
  }
};

/**
 * A standard composite value which maintains a list of
 * names and children itself.
 */
class CompositeValue : public AbstractCompositeValue
{
private:
  csStringArray names;
  csRefArray<Value> children;

protected:
  virtual size_t GetChildCount () { return children.GetSize (); }
  virtual const char* GetName (size_t idx) { return names[idx]; }

public:
  CompositeValue () { }
  virtual ~CompositeValue () { }

  /**
   * Add a child with name to this composite.
   */
  void AddChild (const char* name, Value* value)
  {
    names.Push (name);
    children.Push (value);
  }
  /**
   * Remove all children from this composite.
   */
  void DeleteAll ()
  {
    names.DeleteAll ();
    children.DeleteAll ();
  }

  virtual Value* GetChild (size_t idx) { return children[idx]; }
};

/**
 * A value implementation that directly refers to a float at a given
 * location.
 */
class FloatPointerValue : public Value
{
private:
  float* flt;

public:
  FloatPointerValue (float* f) : flt (f) { }
  virtual ~FloatPointerValue () { }

  virtual ValueType GetType () const { return VALUE_FLOAT; }
  virtual float GetFloatValue () { return *flt; }
  virtual void SetFloatValue (float v)
  {
    if (v == *flt) return;
    *flt = v;
    FireValueChanged ();
  }
};

/**
 * A buffered value is a value that is synchronized with another
 * value but is able to keep changes which can be put back into
 * the original value on request (for example when the user presses
 * 'Save' or 'Apply' to copy the contents of the user interface to
 * the real value).
 */
class BufferedValue : public Value
{
  friend class BufferValueChangeListener;

protected:
  csRef<Value> originalValue;
  bool dirty;

  csRef<BufferValueChangeListener> changeListener;

public:
  BufferedValue (Value* originalValue);
  virtual ~BufferedValue ();

  /**
   * Called when the original value changes.
   * Subclasses of BufferedValue should implement this method
   * so that they can overwrite the internal buffer they keep.
   */
  virtual void ValueChanged () = 0;

  /**
   * Return true if the buffer is dirty compared to the actual
   * real original value.
   */
  virtual bool IsDirty () const { return dirty; }

  /**
   * Call this to apply the value in the buffer to the real value.
   * This will also clear the dirty flag. Subclasses should override
   * this in order to correctly copy the internal buffer to the original value.
   * Subclasses can first test for the dirty flag. If dirty is false
   * they don't have to do anything. Additionaly subclasses should set
   * this flag to false.
   */
  virtual void Apply () = 0;

  /**
   * Factory method to create buffered values of the appropriate type.
   */
  static csRef<BufferedValue> CreateBufferedValue (Value* originalValue);

  virtual ValueType GetType () const { return originalValue->GetType (); }
};

/**
 * A string buffered value.
 */
class StringBufferedValue : public BufferedValue
{
private:
  csString buffer;

public:
  StringBufferedValue (Value* originalValue) : BufferedValue (originalValue)
  {
    CS_ASSERT (originalValue->GetType () == VALUE_STRING);
    ValueChanged ();
  }
  virtual ~StringBufferedValue () { }

  virtual void ValueChanged ()
  {
    buffer = originalValue->GetStringValue ();
    dirty = false;
  }
  virtual void Apply ()
  {
    if (!dirty) return;
    dirty = false;
    originalValue->SetStringValue (buffer);
  }

  virtual const char* GetStringValue () { return buffer; }
  virtual void SetStringValue (const char* str)
  {
    dirty = (buffer != str);
    buffer = str;
    FireValueChanged ();
  }
};

/**
 * A composite buffered value.
 */
class CompositeBufferedValue : public BufferedValue
{
private:
  csStringArray names;
  csRefArray<BufferedValue> buffer;
  size_t idx;	// For the iterator.

public:
  CompositeBufferedValue (Value* originalValue) : BufferedValue (originalValue)
  {
    CS_ASSERT (originalValue->GetType () == VALUE_COMPOSITE);
    ValueChanged ();
  }
  virtual ~CompositeBufferedValue () { }

  virtual void ValueChanged ();
  virtual void Apply ();

  virtual void ResetIterator () { idx = 0; }
  virtual bool HasNext () { return idx < buffer.GetSize (); }
  virtual Value* NextChild (csString* name = 0)
  {
    if (name) *name = names[idx];
    idx++;
    return buffer[idx-1];
  }
  virtual Value* GetChild (size_t idx) { return buffer[idx]; }
  virtual Value* GetChild (const char* name);
};

/**
 * A collection buffered value.
 */
class CollectionBufferedValue : public BufferedValue
{
private:
  csRefArray<BufferedValue> buffer;
  csHash<csPtrKey<BufferedValue>,csPtrKey<Value> > originalToBuffered;
  csHash<csPtrKey<Value>,csPtrKey<BufferedValue> > bufferedToOriginal;

  csRefArray<Value> newvalues;
  csRefArray<Value> deletedvalues;

  size_t idx;	// For the iterator.

public:
  CollectionBufferedValue (Value* originalValue) : BufferedValue (originalValue)
  {
    CS_ASSERT (originalValue->GetType () == VALUE_COLLECTION);
    ValueChanged ();
  }
  virtual ~CollectionBufferedValue () { }

  virtual void ValueChanged ();
  virtual void Apply ();

  virtual void ResetIterator () { idx = 0; }
  virtual bool HasNext () { return idx < buffer.GetSize (); }
  virtual Value* NextChild (csString* name = 0) { idx++; return buffer[idx-1]; }
  virtual Value* GetChild (size_t idx) { return buffer[idx]; }

  virtual bool DeleteValue (Value* child);
  virtual bool AddValue (Value* child);
  //virtual bool UpdateValue (Value* oldChild, Value* child);
};

/**
 * This value exactly mirrors a value as it is selected in a list.
 * When the selection changes this value will automatically change
 * and making changes to this value will automatically reflect on
 * to the value in the list.
 */
class SelectedValue : public wxEvtHandler, public Value
{
  friend class SelectedValueChangeListener;

private:
  wxListCtrl* listCtrl;
  csRef<Value> collectionValue;

  /// The selected item in the list.
  long selection;
  /// The value from the collection we are showing.
  Value* currentValue;
  NullValue nullValue;

  csRef<SelectedValueChangeListener> changeListener;

  // Called when the value from the collection changes.
  void ValueChanged ();

  void OnSelectionChange (wxCommandEvent& event);
  void UpdateToSelection ();

public:
  SelectedValue (wxListCtrl* listCtrl, Value* collectionValue);
  virtual ~SelectedValue ();

  virtual ValueType GetType () const { return currentValue->GetType (); }
  virtual const char* GetStringValue () { return currentValue->GetStringValue (); }
  virtual void SetStringValue (const char* str) { currentValue->SetStringValue (str); }
  virtual long GetLongValue () { return currentValue->GetLongValue (); }
  virtual void SetLongValue (long v) { currentValue->SetLongValue (v); }
  virtual bool GetBoolValue () { return currentValue->GetBoolValue (); }
  virtual void SetBoolValue (bool v) { currentValue->SetBoolValue (v); }
  virtual float GetFloatValue () { return currentValue->GetFloatValue (); }
  virtual void SetFloatValue (float v) { currentValue->SetFloatValue (v); }
  virtual void ResetIterator () { currentValue->ResetIterator (); }
  virtual bool HasNext () { return currentValue->HasNext (); }
  virtual Value* NextChild (csString* name = 0) { return currentValue->NextChild (name); }
  virtual Value* GetChild (size_t idx) { return currentValue->GetChild (idx); }
  virtual Value* GetChild (const char* name) { return currentValue->GetChild (name); }
  virtual bool DeleteValue (Value* child) { return currentValue->DeleteValue (child); }
  virtual bool AddValue (Value* child) { return currentValue->AddValue (child); }
  virtual bool UpdateValue (Value* oldChild, Value* child)
  {
    return currentValue->UpdateValue (oldChild, child);
  }
};

/**
 * A view. This class keeps track of the bindings between
 * models and WX controls for a given logical unit (frame, dialog, panel, ...).
 */
class View : public wxEvtHandler, public csRefCount
{
  friend class ViewValueChangeListener;

private:
  wxWindow* parent;

  // All the bindings.
  struct Binding
  {
    csRef<Value> value;
    wxWindow* component;
    wxEventType eventType;
    Binding () : component (0), eventType (wxEVT_NULL) { }
  };
  typedef csHash<Binding,csPtrKey<wxWindow> > ComponentToBinding;
  typedef csHash<Binding,csPtrKey<Value> > ValueToBinding;
  ComponentToBinding bindingsByComponent;
  ValueToBinding bindingsByValue;

  // Listeners.
  csRef<ViewValueChangeListener> changeListener;

  // Keep track of list headings.
  struct ListHeading
  {
    csStringArray heading;
    csStringArray names;
  };
  typedef csHash<ListHeading,csPtrKey<wxListCtrl> > ListToHeading;
  ListToHeading listToHeading;

  /**
   * Construct a value (composite or single) to a string array as
   * compatible for a given list.
   */
  csStringArray ConstructListRow (const ListHeading& lh, Value* value);

  /// Called by components when they change. Will update the corresponding Value.
  void OnComponentChanged (wxCommandEvent& event);
  /// Called by values when they change. Will update the corresponding component.
  void ValueChanged (Value* value);

  /**
   * Recursively find a component by name. Also supports the '_' notation
   * in the name of the components.
   */
  wxWindow* FindComponentByName (wxWindow* container, const char* name);

public:
  View (wxWindow* parent);
  ~View ();

  /**
   * Bind a value to a WX component. This function will try to find the
   * best match between the given value and the component.
   * Can fail (return false) under the following conditions:
   * - Component has an unsupported type.
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxWindow* component);

  /**
   * Bind a value to a WX component. This function will try to find the
   * best match between the given value and the component.
   * Can fail (return false) under the following conditions:
   * - Component could not be found by this name.
   * - Component has an unsupported type.
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, const char* compName);

  /**
   * Bind a value directly to a text control.
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxTextCtrl* component);

  /**
   * Bind a value directly to a panel. This only works with
   * values of type VALUE_COMPOSITE. Note that it will try
   * to find GUI components on the panel by scanning the names.
   * If a name of such a component contains an underscore ('_')
   * then it will only look at the part of the name before the
   * underscore.
   * The result of binding a value with a panel is that the
   * children of the value will be bound with corresponding
   * interface components on the panel.
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxPanel* component);

  /**
   * Bind a value directly to a list control. This only works with
   * values of type VALUE_COLLECTION.
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxListCtrl* component);

  /**
   * Define a heading for a list control. For every column in the list
   * there is a logical name which will be used as the name of the
   * child value in case the list is populated with composite values.
   * Every column also has a heading which will be used as a heading
   * on the list component.
   * @param heading is a comma separated string with the heading for the list.
   * @param names is a comma separated string with the names of the children.
   * @return false on failure (component could not be found or is not a list).
   */
  bool DefineHeading (const char* listName, const char* heading,
      const char* names);
  bool DefineHeading (wxListCtrl* listCtrl, const char* heading,
      const char* names);
};

} // namespace Ares

#endif // __appares_model_h

