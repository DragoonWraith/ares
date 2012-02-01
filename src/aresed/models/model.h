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
#include <wx/treectrl.h>

class wxWindow;
class wxTextCtrl;
class wxCheckBox;
class wxListCtrl;
class wxTreeCtrl;
class wxStaticText;
class wxButton;
class wxChoicebook;
class wxPanel;
class wxDialog;
class CustomControl;
class UIDialog;

// @@@ Ugly: we can't include uimanager.h because of circular includes.
typedef csHash<csString,csString> DialogResult;

/**
 * This macro creates a new reference for a certain type and makes sure
 * the reference is decremented after it has been given to the function.
 * This macro should be used when you want to make a new object, pass it to
 * some function that itself maintains its own reference and then you
 * immediatelly want to release the reference you own.
 * e.g.: view->AddAction (component, NEWREF (new MyAction ()));
 */
#define NEWREF(T,N) (csRef<T> (csPtr<T> (N)))

namespace Ares
{

#define DO_DEBUG 0

class Value;
class View;

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
  Value* parent;
  csRefArray<ValueChangeListener> listeners;
  bool dirty;

  /**
   * Called when a child changes. Default implementation does
   * nothing.
   */
  virtual void ChildChanged (Value* child) { }

public:
  Value () : parent (0), dirty (true) { }

  /**
   * Notify this value as if it has changed. This will cause
   * the listeners and parent to be notified.
   */
  virtual void FireValueChanged ()
  {
#if DO_DEBUG
    printf ("FireValueChanged: %s\n", Dump ().GetData ());
#endif
    for (size_t i = 0 ; i < listeners.GetSize () ; i++)
      listeners[i]->ValueChanged (this);
    if (parent) parent->ChildChanged (this);
  }

  /**
   * Force a refresh of the data. The default implementation sets a dirty
   * flag and calls FireValueChanged().
   */
  virtual void Refresh () { dirty = true; FireValueChanged (); }

  /**
   * Set the parent of this value.
   */
  virtual void SetParent (Value* parent) { Value::parent = parent; }

  /**
   * Get the parent of this value.
   */
  virtual Value* GetParent () const { return parent; }

  /**
   * Get the type of the value.
   */
  virtual ValueType GetType () const { return VALUE_STRING; }

  /**
   * Get the value if the type is VALUE_STRING.
   * Returns 0 if the value does not represent a string.
   * Exception: if the value is a node in a model then it will
   * be a collection but also return a string representing the
   * name of this node to show in the tree.
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
  virtual Value* GetChildByName (const char* name) { return 0; }

  /**
   * Check if a given value is a direct child of this value.
   */
  virtual bool IsChild (Value* value);

  // -----------------------------------------------------------------------------

  /**
   * Delete a child value from this value. Returns false if this value could not
   * be deleted for some reason. This only works for VALUE_COLLECTION.
   */
  virtual bool DeleteValue (Value* child) { return false; }

  /**
   * Create a new empty value in this value. Returns 0 if this value
   * could not be created for some reason. This only works for VALUE_COLLECTION.
   *
   * The given index is the position at which the new value should be created
   * but this is only a hint. If idx is equal to csArrayItemNotFound then
   * it means after the end of the collection.
   *
   * The selectedValue is a pointer to the value as it is selected in this
   * collection. Can be 0 if nothing is selected. This is useful (for example) in
   * case of a tree where you want the new item to be added to the selected node.
   *
   * The suggestion is a hash of named string values that can serve as a suggestion
   * for creating the new value. Typically this will be the result of a dialog.
   * The format of this suggestion array is application dependend and sometimes it can
   * be an empty container.
   */
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  { return 0; }

  // -----------------------------------------------------------------------------

  /**
   * Make a debug dump of this value. Subclasses can override this
   * to provide more detail.
   * If 'verbose' is false (default) then the returned string should be one-line
   * and can't contain any newlines. If 'verbose' is true then it can have multiple
   * lines but the last line should also have no newline.
   */
  virtual csString Dump (bool verbose = false);
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
 * A string value.
 */
class StringValue : public Value
{
private:
  csString str;

public:
  StringValue (const char* str = "") : str (str) { }
  virtual ~StringValue () { }
  virtual ValueType GetType () const { return VALUE_STRING; }
  virtual void SetStringValue (const char* s) { if (str != s) { str = s; FireValueChanged (); } }
  virtual const char* GetStringValue () { return str; }
};

/**
 * A float value.
 */
class FloatValue : public Value
{
private:
  float f;

public:
  FloatValue (float f = 0.0f) : f (f) { }
  virtual ~FloatValue () { }
  virtual ValueType GetType () const { return VALUE_FLOAT; }
  virtual void SetFloatValue (float fl) { if (fl != f) { f = fl; FireValueChanged (); } }
  virtual float GetFloatValue () { return f; }
};

/**
 * A long value.
 */
class LongValue : public Value
{
private:
  long f;

public:
  LongValue (long f = 0) : f (f) { }
  virtual ~LongValue () { }
  virtual ValueType GetType () const { return VALUE_LONG; }
  virtual void SetLongValue (long fl) { if (fl != f) { f = fl; FireValueChanged (); } }
  virtual long GetLongValue () { return f; }
};

/**
 * A bool value.
 */
class BoolValue : public Value
{
private:
  bool f;

public:
  BoolValue (bool f = false) : f (f) { }
  virtual ~BoolValue () { }
  virtual ValueType GetType () const { return VALUE_BOOL; }
  virtual void SetBoolValue (bool fl) { if (fl != f) { f = fl; FireValueChanged (); } }
  virtual bool GetBoolValue () { return f; }
};

/**
 * A standard collection value which is based on an array of values
 * and which supports iteration and of the children. Subclasses of
 * this class can implement UpdateChildren() in order to fill the
 * array of children the first time iteration is done or when
 * a child is fetched by index.
 */
class StandardCollectionValue : public Value
{
private:
  size_t idx;

protected:
  csRefArray<Value> children;

  /**
   * Override this function to update the children.
   * Default implementation does nothing.
   */
  virtual void UpdateChildren () { }

  /**
   * Release the current children and remove them. UpdateChildren() should call
   * this first if it wants to replace the array of children.
   */
  void ReleaseChildren ()
  {
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      children[i]->SetParent (0);
    children.DeleteAll ();
  }

public:
  StandardCollectionValue () { }
  virtual ~StandardCollectionValue () { }

  virtual ValueType GetType () const { return VALUE_COLLECTION; }

  virtual void ResetIterator () { UpdateChildren (); idx = 0; }
  virtual bool HasNext () { return idx < children.GetSize (); }
  virtual Value* NextChild (csString* name = 0)
  {
    idx++;
    return children[idx-1];
  }
  virtual Value* GetChild (size_t idx)
  {
    UpdateChildren ();
    return children[idx];
  }
  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[*]";
    dump += Value::Dump (verbose);
    return dump;
  }
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
  virtual Value* GetChildByName (const char* name)
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
  virtual ~CompositeValue () { DeleteAll (); }

  /**
   * Add a child with name to this composite.
   */
  void AddChild (const char* name, Value* value)
  {
    names.Push (name);
    children.Push (value);
    value->SetParent (this);
  }
  /**
   * Remove all children from this composite.
   */
  void DeleteAll ()
  {
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      children[i]->SetParent (0);
    names.DeleteAll ();
    children.DeleteAll ();
  }

  virtual Value* GetChild (size_t idx) { return children[idx]; }
  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[#]";
    dump += AbstractCompositeValue::Dump (verbose);
    return dump;
  }
};

/**
 * A value implementation that directly refers to a bool at a given
 * location.
 */
class BoolPointerValue : public Value
{
private:
  bool* bl;

public:
  BoolPointerValue (bool* f) : bl (f) { }
  virtual ~BoolPointerValue () { }

  virtual ValueType GetType () const { return VALUE_BOOL; }
  virtual bool GetBoolValue () { return *bl; }
  virtual void SetBoolValue (bool v)
  {
    if (v == *bl) return;
    *bl = v;
    FireValueChanged ();
  }
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
 * A standard change listener that notifies another value when
 * a value has changed.
 */
class StandardChangeListener : public ValueChangeListener
{
private:
  Value* value;
public:
  StandardChangeListener (Value* value) : value (value) { }
  virtual ~StandardChangeListener () { }
  virtual void ValueChanged (Value*) { value->FireValueChanged (); }
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
protected:
  csRef<Value> originalValue;
  bool dirty;

  class BufChangeListener : public ValueChangeListener
  {
  private:
    BufferedValue* value;
  public:
    BufChangeListener (BufferedValue* value) : value (value) { }
    virtual ~BufChangeListener () { }
    virtual void ValueChanged (Value*) { value->ValueChanged (); }
  };

  csRef<BufChangeListener> changeListener;

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

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Buf]";
    dump += Value::Dump (verbose);
    return dump;
  }
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
#if DO_DEBUG
    printf ("ValueChanged: %s\n", Dump ().GetData ());
#endif
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
  virtual Value* GetChildByName (const char* name);
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
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion);
};

/**
 * A mirroring value. This value refers to another value and
 * simply passes through the changes in both directions. It is useful
 * in case you want to make a binding between several values and a single
 * component and the decision on which value should be chosen depends
 * on other criteria. See ListSelectedValue and TreeSelectedValue for
 * examples of this.
 *
 * A MirrorValue also supports being a composite with mirrored children.
 */
class MirrorValue : public Value
{
private:
  ValueType type;
  csRef<Value> mirroringValue;
  NullValue nullValue;

  class SelChangeListener : public ValueChangeListener
  {
  private:
    MirrorValue* selvalue;
  public:
    SelChangeListener (MirrorValue* selvalue) : selvalue (selvalue) { }
    virtual ~SelChangeListener () { }
    virtual void ValueChanged (Value*) { selvalue->ValueChanged (); }
  };

  csRef<SelChangeListener> changeListener;

  // The following two are only used in case the MirrorValue represents a composite.
  csStringArray names;
  csRefArray<MirrorValue> children;
  size_t idx;

  // Called when the mirrored value changes.
  void ValueChanged ();

public:
  MirrorValue (ValueType type);
  virtual ~MirrorValue ();

  virtual void FireValueChanged ()
  {
    Value::FireValueChanged ();
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      children[i]->FireValueChanged ();
  }

  /**
   * Setup this composite based on another composite. It will
   * create mirrored children with the same type as the given
   * composite but the given value is not set as the current
   * mirroring value.
   */
  void SetupComposite (Value* compositeValue);

  /**
   * If this mirrored value represents a composite then this method
   * can be used to setup the children (in case you can't use SetupComposite()).
   * This setup must match exactly the setup of the value that we're mirroring.
   */
  void AddChild (const char* name, MirrorValue* value)
  {
    names.Push (name);
    children.Push (value);
    value->SetParent (this);
  }
  /**
   * Remove all children from this composite.
   */
  void DeleteAll ()
  {
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      children[i]->SetParent (0);
    names.DeleteAll ();
    children.DeleteAll ();
  }

  /**
   * Set the current value we are mirroring.
   */
  void SetMirrorValue (Value* value);
  /**
   * Get the current mirroring value.
   */
  Value* GetMirrorValue () const { return (const Value*)mirroringValue == &nullValue ? 0 : mirroringValue; }

  virtual ValueType GetType () const { return type; }
  virtual const char* GetStringValue () { return mirroringValue->GetStringValue (); }
  virtual void SetStringValue (const char* str) { mirroringValue->SetStringValue (str); }
  virtual long GetLongValue () { return mirroringValue->GetLongValue (); }
  virtual void SetLongValue (long v) { mirroringValue->SetLongValue (v); }
  virtual bool GetBoolValue () { return mirroringValue->GetBoolValue (); }
  virtual void SetBoolValue (bool v) { mirroringValue->SetBoolValue (v); }
  virtual float GetFloatValue () { return mirroringValue->GetFloatValue (); }
  virtual void SetFloatValue (float v) { mirroringValue->SetFloatValue (v); }

  virtual void ResetIterator () { idx = 0; }
  virtual bool HasNext () { return idx < children.GetSize (); }
  virtual Value* NextChild (csString* name = 0)
  {
    if (name) *name = names[idx];
    idx++;
    return children[idx-1];
  }
  virtual Value* GetChildByName (const char* name)
  {
    csString sname = name;
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      if (sname == names[i]) return children[i];
    return 0;
  }

  virtual bool DeleteValue (Value* child) { return mirroringValue->DeleteValue (child); }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    return mirroringValue->NewValue (idx, selectedValue, suggestion);
  }
  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Mirror]";
    dump += Value::Dump (verbose);
    return dump;
  }
};

class SelectedBoolValue;

/**
 * This value exactly mirrors a value as it is selected in a list.
 * When the selection changes this value will automatically change
 * and making changes to this value will automatically reflect
 * to the value in the list. Note that a ListSelectedValue is also
 * a MirrorValue and if it represents a composite then you need to
 * set it up (manually or with SetupComposite()).
 */
class ListSelectedValue : public wxEvtHandler, public MirrorValue
{
private:
  wxListCtrl* listCtrl;
  csRef<Value> collectionValue;
  csRef<SelectedBoolValue> selectedStateValue;

  /// The selected item in the list.
  long selection;

  void OnSelectionChange (wxCommandEvent& event);
  void UpdateToSelection ();

public:
  ListSelectedValue (wxListCtrl* listCtrl, Value* collectionValue, ValueType type);
  virtual ~ListSelectedValue ();

  /**
   * Get a representation of the selected state with this value.
   */
  Value* GetSelectedState ();
};

/**
 * This value exactly mirrors a value as it is selected in a tree.
 * When the selection changes this value will automatically change
 * and making changes to this value will automatically reflect
 * to the value in the tree. Note that a TreeSelectedValue is also
 * a MirrorValue and if it represents a composite then you need to
 * set it up (manually or with SetupComposite()).
 */
class TreeSelectedValue : public wxEvtHandler, public MirrorValue
{
private:
  wxTreeCtrl* treeCtrl;
  csRef<Value> collectionValue;

  /// The selected item in the tree.
  wxTreeItemId selection;

  void OnSelectionChange (wxCommandEvent& event);
  void UpdateToSelection ();

public:
  TreeSelectedValue (wxTreeCtrl* treeCtrl, Value* collectionValue, ValueType type);
  virtual ~TreeSelectedValue ();
};

/**
 * An action. An action can be coupled to a contact menu for a list
 * or tree or it can be coupled to a button.
 */
class Action : public csRefCount
{
public:
  Action () { }
  virtual ~Action () { }

  /**
   * The name of the action. Will be used as the label of the
   * action on the menu or button.
   */
  virtual const char* GetName () const = 0;
  
  /**
   * Perform the action. Returns false on failure.
   * It gets the view and the component on which the action was set as parameters.
   */
  virtual bool Do (View* view, wxWindow* component) = 0;
};

/**
 * Superclass for the classes that handle new children.
 */
class AbstractNewAction : public Action
{
protected:
  Value* collection;

  /// Do method that has an optional 'dialog' (dialog can be 0).
  bool DoDialog (View* view, wxWindow* component, UIDialog* dialog);

public:
  AbstractNewAction (Value* collection) : collection (collection) { }
  virtual ~AbstractNewAction () { }
};

/**
 * This standard action creates a new default child for a collection.
 * It assumes the collection supports the NewValue() method. It will
 * call the NewValue() method with an empty suggestion array.
 */
class NewChildAction : public AbstractNewAction
{
public:
  NewChildAction (Value* collection) : AbstractNewAction (collection) { }
  virtual ~NewChildAction () { }
  virtual const char* GetName () const { return "New"; }
  virtual bool Do (View* view, wxWindow* component);
};

/**
 * This standard action creates a new child for a collection based
 * on a suggestion from a dialog.
 * It assumes the collection supports the NewValue() method.
 */
class NewChildDialogAction : public AbstractNewAction
{
private:
  UIDialog* dialog;

public:
  NewChildDialogAction (Value* collection, UIDialog* dialog) :
    AbstractNewAction (collection), dialog (dialog) { }
  virtual ~NewChildDialogAction () { }
  virtual const char* GetName () const { return "New..."; }
  virtual bool Do (View* view, wxWindow* component);
};

/**
 * This standard action removes a child from a collection.
 * It assumes the collection supports the DeleteValue() method.
 */
class DeleteChildAction : public Action
{
private:
  Value* collection;

public:
  DeleteChildAction (Value* collection) : collection (collection) { }
  virtual ~DeleteChildAction () { }
  virtual const char* GetName () const { return "Delete"; }
  virtual bool Do (View* view, wxWindow* component);
};

/**
 * A view. This class keeps track of the bindings between
 * models and WX controls for a given logical unit (frame, dialog, panel, ...).
 */
class View : public csRefCount
{
private:
  wxWindow* parent;
  int lastContextID;

  // --------------------------------------------

  // All the bindings.
  struct Binding
  {
    csRef<Value> value;
    wxWindow* component;
    wxEventType eventType;
    // If processing is true we are busy processing an event from this component
    // and in that case we should not process it again and also not process
    // value changes.
    bool processing;
    // If true then this event will change the enabled state of the
    // component instead of changing the value.
    bool changeEnabled;
    Binding () : component (0), eventType (wxEVT_NULL), processing (false),
      changeEnabled (false) { }
  };
  csPDelArray<Binding> bindings;
  typedef csHash<Binding*,csPtrKey<wxWindow> > ComponentToBinding;
  typedef csHash<csArray<Binding*>,csPtrKey<Value> > ValueToBinding;
  ComponentToBinding bindingsByComponent;
  ValueToBinding bindingsByValue;

  // --------------------------------------------

  /**
   * Recursively enable/disable components but only if they are
   * bound to a value.
   */
  void EnableBoundComponents (wxWindow* window, bool enable);
  void EnableBoundComponentsInt (wxWindow* window, bool enable);
  csSet<csPtrKey<wxWindow> > disabledComponents;
  bool CheckIfParentDisabled (wxWindow* window);

  // --------------------------------------------

  // Listeners.
  class ViewChangeListener : public ValueChangeListener
  {
  private:
    View* view;

  public:
    ViewChangeListener (View* view) : view (view) { }
    virtual ~ViewChangeListener () { }
    virtual void ValueChanged (Value* value)
    {
      view->ValueChanged (value);
    }
  };
  csRef<ViewChangeListener> changeListener;

  // --------------------------------------------

  // Keep track of actions in context menus.
  struct ActionDef
  {
    int id;	// Context menu id.
    csRef<Action> action;
  };
  struct RmbContext
  {
    wxWindow* component;
    csArray<ActionDef> actionDefs;
  };
  csArray<RmbContext> rmbContexts;
  /**
   * Find the index of the rmb context for a given component or csArrayItemNotFound
   * if there is no rmb context.
   */
  size_t FindRmbContext (wxWindow* ctrl);

  // Button actions.
  csHash<csRef<Action>,csPtrKey<wxButton> > buttonActions;

  // --------------------------------------------

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

  // --------------------------------------------

  /// Called by components when they change. Will update the corresponding Value.
  void OnComponentChanged (wxCommandEvent& event);
  /// Called when an action is executed.
  void OnActionExecuted (wxCommandEvent& event);
  /// Called when a context menu is desired.
  void OnRMB (wxContextMenuEvent& event);
  /// Called by values when they change. Will update the corresponding component.
  void ValueChanged (Value* value);

  // Handler for wx events.
  class EventHandler : public wxEvtHandler
  {
  private:
    View* view;

  public:
    EventHandler (View* view) : view (view) { }
    void OnComponentChanged (wxCommandEvent& event)
    {
      view->OnComponentChanged (event);
    }
    void OnActionExecuted (wxCommandEvent& event)
    {
      view->OnActionExecuted (event);
    }
    void OnRMB (wxContextMenuEvent& event)
    {
      view->OnRMB (event);
    }
  } eventHandler;

  // --------------------------------------------

  /**
   * Recursively find a component by name. Also supports the '_' notation
   * in the name of the components.
   */
  wxWindow* FindComponentByName (wxWindow* container, const char* name);

  /// Register a binding for a given component and eventtype.
  void RegisterBinding (Value* value, wxWindow* component, wxEventType eventType,
      bool changeEnabled = false);

  /**
   * Build a tree from a value.
   */
  void BuildTree (wxTreeCtrl* treeCtrl, Value* value, wxTreeItemId& parent);

  /**
   * Destroy all bindings.
   */
  void DestroyBindings ();

  /**
   * Destroy all action bindings.
   */
  void DestroyActionBindings ();

  /**
   * Bind the value to a container.
   */
  bool BindContainer (Value* value, wxWindow* component);

  /**
   * Return true if a given value is bound to some component.
   */
  bool IsValueBound (Value* value) const;

public:
  /**
   * Create a new view managing the given WX parent. If the parent is
   * not immediatelly given it must be set later using SetParent().
   */
  View (wxWindow* parent = 0);
  ~View ();

  /**
   * Set the parent for this view.
   */
  void SetParent (wxWindow* parent) { View::parent = parent; }

  //----------------------------------------------------------------

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
   * Bind a value directly to a text control. This works
   * with all single value types (string, long, bool, float).
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxTextCtrl* component);

  /**
   * Bind a value directly to a check box. This works
   * with all single value types (string, long, bool, float).
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxCheckBox* component);

  /**
   * Bind a value directly to a custom control. This works
   * with all value types.
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, CustomControl* component);

  /**
   * Bind a value directly to a choicebook. This works with
   * values of type VALUE_LONG (used as index) or VALUE_STRING
   * (used by name).
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxChoicebook* component);

  /**
   * Bind a value directly to a panel or a dialog. This only works with
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
  bool Bind (Value* value, wxDialog* component);

  /**
   * Bind a value directly to a list control. This only works with
   * values of type VALUE_COLLECTION and children of type VALUE_COMPOSITE.
   * Note that the list should have a valid heading as defined with
   * DefineHeading().
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxListCtrl* component);

  /**
   * Bind a value directly to a tree control. This only works with
   * values of type VALUE_COLLECTION. The GetStringValue() representation
   * of this value will be used as the name of the root. The children
   * of this value will be the children of this root. These children can
   * themselves also have children (note that in all cases the GetStringValue()
   * of every value is used as the representation in the tree).
   * Can fail (return false) under the following conditions:
   * - Value type is not compatible with component type.
   */
  bool Bind (Value* value, wxTreeCtrl* component);

  /**
   * Bind a value to the enabled/disabled state of a component.
   */
  bool BindEnabled (Value* value, wxWindow* component);

  /**
   * Bind a value to the enabled/disabled state of a component.
   * Can fail (return false) under the following conditions:
   * - Component could not be found by this name.
   */
  bool BindEnabled (Value* value, const char* compName);

  //----------------------------------------------------------------

  /**
   * Create a signal between a value to another value. This is a one-directional
   * connection which will cause the second value to get 'fired'
   * in case the first one is fired.
   * If 'dochildren' is true then the children of the destination will also
   * get fired.
   */
  void Signal (Value* source, Value* dest, bool dochildren = false);

  //----------------------------------------------------------------

  /**
   * Create a read-only boolean value that negates the input value (true becomes
   * false and false becomes true). The input value doesn't have to be a boolean
   * value. Other types are also supported.
   */
  csRef<Value> Not (Value* value);

  /**
   * Create a read-only boolean value that returns true if both input values
   * are 'true'. The input values don't have to be boolean values.
   * Other types are also supported.
   */
  csRef<Value> And (Value* value1, Value* value2);

  /**
   * Create a read-only boolean value that returns true if one of the input values
   * is 'true'. The input values don't have to be boolean values.
   * Other types are also supported.
   */
  csRef<Value> Or (Value* value1, Value* value2);

  //----------------------------------------------------------------

  /**
   * Add an action to a component. This works with various types
   * of components and the way the action is added depends on the component
   * type. For lists and trees the action will be added as a context menu
   * item (using AddContextAction()). For buttons it will be added as a
   * simple action that is called when the button is pressed. Returns false
   * on failure (the component type is not supported for actions).
   */
  bool AddAction (wxWindow* component, Action* action);
  bool AddAction (const char* compName, Action* action);

  /**
   * Add an action to a button.
   */
  bool AddAction (wxButton* button, Action* action);

  /**
   * Add a context action to a list or tree control.
   */
  bool AddContextAction (wxWindow* component, Action* action);

  //----------------------------------------------------------------

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

  //----------------------------------------------------------------

  /**
   * Get the selected value from a component (must be a tree or a list control).
   * Returns 0 if no value is selected.
   */
  Value* GetSelectedValue (wxWindow* component);

  /**
   * Given a component (tree or list control), select the value in this
   * component. Returns false if this failed for some reason (like value
   * is not in this component or the component is not a tree or list).
   */
  bool SetSelectedValue (wxWindow* component, Value* value);
};

} // namespace Ares

#endif // __appares_model_h

