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

#include <crystalspace.h>

#include "meshview.h"
#include "uimanager.h"
#include "imagepanel.h"

//-----------------------------------------------------------------------------

MeshView::MeshView (iObjectRegistry* object_reg, wxWindow* parent) :
  object_reg (object_reg)
{
  imagePanel = new ImagePanel (parent);
  parent->GetSizer ()->Add (imagePanel, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
  engine = csQueryRegistry<iEngine> (object_reg);
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  meshOnTexture = 0;
  reldist = 1.0f;
  imagePanel->Connect (wxEVT_MOUSEWHEEL, wxMouseEventHandler (MeshView :: OnMouseWheel), 0, this);
}

MeshView::~MeshView ()
{
  Cleanup ();
  imagePanel->Disconnect (wxEVT_MOUSEWHEEL, wxMouseEventHandler (MeshView :: OnMouseWheel), 0, this);
}

void MeshView::Cleanup ()
{
  RemoveMesh ();
  delete meshOnTexture;
  handle = 0;
  ClearGeometry ();
}

void MeshView::OnMouseWheel (wxMouseEvent& event)
{
  if (event.GetWheelRotation () < 0)
    ChangeRelativeDistance (-.3f);
  else
    ChangeRelativeDistance (.3f);
}

void MeshView::RemoveMesh ()
{
  if (mesh)
  {
    iSector* sector = mesh->GetMovable ()->GetSectors ()->Get (0);
    engine->RemoveObject (mesh);
    if (sector->GetMeshes ()->GetCount () == 0)
    {
      engine->RemoveObject (sector);
    }
    mesh = 0;
  }
}

iSector* MeshView::FindSuitableSector (int& num)
{
  num = 0;
  csString name;
  for (;;)
  {
    name.Format ("__sect__%d", num);
    if (!engine->FindSector (name))
    {
      iSector* sector = engine->CreateSector (name);
      csRef<iLight> light;
      iLightList* ll = sector->GetLights ();
      light = engine->CreateLight (0, csVector3 (-300, 300, -300), 1000, csColor (1, 1, 1));
      ll->Add (light);
      light = engine->CreateLight (0, csVector3 (300, 300, 300), 1000, csColor (1, 1, 1));
      ll->Add (light);
      return sector;
    }
    num++;
  }
}

void MeshView::RotateMesh (float seconds)
{
  if (!mesh) return;
  iMovable* movable = mesh->GetMovable ();
  csReversibleTransform trans = movable->GetTransform ();
  trans.RotateThis (csVector3 (0, 1, 0), seconds);
  movable->SetTransform (trans);
  movable->UpdateMove ();
  UpdateImageButton ();
}

bool MeshView::SetMesh (const char* name)
{
  meshName = name;

  RemoveMesh ();
  delete meshOnTexture;
  meshOnTexture = 0;

  if (!name) return false;
  iMeshFactoryWrapper* factory = engine->FindMeshFactory (name);
  if (!factory) return false;

  reldist = 1.0f;
  meshOnTexture = new csMeshOnTexture (object_reg);
  int num;
  iSector* sector = FindSuitableSector (num);
  mesh = engine->CreateMeshWrapper (factory, name, sector);
  iCamera* cam = meshOnTexture->GetView ()->GetCamera ();
  cam->SetSector (sector);

  if (!handle)
  {
    iTextureManager* txtmgr = g3d->GetTextureManager ();
    handle = txtmgr->CreateTexture (256, 256, csimg2D, "rgba8", CS_TEXTURE_3D);
  }

  // Position camera and render.
  cam->GetTransform ().SetOrigin (csVector3 (0, 0, -10.0f));
  int iw, ih;
  handle->GetRendererDimensions (iw, ih);
  meshOnTexture->ScaleCamera (mesh, iw, ih);
  cam->Move (csVector3 (0, 1, -.5) * reldist);
  cam->GetTransform ().LookAt (-cam->GetTransform ().GetOrigin (), csVector3 (0, 1, 0));

  UpdateImageButton ();

  return true;
}

void MeshView::ChangeRelativeDistance (float d)
{
  reldist += d;
  if (reldist < 0.1f) reldist = 0.1f;

  // Position camera and render.
  iCamera* cam = meshOnTexture->GetView ()->GetCamera ();
  cam->GetTransform ().SetOrigin (csVector3 (0, 0, -10.0f));
  int iw, ih;
  handle->GetRendererDimensions (iw, ih);
  meshOnTexture->ScaleCamera (mesh, iw, ih);
  cam->Move (csVector3 (0, 1, -.5) * reldist);
  cam->GetTransform ().LookAt (-cam->GetTransform ().GetOrigin (), csVector3 (0, 1, 0));
}

size_t MeshView::CreatePen (float r, float g, float b, float width)
{
  iGraphics2D* g2d = g3d->GetDriver2D ();
  csPen* pen = new csPen (g2d, g3d);
  pen->SetColor (r, g, b, 1.0f);
  pen->SetPenWidth (width);
  csPen3D* pen3d = new csPen3D (g2d, g3d);
  pen3d->SetColor (r, g, b, 1.0f);
  pens3d.Push (pen3d);
  return pens.Push (pen);
}

void MeshView::AddSphere (const csVector3& center, float radius, size_t penIdx)
{
  MVSphere mvb;
  mvb.center = center;
  mvb.radius = radius;
  mvb.penIdx = penIdx;
  spheres.Push (mvb);
}

void MeshView::AddBox (const csBox3& box, size_t penIdx)
{
  csPen3D* pen = pens3d[penIdx];
  pen->SetActiveCache (&penCache);
  pen->DrawBox (box);
  pen->SetActiveCache (0);
}

void MeshView::AddCylinder (const csVector3& center, float radius, float length,
      size_t penIdx)
{
  csPen3D* pen = pens3d[penIdx];
  pen->SetActiveCache (&penCache);
  csBox3 box;
  box.SetCenter (center);
  box.SetSize (csVector3 (radius * 2, length, radius * 2));
  pen->DrawCylinder (box, CS_AXIS_Y);
  pen->SetActiveCache (0);
}

void MeshView::AddMesh (const csVector3& center, size_t penIdx)
{
  csRef<iStringSet> stringSet = csQueryRegistryTagInterface<iStringSet> (object_reg,
    "crystalspace.shared.stringset");
  csStringID base_id = stringSet->Request ("base");
  csStringID id = stringSet->Request ("colldet");
  iObjectModel* objmodel = mesh->GetMeshObject ()->GetObjectModel ();
  csRef<iTriangleMesh> trimesh;
  if (objmodel->IsTriangleDataSet (id))
    trimesh = objmodel->GetTriangleData (id);
  else
    trimesh = objmodel->GetTriangleData (base_id);
  if (trimesh)
  {
    csPen3D* pen = pens3d[penIdx];
    pen->SetActiveCache (&penCache);
    csArray<csPen3DCoordinatePair> pairs;

    csVector3* vt = trimesh->GetVertices ();
    size_t pocount = trimesh->GetTriangleCount ();
    csTriangle* po = trimesh->GetTriangles ();
    for (size_t i = 0 ; i < pocount ; i++)
    {
      csTriangle& tri = po[i];
      pairs.Push (csPen3DCoordinatePair (vt[tri.a], vt[tri.c]));
      pairs.Push (csPen3DCoordinatePair (vt[tri.b], vt[tri.a]));
      pairs.Push (csPen3DCoordinatePair (vt[tri.c], vt[tri.b]));
    }

    pen->DrawLines (pairs);
    pen->SetActiveCache (0);
  }
}

static void DrawSphere3D (const csVector3& c, float radius, float fov, csPen* pen,
  int width, int height)
{
  if (c.z < SMALL_Z)
    return;

  float x = c.x, y = c.y, z = c.z;

  float iz = fov / z;
  int px = csQint (x * iz + (width / 2));
  int py = height - 1 - csQint (y * iz + (height / 2));

  radius = fov * radius / z;

  pen->DrawArc (px-radius, py-radius, px+radius, py+radius);
}

void MeshView::RenderSpheres (const csOrthoTransform& camtrans,
    const csReversibleTransform& meshtrans)
{
  if (spheres.GetSize () == 0) return;
  csReversibleTransform trans = camtrans / meshtrans;
  float fov = 256;	// @@@
  for (size_t i = 0 ; i < spheres.GetSize () ; i++)
  {
    const csVector3& c = spheres[i].center;
    float radius  = spheres[i].radius;
    csPen* pen = pens[spheres[i].penIdx];
    DrawSphere3D (trans * c, radius, fov, pen, 256, 256);
  }
}

void MeshView::ClearGeometry ()
{
  penCache.Clear ();
  spheres.DeleteAll ();
}

void MeshView::RenderGeometry ()
{
  g3d->SetRenderTarget (handle);

  iCamera* cam = meshOnTexture->GetView ()->GetCamera ();
  const csOrthoTransform& camtrans = cam->GetTransform ();
  iMovable* movable = mesh->GetMovable ();
  const csReversibleTransform& meshtrans = movable->GetTransform ();

  // Render the 2D primitives:
  g3d->BeginDraw (CSDRAW_2DGRAPHICS);
  RenderSpheres (camtrans, meshtrans);

  // Render the 3D primitives:
  g3d->BeginDraw (CSDRAW_3DGRAPHICS);
  csReversibleTransform oldw2c = g3d->GetWorldToCamera ();
  g3d->SetWorldToCamera (camtrans.GetInverse ());

  penCache.SetTransform (meshtrans);
  penCache.Render (g3d);

  g3d->SetWorldToCamera (oldw2c);

  // Finish.
  g3d->FinishDraw ();
  g3d->SetRenderTarget (0);
}


void MeshView::UpdateImageButton ()
{
  meshOnTexture->Render (mesh, handle, false);

  // Actually make sure the rendermanager renders.
  iRenderManager* rm = engine->GetRenderManager ();
  rm->RenderView (meshOnTexture->GetView ());

  RenderGeometry ();

  // Convert the image to a WX image.
  CS::StructuredTextureFormat format = CS::TextureFormatStrings::ConvertStructured (
      "rgba8");
  csRef<iDataBuffer> buf = handle->Readback (format);

  int channels = 4;
  int width, height;
  handle->GetRendererDimensions (width, height);
  size_t total = width * height;

  wxImage wximage = wxImage (width, height, false);
  // Memory owned and free()d by wxImage
  unsigned char* rgb = (unsigned char*) malloc (total * 3);
  //unsigned char* alpha = (unsigned char*) malloc (total);
  unsigned char* source = (unsigned char*)buf->GetData ();
  for (size_t i = 0; i < total; i++)
  {
    rgb[(3*i)+0] = source[(channels*i)+3];
    rgb[(3*i)+1] = source[(channels*i)+2];
    rgb[(3*i)+2] = source[(channels*i)+1];
    //alpha[i] = source[(channels*i)+0];
  }
  wximage.SetData (rgb);
  //wximage.SetAlpha (alpha);
 
  imagePanel->SetBitmap (wxBitmap (wximage));
  imagePanel->SetSize (wxSize (width,height));
  imagePanel->SetMinSize (wxSize (width,height));
  imagePanel->PaintNow ();
}

