/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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

#ifndef __appares_lightdialog_h
#define __appares_lightdialog_h

#include <crystalspace.h>

#include "edcommon/model.h"
#include "editor/iplugin.h"
#include "editor/iapp.h"
#include "editor/icommand.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/clrpicker.h>

using namespace Ares;

class DynfactDialog;
struct iLightFactory;

/**
 * A value representing a color.
 */
class ColorValue : public CompositeValue
{
private:
  csColor color;

protected:
  virtual void ChildChanged (Value* child);

public:
  ColorValue ();
  virtual ~ColorValue () { }

  void SetColor (const csColor& color);
  const csColor& GetColor () const { return color; }
};

/**
 * The composite value representing a light factory.
 */
class LightFactoryValue : public CompositeValue
{
private:
  csRef<ColorValue> color;
  csRef<ColorValue> specularColor;

public:
  LightFactoryValue ();
  virtual ~LightFactoryValue () { }

  ColorValue* GetColorValue () { return color; }
  ColorValue* GetSpecularColorValue () { return specularColor; }
};

/**
 * The dialog for editing light factories.
 */
class LightDialog : public wxDialog, public View
{
private:
  DynfactDialog* dialog;
  csRef<LightFactoryValue> factoryValue;

  void OnOkButton (wxCommandEvent& event);
  void OnCancelButton (wxCommandEvent& event);
  void OnColorChange (wxColourPickerEvent& event);
  void OnSpecularColorChange (wxColourPickerEvent& event);

public:
  LightDialog (wxWindow* parent, DynfactDialog* dialog);
  virtual ~LightDialog ();

  iLightFactory* Show (iLightFactory* factory);

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_lightdialog_h

