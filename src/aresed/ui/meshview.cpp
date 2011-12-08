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

//-----------------------------------------------------------------------------


MeshView::MeshView (iObjectRegistry* object_reg, wxWindow* parent) :
  object_reg (object_reg)
{
  button = new wxBitmapButton (parent, wxID_ANY, wxNullBitmap, wxDefaultPosition,
      wxDefaultSize, wxBU_AUTODRAW);
  parent->GetSizer ()->Add (button, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
  engine = csQueryRegistry<iEngine> (object_reg);
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
      printf ("sector:%s\n", name.GetData ()); fflush (stdout);
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
  meshOnTexture->GetView ()->GetCamera ()->SetSector (sector);

  csRef<iGraphics3D> g3d = csQueryRegistry<iGraphics3D> (object_reg);
  if (!handle)
  {
    iTextureManager* txtmgr = g3d->GetTextureManager ();
    handle = txtmgr->CreateTexture (256, 256, csimg2D, "rgba8", CS_TEXTURE_3D);
  }

  // Position camera and render.
  iCamera* cam = meshOnTexture->GetView ()->GetCamera ();
  cam->GetTransform ().SetOrigin (csVector3 (0, 0, -10.0f));
  int iw, ih;
  handle->GetRendererDimensions (iw, ih);
  meshOnTexture->ScaleCamera (mesh, iw, ih);
  meshOnTexture->Render (mesh, handle, false);
  cam->Move (csVector3 (0, 0, -.5));

  // Actually make sure the rendermanager renders.
  iRenderManager* rm = engine->GetRenderManager ();
  rm->RenderView (meshOnTexture->GetView ());

  // Convert the image to a WX image.
  CS::StructuredTextureFormat format = CS::TextureFormatStrings::ConvertStructured (
      "rgba8");
  csRef<iDataBuffer> buf = handle->Readback (format);

  int channels = 4;
  int width, height;
  handle->GetRendererDimensions (width, height);
  size_t total = width * height;

  wxImage wximage = wxImage (width, height, false);
  unsigned char* rgb = 0;
  rgb = (unsigned char*) malloc (total * 3); //Memory owned and free()d by wxImage
  unsigned char* source = (unsigned char*)buf->GetData ();
  for (size_t i = 0; i < total; i++)
  {
    rgb[(3*i)+0] = source[(channels*i)+3];
    rgb[(3*i)+1] = source[(channels*i)+2];
    rgb[(3*i)+2] = source[(channels*i)+1];
  }
  wximage.SetData (rgb);
 
  // Put the image on a button.
  button->SetBitmapLabel (wxBitmap (wximage));
  button->SetMinSize (wxSize (width,height));
  return true;
}

