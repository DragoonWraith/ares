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
#include "decalmode.h"
#include "iengine/sector.h"
#include "iengine/meshgen.h"
#include "editor/i3dview.h"

#include <wx/xrc/xmlres.h>

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(DecalMode::Panel, wxPanel)
END_EVENT_TABLE()

SCF_IMPLEMENT_FACTORY (DecalMode)

//---------------------------------------------------------------------------

DecalMode::DecalMode (iBase* parent) : scfImplementationType (this, parent)
{
  name = "Decals";
  panel = 0;
}

void DecalMode::SetTopLevelParent (wxWindow* toplevel)
{
}

void DecalMode::BuildMainPanel (wxWindow* parent)
{
  if (panel)
  {
    //parent->GetSizer ()->Remove (panel);
    parent->GetSizer ()->Detach (panel);
    delete panel;
  }
  panel = new Panel (this);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("DecalModePanel"));
}

bool DecalMode::Initialize (iObjectRegistry* object_reg)
{
  if (!ViewMode::Initialize (object_reg)) return false;
  decalMgr = csQueryRegistry<iDecalManager> (object_reg);
  return true;
}

void DecalMode::UpdateTemplateList ()
{
  wxListBox* tplList = XRCCTRL (*panel, "templateListBox", wxListBox);
  tplList->Clear ();
  wxArrayString templates;

#if 0
  for (size_t i = 0 ; i < nature->GetFoliageDensityMapCount () ; i++)
  {
    wxString name = wxString::FromUTF8 (nature->GetFoliageDensityMapName (i));
    foliage.Add (name);
  }
#endif
  tplList->InsertItems (templates, 0);
}

#if 0
bool DecalMode::OnTypeListSelection (const CEGUI::EventArgs&)
{
  return true;
}
#endif

void DecalMode::Start ()
{
  ViewMode::Start ();
  UpdateTemplateList ();
}

void DecalMode::Stop ()
{
  ViewMode::Stop ();
}

void DecalMode::MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
    const csVector3& pos, uint button, uint32 modifiers)
{
}

void DecalMode::MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos)
{
}

void DecalMode::MarkerStopDragging (iMarker* marker, iMarkerHitArea* area)
{
}

void DecalMode::FramePre()
{
  ViewMode::FramePre ();
}

void DecalMode::Frame3D()
{
  ViewMode::Frame3D ();
}

void DecalMode::Frame2D()
{
  ViewMode::Frame2D ();
}

bool DecalMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  return ViewMode::OnKeyboard (ev, code);
}

bool DecalMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
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

bool DecalMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
{
  return ViewMode::OnMouseUp (ev, but, mouseX, mouseY);
}

bool DecalMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return ViewMode::OnMouseMove (ev, mouseX, mouseY);
}

