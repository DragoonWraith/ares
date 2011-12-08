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

#include "../apparesed.h"
#include "dynfactdialog.h"
#include "uimanager.h"
#include "listctrltools.h"
#include "meshview.h"
#include "treeview.h"
#include "../models/dynfactmodel.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(DynfactDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), DynfactDialog :: OnOkButton)
  EVT_TREE_SEL_CHANGED (XRCID("factoryTree"), DynfactDialog :: OnFactoryChanged)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

class RotMeshTimer : public scfImplementation1<RotMeshTimer, iTimerEvent>
{
private:
  DynfactDialog* df;

public:
  RotMeshTimer (DynfactDialog* df) : scfImplementationType (this), df (df) { }
  virtual ~RotMeshTimer () { }
  virtual bool Perform (iTimerEvent* ev) { df->Tick (); return true; }
};

//--------------------------------------------------------------------------

void DynfactDialog::OnOkButton (wxCommandEvent& event)
{
  csRef<iEventTimer> timer = csEventTimer::GetStandardTimer (uiManager->GetApp ()->GetObjectRegistry ());
  timer->RemoveTimerEvent (timerOp);
  EndModal (TRUE);
}

void DynfactDialog::Show ()
{
  selIndex = -1;
  meshTreeView->Refresh ();

  csRef<iEventTimer> timer = csEventTimer::GetStandardTimer (uiManager->GetApp ()->GetObjectRegistry ());
  timer->AddTimerEvent (timerOp, 25);

  ShowModal ();
}

void DynfactDialog::Tick ()
{
  iVirtualClock* vc = uiManager->GetApp ()->GetVC ();
  meshView->RotateMesh (vc->GetElapsedSeconds ());
}

void DynfactDialog::OnFactoryChanged (wxTreeEvent& event)
{
  wxTreeItemId item = event.GetItem ();
  wxTreeCtrl* tree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);
  csString meshName = (const char*)tree->GetItemText (item).mb_str (wxConvUTF8);
  printf ("mesh=%s\n", meshName.GetData ()); fflush (stdout);
  meshView->SetMesh (meshName);
}

DynfactDialog::DynfactDialog (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("DynfactDialog"));
  wxPanel* panel = XRCCTRL (*this, "meshPanel", wxPanel);
  meshView = new MeshView (uiManager->GetApp ()->GetObjectRegistry (), panel);
  wxTreeCtrl* tree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);
  meshTreeView = new TreeCtrlView (tree, uiManager->GetApp ()->GetAresView ()
      ->GetDynfactRowModel ());
  meshTreeView->SetRootName ("Categories");
  timerOp.AttachNew (new RotMeshTimer (this));
}

DynfactDialog::~DynfactDialog ()
{
  delete meshView;
  delete meshTreeView;
}


