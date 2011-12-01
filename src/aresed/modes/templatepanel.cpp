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

#include "templatepanel.h"
#include "entitymode.h"
#include "../apparesed.h"
#include "../ui/uimanager.h"
#include "../ui/listctrltools.h"

#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "propclass/chars.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(EntityTemplatePanel, wxPanel)
  EVT_CONTEXT_MENU (EntityTemplatePanel :: OnContextMenu)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

EntityTemplatePanel::EntityTemplatePanel (wxWindow* parent, UIManager* uiManager,
    EntityMode* emode) :
  uiManager (uiManager), emode (emode), tpl (0)
{
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("EntityTemplatePanel"));

  wxListCtrl* list;

  list = XRCCTRL (*this, "templateParentsList", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Template", 100);

  list = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Value", 100);

  list = XRCCTRL (*this, "templateClassList", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Class", 100);
}

EntityTemplatePanel::~EntityTemplatePanel ()
{
}

void EntityTemplatePanel::OnContextMenu (wxContextMenuEvent& event)
{
}

void EntityTemplatePanel::SwitchToTpl (iCelEntityTemplate* tpl)
{
  EntityTemplatePanel::tpl = tpl;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  wxListCtrl* parentsList = XRCCTRL (*this, "templateParentsList", wxListCtrl);
  parentsList->DeleteAllItems ();
  csRef<iCelEntityTemplateIterator> itParents = tpl->GetParents ();
  while (itParents->HasNext ())
  {
    iCelEntityTemplate* parent = itParents->Next ();
    ListCtrlTools::AddRow (parentsList, parent->GetName (), 0);
  }

  wxListCtrl* charList = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  charList->DeleteAllItems ();
  csRef<iCharacteristicsIterator> itChars = tpl->GetCharacteristics ()->GetAllCharacteristics ();
  while (itChars->HasNext ())
  {
    float f;
    csString name = itChars->Next (f);
    csString value; value.Format ("%g", f);
    ListCtrlTools::AddRow (charList, name.GetData (), value.GetData (), 0);
  }

  wxListCtrl* classList = XRCCTRL (*this, "templateClassList", wxListCtrl);
  classList->DeleteAllItems ();
  const csSet<csStringID>& classes = tpl->GetClasses ();
  csSet<csStringID>::GlobalIterator itClass = classes.GetIterator ();
  while (itClass.HasNext ())
  {
    csStringID classID = itClass.Next ();
    csString className = pl->FetchString (classID);
    ListCtrlTools::AddRow (classList, className.GetData (), 0);
  }
}


