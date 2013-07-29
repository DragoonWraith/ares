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
#include "edcommon/model.h"
#include "physicallayer/datatype.h"

#include "cseditor/wx/propgrid/propgrid.h"

struct iGeometryGenerator;
struct iCelEntityTemplate;
struct iCelEntityTemplateIterator;
struct iCelPropertyClassTemplate;
struct iQuestFactory;
struct iQuestStateFactory;
struct iRewardFactoryArray;
struct iRewardFactory;
struct iCelSequenceFactory;
struct iTriggerFactory;
struct iQuestManager;
struct iQuestTriggerResponseFactory;
struct iCelParameterIterator;
struct iParameterManager;
struct iParameter;

class PropertyClassPanel;
class TriggerPanel;
class RewardPanel;
class SequencePanel;
class EntityTemplatePanel;
class EntityMode;
struct iAresEditor;

enum
{
  ID_Template_Add = wxID_HIGHEST + 10000,
  ID_Template_Delete,
};

//==================================================================================

/// A copy of a property class template.
struct PropertyClassCopy
{
  csString name;
  csString tag;
  csRef<iDocument> doc;
  csRef<iDocumentNode> node;

  iCelPropertyClassTemplate* Create (EntityMode* em, iCelEntityTemplate* tpl,
      const char* overridetag = 0);
};

/// A copy of an entity template.
struct EntityCopy
{
  csString name;
  csRef<iDocument> doc;
  csRef<iDocumentNode> node;

  iCelEntityTemplate* Create (EntityMode* em, const char* overridename);
};

/// A copy of a quest factory.
struct QuestCopy
{
  csString name;
  csRef<iDocument> doc;
  csRef<iDocumentNode> node;

  iQuestFactory* Create (iQuestManager* questMgr, const char* overridename);
};

//==================================================================================

class PcEditorSupport : public csRefCount
{
protected:
  iCelPlLayer* pl;
  csString name;
  EntityMode* emode;

public:
  PcEditorSupport (const char* name, EntityMode* emode);
  virtual ~PcEditorSupport () { }

  const csString& GetName () { return name; }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl) = 0;
  virtual bool Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName) = 0;
  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value) = 0;
  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu) { }
};


//==================================================================================

class EntityMode : public scfImplementationExt1<EntityMode, EditingMode, iComponent>
{
private:
  csRef<iCelPlLayer> pl;

  Ares::View view;
  csRef<Ares::Value> questsValue;

  //--- For the property grid.
  wxPropertyGrid* detailGrid;

  // Last property as detected by the context menu handler.
  wxPGProperty* contextLastProperty;

  wxArrayString typesArray;
  wxArrayInt typesArrayIdx;
  wxArrayString pctypesArray;

  csHash<csRef<PcEditorSupport>, csString> editors;

  bool UpdatePCFromGrid (iCelPropertyClassTemplate* pctpl, const csString& propname,
      const csString& selectedPropName);
  bool ValidateGridChange (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, const csString& value);
  void UpdateTemplateParentsFromGrid ();
  void UpdateTemplateClassesFromGrid ();
  void UpdateCharacteristicFromGrid (wxPGProperty* property, const csString& propName);
  bool ValidateTemplateParentsFromGrid (wxPropertyGridEvent& event);

  void RegisterEditor (PcEditorSupport* editor);
  void BuildDetailGrid ();
  void FillDetailGrid (iCelEntityTemplate* tpl);
  //-----------------------

  csString GetRewardsLabel (iRewardFactoryArray* rewards);
  void BuildRewardGraph (iRewardFactoryArray* rewards,
      const char* parentKey, const char* pcKey);
  void BuildStateGraph (iQuestStateFactory* state, const char* stateKey,
      const char* pcKey);
  void BuildQuestGraph (iQuestFactory* questFact, const char* pcKey, bool fullquest,
      const csString& defaultState);
  void BuildQuestGraph (iCelPropertyClassTemplate* pctpl, const char* pcKey);
  void BuildTemplateGraph (const char* templateName);
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

  csString activeNode;		// Currently selected node.
  csString currentTemplate;
  iQuestFactory* editQuestMode;	// If true we're editing a quest.
  csString contextMenuNode;	// Node that is being used for the context menu.
  csString GetContextMenuNode ();

  void SelectTemplate (iCelEntityTemplate* tpl);
  void SelectQuest (iQuestFactory* tpl);
  void SelectPC (iCelPropertyClassTemplate* pctpl);

  PropertyClassPanel* pcPanel;
  TriggerPanel* triggerPanel;
  RewardPanel* rewardPanel;
  SequencePanel* sequencePanel;
  EntityTemplatePanel* tplPanel;

  int idDelete, idCreate, idEditQuest, idNewState, idNewSequence, idDefaultState;
  int idCreateTrigger, idCreateReward, idCreateRewardOnInit, idCreateRewardOnExit;
  int idRewardUp, idRewardDown;

  int idNewChar, idDelChar;

  // Fetch a property class template from a given graph key.
  iCelPropertyClassTemplate* GetPCTemplate (const char* key);

  // Get the selected state.
  iQuestStateFactory* GetSelectedState (const char* key);
  csString GetSelectedStateName (const char* key);

  // Get the current quest.
  iQuestFactory* GetSelectedQuest (const char* key);

  /**
   * Smart way to get the active node. If no active node is set then
   * this function will check if there is a selected quest or template and then
   * construct a node string from that.
   */
  csString GetActiveNode ();

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

  void CopySelected ();
  void DeleteSelected ();
  void DeleteItem (const char* item);
  void Paste ();

  QuestCopy questCopy;
  EntityCopy entityCopy;
  PropertyClassCopy pcCopy;
  QuestCopy Copy (iQuestFactory* questFact);
  EntityCopy Copy (iCelEntityTemplate* tpl);
  PropertyClassCopy Copy (iCelPropertyClassTemplate* pctpl);
  void ClearCopy ();
  bool HasPaste ();

public:
  EntityMode (iBase* parent);
  virtual ~EntityMode ();

  virtual bool Initialize (iObjectRegistry* object_reg);

  virtual void SetTopLevelParent (wxWindow* toplevel);
  virtual bool HasMainPanel () const { return true; }
  virtual void BuildMainPanel (wxWindow* parent);

  iAresEditor* GetApplication () const;
  iObjectRegistry* GetObjectRegistry () const { return object_reg; }
  iCelPlLayer* GetPL () const { return pl; }
  iParameterManager* GetPM () const;

  iQuestManager* GetQuestManager () const { return questMgr; }
  iCelPropertyClassTemplate* GetSelectedPC ()
  {
    return GetPCTemplate (GetContextMenuNode ());
  }
  csString GetQuestName (iCelPropertyClassTemplate* pctpl);

  /// Register modification of the current template and refresh the visuals.
  void RegisterModification (iCelEntityTemplate* tpl = 0);
  /// Register modification of a quest and refresh the visuals.
  void RegisterModification (iQuestFactory* quest);

  /// Refresh the view. The tiven pctpl is optional and will be used if given.
  void RefreshView (iCelPropertyClassTemplate* pctpl = 0);

  /// Refresh the mode.
  virtual void Refresh ();

  virtual void Start ();
  virtual void Stop ();

  virtual bool Command (csStringID id, const csString& args);
  virtual bool IsCommandValid (csStringID id, const csString& args,
      iSelection* selection, size_t pastesize);
  csPtr<iString> GetAlternativeLabel (csStringID id,
      iSelection* selection, size_t pastesize);

  virtual void SelectResource (iObject* resource);

  virtual void AllocContextHandlers (wxFrame* frame);
  virtual void AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY);

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);

  void OnPropertyGridChanging (wxPropertyGridEvent& event);
  void OnPropertyGridChanged (wxPGProperty* selectedProperty);
  void OnPropertyGridChanged (wxPropertyGridEvent& event);
  void OnPropertyGridButton (wxCommandEvent& event);
  void OnTemplateSelect ();
  void OnQuestSelect ();
  void OnDelete ();
  void OnCreatePC ();
  void OnEditQuest ();
  void OnNewState ();
  void OnNewSequence ();
  void OnDefaultState ();
  void OnCreateTrigger ();
  void OnCreateReward (int type); // 0 == normal, 1 == oninit, 2 == onexit
  void OnRewardMove (int dir);
  void OnContextMenu (wxContextMenuEvent& event);

  void PcQuest_OnNewParameter ();
  void PcQuest_OnDelParameter ();
  void PcProp_OnNewProperty ();
  void PcProp_OnDelProperty ();
  void OnNewCharacteristic ();
  void OnDeleteCharacteristic ();

  void AskNewTemplate ();
  void OnTemplateDel (const char* tplName);
  void AskNewQuest ();
  void OnQuestDel (const char* questName);
  void OnRenameQuest (const char* questName);
  void OnRenameTemplate (const char* questName);

  void PCWasEdited (iCelPropertyClassTemplate* pctpl);
  void ActivateNode (const char* nodeName);

  // Property grid support stuff.
  void AppendPar (
    wxPGProperty* parent, const char* partype,
    const char* name, celDataType type, const char* value);
  void AppendButtonPar (
    wxPGProperty* parent, const char* partype, const char* type, const char* name);
  void AppendTemplatesPar (wxPGProperty* parent, iCelEntityTemplateIterator* it, const char* partype);
  void AppendClassesPar (wxPGProperty* parentProp, csSet<csStringID>::GlobalIterator* it,
      const char* partype);
  void AppendCharacteristics (wxPGProperty* parentProp, iCelEntityTemplate* tpl);
  iCelPropertyClassTemplate* GetPCForProperty (wxPGProperty* property, csString& pcPropName,
      csString& selectedPropName);
  csString GetPropertyValueAsString (const csString& property, const char* sub);
  wxPropertyGrid* GetDetailGrid () const { return detailGrid; }
  //---

  class Panel : public wxPanel
  {
  public:
    Panel(EntityMode* s)
      : wxPanel (), s (s)
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
    void OnRewardUp (wxCommandEvent& event) { s->OnRewardMove (-1); }
    void OnRewardDown (wxCommandEvent& event) { s->OnRewardMove (1); }
    void OnTemplateSelect (wxListEvent& event) { s->OnTemplateSelect (); }
    void OnQuestSelect (wxListEvent& event) { s->OnQuestSelect (); }
    void OnPropertyGridChanging (wxPropertyGridEvent& event) { s->OnPropertyGridChanging (event); }
    void OnPropertyGridChanged (wxPropertyGridEvent& event) { s->OnPropertyGridChanged (event); }
    void OnPropertyGridButton (wxCommandEvent& event) { s->OnPropertyGridButton (event); }
    void OnContextMenu (wxContextMenuEvent& event) { s->OnContextMenu (event); }
    void PcQuest_OnNewParameter (wxCommandEvent& event) { s->PcQuest_OnNewParameter (); }
    void PcQuest_OnDelParameter (wxCommandEvent& event) { s->PcQuest_OnDelParameter (); }
    void PcProp_OnNewProperty (wxCommandEvent& event) { s->PcProp_OnNewProperty (); }
    void PcProp_OnDelProperty (wxCommandEvent& event) { s->PcProp_OnDelProperty (); }
    void OnNewCharacteristic (wxCommandEvent& event) { s->OnNewCharacteristic (); }
    void OnDeleteCharacteristic (wxCommandEvent& event) { s->OnDeleteCharacteristic (); }

  private:
    EntityMode* s;

    DECLARE_EVENT_TABLE()
  };
  Panel* panel;
};

#endif // __aresed_entitymode_h

