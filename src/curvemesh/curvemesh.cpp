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

#define VERBOSE 0


CS_PLUGIN_NAMESPACE_BEGIN(CurvedMesh)
{

//---------------------------------------------------------------------------------------

void ClingyPath::SetBasePoints (const csArray<PathEntry> pts)
{
  basePoints = pts;
  size_t l = pts.GetSize ();

  float totalDistance = 0.0f;
  for (size_t i = 0 ; i < l-1 ; i++)
  {
    float dist = sqrt (csSquaredDist::PointPoint (pts[i].pos, pts[i+1].pos));
    totalDistance += dist;
  }
  float travelDistance = 0.0f;

  basePoints[0].time = 0.0f;
  for (size_t i = 1 ; i < l ; i++)
  {
    travelDistance += sqrt (csSquaredDist::PointPoint (pts[i-1].pos, pts[i].pos));
    basePoints[i].time = travelDistance / totalDistance;
  }
}

void ClingyPath::RefreshWorkingPath ()
{
  points = basePoints;
}

static bool HeightDiff (const csVector3& pos, iMeshWrapper* thisMesh, float& dy)
{
  const csReversibleTransform& meshtrans = thisMesh->GetMovable ()->GetTransform ();
  csVector3 p = meshtrans.This2Other (pos);
  iSector* sector = thisMesh->GetMovable ()->GetSectors ()->Get (0);
  csSectorHitBeamResult result = sector->HitBeamPortals (
      p + csVector3 (0, 2, 0), p - csVector3 (0, 2, 0));
  if (result.mesh)
  {
    dy = p.y - result.isect.y;
    return true;
  }
  else
  {
    dy = 0.0f;
    return false;
  }
}

//#define BOTTOM_MARGIN .02f
//#define TOP_MARGIN 1.0f
#define BOTTOM_MARGIN .1f
#define TOP_MARGIN .1f

static float cmax (bool u1, float v1, bool u2, float v2)
{
  if (u1 && u2)
    return MAX (v1, v2);
  else if (u1)
    return v1;
  return v2;
}

void ClingyPath::FitToTerrain (size_t idx, float width, iMeshWrapper* thisMesh)
{
  const csVector3& pos = points[idx].pos;
  csVector3 right = (width / 2.0) * (points[idx].front % points[idx].up);
  csVector3 rightPos = pos + right;
  csVector3 leftPos = pos - right;

  float dyL, dy, dyR;
  bool hL = HeightDiff (leftPos, thisMesh, dyL);
  bool h  = HeightDiff (pos, thisMesh, dy);
  bool hR = HeightDiff (rightPos, thisMesh, dyR);

  if ((hL && dyL > TOP_MARGIN) || (hR && dyR > TOP_MARGIN))
  {
    // First we lower the segment. After that we check if it is
    // not too low.
    float lowerY = cmax (hL, dyL-TOP_MARGIN, hR, dyR-TOP_MARGIN);
    points[idx].pos.y -= lowerY;
    dy -= lowerY;
    dyL -= lowerY;
    dyR -= lowerY;
#   if VERBOSE
    printf ("        FitToTerrain %d, dy=%g/%g/%g, lowerY=%g\n",
	idx, dyL, dy, dyR, lowerY); fflush (stdout);
#   endif
  }

  if ((hL && dyL < BOTTOM_MARGIN) || (h && dy < BOTTOM_MARGIN) || (hR && dyR < BOTTOM_MARGIN))
  {
    // Some part of this point gets (almost) under the terrain. Need to raise
    // everything.
    float raiseY = cmax (hL, BOTTOM_MARGIN-dyL, hR, BOTTOM_MARGIN-dyR);
    raiseY = cmax (true, raiseY, h, BOTTOM_MARGIN-dy);
    points[idx].pos.y += raiseY;
#   if VERBOSE
    printf ("        FitToTerrain %d, dy=%g/%g/%g, raiseY=%g\n",
	idx, dyL, dy, dyR, raiseY); fflush (stdout);
#   endif
  }
  else
  {
#   if VERBOSE
    printf ("        FitToTerrain %d, dy=%g/%g/%g\n", idx, dyL, dy, dyR); fflush (stdout);
#   endif
  }
}

void ClingyPath::FixSlope (size_t idx)
{
  if (points.GetSize () <= 1) return;
  const csVector3& pos = points[idx].pos;
  csVector3 fr;
  if (idx == 0)
  {
    csVector3& p2 = points[idx+1].pos;
    fr = (p2 - pos).Unit ();
  }
  else if (idx == points.GetSize ()-1)
  {
    csVector3& p1 = points[idx-1].pos;
    fr = (pos - p1).Unit ();
  }
  else
  {
    csVector3& p1 = points[idx-1].pos;
    csVector3& p2 = points[idx+1].pos;
    fr = ((p2 - pos).Unit () + (pos - p1).Unit ()) / 2.0;
  }
  csVector3 right = fr % points[idx].up;
  points[idx].up = - (fr % right).Unit ();
}

static float Distance2d (const csVector3& p1, const csVector3& p2)
{
  csVector2 p1two (p1.x, p1.z);
  csVector2 p2two (p2.x, p2.z);
  csVector2 d = p1two - p2two;
  return sqrt (d*d);
}

float ClingyPath::GetTotalDistance ()
{
  size_t l = points.GetSize ();
  float totalDistance = 0.0f;
  for (size_t i = 0 ; i < l-1 ; i++)
  {
    float dist = sqrt (csSquaredDist::PointPoint (points[i].pos, points[i+1].pos));
    //float dist = Distance2d (points[i].pos, points[i+1].pos);
    totalDistance += dist;
  }
  return totalDistance;
}

void ClingyPath::GeneratePath (csPath& path)
{
  size_t l = points.GetSize ();
  path.Setup (l);
  for (size_t i = 0 ; i < l ; i++)
  {
    path.SetTime (i, points[i].time);
    path.SetPositionVector (i, points[i].pos);
    path.SetForwardVector (i, points[i].front);
    path.SetUpVector (i, points[i].up);
#   if VERBOSE
    printf ("        path %d (%g): pos:%g,%g,%g front:%g,%g,%g up:%g,%g,%g\n",
	i, points[i].time,
	points[i].pos.x, points[i].pos.y, points[i].pos.z,
	points[i].front.x, points[i].front.y, points[i].front.z,
	points[i].up.x, points[i].up.y, points[i].up.z);
#   endif
  }
}

void ClingyPath::GetSegmentTime (const csPath& path, size_t segIdx,
    float& startTime, float& endTime)
{
  const float* times = path.GetTimes ();
  startTime = times[segIdx];
  endTime = times[segIdx+1];
}

#define PATH_STEP .1f
#define LOOSE_BOTTOM_MARGIN .02f
#define LOOSE_TOP_MARGIN 1.0f

void ClingyPath::CalcMinMaxDY (size_t segIdx, float width, iMeshWrapper* thisMesh,
    float& maxRaiseY, float& maxLowerY)
{
  csPath path (1);
  GeneratePath (path);
  float startTime, endTime;
  GetSegmentTime (path, segIdx, startTime, endTime);

  maxRaiseY = -1.0f;
  maxLowerY = -1.0f;

  float dist = sqrt (csSquaredDist::PointPoint (points[segIdx].pos, points[segIdx+1].pos));
  int steps = int (dist / PATH_STEP);
  if (steps <= 1) return;	// Segment is too small. Don't do anything.
  float timeStep = (endTime-startTime) / float (steps);
  if (timeStep < 0.0001) return;	// Segment is too small. Don't do anything.
# if VERBOSE
  printf ("  CalcMinMaxDY: segIdx=%d dist=%g steps=%d time:%g/%g timeStep=%g\n",
      segIdx, dist, steps, startTime, endTime, timeStep);
  Dump (10);
# endif
  for (float t = startTime ; t <= endTime ; t += timeStep)
  {
    path.CalculateAtTime (t);
    csVector3 pos, front, up;
    path.GetInterpolatedPosition (pos);
    path.GetInterpolatedForward (front);
    path.GetInterpolatedUp (up);
    csVector3 right = (width / 2.0) * (front % up);
    csVector3 rightPos = pos + right;
    csVector3 leftPos = pos - right;

    float dyL, dy, dyR;
    bool hL = HeightDiff (leftPos, thisMesh, dyL);
    bool h  = HeightDiff (pos, thisMesh, dy);
    bool hR = HeightDiff (rightPos, thisMesh, dyR);
    if ((hL && dyL > LOOSE_TOP_MARGIN) || (hR && dyR > LOOSE_TOP_MARGIN))
    {
      float lowerY = cmax (hL, dyL-LOOSE_TOP_MARGIN, hR, dyR-LOOSE_TOP_MARGIN);
      if (lowerY > 0.0f)
      {
	if (hL && dyL-lowerY < LOOSE_BOTTOM_MARGIN)
	  lowerY = dyL - LOOSE_BOTTOM_MARGIN;
	if (hR && dyR-lowerY < LOOSE_BOTTOM_MARGIN)
	  lowerY = dyR - LOOSE_BOTTOM_MARGIN;
	if (h && dy-lowerY < LOOSE_BOTTOM_MARGIN)
	  lowerY = dy - LOOSE_BOTTOM_MARGIN;
      }
      if (lowerY > maxLowerY) maxLowerY = lowerY;
#     if VERBOSE
      printf ("        CalcMinMaxDY %g, dy=%g/%g/%g, lowerY=%g\n",
	  t, dyL, dy, dyR, lowerY); fflush (stdout);
#     endif
    }

    if ((hL && dyL < LOOSE_BOTTOM_MARGIN) || (h && dy < LOOSE_BOTTOM_MARGIN) || (hR && dyR < LOOSE_BOTTOM_MARGIN))
    {
      // Some part of this point gets (almost) under the terrain. Need to raise
      // everything.
      float raiseY = cmax (hL, LOOSE_BOTTOM_MARGIN-dyL, hR, LOOSE_BOTTOM_MARGIN-dyR);
      raiseY = cmax (true, raiseY, h, LOOSE_BOTTOM_MARGIN-dy);
      if (raiseY > maxRaiseY) maxRaiseY = raiseY;
#     if VERBOSE
      printf ("        CalcMinMaxDY %g, dy=%g/%g/%g, raiseY=%g\n",
	  t, dyL, dy, dyR, raiseY); fflush (stdout);
#     endif
    }
    else
    {
#     if VERBOSE
      printf ("        CalcMinMaxDY %g, dy=%g/%g/%g\n",
	  t, dyL, dy, dyR); fflush (stdout);
#     endif
    }
  }
}

static float FindHorizontalMiddlePoint (csPath& path,
    float error,
    float t1, const csVector3& pos1,
    float t2, const csVector3& pos2)
{
  float time = (t1+t2) / 2.0f;
  path.CalculateAtTime (time);
  csVector3 pos;
  path.GetInterpolatedPosition (pos);
  float d1 = Distance2d (pos1, pos);
  float d2 = Distance2d (pos, pos2);
# if VERBOSE
  printf ("t1=%g t2=%g time=%g d1=%g d2=%g error=%g\n",
      t1, t2, time, d1, d2, error); fflush (stdout);
# endif
  if (fabs (d1-d2) < error)
    return time;
  if (d1 > d2)
    return FindHorizontalMiddlePoint (path, error, t1, pos1, time, pos);
  else
    return FindHorizontalMiddlePoint (path, error, time, pos, t2, pos2);
}

void ClingyPath::SplitSegment (size_t segIdx)
{
  csPath path (1);
  GeneratePath (path);
  float startTime, endTime;
  GetSegmentTime (path, segIdx, startTime, endTime);

  //float time = (startTime+endTime) / 2.0f;
  csVector3 pos1, pos2;
  path.CalculateAtTime (startTime);
  path.GetInterpolatedPosition (pos1);
  path.CalculateAtTime (endTime);
  path.GetInterpolatedPosition (pos2);
  float time = FindHorizontalMiddlePoint (path, 0.01f, startTime, pos1,
      endTime, pos2);

  path.CalculateAtTime (time);
  PathEntry pe;
  pe.time = time;
  path.GetInterpolatedPosition (pe.pos);
  path.GetInterpolatedForward (pe.front);
  path.GetInterpolatedUp (pe.up);
# if VERBOSE
  printf ("  Split segment %d (time: %g/%g -> %g) pos=%g,%g,%g\n",
      segIdx, startTime, endTime, time, pe.pos.x, pe.pos.y, pe.pos.z);
  printf ("      seg1: %g,%g,%g\n",
      points[segIdx].pos.x, points[segIdx].pos.y, points[segIdx].pos.z);
  printf ("      seg2: %g,%g,%g\n",
      points[segIdx+1].pos.x, points[segIdx+1].pos.y, points[segIdx+1].pos.z);
  fflush (stdout);
# endif

  points.Insert (segIdx+1, pe);
}

void ClingyPath::Flatten (iMeshWrapper* thisMesh, float width)
{
  RefreshWorkingPath ();

  // Flatten the terrain first.
  for (size_t i = 0 ; i < points.GetSize () ; i++)
    FitToTerrain (i, width, thisMesh);
  Dump (2);

  // Now fix the slope.
  for (size_t i = 0 ; i < points.GetSize () ; i++)
    FixSlope (i);

  size_t segIdx = 0;
  float maxRaiseY, maxLowerY;
  while (segIdx < points.GetSize ()-1)
  {
    CalcMinMaxDY (segIdx, width, thisMesh, maxRaiseY, maxLowerY);
    if (maxRaiseY > 0.0f || maxLowerY > 0.0f)
    {
      // The segment needs improving. Let's split it.
      SplitSegment (segIdx);
      FitToTerrain (segIdx+1, width, thisMesh);
      FixSlope (segIdx);
      FixSlope (segIdx+1);
      FixSlope (segIdx+2);
    }
    else
    {
      // This segment is ok. We can go to the next.
      segIdx++;
    }
  }
}

void ClingyPath::Dump (int indent)
{
# if VERBOSE
  static char* sspaces = "                                                    ";
  char spaces[100];
  strcpy (spaces, sspaces);
  spaces[indent] = 0;
  for (size_t i = 0 ; i < points.GetSize () ; i++)
    printf ("%s%d: pos:%g,%g,%g front:%g,%g,%g up:%g,%g,%g\n", spaces, i,
	points[i].pos.x, points[i].pos.y, points[i].pos.z,
	points[i].front.x, points[i].front.y, points[i].front.z,
	points[i].up.x, points[i].up.y, points[i].up.z
	);
  fflush (stdout);
  spaces[indent] = ' ';
# endif
}

//---------------------------------------------------------------------------------------

CurvedFactory::CurvedFactory (CurvedMeshCreator* creator, const char* name) :
  scfImplementationType (this), creator (creator), name (name)
{
  material = 0;
  width = 1.0f;
  sideHeight = 0.4f;
  offsetHeight = 0.1f;

  factory = creator->engine->CreateMeshFactory (
	"crystalspace.mesh.object.genmesh", name);
  state = scfQueryInterface<iGeneralFactoryState> (factory->GetMeshObjectFactory ());
}

CurvedFactory::~CurvedFactory ()
{
}

void CurvedFactory::GenerateGeometry (iMeshWrapper* thisMesh)
{
# if VERBOSE
  printf ("#############################################################\n");
  fflush (stdout);
# endif
  csFlags oldFlags = thisMesh->GetFlags ();
  thisMesh->GetFlags ().Set (CS_ENTITY_NOHITBEAM);

  clingyPath.SetBasePoints (anchorPoints);
# if VERBOSE
  printf ("GenerateGeometry: Flatten\n"); fflush (stdout);
# endif
  clingyPath.Flatten (thisMesh, width);
  csPath path (1);
# if VERBOSE
  printf ("GenerateGeometry: GeneratePath\n"); fflush (stdout);
# endif
  clingyPath.GeneratePath (path);
  printf ("Path has %d control points\n",
      clingyPath.GetWorkingPointCount ());
  fflush (stdout);
  float totalDistance = clingyPath.GetTotalDistance ();

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

  thisMesh->GetFlags ().SetAll (oldFlags.Get ());
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
  if (fabs (sideHeight) < 0.0001f) sideHeight = 0.2f;
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

  return true;
}

//---------------------------------------------------------------------------------------

CurvedFactoryTemplate::CurvedFactoryTemplate (CurvedMeshCreator* creator,
    const char* name) : scfImplementationType (this), creator (creator), name (name)
{
  width = 1.0f;
  sideHeight = 0.2f;
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

void CurvedMeshCreator::DeleteCurvedFactoryTemplates ()
{
  factoryTemplates.DeleteAll ();
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

