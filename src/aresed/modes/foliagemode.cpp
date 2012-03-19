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

#include "../apparesed.h"
#include "foliagemode.h"
#include "iengine/sector.h"
#include "iengine/meshgen.h"

#include <wx/xrc/xmlres.h>

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(FoliageMode::Panel, wxPanel)
END_EVENT_TABLE()

//---------------------------------------------------------------------------

FoliageMode::FoliageMode (wxWindow* parent, i3DView* view,
    iObjectRegistry* object_reg)
  : ViewMode (view, object_reg, "Foliage")
{
  panel = new Panel (parent, this);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("FoliageModePanel"));
  nature = csQueryRegistry<iNature> (object_reg);
}

void FoliageMode::UpdateTypeList ()
{
  wxListBox* foliageList = XRCCTRL (*panel, "foliageListBox", wxListBox);
  foliageList->Clear ();
  wxArrayString foliage;

  for (size_t i = 0 ; i < nature->GetFoliageDensityMapCount () ; i++)
  {
    wxString name = wxString::FromUTF8 (nature->GetFoliageDensityMapName (i));
    foliage.Add (name);
  }
  foliageList->InsertItems (foliage, 0);
}

#if 0
bool FoliageMode::OnTypeListSelection (const CEGUI::EventArgs&)
{
  return true;
}
#endif

void FoliageMode::Start ()
{
  ViewMode::Start ();
  UpdateTypeList ();
  // @@@ Hardcoded!
  iSector* sector = view3d->GetCsCamera ()->GetSector ();
  meshgen = sector->GetMeshGeneratorByName ("grass");
}

void FoliageMode::Stop ()
{
  ViewMode::Stop ();
}

void FoliageMode::MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
    const csVector3& pos, uint button, uint32 modifiers)
{
}

void FoliageMode::MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos)
{
}

void FoliageMode::MarkerStopDragging (iMarker* marker, iMarkerHitArea* area)
{
}

void FoliageMode::FramePre()
{
  ViewMode::FramePre ();
}

void FoliageMode::Frame3D()
{
  ViewMode::Frame3D ();
}

void FoliageMode::Frame2D()
{
  ViewMode::Frame2D ();
}

bool FoliageMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  return ViewMode::OnKeyboard (ev, code);
}

bool FoliageMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (ViewMode::OnMouseDown (ev, but, mouseX, mouseY))
    return true;

  csSegment3 seg = view3d->GetMouseBeam ();
  csVector3 isect;
  if (view3d->TraceBeamTerrain (seg.Start (), seg.End (), isect))
  {
#if 0
    CEGUI::ListboxItem* item = typeList->getFirstSelectedItem ();
    if (!item) return false;
    csString factorMapID = item->getText ().c_str ();
    const CS::Math::Matrix4& world2map = meshgen->GetWorldToMapTransform (factorMapID);
    int width = meshgen->GetDensityFactorMapWidth (factorMapID);
    int height = meshgen->GetDensityFactorMapHeight (factorMapID);
    isect.y = 0;
    csVector4 mapCoord (world2map * csVector4 (isect));
    iImage* image = nature->GetFoliageDensityMapImage (factorMapID);
    csRef<iDataBuffer> buf = image->GetRawData ();
    char* mapPtr = buf->GetData ();

    int mapX = int (mapCoord.x * width);
    printf ("mapX=%d width=%d\n", mapX, width); fflush (stdout);
    if ((mapX < 0) || (mapX >= width)) return false;
    int mapY = int (mapCoord.y * height);
    printf ("mapY=%d height=%d\n", mapY, height); fflush (stdout);
    if ((mapY < 0) || (mapY >= height)) return false;
    uint8 val = mapPtr[mapY * width + mapX];
    float density = val * (1.0f/255.0f);
    printf ("density=%g\n", density); fflush (stdout);
    if (val <= 250) val += 5;
    mapPtr[mapY * width + mapX] = val;
    meshgen->UpdateDensityFactorMap (factorMapID, image);
    //meshgen->ClearPosition (isect);
    meshgen->SetDefaultDensityFactor (meshgen->GetDefaultDensityFactor ());
    return true;
#endif
  }
  return false;
}

bool FoliageMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
{
  return ViewMode::OnMouseUp (ev, but, mouseX, mouseY);
}

bool FoliageMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return ViewMode::OnMouseMove (ev, mouseX, mouseY);
}

