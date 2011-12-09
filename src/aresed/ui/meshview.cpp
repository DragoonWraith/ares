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

void MeshView::AddBox (const csBox3& box, int r, int g, int b)
{
  iGraphics2D* g2d = g3d->GetDriver2D ();
  MVBox mvb;
  mvb.box = box;
  mvb.color = g2d->FindRGB (r, g, b);
  boxes.Push (mvb);
}

void MeshView::Render2D ()
{
  if (boxes.GetSize () == 0) return;

  g3d->SetRenderTarget (handle);
  g3d->BeginDraw (CSDRAW_2DGRAPHICS);
  //iGraphics2D* g2d = g3d->GetDriver2D ();

  iCamera* cam = meshOnTexture->GetView ()->GetCamera ();
  const csOrthoTransform& camtrans = cam->GetTransform ();
  iMovable* movable = mesh->GetMovable ();
  const csReversibleTransform& meshtrans = movable->GetTransform ();
  csReversibleTransform trans = camtrans / meshtrans;
  float fov = 256;	// @@@
  for (size_t i = 0 ; i < boxes.GetSize () ; i++)
  {
    const csBox3& b = boxes[i].box;
    int color = boxes[i].color;
    csVector3 xyz = trans * b.GetCorner (CS_BOX_CORNER_xyz);
    csVector3 Xyz = trans * b.GetCorner (CS_BOX_CORNER_Xyz);
    csVector3 xYz = trans * b.GetCorner (CS_BOX_CORNER_xYz);
    csVector3 xyZ = trans * b.GetCorner (CS_BOX_CORNER_xyZ);
    csVector3 XYz = trans * b.GetCorner (CS_BOX_CORNER_XYz);
    csVector3 XyZ = trans * b.GetCorner (CS_BOX_CORNER_XyZ);
    csVector3 xYZ = trans * b.GetCorner (CS_BOX_CORNER_xYZ);
    csVector3 XYZ = trans * b.GetCorner (CS_BOX_CORNER_XYZ);
    g3d->DrawLine (xyz, Xyz, fov, color);
    g3d->DrawLine (Xyz, XYz, fov, color);
    g3d->DrawLine (XYz, xYz, fov, color);
    g3d->DrawLine (xYz, xyz, fov, color);
    g3d->DrawLine (xyZ, XyZ, fov, color);
    g3d->DrawLine (XyZ, XYZ, fov, color);
    g3d->DrawLine (XYZ, xYZ, fov, color);
    g3d->DrawLine (xYZ, xyZ, fov, color);
    g3d->DrawLine (xyz, xyZ, fov, color);
    g3d->DrawLine (xYz, xYZ, fov, color);
    g3d->DrawLine (Xyz, XyZ, fov, color);
    g3d->DrawLine (XYz, XYZ, fov, color);
  }

  g3d->FinishDraw ();
  g3d->SetRenderTarget (0);
}


void MeshView::UpdateImageButton ()
{
  meshOnTexture->Render (mesh, handle, false);

  // Actually make sure the rendermanager renders.
  iRenderManager* rm = engine->GetRenderManager ();
  rm->RenderView (meshOnTexture->GetView ());

  Render2D ();

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

