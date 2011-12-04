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
   * Start the update from the given list. This is called by the list control
   * when it is time to update the data. This call is followed by a series of
   * UpdateRow() calls and finished by a call to FinishUpdate().
   */
  virtual void StartUpdate () = 0;

  /**
   * Update a row.
   * Returns false in case of error.
   */
  virtual bool UpdateRow (const csStringArray& row) = 0;

  /**
   * Finish the update.
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

#endif // __appares_rowmodel_h

