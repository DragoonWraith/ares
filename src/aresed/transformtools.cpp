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

#include "apparesed.h"
#include "transformtools.h"
#include "selection.h"

csVector3 TransformTools::GetCenterSelected (Selection* selection)
{
  csVector3 center (0);
  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    const csBox3& box = dynobj->GetFactory ()->GetBBox ();
    const csReversibleTransform& tr = dynobj->GetTransform ();
    center += tr.This2Other (box.GetCenter ());
  }
  center /= selection->GetSize ();
  return center;
}

void TransformTools::Move (Selection* selection,
    const csVector3& baseVector, bool slow, bool fast)
{
  csVector3 vector = baseVector;
  if (slow) vector *= 0.01f;
  else if (!fast) vector *= 0.1f;

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;

    dynobj->MakeKinematic ();
    csReversibleTransform& trans = mesh->GetMovable ()->GetTransform ();
    trans.Translate (trans.This2OtherRelative (vector));
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

void TransformTools::Rotate (Selection* selection, float baseAngle,
    bool slow, bool fast)
{
  float angle = baseAngle;
  if (slow) angle /= 180.0;
  else if (fast) angle /= 2.0;
  else angle /= 8.0;

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;

    dynobj->MakeKinematic ();
    mesh->GetMovable ()->GetTransform ().RotateOther (csVector3 (0, 1, 0), angle);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

static float GetAngle (float x1, float y1, float x2, float y2)
{
  float dot = x1 * x2 + y1 * y2;
  float angle = acos (dot);
  return angle;
}

static csVector3 FindBiggestHorizontalMovement (
    const csBox3& box1, const csReversibleTransform& trans1,
    const csBox3& box2, const csReversibleTransform& trans2)
{
  // The origin of the second box in the 3D space of the first box.
  csVector3 o2Tr = trans1.Other2This (trans2.GetOrigin ());
  // Transformed bounding box.
  csBox3 box2Tr;
  box2Tr.StartBoundingBox (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (0))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (1))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (2))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (3))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (4))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (5))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (6))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (7))));

  float xr = o2Tr.x - box1.MaxX () + box2Tr.MinX ();
  float xl = box1.MinX () - o2Tr.x - box2Tr.MaxX ();
  float zr = o2Tr.z - box1.MaxZ () + box2Tr.MinZ ();
  float zl = box1.MinZ () - o2Tr.z - box2Tr.MaxZ ();
  csVector3 newpos = trans2.GetOrigin ();
  if (xr >= xl && xr >= zr && xr >= zl)
    newpos = trans1.This2Other (o2Tr + csVector3 (-xr, 0, -o2Tr.z));
  else if (xl >= xr && xl >= zr && xl >= zl)
    newpos = trans1.This2Other (o2Tr + csVector3 (xl, 0, -o2Tr.z));
  else if (zr >= xl && zr >= xr && zr >= zl)
    newpos = trans1.This2Other (o2Tr + csVector3 (-o2Tr.x, 0, -zr));
  else if (zl >= xl && zl >= xr && zl >= zr)
    newpos = trans1.This2Other (o2Tr + csVector3 (-o2Tr.x, 0, zl));
  return newpos;
}

/**
 * Modify the slave transform so that it is nicely aligned to the master
 * transform (horizontally) but with the smallest possible rotation.
 */
static void FindBestAlignedTransform (const csReversibleTransform& masterTrans,
    csReversibleTransform& slaveTrans)
{
  csReversibleTransform masterCopy = masterTrans;
  int i = 4;
  while (i > 0)
  {
    csVector3 front = masterCopy.GetFront ();
    csVector3 fr = slaveTrans.GetFront ();
    float angle = GetAngle (front.x, front.z, fr.x, fr.z);
    if (fabs (angle) <= (PI / 4.0))
    {
      slaveTrans.SetO2T (masterCopy.GetO2T ());
      return;
    }
    i--;
    masterCopy.RotateOther (csVector3 (0, 1, 0), PI/2.0);
  }
}

void TransformTools::AlignSelectedObjects (Selection* selection)
{
  if (selection->GetSize () <= 1) return;
  if (!selection->GetFirst ()->GetMesh ()) return;

  const csReversibleTransform& trans = selection->GetFirst ()->GetMesh ()->GetMovable ()->GetTransform ();

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (dynobj == selection->GetFirst ()) continue;
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;
    dynobj->MakeKinematic ();
    csReversibleTransform& tr = mesh->GetMovable ()->GetTransform ();
    FindBestAlignedTransform (trans, tr);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

void TransformTools::StackSelectedObjects (Selection* selection)
{
  if (selection->GetSize () <= 1) return;
  if (!selection->GetFirst ()->GetMesh ()) return;

  csReversibleTransform firstTrans = selection->GetFirst ()->GetMesh ()->GetMovable ()->GetTransform ();
  csBox3 firstBbox = selection->GetFirst ()->GetFactory ()->GetPhysicsBBox ();

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (dynobj == selection->GetFirst ()) continue;
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;
    dynobj->MakeKinematic ();
    const csBox3& bbox = dynobj->GetFactory ()->GetPhysicsBBox ();
    csReversibleTransform& tr = mesh->GetMovable ()->GetTransform ();
    csVector3 v = firstTrans.GetOrigin ();
    v.y = firstTrans.GetOrigin ().y + firstBbox.MaxY () - bbox.MinY ();
    tr.SetOrigin (v);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();

    // Next stack we perform relative to the previous one we stacked.
    firstTrans = tr;
    firstBbox = bbox;
  }
}

void TransformTools::SameYSelectedObjects (Selection* selection)
{
  if (selection->GetSize () <= 1) return;
  if (!selection->GetFirst ()->GetMesh ()) return;

  csReversibleTransform trans = selection->GetFirst ()->GetMesh ()->GetMovable ()->GetTransform ();
  float y = trans.GetOrigin ().y;

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (dynobj == selection->GetFirst ()) continue;
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;
    dynobj->MakeKinematic ();
    csReversibleTransform& tr = mesh->GetMovable ()->GetTransform ();
    csVector3 v = tr.GetOrigin ();
    v.y = y;
    tr.SetOrigin (v);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

void TransformTools::SetPosSelectedObjects (Selection* selection)
{
  if (selection->GetSize () <= 1) return;
  if (!selection->GetFirst ()->GetMesh ()) return;

  csReversibleTransform firstTrans = selection->GetFirst ()->GetMesh ()->GetMovable ()
    ->GetTransform ();
  csBox3 firstBbox = selection->GetFirst ()->GetFactory ()->GetPhysicsBBox ();

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (dynobj == selection->GetFirst ()) continue;
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;

    dynobj->MakeKinematic ();
    csReversibleTransform& tr = mesh->GetMovable ()->GetTransform ();
    const csBox3& bbox = dynobj->GetFactory ()->GetPhysicsBBox ();
    csVector3 newpos = FindBiggestHorizontalMovement (firstBbox, firstTrans, bbox, tr);
    tr.SetOrigin (newpos);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();

    // Next SetPos we perform relative to the previous one we stacked.
    firstTrans = tr;
    firstBbox = bbox;
  }
}

void TransformTools::RotResetSelectedObjects (Selection* selection)
{
  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;

    dynobj->MakeKinematic ();
    mesh->GetMovable ()->GetTransform ().LookAt (csVector3 (0, 0, 1), csVector3 (0, 1, 0));
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

