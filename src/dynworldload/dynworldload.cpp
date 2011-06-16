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


CS_PLUGIN_NAMESPACE_BEGIN(DynWorldLoader)
{

SCF_IMPLEMENT_FACTORY (DynamicWorldLoader)

enum
{
  XMLTOKEN_FACTORY,
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
  dynworld = csLoadPluginCheck<iDynamicWorld> (object_reg, "utility.dynamicworld");
  if (!dynworld)
  {
    printf ("No dynamic world plugin!\n");
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

  xmltokens.Register ("factory", XMLTOKEN_FACTORY);
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

bool DynamicWorldLoader::ParseFoliageDensity (iDocumentNode* node)
{
  csString name = node->GetAttributeValue ("name");
  csString image = node->GetAttributeValue ("image");
  nature->RegisterFoliageDensityMap (name, image);
  return true;
}

bool DynamicWorldLoader::ParseRoom (iDocumentNode* node)
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

bool DynamicWorldLoader::ParseCurve (iDocumentNode* node)
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

bool DynamicWorldLoader::ParseFactory (iDocumentNode* node)
{
  csString name = node->GetAttributeValue ("name");
  float maxradius = 1.0f;
  if (node->GetAttribute ("maxradius"))
    maxradius = node->GetAttributeValueAsFloat ("maxradius");
  float imposterradius = -1.0f;
  if (node->GetAttribute ("imposterradius"))
    imposterradius = node->GetAttributeValueAsFloat ("imposterradius");
  float mass = 1.0f;
  if (node->GetAttribute ("mass"))
    mass = node->GetAttributeValueAsFloat ("mass");
  iDynamicFactory* fact = dynworld->AddFactory (name, maxradius, imposterradius);
  if (!fact)
  {
    synldr->ReportError ("dynworld.loader", node,
	"Could not add factory '%s'!", name.GetData ());
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
	fact->SetAttribute (child->GetAttributeValue ("name"),
	    child->GetAttributeValue ("value"));
	break;
      case XMLTOKEN_BOX:
	{
	  csRef<iDocumentNode> offsetNode = child->GetNode ("offset");
	  csRef<iDocumentNode> sizeNode = child->GetNode ("size");
	  if (!offsetNode && !sizeNode)
	  {
	    const csBox3& bbox = fact->GetBBox ();
	    fact->AddRigidBox (bbox.GetCenter (), bbox.GetSize (), mass);
	  }
	  else
	  {
	    csVector3 offset (0), size;
	    if (offsetNode && !synldr->ParseVector (offsetNode, offset))
	    {
	      synldr->ReportError ("dynworld.loader", child,
		  "Error loading 'offset' for factory '%s'!", name.GetData ());
	      return false;
	    }

	    if (!sizeNode || !synldr->ParseVector (sizeNode, size))
	    {
	      synldr->ReportError ("dynworld.loader", child,
		"Error loading 'size' for factory '%s'!", name.GetData ());
	      return false;
	    }
	    fact->AddRigidBox (offset, size, mass);
	  }
	}
	break;
      case XMLTOKEN_CYLINDER:
	{
	  float radius = -1, length = -1;
	  csRef<iDocumentNode> offsetNode = child->GetNode ("offset");
	  if (!child->GetAttribute ("radius") && !child->GetAttribute ("length") &&
	      !offsetNode)
	  {
	    const csBox3& bbox = fact->GetBBox ();
	    csVector3 size = bbox.GetSize ();
	    radius = size.x;
	    if (size.z > radius) radius = size.z;
	    radius /= 2.0;
	    length = size.y;
	    fact->AddRigidCylinder (radius, length, bbox.GetCenter (), mass);
	  }
	  else
	  {
	    radius = child->GetAttributeValueAsFloat ("radius");
	    length = child->GetAttributeValueAsFloat ("length");
	    csVector3 offset (0);
	    if (offsetNode && !synldr->ParseVector (offsetNode, offset))
	    {
	      synldr->ReportError ("dynworld.loader", child,
		  "Error loading 'offset' for factory '%s'!", name.GetData ());
	      return false;
	    }

	    if (radius < 0)
	    {
	      synldr->ReportError ("dynworld.loader", child,
		"Error loading 'radius' for factory '%s'!", name.GetData ());
	      return false;
	    }
	    if (length < 0)
	    {
	      synldr->ReportError ("dynworld.loader", child,
		"Error loading 'length' for factory '%s'!", name.GetData ());
	      return false;
	    }
	    fact->AddRigidCylinder (radius, length, offset, mass);
	  }
	}
	break;
      case XMLTOKEN_SPHERE:
	{
	  float radius = -1;
	  csRef<iDocumentNode> offsetNode = child->GetNode ("offset");
	  if (!child->GetAttribute ("radius") && !offsetNode)
	  {
	    const csBox3& bbox = fact->GetBBox ();
	    csVector3 size = bbox.GetSize ();
	    radius = size.x;
	    if (size.y > radius) radius = size.y;
	    if (size.z > radius) radius = size.z;
	    radius /= 2.0;
	    fact->AddRigidSphere (radius, bbox.GetCenter (), mass);
	  }
	  else
	  {
	    radius = child->GetAttributeValueAsFloat ("radius");
	    csVector3 offset (0);
	    if (offsetNode && !synldr->ParseVector (offsetNode, offset))
	    {
	      synldr->ReportError ("dynworld.loader", child,
		  "Error loading 'offset' for factory '%s'!", name.GetData ());
	      return false;
	    }

	    if (radius < 0)
	    {
	      synldr->ReportError ("dynworld.loader", child,
		"Error loading 'radius' for factory '%s'!", name.GetData ());
	      return false;
	    }
	    fact->AddRigidSphere (radius, offset, mass);
	  }
	}
	break;
      case XMLTOKEN_MESH:
	{
	  csVector3 offset (0);
	  csRef<iDocumentNode> offsetNode = child->GetNode ("offset");
	  if (offsetNode && !synldr->ParseVector (offsetNode, offset))
	  {
	    synldr->ReportError ("dynworld.loader", child,
		  "Error loading 'offset' for factory '%s'!", name.GetData ());
	    return false;
	  }
	  fact->AddRigidMesh (offset, mass);
	}
	break;
      case XMLTOKEN_CONVEXMESH:
	{
	  csVector3 offset (0);
	  csRef<iDocumentNode> offsetNode = child->GetNode ("offset");
	  if (offsetNode && !synldr->ParseVector (offsetNode, offset))
	  {
	    synldr->ReportError ("dynworld.loader", child,
		  "Error loading 'offset' for factory '%s'!", name.GetData ());
	    return false;
	  }
	  fact->AddRigidConvexMesh (offset, mass);
	}
	break;
      default:
        synldr->ReportBadToken (child);
	return false;
    }
  }
  return true;
}

csPtr<iBase> DynamicWorldLoader::Parse (iDocumentNode* node,
  	iStreamSource* ssource, iLoaderContext* ldr_context,
  	iBase* context)
{
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csStringID id = xmltokens.Request (child->GetValue ());
    switch (id)
    {
      case XMLTOKEN_FACTORY:
	if (!ParseFactory (child)) return 0;
	break;
      case XMLTOKEN_CURVE:
	if (!ParseCurve (child)) return 0;
	break;
      case XMLTOKEN_ROOM:
	if (!ParseRoom (child)) return 0;
	break;
      case XMLTOKEN_FOLIAGEDENSITY:
	if (!ParseFoliageDensity (child)) return 0;
	break;
      default:
        synldr->ReportBadToken (child);
	return 0;
    }
  }

  IncRef ();
  return this;
}

}
CS_PLUGIN_NAMESPACE_END(DynWorldLoader)

