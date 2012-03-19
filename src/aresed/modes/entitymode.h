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
#include "edcommon/editmodes.h"

struct iGeometryGenerator;
struct iCelEntityTemplate;
struct iCelPropertyClassTemplate;
struct iQuestFactory;
struct iQuestStateFactory;
struct iRewardFactoryArray;
struct iRewardFactory;
struct iCelSequenceFactory;
struct iTriggerFactory;
struct iQuestManager;
struct iQuestTriggerResponseFactory;

class PropertyClassPanel;
class TriggerPanel;
class RewardPanel;
class SequencePanel;
class EntityTemplatePanel;

enum
{
  ID_Template_Add = wxID_HIGHEST + 10000,
  ID_Template_Delete,
};

class EntityMode : public EditingMode
{
private:
  csRef<iCelPlLayer> pl;

  void SetupItems ();

  csString GetRewardsLabel (iRewardFactoryArray* rewards);
  void BuildRewardGraph (iRewardFactoryArray* rewards,
      const char* parentKey, const char* pcKey);
  void BuildStateGraph (iQuestStateFactory* state, const char* stateKey,
      const char* pcKey);
  void BuildQuestGraph (iQuestFactory* questFact, const char* pcKey, bool fullquest,
      const csString& defaultState);
  void BuildQuestGraph (iCelPropertyClassTemplate* pctpl, const char* pcKey);
  void BuildTemplateGraph (const char* templateName);
  csString GetQuestName (iCelPropertyClassTemplate* pctpl);
  csString GetExtraPCInfo (iCelPropertyClassTemplate* pctpl);
  void GetPCKeyLabel (iCelPropertyClassTemplate* pctpl, csString& key, csString& label);
  const char* GetRewardType (iRewardFactory* reward);
  const char* GetTriggerType (iTriggerFactory* reward);

  iMarkerColor* thinLinkColor;
  iMarkerColor* arrowLinkColor;

  csRef<iGraphNodeStyle> styleTemplate;
  csRef<iGraphNodeStyle> stylePC;
  csRef<iGraphNodeStyle> styleSequence;
  csRef<iGraphNodeStyle> styleState;
  csRef<iGraphNodeStyle> styleStateDefault;
  csRef<iGraphNodeStyle> styleResponse;
  csRef<iGraphNodeStyle> styleReward;

  csRef<iGraphLinkStyle> styleThickLink;
  csRef<iGraphLinkStyle> styleThinLink;
  csRef<iGraphLinkStyle> styleArrowLink;

  iGraphView* graphView;
  iMarkerColor* NewColor (const char* name,
    float r0, float g0, float b0, float r1, float g1, float b1, bool fill);
  iMarkerColor* NewColor (const char* name,
    float r0, float g0, float b0, float r1, float g1, float b1,
    float r2, float g2, float b2, bool fill);
  void InitColors ();

  csString currentTemplate;
  bool editQuestMode;		// If true we're editing a quest.
  csString contextMenuNode;	// Node that is being used for the context menu.
  csString GetContextMenuNode ();

  PropertyClassPanel* pcPanel;
  TriggerPanel* triggerPanel;
  RewardPanel* rewardPanel;
  SequencePanel* sequencePanel;
  EntityTemplatePanel* tplPanel;

  int idDelete, idCreate, idEditQuest, idNewState, idNewSequence, idDefaultState;
  int idCreateTrigger, idCreateReward, idCreateRewardOnInit, idCreateRewardOnExit;

  // Fetch a property class template from a given graph key.
  iCelPropertyClassTemplate* GetPCTemplate (const char* key);

  // Get the selected state.
  iQuestStateFactory* GetSelectedState (const char* key);
  csString GetSelectedStateName (const char* key);

  // Get the current quest.
  iQuestFactory* GetSelectedQuest (const char* key);

  // Get the name of the trigger.
  iQuestTriggerResponseFactory* GetSelectedTriggerResponse (const char* key);
  bool IsOnInit (const char* key);
  bool IsOnExit (const char* key);
  csRef<iRewardFactoryArray> GetSelectedReward (const char* key, size_t& idx);
  iCelSequenceFactory* GetSelectedSequence (const char* key);

  csRef<iFont> font;
  csRef<iFont> fontBold;
  csRef<iFont> fontLarge;

  csRef<iQuestManager> questMgr;

public:
  EntityMode (wxWindow* parent, i3DView* view, iObjectRegistry* object_reg);
  virtual ~EntityMode ();

  iQuestManager* GetQuestManager () const { return questMgr; }
  iCelPropertyClassTemplate* GetSelectedPC ()
  {
    return GetPCTemplate (GetContextMenuNode ());
  }

  /// Refresh the view. The tiven pctpl is optional and will be used if given.
  void RefreshView (iCelPropertyClassTemplate* pctpl = 0);

  virtual void Start ();
  virtual void Stop ();

  virtual void AllocContextHandlers (wxFrame* frame);
  virtual void AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY);

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);

  void OnTemplateSelect ();
  void OnDelete ();
  void OnCreatePC ();
  void OnEditQuest ();
  void OnNewState ();
  void OnNewSequence ();
  void OnDefaultState ();
  void OnCreateTrigger ();
  void OnCreateReward (int type); // 0 == normal, 1 == oninit, 2 == onexit
  void OnContextMenu (wxContextMenuEvent& event);
  void OnTemplateAdd ();
  void OnTemplateDel ();

  void PCWasEdited (iCelPropertyClassTemplate* pctpl);
  void ActivateNode (const char* nodeName);

  class Panel : public wxPanel
  {
  public:
    Panel(wxWindow* parent, EntityMode* s)
      : wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), s (s)
    {}

    void OnDelete (wxCommandEvent& event) { s->OnDelete (); }
    void OnCreatePC (wxCommandEvent& event) { s->OnCreatePC (); }
    void OnEditQuest (wxCommandEvent& event) { s->OnEditQuest (); }
    void OnNewState (wxCommandEvent& event) { s->OnNewState (); }
    void OnNewSequence (wxCommandEvent& event) { s->OnNewSequence (); }
    void OnDefaultState (wxCommandEvent& event) { s->OnDefaultState (); }
    void OnCreateTrigger (wxCommandEvent& event) { s->OnCreateTrigger (); }
    void OnCreateReward (wxCommandEvent& event) { s->OnCreateReward (0); }
    void OnCreateRewardOnInit (wxCommandEvent& event) { s->OnCreateReward (1); }
    void OnCreateRewardOnExit (wxCommandEvent& event) { s->OnCreateReward (2); }
    void OnTemplateSelect (wxCommandEvent& event) { s->OnTemplateSelect (); }
    void OnContextMenu (wxContextMenuEvent& event) { s->OnContextMenu (event); }
    void OnTemplateAdd (wxCommandEvent& event) { s->OnTemplateAdd (); }
    void OnTemplateDel (wxCommandEvent& event) { s->OnTemplateDel (); }

  private:
    EntityMode* s;

    DECLARE_EVENT_TABLE()
  };
  Panel* panel;
};

#endif // __aresed_entitymode_h

