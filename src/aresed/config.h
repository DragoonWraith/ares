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

#ifndef __aresed_config_h
#define __aresed_config_h

#include "physicallayer/datatype.h"

class AppAresEditWX;
struct iDocumentNode;

struct KnownParameter
{
  csString name;
  celDataType type;
  csString value;
};


struct KnownMessage
{
  csString name;
  csArray<KnownParameter> parameters;
};

class AresConfig
{
private:
  AppAresEditWX* app;
  csArray<KnownMessage> messages;

  bool ParseKnownMessages (iDocumentNode* knownmessagesNode);

public:
  AresConfig (AppAresEditWX* app);

  /**
   * Read the configuration file. Returns false on failure.
   * The error will already be reported by then.
   */
  bool ReadConfig ();

  /// Get all known messages.
  const csArray<KnownMessage>& GetMessages () const { return messages; }

  /// Get a known message by name.
  const KnownMessage* GetKnownMessage (const char* name) const;
};

#endif // __aresed_config_h

