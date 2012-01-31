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

#ifndef __appares_dynfactdialog_h
#define __appares_dynfactdialog_h

#include <crystalspace.h>

#include "../models/model.h"
#include "meshview.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

class TreeCtrlView;
class ListCtrlView;
class FactoryEditorModel;

class DynfactDialog;
class ColliderCollectionValue;

class UIManager;

using namespace Ares;

/**
 * A mesh view that knows how to setup the colliders
 * for the given mesh.
 */
class DynfactMeshView : public MeshView
{
private:
  DynfactDialog* dialog;
  size_t normalPen;
  size_t hilightPen;

  void SetupColliderGeometry ();

public:
  DynfactMeshView (DynfactDialog* dialog, iObjectRegistry* object_reg, wxWindow* parent);
  virtual ~DynfactMeshView () { }

  virtual void SyncValue (Ares::Value* value);
};

/**
 * Action to edit the category of an item.
 */
class EditCategoryAction : public Action
{
private:
  DynfactDialog* dialog;

public:
  EditCategoryAction (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~EditCategoryAction () { }
  virtual const char* GetName () const { return "Edit category..."; }
  virtual bool Do (View* view, wxWindow* component);
};

/**
 * The composite value representing the selected dynamic factory.
 */
class DynfactValue : public CompositeValue
{
private:
  DynfactDialog* dialog;

protected:
  virtual void ChildChanged (Value* child);

public:
  DynfactValue (DynfactDialog* dialog);
  virtual ~DynfactValue () { }
};

/**
 * The dialog for editing dynamic factories.
 */
class DynfactDialog : public wxDialog, public View
{
private:
  UIManager* uiManager;
  csRef<iTimerEvent> timerOp;

  DynfactMeshView* meshView;
  csRef<TreeSelectedValue> factorySelectedValue;

  csRef<DynfactValue> dynfactValue;
  csRef<ListSelectedValue> colliderSelectedValue;

  UIDialog* factoryDialog;

  void OnOkButton (wxCommandEvent& event);

public:
  DynfactDialog (wxWindow* parent, UIManager* uiManager);
  ~DynfactDialog ();

  void Show ();
  void Tick ();

  UIManager* GetUIManager () const { return uiManager; }

  iDynamicFactory* GetCurrentFactory ();
  long GetSelectedCollider ();
  long GetSelectedPivot ();
  long GetSelectedJoint ();
  Value* GetColliderSelectedValue () const { return colliderSelectedValue; }

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_dynfactdialog_h

