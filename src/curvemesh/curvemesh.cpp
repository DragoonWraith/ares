/*
The MIT License

Copyright (c) 2010 by Jorrit Tyberghein

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
#include "cssysdef.h"

#include "csutil/event.h"
#include "csutil/eventnames.h"
#include "csutil/eventhandlers.h"
#include "csutil/stringarray.h"
#include "csutil/csstring.h"
#include "csutil/xmltiny.h"
#include "csutil/scfstr.h"
#include "csutil/scanstr.h"
#include "iutil/objreg.h"
#include "iutil/object.h"
#include "iutil/eventh.h"
#include "iutil/plugin.h"
#include "csgeom/tri.h"

#include "imap/services.h"
#include "iengine/movable.h"
#include "iengine/sector.h"

#include "curvemesh.h"



CS_PLUGIN_NAMESPACE_BEGIN(CurvedMesh)
{

//---------------------------------------------------------------------------------------

CurvedFactory::CurvedFactory (CurvedMeshCreator* creator, const char* name) :
  scfImplementationType (this), creator (creator), name (name)
{
  material = 0;
  width = 1.0;
  sideHeight = 0.4;
  offsetHeight = 0.1;
  flattenMesh = 0;
}

CurvedFactory::~CurvedFactory ()
{
}

void CurvedFactory::FlattenToGround (iMeshWrapper* mesh)
{
  flattenMesh = mesh;
}

void CurvedFactory::FlattenPointsToMesh (csVector3& leftPos, csVector3& rightPos)
{
  const csReversibleTransform& meshtrans = flattenMesh->GetMovable ()->GetTransform ();
  iSector* sector = flattenMesh->GetMovable ()->GetSectors ()->Get (0);
  csFlags oldFlags = flattenMesh->GetFlags ();
  flattenMesh->GetFlags ().Set (CS_ENTITY_NOHITBEAM);
  rightPos = meshtrans.This2Other (rightPos);
  leftPos = meshtrans.This2Other (leftPos);
  csSectorHitBeamResult resultRight = sector->HitBeamPortals (rightPos + csVector3 (0, 2, 0), rightPos - csVector3 (0, 2, 0));
  csSectorHitBeamResult resultLeft = sector->HitBeamPortals (leftPos + csVector3 (0, 2, 0), leftPos - csVector3 (0, 2, 0));
  flattenMesh->GetFlags ().SetAll (oldFlags.Get ());
  if (resultRight.mesh)
    rightPos = resultRight.isect;
  rightPos = meshtrans.Other2This (rightPos);
  if (resultLeft.mesh)
    leftPos = resultLeft.isect;
  leftPos = meshtrans.Other2This (leftPos);
}

void CurvedFactory::GeneratePath (csPath& path, const csArray<PathEntry>& pts)
{
  size_t l = pts.GetSize ();
  size_t i;

  //float totalDistance = GetTotalDistance (pts);
  //float travelDistance = 0.0f;

  float* times = new float[l];
  for (i = 0 ; i < l ; i++)
  {
    //if (i > 0)
      //travelDistance += sqrt (csSquaredDist::PointPoint (pts[i-1].pos, pts[i].pos));
    //times[i] = travelDistance / totalDistance;
    times[i] = float (i) / float (l-1);
    //float old = float (i) / float (l-1);
    //printf ("i, new:%g old:%g\n", times[i], old); fflush (stdout);
  }
  path.SetTimes (times);
  delete[] times;

  for (i = 0 ; i < l ; i++)
  {
    path.SetPositionVector (i, pts[i].pos);
    path.SetForwardVector (i, pts[i].front);
    path.SetUpVector (i, pts[i].up);
  }
}

float CurvedFactory::GetTotalDistance (const csArray<PathEntry>& pts)
{
  size_t l = pts.GetSize ();
  float totalDistance = 0.0f;
  for (size_t i = 0 ; i < l-1 ; i++)
  {
    float dist = sqrt (csSquaredDist::PointPoint (pts[i].pos, pts[i+1].pos));
    totalDistance += dist;
  }
  return totalDistance;
}

void CurvedFactory::GenerateFactory ()
{
  if (!factory)
  {
    factory = creator->engine->CreateMeshFactory (
	"crystalspace.mesh.object.genmesh", name);
    state = scfQueryInterface<iGeneralFactoryState> (factory->GetMeshObjectFactory ());
  }

  GeneratePoints ();
  csPath path (points.GetSize ());
  GeneratePath (path, points);
  float totalDistance = GetTotalDistance (points);

  // @@@todo, the entire detail on the path should be customizable. Also it should
  // use less detail on relatively straight lines.
  float samplesPerUnit = 1.0;
  // Calculate a rounded number of samples from the samplesPerUnit and
  // then correct samplesPerUnit so that the samples are evenly spread.
  size_t samples = size_t (totalDistance / samplesPerUnit) + 1;
  samplesPerUnit = totalDistance / (samples - 1);

  state->SetVertexCount (samples * 4);
  state->SetTriangleCount ((samples-1) * 6);

  csVector3 pos, front, up, prevPos;
  csVector3* vertices = state->GetVertices ();
  csVector3* normals = state->GetNormals ();
  csVector2* texels = state->GetTexels ();

  path.CalculateAtTime (0);
  path.GetInterpolatedPosition (prevPos);

  float traveledDistance = 0;
  for (size_t i = 0 ; i < samples ; i++)
  {
    float time = float (i) / float (samples-1);
    path.CalculateAtTime (time);
    path.GetInterpolatedPosition (pos);
    traveledDistance += sqrt (csSquaredDist::PointPoint (pos, prevPos));
    //printf ("%d time=%g pos=%g,%g,%g dist=%g\n", i, time, pos.x, pos.y, pos.z, traveledDistance); fflush (stdout);
    prevPos = pos;

    path.GetInterpolatedForward (front);
    path.GetInterpolatedUp (up);

    csVector3 right = (width / 2.0) * (front % up);
    csVector3 down = -up.Unit () * sideHeight;
    csVector3 offsetUp = up.Unit () * offsetHeight;

    csVector3 rightPos = pos + right;
    csVector3 leftPos = pos - right;
    //if (flattenMesh)
      //FlattenPointsToMesh (leftPos, rightPos);
    rightPos += offsetUp;
    leftPos += offsetUp;

    *vertices++ = rightPos;
    *vertices++ = leftPos;
    *vertices++ = rightPos + down;
    *vertices++ = leftPos + down;
    *normals++ = (up*.8+right*.2).Unit ();
    *normals++ = (up*.8-right*.2).Unit ();
    *normals++ = right;
    *normals++ = -right;
    *texels++ = csVector2 (0, traveledDistance / width);
    *texels++ = csVector2 (1, traveledDistance / width);
    *texels++ = csVector2 (-width/sideHeight, traveledDistance / width);
    *texels++ = csVector2 (width/sideHeight, traveledDistance / width);
  }

  csTriangle* tris = state->GetTriangles ();
  int vtidx = 0;
  for (size_t i = 0 ; i < samples-1 ; i++)
  {
    *tris++ = csTriangle (vtidx+5, vtidx+1, vtidx+0);
    *tris++ = csTriangle (vtidx+4, vtidx+5, vtidx+0);
    *tris++ = csTriangle (vtidx+6, vtidx+4, vtidx+0);
    *tris++ = csTriangle (vtidx+2, vtidx+6, vtidx+0);
    *tris++ = csTriangle (vtidx+3, vtidx+1, vtidx+7);
    *tris++ = csTriangle (vtidx+7, vtidx+1, vtidx+5);
    vtidx += 4;
  }

  factory->GetMeshObjectFactory ()->SetMaterialWrapper (material);
  state->Invalidate ();
}

void CurvedFactory::SetMaterial (const char* materialName)
{
  material = creator->engine->FindMaterial (materialName);
  if (!material)
  {
    printf ("Could not find material '%s' for curve factory '%s'!\n",
	materialName, name.GetData ());
    fflush (stdout);
  }
}

void CurvedFactory::SetCharacteristics (float width, float sideHeight)
{
  CurvedFactory::width = width;
  CurvedFactory::sideHeight = sideHeight;
}

size_t CurvedFactory::AddPoint (const csVector3& pos, const csVector3& front,
      const csVector3& up)
{
  return anchorPoints.Push (PathEntry (pos, front.Unit (), up.Unit ()));
}

void CurvedFactory::ChangePoint (size_t idx, const csVector3& pos,
    const csVector3& front, const csVector3& up)
{
  anchorPoints[idx] = PathEntry (pos, front.Unit (), up.Unit ());
}

void CurvedFactory::DeletePoint (size_t idx)
{
  anchorPoints.DeleteIndex (idx);
}

void CurvedFactory::GeneratePoints ()
{
  if (flattenMesh)
  {
    csFlags oldFlags = flattenMesh->GetFlags ();
    flattenMesh->GetFlags ().Set (CS_ENTITY_NOHITBEAM);
    iSector* sector = flattenMesh->GetMovable ()->GetSectors ()->Get (0);
    const csReversibleTransform& meshtrans = flattenMesh->GetMovable ()->GetTransform ();

    csPath anchorPath (anchorPoints.GetSize ());
    GeneratePath (anchorPath, anchorPoints);

    float totalDistance = GetTotalDistance (anchorPoints);
    size_t count = size_t (totalDistance) + 1;
    points.DeleteAll ();

    for (size_t i = 0 ; i <= count ; i++)
    {
      float time = float (i) / float (count);
      anchorPath.CalculateAtTime (time);
      csVector3 pos, front, up;
      anchorPath.GetInterpolatedPosition (pos);
      anchorPath.GetInterpolatedForward (front);
      anchorPath.GetInterpolatedUp (up);
      pos = meshtrans.This2Other (pos);
      // @@@ Ignore transformation for front/up?
      csSectorHitBeamResult result = sector->HitBeamPortals (
	  pos + csVector3 (0, 10, 0), pos - csVector3 (0, 10, 0));
      if (result.mesh)
      {
	pos.y = result.isect.y;
      }
      pos = meshtrans.Other2This (pos);
      points.Push (PathEntry (pos, front, up));
    }

    flattenMesh->GetFlags ().SetAll (oldFlags.Get ());
  }
  else
  {
    points = anchorPoints;
  }
}

void CurvedFactory::Save (iDocumentNode* node, iSyntaxService* syn)
{
  node->SetAttribute ("name", name);
  node->SetAttributeAsFloat ("width", width);
  node->SetAttributeAsFloat ("sideheight", sideHeight);
  node->SetAttribute ("material", material->QueryObject ()->GetName ());
  size_t i;
  for (i = 0 ; i < anchorPoints.GetSize () ; i++)
  {
    csRef<iDocumentNode> el = node->CreateNodeBefore (CS_NODE_ELEMENT);
    el->SetValue ("p");
    csString vector;
    vector.Format ("%g %g %g", anchorPoints[i].pos.x, anchorPoints[i].pos.y, anchorPoints[i].pos.z);
    el->SetAttribute ("pos", (const char*)vector);
    vector.Format ("%g %g %g", anchorPoints[i].front.x, anchorPoints[i].front.y, anchorPoints[i].front.z);
    el->SetAttribute ("front", (const char*)vector);
    vector.Format ("%g %g %g", anchorPoints[i].up.x, anchorPoints[i].up.y, anchorPoints[i].up.z);
    el->SetAttribute ("up", (const char*)vector);
  }
}

bool CurvedFactory::Load (iDocumentNode* node, iSyntaxService* syn)
{
  width = node->GetAttributeValueAsFloat ("width");
  if (fabs (width) < .0001) width = 1.0;
  sideHeight = node->GetAttributeValueAsFloat ("sideheight");
  if (fabs (sideHeight) < .0001) sideHeight = 0.2;
  csString materialName = node->GetAttributeValue ("material");
  SetMaterial (materialName);
  anchorPoints.DeleteAll ();
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csString value = child->GetValue ();
    if (value == "p")
    {
      csVector3 pos, front (0, 0, 1), up (0, 1, 0);
      csString vector = child->GetAttributeValue ("pos");
      if (vector.Length () > 0)
	csScanStr ((const char*)vector, "%f %f %f", &pos.x, &pos.y, &pos.z);
      vector = child->GetAttributeValue ("front");
      if (vector.Length () > 0)
	csScanStr ((const char*)vector, "%f %f %f", &front.x, &front.y, &front.z);
      vector = child->GetAttributeValue ("up");
      if (vector.Length () > 0)
	csScanStr ((const char*)vector, "%f %f %f", &up.x, &up.y, &up.z);
      AddPoint (pos, front, up);
    }
  }
  GenerateFactory ();

  return true;
}

//---------------------------------------------------------------------------------------

CurvedFactoryTemplate::CurvedFactoryTemplate (CurvedMeshCreator* creator,
    const char* name) : scfImplementationType (this), creator (creator), name (name)
{
  width = 1.0;
  sideHeight = 0.2;
}

CurvedFactoryTemplate::~CurvedFactoryTemplate ()
{
}

void CurvedFactoryTemplate::SetAttribute (const char* name, const char* value)
{
  attributes.Put (name, value);
}

const char* CurvedFactoryTemplate::GetAttribute (const char* name) const
{
  return attributes.Get (name, (const char*)0);
}

void CurvedFactoryTemplate::SetMaterial (const char* materialName)
{
  material = materialName;
}

void CurvedFactoryTemplate::SetCharacteristics (float width, float sideHeight)
{
  CurvedFactoryTemplate::width = width;
  CurvedFactoryTemplate::sideHeight = sideHeight;
}

size_t CurvedFactoryTemplate::AddPoint (const csVector3& pos, const csVector3& front,
      const csVector3& up)
{
  return points.Push (PathEntry (pos, front.Unit (), up.Unit ()));
}

//---------------------------------------------------------------------------------------

SCF_IMPLEMENT_FACTORY (CurvedMeshCreator)

CurvedMeshCreator::CurvedMeshCreator (iBase *iParent)
  : scfImplementationType (this, iParent)
{  
  object_reg = 0;
}

CurvedMeshCreator::~CurvedMeshCreator ()
{
}

bool CurvedMeshCreator::Initialize (iObjectRegistry *object_reg)
{
  CurvedMeshCreator::object_reg = object_reg;
  engine = csQueryRegistry<iEngine> (object_reg);
  return true;
}

void CurvedMeshCreator::DeleteFactories ()
{
  size_t i;
  for (i = 0 ; i < factories.GetSize () ; i++)
    engine->RemoveObject (factories[i]->GetFactory ());
  factories.DeleteAll ();
  factory_hash.Empty ();
}

iCurvedFactoryTemplate* CurvedMeshCreator::AddCurvedFactoryTemplate (const char* name)
{
  CurvedFactoryTemplate* cf = new CurvedFactoryTemplate (this, name);
  factoryTemplates.Push (cf);
  cf->DecRef ();
  return cf;
}

CurvedFactoryTemplate* CurvedMeshCreator::FindFactoryTemplate (const char* name)
{
  size_t i;
  csString tn = name;
  for (i = 0 ; i < factoryTemplates.GetSize () ; i++)
  {
    if (tn == factoryTemplates[i]->GetName ())
      return factoryTemplates[i];
  }
  return 0;
}

iCurvedFactory* CurvedMeshCreator::AddCurvedFactory (const char* name, const char* templatename)
{
  CurvedFactoryTemplate* cftemp = FindFactoryTemplate (templatename);
  if (!cftemp)
  {
    printf ("ERROR! Can't find factory template '%s'!\n", templatename);
    return 0;
  }
  CurvedFactory* cf = new CurvedFactory (this, name);
  cf->SetMaterial (cftemp->GetMaterial ());
  cf->SetCharacteristics (cftemp->GetWidth (), cftemp->GetSideHeight ());
  const csArray<PathEntry>& points = cftemp->GetPoints ();
  for (size_t i = 0 ; i < points.GetSize () ; i++)
    cf->AddPoint (points[i].pos, points[i].front, points[i].up);
  factories.Push (cf);
  factory_hash.Put (name, cf);
  cf->DecRef ();
  return cf;
}

void CurvedMeshCreator::Save (iDocumentNode* node)
{
  csRef<iSyntaxService> syn = csQueryRegistryOrLoad<iSyntaxService> (object_reg,
      "crystalspace.syntax.loader.service.text");
  for (size_t i = 0 ; i < factories.GetSize () ; i++)
  {
    csRef<iDocumentNode> el = node->CreateNodeBefore (CS_NODE_ELEMENT);
    el->SetValue ("fact");
    factories[i]->Save (el, syn);
  }
}

csRef<iString> CurvedMeshCreator::Load (iDocumentNode* node)
{
  csRef<iSyntaxService> syn = csQueryRegistryOrLoad<iSyntaxService> (object_reg,
      "crystalspace.syntax.loader.service.text");

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csString value = child->GetValue ();
    if (value == "fact")
    {
      csRef<CurvedFactory> cfact;
      csString name = child->GetAttributeValue ("name");
      cfact.AttachNew (new CurvedFactory (this, name));
      if (!cfact->Load (child, syn))
      {
        csRef<iString> str;
        str.AttachNew (new scfString ("Error loading factory!"));
        return str;
      }
      factories.Push (cfact);
      factory_hash.Put (name, cfact);
    }
  }

  return 0;
}

}
CS_PLUGIN_NAMESPACE_END(CurvedMesh)

