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

#ifndef __appares_listctrltools_h
#define __appares_listctrltools_h

#include <csutil/stringarray.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

class ListCtrlTools
{
public:
  /**
   * Read a row from the list control.
   */
  static csStringArray ReadRow (wxListCtrl* list, int row);

  /**
   * Add a row to the list control (at the end).
   */
  static long AddRow (wxListCtrl* list, const char* value, ...);

  /**
   * Set the color of some row.
   */
  static void ColorRow (wxListCtrl* list, int idx,
    unsigned char red, unsigned char green, unsigned char blue);

  /**
   * Set the background color of some row.
   */
  static void BackgroundColorRow (wxListCtrl* list, int idx,
    unsigned char red, unsigned char green, unsigned char blue);

  /**
   * Add a column.
   */
  static void SetColumn (wxListCtrl* list, int idx, const char* name, int width);
};

#endif // __appares_listctrltools_h

