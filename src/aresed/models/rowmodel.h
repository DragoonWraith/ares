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

#ifndef __appares_rowmodel_h
#define __appares_rowmodel_h

#include <csutil/stringarray.h>

class UIDialog;
class EditorModel;

/**
 * This class represents a row model. Instances of this interface
 * can be used by supporting views (like ListCtrlView).
 */
class RowModel : public csRefCount
{
public:
  /**
   * Reset the iterator for this rowmodel.
   * This method (together with HasRows() and NextRow()) are used to fetch
   * all the data so that the list can be filled.
   */
  virtual void ResetIterator () = 0;

  /**
   * Check if there are still rows to process.
   */
  virtual bool HasRows () = 0;

  /**
   * Get the next row.
   */
  virtual csStringArray NextRow () = 0;

  // -----------------------------------------------------------------------------

  /**
   * Delete a row. Returns false if this row could not be deleted for some
   * reason.
   */
  virtual bool DeleteRow (const csStringArray& row) = 0;

  /**
   * Add a new row. Returns false if the new row could not be added.
   */
  virtual bool AddRow (const csStringArray& row) = 0;

  /**
   * Update a row. Returns false if this row could not be updated.
   * The default implementation just calls DeleteRow() first and then
   * AddRow() with the new row.
   */
  virtual bool UpdateRow (const csStringArray& oldRow,
      const csStringArray& row)
  {
    if (!DeleteRow (oldRow)) return false;
    if (!AddRow (row))
    {
      AddRow (oldRow);
      return false;
    }
    return true;
  }

  /**
   * Finish the update. This can give the model a chance to refresh
   * certain things.
   */
  virtual void FinishUpdate () { }

  // -----------------------------------------------------------------------------

  /**
   * Get the columns for the list as a single ',' seperated string.
   */
  virtual const char* GetColumns () = 0;

  /**
   * Return true if this datamodel allows editing of rows.
   */
  virtual bool IsEditAllowed () const { return true; }

  // -----------------------------------------------------------------------------

  /**
   * Return a dialog with which a row in this model can be edited.
   * This can return 0 in which case the view should call EditRow()
   * for more controlled editing. The dialog should support the fields
   * with the same name as the columns in the model.
   */
  virtual UIDialog* GetEditorDialog () { return 0; }

  /**
   * Edit a given row and return the updated row.
   * If the given row is empty (contains no items) then
   * the editor will create a new row.
   * If the returned row is empty then the dialog was canceled.
   */
  virtual csStringArray EditRow (const csStringArray& origRow)
  {
    return csStringArray ();
  }
};

/**
 * Listen to dirty changes.
 */
struct DirtyListener : public csRefCount
{
  virtual void DirtyChanged (bool dirty) = 0;
};

/**
 * An editor for a model. This is typically used in combination
 * with a model and a view where you want to provide an editor
 * that automatically updates whenever the selected row in the view
 * changes.
 */
class EditorModel : public csRefCount
{
public:
  /**
   * Update the editor from a given row. The format of the row will
   * be the same as the one from the associated model. If the row
   * is empty then the editor must be cleared.
   */
  virtual void Update (const csStringArray& row) = 0;

  /**
   * Return the current contents of the editor in a row compatible
   * with the associated model.
   */
  virtual csStringArray Read () = 0;

  /**
   * Return true if the contents of this editor is dirty. Doing Update()
   * should automatically clear the dirty flag. If this editor doesn't support
   * the notion of 'dirty' then it should always return true.
   */
  virtual bool IsDirty () { return true; }

  /**
   * Set/clear the dirty flag.
   */
  virtual void SetDirty (bool dirty) { }

  /**
   * Add dirty listener.
   */
  virtual void AddDirtyListener (DirtyListener* listener) { }

  /**
   * Remove dirty listener.
   */
  virtual void RemoveDirtyListener (DirtyListener* listener) { }
};

#endif // __appares_rowmodel_h

