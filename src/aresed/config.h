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

#include "csutil/stringarray.h"
#include "editor/iconfig.h"

class AppAresEditWX;
struct iDocumentNode;

struct PluginConfig
{
  csString plugin;
  csString tooltip;
  bool mainMode;
  csStringArray resources;

  PluginConfig () : mainMode (false) { }
};

class AresConfig : public scfImplementation1<AresConfig, iEditorConfig>
{
private:
  AppAresEditWX* app;
  csArray<KnownMessage> messages;
  csArray<PluginConfig> plugins;

  csRef<iDocument> ReadConfigDocument ();
  bool ParseKnownMessages (iDocumentNode* knownmessagesNode);
  bool ParsePlugins (iDocumentNode* pluginsNode);
  bool ParseMenus (wxMenuBar* menuBar, iDocumentNode* menusNode);
  bool ParseMenuItems (wxMenu* menu, iDocumentNode* itemsNode);

public:
  AresConfig (AppAresEditWX* app);
  virtual ~AresConfig () { }

  /**
   * Read the configuration file. Returns false on failure.
   * The error will already be reported by then.
   */
  bool ReadConfig ();

  /// Get all known messages.
  virtual const csArray<KnownMessage>& GetMessages () const { return messages; }

  /// Get a known message by name.
  virtual const KnownMessage* GetKnownMessage (const char* name) const;

  /// Get all plugins.
  const csArray<PluginConfig>& GetPlugins () const { return plugins; }

  /**
   * Build the menu bar out of the config.
   */
  wxMenuBar* BuildMenuBar ();
};

#endif // __aresed_config_h

