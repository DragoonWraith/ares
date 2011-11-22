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

#include "pcdialog.h"
#include "uimanager.h"
#include "physicallayer/entitytpl.h"
#include "../apparesed.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PropertyClassDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), PropertyClassDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), PropertyClassDialog :: OnCancelButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void PropertyClassDialog::OnOkButton (wxCommandEvent& event)
{
  pctpl = 0;
  EndModal (TRUE);
}

void PropertyClassDialog::OnCancelButton (wxCommandEvent& event)
{
  pctpl = 0;
  EndModal (TRUE);
}

void PropertyClassDialog::Show ()
{
  ShowModal ();
}

static size_t FindNotebookPage (wxChoicebook* book, const char* name)
{
  wxString iname (name, wxConvUTF8);
  for (size_t i = 0 ; i < book->GetPageCount () ; i++)
  {
    wxString pageName = book->GetPageText (i);
    if (pageName == iname) return i;
  }
  return csArrayItemNotFound;
}

void PropertyClassDialog::SwitchToPC (iCelPropertyClassTemplate* pctpl)
{
  PropertyClassDialog::pctpl = pctpl;

  csString pcName = pctpl->GetName ();
  csString tagName = pctpl->GetTag ();

  wxChoicebook* book = XRCCTRL (*this, "pcChoicebook", wxChoicebook);
  size_t pageIdx = FindNotebookPage (book, pcName);
  book->ChangeSelection (pageIdx);

  wxTextCtrl* text = XRCCTRL (*this, "tagTextCtrl", wxTextCtrl);
  text->SetValue (wxString (tagName, wxConvUTF8));
}

PropertyClassDialog::PropertyClassDialog (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("PropertyClassDialog"));
}

PropertyClassDialog::~PropertyClassDialog ()
{
}


