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

#include "celldialog.h"
#include "uimanager.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CellDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), CellDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), CellDialog :: OnCancelButton)
  EVT_BUTTON (XRCID("addCellButton"), CellDialog :: OnAddCellButton)
  EVT_BUTTON (XRCID("delCellButton"), CellDialog :: OnDelCellButton)
  EVT_LISTBOX (XRCID("cellListBox"), CellDialog :: OnCellSelected)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void CellDialog::OnOkButton (wxCommandEvent& event)
{
  EndModal (TRUE);
}

void CellDialog::OnCancelButton (wxCommandEvent& event)
{
  EndModal (TRUE);
}

void CellDialog::OnAddCellButton (wxCommandEvent& event)
{
}

void CellDialog::OnDelCellButton (wxCommandEvent& event)
{
}

void CellDialog::OnCellSelected (wxCommandEvent& event)
{
}

void CellDialog::Show ()
{
  wxButton* delButton = XRCCTRL (*this, "delCellButton", wxButton);
  delButton->Disable ();
  ShowModal ();
}

CellDialog::CellDialog (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("CellDialog"));
}

CellDialog::~CellDialog ()
{
}


