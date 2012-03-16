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
#include "apparesed.h"
#include "config.h"

//---------------------------------------------------------------------------

AresConfig::AresConfig (AppAresEditWX* app) : app (app)
{
}

bool AresConfig::ParseKnownMessages (iDocumentNode* knownmessagesNode)
{
  csRef<iDocumentNodeIterator> it = knownmessagesNode->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csString value = child->GetValue ();
    if (value == "message")
    {
      KnownMessage msg;
      msg.name = child->GetAttributeValue ("name");
      msg.description = child->GetAttributeValue ("description");
      csRef<iDocumentNodeIterator> parit = child->GetNodes ();
      while (parit->HasNext ())
      {
        csRef<iDocumentNode> parNode = parit->Next ();
        if (parNode->GetType () != CS_NODE_ELEMENT) continue;
        csString parValue = parNode->GetValue ();
	if (parValue == "par")
	{
	  KnownParameter par;
	  par.name = parNode->GetAttributeValue ("name");
	  par.value = parNode->GetAttributeValue ("value");
	  csString typeS = parNode->GetAttributeValue ("type");
	  celDataType type;
	  if (typeS == "string") type = CEL_DATA_STRING;
	  else if (typeS == "long") type = CEL_DATA_LONG;
	  else if (typeS == "bool") type = CEL_DATA_BOOL;
	  else if (typeS == "float") type = CEL_DATA_FLOAT;
	  else if (typeS == "vector2") type = CEL_DATA_VECTOR2;
	  else if (typeS == "vector3") type = CEL_DATA_VECTOR3;
	  else if (typeS == "vector4") type = CEL_DATA_VECTOR4;
	  else if (typeS == "color") type = CEL_DATA_COLOR;
	  else if (typeS == "color4") type = CEL_DATA_COLOR4;
	  else type = CEL_DATA_NONE;
	  par.type = type;
	  msg.parameters.Push (par);
	}
	else
	{
          return app->ReportError ("Error parsing 'aresedconfig.xml', unknown element '%s'!", parValue.GetData ());
	}
      }
      messages.Push (msg);
    }
    else
    {
      return app->ReportError ("Error parsing 'aresedconfig.xml', unknown element '%s'!", value.GetData ());
    }
  }
  return true;
}

bool AresConfig::ReadConfig ()
{
  csRef<iVFS> vfs = csQueryRegistry<iVFS> (app->GetObjectRegistry ());
  csRef<iDocumentSystem> docsys;
  docsys = csQueryRegistry<iDocumentSystem> (app->GetObjectRegistry ());
  if (!docsys)
    docsys.AttachNew (new csTinyDocumentSystem ());

  csRef<iDocument> doc = docsys->CreateDocument ();
  csRef<iDataBuffer> buf = vfs->ReadFile ("/appdata/aresedconfig.xml");
  if (!buf)
  {
    return app->ReportError ("Could not open config file '/appdata/aresedconfig.xml'!");
  }
  const char* error = doc->Parse (buf->GetData ());
  if (error)
  {
    return app->ReportError ("Error parsing 'aresedconfig.xml': %s!", error);
  }

  csRef<iDocumentNode> root = doc->GetRoot ();
  csRef<iDocumentNode> configNode = root->GetNode ("config");
  if (!configNode)
  {
    return app->ReportError ("Error 'aresedconfig.xml' is missing a 'config' node!");
  }
  csRef<iDocumentNode> knownmessagesNode = configNode->GetNode ("knownmessages");
  if (knownmessagesNode)
  {
    if (!ParseKnownMessages (knownmessagesNode))
      return false;
  }
  return true;
}

const KnownMessage* AresConfig::GetKnownMessage (const char* name) const
{
  for (size_t i = 0 ; i < messages.GetSize () ; i++)
  {
    if (messages.Get (i).name == name)
      return &messages.Get (i);
  }
  return 0;
}

