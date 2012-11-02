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

#include "projectdata.h"
#include "uimanager.h"
#include "edcommon/listctrltools.h"
#include "../apparesed.h"
#include "edcommon/uitools.h"
#include "iassetmanager.h"

#include <wx/html/htmlwin.h>

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ProjectDataDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), ProjectDataDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), ProjectDataDialog :: OnCancelButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void ProjectDataDialog::OnOkButton (wxCommandEvent& event)
{
  iAssetManager* assetMgr = uiManager->GetApp ()->GetAssetManager ();
  iProjectData* data = assetMgr->GetProjectData ();
  data->SetName (UITools::GetValue (this, "name_Text"));
  data->SetShortDescription (UITools::GetValue (this, "shortDescription_Text"));
  data->SetDescription (UITools::GetValue (this, "description_Text"));
  assetMgr->RegisterModification ();
  EndModal (TRUE);
}

void ProjectDataDialog::OnCancelButton (wxCommandEvent& event)
{
  EndModal (TRUE);
}

void ProjectDataDialog::Show ()
{
  iAssetManager* assetMgr = uiManager->GetApp ()->GetAssetManager ();
  iProjectData* data = assetMgr->GetProjectData ();
  UITools::SetValue (this, "name_Text", data->GetName ());
  UITools::SetValue (this, "shortDescription_Text", data->GetShortDescription ());
  UITools::SetValue (this, "description_Text", data->GetDescription ());
  ShowModal ();
}

ProjectDataDialog::ProjectDataDialog (wxWindow* parent, iObjectRegistry* object_reg,
    UIManager* uiManager) : object_reg (object_reg), uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("ProjectDataDialog"));
}

ProjectDataDialog::~ProjectDataDialog ()
{
}


