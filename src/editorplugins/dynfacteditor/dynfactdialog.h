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

#include "edcommon/model.h"
#include "editor/iplugin.h"
#include "editor/iapp.h"
#include "editor/icommand.h"
#include "meshview.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

#include "imesh/bodymesh.h"

class TreeCtrlView;
class ListCtrlView;
class FactoryEditorModel;

class DynfactDialog;
class ColliderCollectionValue;

struct iUIManager;
struct iAresEditor;
struct iDynamicFactory;
struct iCelPlLayer;

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
  size_t originXPen;
  size_t originYPen;
  size_t originZPen;
  size_t bonePen;
  size_t boneActivePen;
  size_t boneHiPen;

  csRef<CS::Mesh::iAnimatedMesh> animesh;

  void SetupColliderGeometry ();

  bool showBodies, showJoints, showBones, showOrigin;

public:
  DynfactMeshView (DynfactDialog* dialog, iObjectRegistry* object_reg, wxWindow* parent);
  virtual ~DynfactMeshView () { }

  virtual void SyncValue (Ares::Value* value);
  void Refresh ();

  virtual bool SetMesh (const char* name);

  void ShowBodies (bool s) { showBodies = s; Refresh (); }
  void ShowJoints (bool s) { showJoints = s; Refresh (); }
  void ShowBones (bool s) { showBones = s; Refresh (); }
  void ShowOrigin (bool s) { showOrigin = s; Refresh (); }
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
  virtual bool IsActive (View* view, wxWindow* component);
};

/**
 * Action to create a joint for a bone.
 */
class CreateJointAction : public Action
{
private:
  DynfactDialog* dialog;

public:
  CreateJointAction (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~CreateJointAction () { }
  virtual const char* GetName () const { return "Create joint"; }
  virtual bool Do (View* view, wxWindow* component);
  virtual bool IsActive (View* view, wxWindow* component);
};

/**
 * Action to enable ragdolls
 */
class EnableRagdollAction : public Action
{
private:
  DynfactDialog* dialog;

public:
  EnableRagdollAction (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~EnableRagdollAction () { }
  virtual const char* GetName () const { return "Enable ragdoll"; }
  virtual bool Do (View* view, wxWindow* component);
  virtual bool IsActive (View* view, wxWindow* component);
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
 * The composite value representing the selected bone.
 */
class BoneValue : public CompositeValue
{
private:
  DynfactDialog* dialog;

protected:
  virtual void ChildChanged (Value* child);

public:
  BoneValue (DynfactDialog* dialog);
  virtual ~BoneValue () { }
};

/**
 * A float value that calculates v = offset +- size/2 in both directions.
 */
class Offset2MinMaxValue : public FloatValue
{
private:
  csRef<StandardChangeListener> listener;
  csRef<Value> offset;
  csRef<Value> size;
  bool operatorPlus;

public:
  Offset2MinMaxValue (Value* offset, Value* size, bool operatorPlus);
  virtual ~Offset2MinMaxValue ();
  virtual void SetFloatValue (float fl)
  {
    if (operatorPlus)
    {
      float min = offset->GetFloatValue () - size->GetFloatValue () / 2.0f;
      offset->SetFloatValue ((fl+min) / 2.0f);
      size->SetFloatValue (fl-min);
    }
    else
    {
      float max = offset->GetFloatValue () + size->GetFloatValue () / 2.0f;
      offset->SetFloatValue ((fl+max) / 2.0f);
      size->SetFloatValue (max-fl);
    }
  }
  virtual float GetFloatValue ()
  {
    if (operatorPlus)
      return offset->GetFloatValue () + size->GetFloatValue () / 2.0f;
    else
      return offset->GetFloatValue () - size->GetFloatValue () / 2.0f;
  }
};

/**
 * Automatically populate a category with children.
 */
class AutoNewChildAction : public Action
{
private:
  DynfactDialog* dialog;

public:
  AutoNewChildAction (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~AutoNewChildAction () { }
  virtual const char* GetName () const { return "Automatic Factories"; }
  virtual bool Do (View* view, wxWindow* component);
  virtual bool IsActive (View* view, wxWindow* component);
};

/**
 * Create an invisible dynamic factory.
 */
class NewInvisibleChildAction : public AbstractNewAction
{
private:
  DynfactDialog* dialog;

public:
  NewInvisibleChildAction (DynfactDialog* dialog, Value* collection) :
    AbstractNewAction (collection), dialog (dialog) { }
  virtual ~NewInvisibleChildAction () { }
  virtual const char* GetName () const { return "New Invisible..."; }
  virtual bool Do (View* view, wxWindow* component);
};

/**
 * Create a light dynamic factory.
 */
class NewLightChildAction : public Action
{
private:
  Value* collection;
  DynfactDialog* dialog;

public:
  NewLightChildAction (DynfactDialog* dialog, Value* collection) :
    collection (collection), dialog (dialog) { }
  virtual ~NewLightChildAction () { }
  virtual const char* GetName () const { return "New Light..."; }
  virtual bool Do (View* view, wxWindow* component);
};

/**
 * Edit a light dynamic factory.
 */
class EditLightChildAction : public Action
{
private:
  Value* collection;
  DynfactDialog* dialog;

public:
  EditLightChildAction (DynfactDialog* dialog, Value* collection) :
    collection (collection), dialog (dialog) { }
  virtual ~EditLightChildAction () { }
  virtual const char* GetName () const { return "Edit Light..."; }
  virtual bool Do (View* view, wxWindow* component);
  virtual bool IsActive (View* view, wxWindow* component);
};

/**
 * The dialog for editing dynamic factories.
 */
class DynfactDialog : public wxDialog,
  public scfImplementation3<DynfactDialog, iEditorPlugin, iComponent,
	iCommandHandler>
{
private:
  View view;
  iAresEditor* app;
  iObjectRegistry* object_reg;
  csRef<iTimerEvent> timerOp;
  csRef<iCelPlLayer> pl;
  csRef<iEngine> engine;
  csRef<iVirtualClock> vc;

  static csStringID ID_Show;

  csRef<CS::Animation::iBodyManager> bodyManager;

  csSet<iDynamicFactory*> dirtyFactories;
  csWeakRefArray<iDynamicFactory> dirtyFactoriesWeakArray;

  DynfactMeshView* meshView;
  csRef<ListSelectedValue> bonesSelectedValue;
  csRef<ListSelectedValue> bonesColliderSelectedValue;
  csRef<ListSelectedValue> colliderSelectedValue;
  csRef<ListSelectedValue> pivotsSelectedValue;
  csRef<ListSelectedValue> jointsSelectedValue;
  csRef<TreeSelectedValue> factorySelectedValue;

  csRef<DynfactValue> dynfactValue;
  csRef<BoneValue> boneValue;

  csRef<iUIDialog> factoryDialog;
  csRef<iUIDialog> attributeDialog;
  csRef<iUIDialog> selectBoneDialog;

  void OnOkButton (wxCommandEvent& event);
  void OnShowBodies (wxCommandEvent& event);
  void OnShowOrigin (wxCommandEvent& event);
  void OnShowBones (wxCommandEvent& event);
  void OnShowJoints (wxCommandEvent& event);

  void SetupDialogs ();
  void SetupListHeadings ();
  void SetupColliderEditor (Value* colSelValue, const char* suffix);
  void SetupJointsEditor (Value* jointsSelValue, const char* suffix);
  void SetupSelectedValues ();
  void SetupActions ();

public:
  DynfactDialog (iBase* parent);
  virtual ~DynfactDialog ();

  virtual bool Initialize (iObjectRegistry* object_reg);

  virtual void SetApplication (iAresEditor* app);
  virtual void SetParent (wxWindow* parent);
  virtual const char* GetPluginName () const { return "DynfactEditor"; }
  virtual bool Command (csStringID id, const csString& args)
  {
    if (id == ID_Show) { Show (); return true; }
    return false;
  }
  virtual bool IsCommandValid (csStringID id, const csString& args,
      iSelection* selection, size_t pastesize)
  {
    return true;
  }
  virtual csPtr<iString> GetAlternativeLabel (csStringID id,
      iSelection* selection, size_t pastesize)
  {
    return 0;
  }

  iObjectRegistry* GetObjectRegistry () const { return object_reg; }
  iCelPlLayer* GetPL () const { return pl; }
  iEngine* GetEngine () const { return engine; }
  iAresEditor* GetApplication () const { return app; }
  iUIManager* GetUIManager () const { return app->GetUI (); }

  void AddDirtyFactory (iDynamicFactory* fact);

  void Show ();
  void Tick ();

  /// Calculate the best-fit for a given collider.
  void FitCollider (Value* colSelValue, const csBox3& bbox, csColliderGeometryType type);
  void FitCollider (iDynamicFactory* fact, csColliderGeometryType type);
  void FitCollider (CS::Animation::BoneID id, csColliderGeometryType type);

  CS::Animation::iBodyManager* GetBodyManager () const { return bodyManager; }

  iDynamicFactory* GetCurrentFactory ();
  CS::Animation::iBodyBone* GetCurrentBone ();
  CS::Animation::iBodySkeleton* GetCurrentBodySkeleton ();
  CS::Animation::iSkeletonFactory* GetSkeletonFactory (const char* factName);
  DynfactMeshView* GetMeshView () const { return meshView; }

  long GetSelectedCollider ();
  long GetSelectedBoneCollider ();
  long GetSelectedPivot ();
  long GetSelectedJoint ();
  csString GetSelectedBone ();
  Value* GetColliderSelectedValue () const { return colliderSelectedValue; }

  void UpdateRagdoll ();

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_dynfactdialog_h

