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

#ifndef __ARES_CURVEMESH_IMP_H__
#define __ARES_CURVEMESH_IMP_H__

#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "csutil/weakref.h"
#include "csutil/set.h"
#include "csutil/hash.h"
#include "csutil/eventhandlers.h"
#include "csutil/refarr.h"
#include "csutil/parray.h"
#include "iutil/comp.h"
#include "iutil/virtclk.h"

#include "csgeom/vector3.h"
#include "csgeom/math3d.h"
#include "csgeom/path.h"

#include "imesh/genmesh.h"
#include "iengine/material.h"
#include "iengine/engine.h"
#include "iengine/mesh.h"

#include "include/igeometrygen.h"
#include "include/icurvemesh.h"

CS_PLUGIN_NAMESPACE_BEGIN(CurvedMesh)
{

struct PathEntry
{
  csVector3 pos, front, up;
  float time;
  PathEntry () { }
  PathEntry (const csVector3& pos, const csVector3& front, const csVector3& up) :
    pos (pos), front (front), up (up) { }
};

class ClingyPath
{
private:
  /// The base points.
  csArray<PathEntry> basePoints;

  /// The working points.
  csArray<PathEntry> points;

  /**
   * Given a segment index, return the start and end time
   * on the path.
   */
  void GetSegmentTime (const csPath& path, size_t segIdx,
      float& startTime, float& endTime);

public:
  /// Set the base points.
  void SetBasePoints (const csArray<PathEntry> pts);

  /**
   * Refresh working path. Copy the base path to the working path.
   */
  void RefreshWorkingPath ();

  /**
   * Fit a point of the working path to the ground.
   */
  void FitToTerrain (size_t idx, float width, iMeshWrapper* thisMesh);

  /**
   * Fix slope of the working path at this index so that the up vector points
   * correctly depending on the height of the neighbouring points.
   */
  void FixSlope (size_t idx);

  /**
   * Calculate how much this segment would have to raise or lower in
   * order to better fit the landscape.
   */
  void CalcMinMaxDY (size_t segIdx, float width, iMeshWrapper* thisMesh,
      float& maxRaiseY, float& maxLowerY);

  /**
   * Split a segment.
   */
  void SplitSegment (size_t segIdx);

  /**
   * The whole thing. Take the base points and generate a new set
   * of points that nicely matches the landscape.
   */
  void Flatten (iMeshWrapper* thisMesh, float width);

  /// Generate a path from the working points.
  void GeneratePath (csPath& path);

  /// Calculate the total distance of the path.
  float GetTotalDistance ();

  /// Dump this path.
  void Dump (int indent);

  /// Get the number of working points.
  size_t GetWorkingPointCount () const { return points.GetSize (); }
};

class CurvedMeshCreator;

class CurvedFactory : public scfImplementation2<CurvedFactory, iCurvedFactory,
  iGeometryGenerator>
{
private:
  CurvedMeshCreator* creator;
  csString name;
  iMaterialWrapper* material;
  float width;
  float sideHeight;
  float offsetHeight;

  csRef<iMeshFactoryWrapper> factory;
  csRef<iGeneralFactoryState> state;

  ClingyPath clingyPath;
  csArray<PathEntry> anchorPoints;

public:
  CurvedFactory (CurvedMeshCreator* creator, const char* name);
  virtual ~CurvedFactory ();

  virtual const char* GetName () const { return name; }
  virtual iMeshFactoryWrapper* GetFactory () { return factory; }
  virtual void SetMaterial (const char* materialName);
  virtual void SetCharacteristics (float width, float sideHeight);
  virtual float GetWidth () const { return width; }
  virtual float GetSideHeight () const { return sideHeight; }
  virtual size_t AddPoint (const csVector3& pos, const csVector3& front,
      const csVector3& up);
  virtual void ChangePoint (size_t idx, const csVector3& pos, const csVector3& front,
      const csVector3& up);
  virtual void DeletePoint (size_t idx);
  virtual size_t GetPointCount () const
  {
    return anchorPoints.GetSize ();
  }
  virtual const csVector3& GetPosition (size_t idx) const
  {
    return anchorPoints[idx].pos;
  }
  virtual const csVector3& GetFront (size_t idx) const
  {
    return anchorPoints[idx].front;
  }
  virtual const csVector3& GetUp (size_t idx) const
  {
    return anchorPoints[idx].up;
  }

  void Save (iDocumentNode* node, iSyntaxService* syn);
  bool Load (iDocumentNode* node, iSyntaxService* syn);

  virtual void GenerateGeometry (iMeshWrapper* mesh);
};

class CurvedFactoryTemplate : public scfImplementation1<CurvedFactoryTemplate,
  iCurvedFactoryTemplate>
{
private:
  CurvedMeshCreator* creator;
  csString name;
  csString material;
  float width;
  float sideHeight;

  csArray<PathEntry> points;
  csHash<csString,csString> attributes;

public:
  CurvedFactoryTemplate (CurvedMeshCreator* creator, const char* name);
  virtual ~CurvedFactoryTemplate ();

  virtual const char* GetName () const { return name; }
  virtual void SetAttribute (const char* name, const char* value);
  virtual const char* GetAttribute (const char* name) const;

  virtual void SetMaterial (const char* materialName);
  virtual void SetCharacteristics (float width, float sideHeight);
  virtual size_t AddPoint (const csVector3& pos, const csVector3& front,
      const csVector3& up);

  const char* GetMaterial () const { return material; }
  float GetWidth () const { return width; }
  float GetSideHeight () const { return sideHeight; }
  const csArray<PathEntry>& GetPoints () const { return points; }
  //void Save (iDocumentNode* node, iSyntaxService* syn);
  //bool Load (iDocumentNode* node, iSyntaxService* syn);
};

class CurvedMeshCreator : public scfImplementation2<CurvedMeshCreator,
  iCurvedMeshCreator, iComponent>
{
public:
  iObjectRegistry *object_reg;
  csRef<iEngine> engine;
  csRefArray<CurvedFactory> factories;
  csHash<CurvedFactory*,csString> factory_hash;
  csRefArray<CurvedFactoryTemplate> factoryTemplates;

  CurvedFactoryTemplate* FindFactoryTemplate (const char* name);

public:
  CurvedMeshCreator (iBase *iParent);
  virtual ~CurvedMeshCreator ();
  virtual bool Initialize (iObjectRegistry *object_reg);

  virtual iCurvedFactoryTemplate* AddCurvedFactoryTemplate (const char* name);
  virtual size_t GetCurvedFactoryTemplateCount () const
  {
    return factoryTemplates.GetSize ();
  }
  virtual iCurvedFactoryTemplate* GetCurvedFactoryTemplate (size_t idx) const
  {
    return factoryTemplates[idx];
  }
  virtual void DeleteCurvedFactoryTemplates ();

  virtual void DeleteFactories ();
  virtual iCurvedFactory* AddCurvedFactory (const char* name, const char* templatename);
  virtual size_t GetCurvedFactoryCount () const
  {
    return factories.GetSize ();
  }
  virtual iCurvedFactory* GetCurvedFactory (size_t idx) const
  {
    return factories[idx];
  }
  virtual iCurvedFactory* GetCurvedFactory (const char* name) const
  {
    return factory_hash.Get (name, 0);
  }

  virtual void Save (iDocumentNode* node);
  virtual csRef<iString> Load (iDocumentNode* node);
};

}
CS_PLUGIN_NAMESPACE_END(CurvedMesh)

#endif // __ARES_CURVEMESH_IMP_H__
