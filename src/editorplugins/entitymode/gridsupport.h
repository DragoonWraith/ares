/*
The MIT License

Copyright (c) 2013 by Jorrit Tyberghein

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

#ifndef __aresed_gridsupport_h
#define __aresed_gridsupport_h

#include "csutil/csstring.h"
#include "physicallayer/datatype.h"

#include <wx/xrc/xmlres.h>
#include "cseditor/wx/propgrid/propgrid.h"
#include "cseditor/wx/propgrid/propdev.h"

struct iCelPlLayer;
struct iUIManager;
struct iParameterManager;
class EntityMode;


// Refresh types are ordered by priority.
// More important refresh types come later.
enum RefreshType
{
  REFRESH_NOCHANGE = 0,		// Nothing has changed, no refresh needed.
  REFRESH_NO,			// There was a change but no refresh needed.
  REFRESH_PC,			// Only PC has to be refreshed.
  REFRESH_STATE,		// Only state has to be refreshed.
  REFRESH_SEQUENCE,		// Only sequence has to be refreshed.
  REFRESH_TEMPLATE,		// Only template stuff has to be refreshed.
  REFRESH_FULL			// Full refresh is required.
};

class GridSupport : public csRefCount
{
protected:
  iCelPlLayer* pl;
  iUIManager* ui;
  iParameterManager* pm;
  csString name;
  EntityMode* emode;
  wxPropertyGrid* detailGrid;

  wxArrayString typesArray;
  wxArrayInt typesArrayIdx;

  int RegisterContextMenu (wxObjectEventFunction handler);
  wxPGProperty* AppendButtonPar (
    wxPGProperty* parent, const char* partype, const char* type, const char* name);
  void AppendPar (
    wxPGProperty* parent, const char* partype,
    const char* name, celDataType type, const char* value);
  void AppendColorPar (
    wxPGProperty* parent, const char* label, const char* name,
    const char* red, const char* green, const char* blue);
  void AppendVectorPar (
    wxPGProperty* parent, const char* label, const char* name,
    const char* x, const char* y, const char* z);
  wxPGProperty* AppendStringPar (wxPGProperty* parent,
    const char* label, const char* name, const char* value);
  wxPGProperty* AppendBoolPar (wxPGProperty* parent,
    const char* label, const char* name, bool value);
  wxPGProperty* AppendIntPar (wxPGProperty* parent,
    const char* label, const char* name, int value);
  wxPGProperty* AppendEnumPar (wxPGProperty* parent,
    const char* label, const char* name, const wxArrayString& labels, const wxArrayInt& values,
    int value = 0);
  wxPGProperty* AppendEditEnumPar (wxPGProperty* parent,
    const char* label, const char* name, const wxArrayString& labels, const wxArrayInt& values,
    const char* value = 0);

  csString GetPropertyValueAsString (const csString& property, const char* sub);
  int GetPropertyValueAsInt (const csString& property, const char* sub);

  csString GetPropertyName (wxPGProperty* property);

public:
  GridSupport (const char* name, EntityMode* emode);
  virtual ~GridSupport () { }

  const csString& GetName () { return name; }
};


#endif // __aresed_gridsupport_h

