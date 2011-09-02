/*
The MIT License

Copyright (c) 2010 by Jorrit Tyberghein

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
#include "cssysdef.h"

#include "csgeom/box.h"
#include "csutil/scanstr.h"
#include "iutil/plugin.h"
#include "iutil/document.h"
#include "imap/services.h"
#include "iengine/mesh.h"
#include "imesh/genmesh.h"

#include "dynworldload.h"

#include "physicallayer/pl.h"
#include "physicallayer/propclas.h"


CS_PLUGIN_NAMESPACE_BEGIN(DynWorldLoader)
{

SCF_IMPLEMENT_FACTORY (DynamicWorldLoader)

enum
{
  XMLTOKEN_CURVE,
  XMLTOKEN_ROOM,
  XMLTOKEN_ATTR,
  XMLTOKEN_BOX,
  XMLTOKEN_CYLINDER,
  XMLTOKEN_SPHERE,
  XMLTOKEN_MESH,
  XMLTOKEN_CONVEXMESH,
  XMLTOKEN_MATERIAL,
  XMLTOKEN_POINT,
  XMLTOKEN_FOLIAGEDENSITY
};

//---------------------------------------------------------------------------------------

DynamicWorldLoader::DynamicWorldLoader (iBase *parent) :
  scfImplementationType (this, parent)
{
}

DynamicWorldLoader::~DynamicWorldLoader ()
{
}

bool DynamicWorldLoader::Initialize (iObjectRegistry *object_reg)
{
  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  if (!pl)
  {
    printf ("Can't find CEL physical layer!\n");
    return false;
  }
  nature = csLoadPluginCheck<iNature> (object_reg, "utility.nature");
  if (!nature)
  {
    printf ("No nature plugin!\n");
    return false;
  }
  roomMeshCreator = csLoadPluginCheck<iRoomMeshCreator> (object_reg, "utility.rooms");
  if (!roomMeshCreator)
  {
    printf ("No room mesh creator plugin!\n");
    return false;
  }
  curvedMeshCreator = csLoadPluginCheck<iCurvedMeshCreator> (object_reg, "utility.curvemesh");
  if (!curvedMeshCreator)
  {
    printf ("No curved mesh creator plugin!\n");
    return false;
  }
  synldr = csQueryRegistry<iSyntaxService> (object_reg);
  if (!synldr)
  {
    printf ("No syntax service!\n");
    return false;
  }

  xmltokens.Register ("curve", XMLTOKEN_CURVE);
  xmltokens.Register ("room", XMLTOKEN_ROOM);
  xmltokens.Register ("attr", XMLTOKEN_ATTR);
  xmltokens.Register ("box", XMLTOKEN_BOX);
  xmltokens.Register ("cylinder", XMLTOKEN_CYLINDER);
  xmltokens.Register ("sphere", XMLTOKEN_SPHERE);
  xmltokens.Register ("mesh", XMLTOKEN_MESH);
  xmltokens.Register ("convexmesh", XMLTOKEN_CONVEXMESH);
  xmltokens.Register ("material", XMLTOKEN_MATERIAL);
  xmltokens.Register ("point", XMLTOKEN_POINT);
  xmltokens.Register ("foliagedensity", XMLTOKEN_FOLIAGEDENSITY);

  return true;
}

bool DynamicWorldLoader::ParseFoliageDensity (iDocumentNode* node,
    iPcDynamicWorld* dynworld)
{
  csString name = node->GetAttributeValue ("name");
  csString image = node->GetAttributeValue ("image");
  nature->RegisterFoliageDensityMap (name, image);
  return true;
}

bool DynamicWorldLoader::ParseRoom (iDocumentNode* node, iPcDynamicWorld* dynworld)
{
  csString name = node->GetAttributeValue ("name");

  iRoomFactoryTemplate* cft = roomMeshCreator->AddRoomFactoryTemplate (name);
  if (!cft)
  {
    synldr->ReportError ("dynworld.loader", node,
	"Could not add room factory template '%s'!", name.GetData ());
    return false;
  }

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csStringID id = xmltokens.Request (child->GetValue ());
    switch (id)
    {
      case XMLTOKEN_ATTR:
	cft->SetAttribute (child->GetAttributeValue ("name"),
	    child->GetAttributeValue ("value"));
	break;
      case XMLTOKEN_MATERIAL:
	cft->SetMaterial (child->GetAttributeValue ("name"));
	break;
      case XMLTOKEN_ROOM:
	{
	  csVector3 tl, br;
	  csString vector = child->GetAttributeValue ("tl");
	  if (vector.Length () > 0)
	    csScanStr ((const char*)vector, "%f %f %f", &tl.x, &tl.y, &tl.z);
	  vector = child->GetAttributeValue ("br");
	  if (vector.Length () > 0)
	    csScanStr ((const char*)vector, "%f %f %f", &br.x, &br.y, &br.z);
	  cft->AddRoom (tl, br);
	}
	break;
      default:
        synldr->ReportBadToken (child);
	return false;
    }
  }
  return true;
}

bool DynamicWorldLoader::ParseCurve (iDocumentNode* node, iPcDynamicWorld* dynworld)
{
  csString name = node->GetAttributeValue ("name");
  float width = 1.0f;
  if (node->GetAttribute ("width"))
    width = node->GetAttributeValueAsFloat ("width");
  float sideheight = .1f;
  if (node->GetAttribute ("sideheight"))
    sideheight = node->GetAttributeValueAsFloat ("sideheight");

  iCurvedFactoryTemplate* cft = curvedMeshCreator->AddCurvedFactoryTemplate (name);
  if (!cft)
  {
    synldr->ReportError ("dynworld.loader", node,
	"Could not add curved factory template '%s'!", name.GetData ());
    return false;
  }
  cft->SetCharacteristics (width, sideheight);

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csStringID id = xmltokens.Request (child->GetValue ());
    switch (id)
    {
      case XMLTOKEN_ATTR:
	cft->SetAttribute (child->GetAttributeValue ("name"),
	    child->GetAttributeValue ("value"));
	break;
      case XMLTOKEN_MATERIAL:
	cft->SetMaterial (child->GetAttributeValue ("name"));
	break;
      case XMLTOKEN_POINT:
	{
	  csVector3 pos (0), front (0, 0, 1), up (0, 1, 0);
	  csString vector = child->GetAttributeValue ("pos");
	  if (vector.Length () > 0)
	    csScanStr ((const char*)vector, "%f %f %f", &pos.x, &pos.y, &pos.z);
	  vector = child->GetAttributeValue ("front");
	  if (vector.Length () > 0)
	    csScanStr ((const char*)vector, "%f %f %f", &front.x, &front.y, &front.z);
	  vector = child->GetAttributeValue ("up");
	  if (vector.Length () > 0)
	    csScanStr ((const char*)vector, "%f %f %f", &up.x, &up.y, &up.z);
	  cft->AddPoint (pos, front, up);
	}
	break;
      default:
        synldr->ReportBadToken (child);
	return false;
    }
  }
  return true;
}

bool DynamicWorldLoader::Parse (iDocumentNode* child, iPcDynamicWorld* dynworld)
{
  csStringID id = xmltokens.Request (child->GetValue ());
  switch (id)
  {
    case XMLTOKEN_CURVE:
      if (!ParseCurve (child, dynworld)) return false;
      break;
    case XMLTOKEN_ROOM:
      if (!ParseRoom (child, dynworld)) return false;
      break;
    case XMLTOKEN_FOLIAGEDENSITY:
      if (!ParseFoliageDensity (child, dynworld)) return false;
      break;
    default:
      return false;
  }
  return true;
}

}
CS_PLUGIN_NAMESPACE_END(DynWorldLoader)

