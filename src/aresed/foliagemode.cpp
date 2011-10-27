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
#include "foliagemode.h"
#include "iengine/sector.h"
#include "iengine/meshgen.h"

//---------------------------------------------------------------------------

FoliageMode::FoliageMode (AppAresEdit* aresed, AresEdit3DView* aresed3d)
  : EditingMode (aresed, aresed3d, "Foliage")
{
#if 0
  CEGUI::WindowManager* winMgr = aresed->GetCEGUI ()->GetWindowManagerPtr ();
  //CEGUI::Window* btn;

  typeList = static_cast<CEGUI::MultiColumnList*>(winMgr->getWindow("Ares/FoliageWindow/Types"));
  typeList->subscribeEvent(CEGUI::MultiColumnList::EventSelectionChanged,
    CEGUI::Event::Subscriber(&FoliageMode::OnTypeListSelection, this));
#endif
}

void FoliageMode::UpdateTypeList ()
{
#if 0
  iNature* nature = aresed3d->GetNature ();
  typeList->resetList ();
  for (size_t i = 0 ; i < nature->GetFoliageDensityMapCount () ; i++)
  {
    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem (CEGUI::String (
	  nature->GetFoliageDensityMapName (i)));
    item->setTextColours (CEGUI::colour(0,0,0));
    item->setSelectionBrushImage ("ice", "TextSelectionBrush");
    item->setSelectionColours (CEGUI::colour(0.5f,0.5f,1));
    uint colid = typeList->getColumnID (0);
    typeList->addRow (item, colid);
  }
#endif
}

#if 0
bool FoliageMode::OnTypeListSelection (const CEGUI::EventArgs&)
{
  return true;
}
#endif

void FoliageMode::Start ()
{
  UpdateTypeList ();
  // @@@ Hardcoded!
  iSector* sector = aresed3d->GetCsCamera ()->GetSector ();
  meshgen = sector->GetMeshGeneratorByName ("grass");
}

void FoliageMode::Stop ()
{
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
}

void FoliageMode::Frame3D()
{
}

void FoliageMode::Frame2D()
{
}

bool FoliageMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  return false;
}

bool FoliageMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  csSegment3 seg = aresed3d->GetMouseBeam ();
  csVector3 isect;
  if (aresed3d->TraceBeamTerrain (seg.Start (), seg.End (), isect))
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
    iNature* nature = aresed3d->GetNature ();
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
  return false;
}

bool FoliageMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return false;
}

