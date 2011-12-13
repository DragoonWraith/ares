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
}

MeshView::~MeshView ()
{
  RemoveMesh ();
  delete meshOnTexture;
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
  iMeshFactoryWrapper* factory = engine->FindMeshFactory (name);
  if (!factory) return false;
  printf ("Rendering mesh %s\n", name); fflush (stdout);
  RemoveMesh ();
  delete meshOnTexture;
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
  cam->Move (csVector3 (0, 1, -.5));
  cam->GetTransform ().LookAt (-cam->GetTransform ().GetOrigin (), csVector3 (0, 1, 0));

  UpdateImageButton ();
  return true;
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
  MVBox mvb;
  mvb.box = box;
  mvb.penIdx = penIdx;
  boxes.Push (mvb);
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

static void DrawLine3D (const csVector3& v1, const csVector3& v2, float fov,
  int width, int height, csArray<csPenCoordinatePair>& pairs)
{
  if (v1.z < SMALL_Z && v2.z < SMALL_Z)
    return;

  float x1 = v1.x, y1 = v1.y, z1 = v1.z;
  float x2 = v2.x, y2 = v2.y, z2 = v2.z;

  if (z1 < SMALL_Z)
  {
    // x = t*(x2-x1)+x1;
    // y = t*(y2-y1)+y1;
    // z = t*(z2-z1)+z1;
    float t = (SMALL_Z - z1) / (z2 - z1);
    x1 = t * (x2 - x1) + x1;
    y1 = t * (y2 - y1) + y1;
    z1 = SMALL_Z;
  }
  else if (z2 < SMALL_Z)
  {
    // x = t*(x2-x1)+x1;
    // y = t*(y2-y1)+y1;
    // z = t*(z2-z1)+z1;
    float t = (SMALL_Z - z1) / (z2 - z1);
    x2 = t * (x2 - x1) + x1;
    y2 = t * (y2 - y1) + y1;
    z2 = SMALL_Z;
  }
  float iz1 = fov / z1;
  int px1 = csQint (x1 * iz1 + (width / 2));
  int py1 = height - 1 - csQint (y1 * iz1 + (height / 2));
  float iz2 = fov / z2;
  int px2 = csQint (x2 * iz2 + (width / 2));
  int py2 = height - 1 - csQint (y2 * iz2 + (height / 2));

  pairs.Push (csPenCoordinatePair (px1, py1, px2, py2));
}

void MeshView::RenderSpheres (const csReversibleTransform& trans)
{
  if (spheres.GetSize () == 0) return;
  float fov = 256;	// @@@
  for (size_t i = 0 ; i < spheres.GetSize () ; i++)
  {
    const csVector3& c = spheres[i].center;
    float radius  = spheres[i].radius;
    csPen* pen = pens[spheres[i].penIdx];
    DrawSphere3D (trans * c, radius, fov, pen, 256, 256);
  }
}

void MeshView::RenderBoxes ()
{
  if (boxes.GetSize () == 0) return;

  iCamera* cam = meshOnTexture->GetView ()->GetCamera ();
  const csOrthoTransform& camtrans = cam->GetTransform ();
  csReversibleTransform oldw2c = g3d->GetWorldToCamera ();
  g3d->SetWorldToCamera (camtrans.GetInverse ());
  iMovable* movable = mesh->GetMovable ();
  const csReversibleTransform& meshtrans = movable->GetTransform ();

  for (size_t i = 0 ; i < boxes.GetSize () ; i++)
  {
    const csBox3& b = boxes[i].box;
    csPen3D* pen = pens3d[boxes[i].penIdx];
    pen->SetTransform (meshtrans);
    pen->DrawBox (b);
  }

  g3d->SetWorldToCamera (oldw2c);
}

void MeshView::ClearGeometry ()
{
  boxes.DeleteAll ();
  spheres.DeleteAll ();
}

void MeshView::RenderGeometry ()
{
  g3d->SetRenderTarget (handle);
  g3d->BeginDraw (CSDRAW_2DGRAPHICS);
  //iGraphics2D* g2d = g3d->GetDriver2D ();

  iCamera* cam = meshOnTexture->GetView ()->GetCamera ();
  const csOrthoTransform& camtrans = cam->GetTransform ();
  iMovable* movable = mesh->GetMovable ();
  const csReversibleTransform& meshtrans = movable->GetTransform ();
  csReversibleTransform trans = camtrans / meshtrans;
  RenderSpheres (trans);

  g3d->BeginDraw (CSDRAW_3DGRAPHICS);
  RenderBoxes ();

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
  imagePanel->SetMinSize (wxSize (width,height));
  imagePanel->PaintNow ();
}

