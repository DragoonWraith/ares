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

#ifndef __aresed_entitymode_h
#define __aresed_entitymode_h

#include "csutil/csstring.h"
#include "editmodes.h"

struct iGeometryGenerator;
struct iCelPropertyClassTemplate;
struct iQuestStateFactory;
struct iRewardFactoryArray;

class EntityMode : public EditingMode
{
private:
  void SetupItems ();

  void BuildNewStateConnections (iRewardFactoryArray* rewards,
      const char* parentKey, const char* pcNodeName, const char* newKey = 0);
  void BuildStateGraph (iQuestStateFactory* state, const char* stateNameKey,
      const char* pcNodeName);
  void BuildQuestGraph (iCelPropertyClassTemplate* pctpl, const char* pcNodeName);
  void BuildTemplateGraph (const char* templateName);

  iMarkerColor* templateColorFG, * templateColorBG;
  iMarkerColor* pcColorFG, * pcColorBG;
  iMarkerColor* stateColorFG, * stateColorBG;
  iMarkerColor* thinLinkColor;

  iGraphView* view;
  iMarkerColor* NewColor (const char* name,
    float r0, float g0, float b0, float r1, float g1, float b1, bool fill);
  void InitColors ();

public:
  EntityMode (wxWindow* parent, AresEdit3DView* aresed3d);
  virtual ~EntityMode ();

  virtual void Start ();
  virtual void Stop ();

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);

  void OnTemplateSelect ();

  class Panel : public wxPanel
  {
  public:
    Panel(wxWindow* parent, EntityMode* s)
      : wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), s (s)
    {}

    void OnTemplateSelect (wxCommandEvent& event)
    {
      s->OnTemplateSelect ();
    }

  private:
    EntityMode* s;

    DECLARE_EVENT_TABLE()
  };
  Panel* panel;
};

#endif // __aresed_entitymode_h

