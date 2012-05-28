/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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
#include "dynfactdialog.h"
#include "lightdialog.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "edcommon/uitools.h"
#include "edcommon/tools.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(LightDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), LightDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), LightDialog :: OnCancelButton)
  EVT_COLOURPICKER_CHANGED (XRCID("color_Picker"), LightDialog :: OnColorChange)
  EVT_COLOURPICKER_CHANGED (XRCID("specularColor_Picker"), LightDialog :: OnSpecularColorChange)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

ColorValue::ColorValue ()
{
  AddChild ("r", NEWREF(Value,new FloatValue(0.0f)));
  AddChild ("g", NEWREF(Value,new FloatValue(0.0f)));
  AddChild ("b", NEWREF(Value,new FloatValue(0.0f)));
}

void ColorValue::ChildChanged (Value* child)
{
  color.red = GetChildByName ("r")->GetFloatValue ();
  color.green = GetChildByName ("g")->GetFloatValue ();
  color.blue = GetChildByName ("b")->GetFloatValue ();
}

void ColorValue::SetColor (const csColor& color)
{
  ColorValue::color = color;
  GetChildByName ("r")->SetFloatValue (color.red);
  GetChildByName ("g")->SetFloatValue (color.green);
  GetChildByName ("b")->SetFloatValue (color.blue);
}

//--------------------------------------------------------------------------

LightFactoryValue::LightFactoryValue ()
{
  AddChild ("name", NEWREF(Value,new StringValue ("")));
  AddChild ("type", NEWREF(Value,new StringValue ("point")));
  AddChild ("dynamicType", NEWREF(Value,new StringValue ("dynamic")));
  AddChild ("attenuation", NEWREF(Value,new StringValue ("linear")));
  AddChild ("c", NEWREF(Value,new FloatValue (0.0f)));
  AddChild ("l", NEWREF(Value,new FloatValue (0.0f)));
  AddChild ("q", NEWREF(Value,new FloatValue (0.0f)));
  AddChild ("cutoffRadius", NEWREF(Value,new FloatValue (5.0f)));
  AddChild ("directionalCutoffRadius", NEWREF(Value,new FloatValue (5.0f)));
  AddChild ("specular", NEWREF(Value,new BoolValue (false)));
  AddChild ("innerSpotlightFalloff", NEWREF(Value,new FloatValue (1.0f)));
  AddChild ("outerSpotlightFalloff", NEWREF(Value,new FloatValue (0.0f)));

  specularColor.AttachNew (new ColorValue ());
  color.AttachNew (new ColorValue ());
  AddChild ("specColor", specularColor);
  AddChild ("normColor", color);
}

//--------------------------------------------------------------------------

void LightDialog::OnOkButton (wxCommandEvent& event)
{
  EndModal (TRUE);
}

void LightDialog::OnCancelButton (wxCommandEvent& event)
{
  EndModal (FALSE);
}

void LightDialog::OnColorChange (wxColourPickerEvent& event)
{
  wxColour col = event.GetColour ();
  ColorValue* value = factoryValue->GetColorValue ();
  csColor color (col.Red () / 255.0, col.Green () / 255.0, col.Blue () / 255.0);
  value->SetColor (color);
}

void LightDialog::OnSpecularColorChange (wxColourPickerEvent& event)
{
  wxColour col = event.GetColour ();
  ColorValue* value = factoryValue->GetSpecularColorValue ();
  csColor color (col.Red () / 255.0, col.Green () / 255.0, col.Blue () / 255.0);
  value->SetColor (color);
}

iLightFactory* LightDialog::Show (iLightFactory* factory)
{
  if (factory)
  {
    factoryValue->GetChildByName ("name")->SetStringValue (
	factory->QueryObject ()->GetName ());
    csLightType type = factory->GetType ();
    factoryValue->GetChildByName ("type")->SetStringValue (
	type == CS_LIGHT_POINTLIGHT ? "point" :
	type == CS_LIGHT_DIRECTIONAL ? "directional" :
	"spot");
    csLightDynamicType dynType = factory->GetDynamicType ();
    factoryValue->GetChildByName ("dynamicType")->SetStringValue (
	dynType == CS_LIGHT_DYNAMICTYPE_STATIC ? "static" :
	dynType == CS_LIGHT_DYNAMICTYPE_PSEUDO ? "pseudo" :
	"dynamic");
    csLightAttenuationMode mode = factory->GetAttenuationMode ();
    factoryValue->GetChildByName ("attenuation")->SetStringValue (
	mode == CS_ATTN_NONE ? "none" :
	mode == CS_ATTN_LINEAR ? "linear" :
	mode == CS_ATTN_INVERSE ? "inverse" :
	mode == CS_ATTN_REALISTIC ? "realistic" :
	mode == CS_ATTN_CLQ ? "clq" :
	"none");
    const csVector4& attn = factory->GetAttenuationConstants ();
    factoryValue->GetChildByName ("c")->SetFloatValue (attn.x);
    factoryValue->GetChildByName ("l")->SetFloatValue (attn.y);
    factoryValue->GetChildByName ("q")->SetFloatValue (attn.z);
    factoryValue->GetChildByName ("cutoffRadius")->SetFloatValue (
	factory->GetCutoffDistance ());
    factoryValue->GetChildByName ("directionalCutoffRadius")->SetFloatValue (
	factory->GetDirectionalCutoffRadius ());
    factoryValue->GetChildByName ("specular")->SetBoolValue (
	factory->IsSpecularColorUsed ());

    const csColor& specColor = factory->GetSpecularColor ();
    factoryValue->GetSpecularColorValue ()->SetColor (specColor);
    wxColourPickerCtrl* specColorPicker = XRCCTRL (*this, "specularColor_Picker", wxColourPickerCtrl);
    specColorPicker->SetColour (wxColour (
	  int (specColor.red * 255.1),
	  int (specColor.green * 255.1),
	  int (specColor.blue * 255.1)));

    const csColor& color = factory->GetColor ();
    factoryValue->GetColorValue ()->SetColor (color);
    wxColourPickerCtrl* colorPicker = XRCCTRL (*this, "color_Picker", wxColourPickerCtrl);
    colorPicker->SetColour (wxColour (
	  int (color.red * 255.1),
	  int (color.green * 255.1),
	  int (color.blue * 255.1)));

    float inner, outer;
    factory->GetSpotLightFalloff (inner, outer);
    factoryValue->GetChildByName ("innerSpotlightFalloff")->SetFloatValue (inner);
    factoryValue->GetChildByName ("outerSpotlightFalloff")->SetFloatValue (outer);
  }

  if (ShowModal ())
  {
    csString n = factoryValue->GetChildByName ("name")->GetStringValue ();
    if (!factory)
    {
      factory = dialog->GetEngine ()->FindLightFactory (n);
      if (!factory)
        factory = dialog->GetEngine ()->CreateLightFactory (n);
    }
    else
    {
      factory->QueryObject ()->SetName (n);
    }
    csString type = factoryValue->GetChildByName ("type")->GetStringValue ();
    if (type == "point") factory->SetType (CS_LIGHT_POINTLIGHT);
    else if (type == "directional") factory->SetType (CS_LIGHT_DIRECTIONAL);
    else factory->SetType (CS_LIGHT_SPOTLIGHT);
    csString dynType = factoryValue->GetChildByName ("dynamicType")->GetStringValue ();
    if (dynType == "static") factory->SetDynamicType (CS_LIGHT_DYNAMICTYPE_STATIC);
    else if (dynType == "pseudo") factory->SetDynamicType (CS_LIGHT_DYNAMICTYPE_PSEUDO);
    else factory->SetDynamicType (CS_LIGHT_DYNAMICTYPE_DYNAMIC);

    factory->SetColor (factoryValue->GetColorValue ()->GetColor ());
    if (factoryValue->GetChildByName ("specular")->GetBoolValue ())
      factory->SetSpecularColor (factoryValue->GetSpecularColorValue ()->GetColor ());

    csString attn = factoryValue->GetChildByName ("attenuation")->GetStringValue ();
    if (attn == "none") factory->SetAttenuationMode (CS_ATTN_NONE);
    else if (attn == "linear") factory->SetAttenuationMode (CS_ATTN_LINEAR);
    else if (attn == "inverse") factory->SetAttenuationMode (CS_ATTN_INVERSE);
    else if (attn == "realistic") factory->SetAttenuationMode (CS_ATTN_REALISTIC);
    else if (attn == "clq") factory->SetAttenuationMode (CS_ATTN_CLQ);
    else factory->SetAttenuationMode (CS_ATTN_NONE);
    csVector4 attnConstants (0, 0, 0, 0);
    attnConstants.x = factoryValue->GetChildByName ("c")->GetFloatValue ();
    attnConstants.y = factoryValue->GetChildByName ("l")->GetFloatValue ();
    attnConstants.z = factoryValue->GetChildByName ("q")->GetFloatValue ();
    factory->SetAttenuationConstants (attnConstants);

    factory->SetCutoffDistance (factoryValue->GetChildByName ("cutoffRadius")->GetFloatValue ());
    factory->SetDirectionalCutoffRadius (factoryValue->GetChildByName ("directionalCutoffRadius")->GetFloatValue ());
    factory->SetSpotLightFalloff (
	factoryValue->GetChildByName ("innerSpotlightFalloff")->GetFloatValue (),
	factoryValue->GetChildByName ("outerSpotlightFalloff")->GetFloatValue ());
    return factory;
  }
  return 0;
}

LightDialog::LightDialog (wxWindow* parent, DynfactDialog* dialog) : View (this)
{
  LightDialog::dialog = dialog;
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("LightEditorDialog"));

  factoryValue.AttachNew (new LightFactoryValue ());
  Bind (factoryValue, this);
  BindEnabled (factoryValue->GetChildByName ("specular"), "specularColor_Picker");

  Layout ();
  Fit ();
}

LightDialog::~LightDialog ()
{
}


