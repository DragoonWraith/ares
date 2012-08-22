// This file is automatically generated.
#include "cssysdef.h"
#include "csutil/scf.h"

// Put static linking stuff into own section.
// The idea is that this allows the section to be swapped out but not
// swapped in again b/c something else in it was needed.
#if !defined(CS_DEBUG) && defined(CS_COMPILER_MSVC)
#pragma const_seg(".CSmetai")
#pragma comment(linker, "/section:.CSmetai,r")
#pragma code_seg(".CSmeta")
#pragma comment(linker, "/section:.CSmeta,er")
#pragma comment(linker, "/merge:.CSmetai=.CSmeta")
#endif

namespace csStaticPluginInit
{
static char const metainfo_entitymode[] =
"<?xml version=\"1.0\"?>"
"<!-- entitymode.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>ares.editor.modes.entity</name>"
"        <implementation>EntityMode</implementation>"
"        <description>AresEd Entity Editing Mode</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef EntityMode_FACTORY_REGISTER_DEFINED 
  #define EntityMode_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(EntityMode) 
  #endif

class entitymode
{
SCF_REGISTER_STATIC_LIBRARY(entitymode,metainfo_entitymode)
  #ifndef EntityMode_FACTORY_REGISTERED 
  #define EntityMode_FACTORY_REGISTERED 
    EntityMode_StaticInit EntityMode_static_init__; 
  #endif
public:
 entitymode();
};
entitymode::entitymode() {}

}
