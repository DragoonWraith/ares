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

#ifndef __appares_templatepanel_h
#define __appares_templatepanel_h

#include <crystalspace.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/choicebk.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

#include "edcommon/model.h"

struct iUIManager;
class EntityMode;
struct iCelPlLayer;
struct iCelEntityTemplate;

class ListCtrlView;
class ParentsCollectionValue;
class CharacteristicsRowModel;
class ClassesRowModel;

class EntityTemplatePanel : public wxPanel, public Ares::View
{
private:
  iUIManager* uiManager;
  iCelPlLayer* pl;
  EntityMode* emode;
  wxSizer* parentSizer;
  iCelEntityTemplate* tpl;

  csRef<ParentsCollectionValue> parentsCollectionValue;

  //ListCtrlView* parentsView;
  //csRef<ParentsRowModel> parentsModel;
  ListCtrlView* characteristicsView;
  csRef<CharacteristicsRowModel> characteristicsModel;
  ListCtrlView* classesView;
  csRef<ClassesRowModel> classesModel;

public:
  EntityTemplatePanel (wxWindow* parent, iUIManager* uiManager, EntityMode* emode);
  ~EntityTemplatePanel();

  iCelPlLayer* GetPL () const { return pl; }
  iUIManager* GetUIManager () const { return uiManager; }

  // Switch this dialog to editing of a template.
  void SwitchToTpl (iCelEntityTemplate* tpl);

  void Show () { wxPanel::Show (); parentSizer->Layout (); }
  void Hide () { wxPanel::Hide (); parentSizer->Layout (); }
  bool IsVisible () const { return IsShown (); }
};

#endif // __appares_templatepanel_h

