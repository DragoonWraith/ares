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
static char const metainfo_mainmode[] =
"<?xml version=\"1.0\"?>"
"<!-- mainmode.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>ares.editor.modes.main</name>"
"        <implementation>MainMode</implementation>"
"        <description>AresEd Main Editing Mode</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef MainMode_FACTORY_REGISTER_DEFINED 
  #define MainMode_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(MainMode) 
  #endif

class mainmode
{
SCF_REGISTER_STATIC_LIBRARY(mainmode,metainfo_mainmode)
  #ifndef MainMode_FACTORY_REGISTERED 
  #define MainMode_FACTORY_REGISTERED 
    MainMode_StaticInit MainMode_static_init__; 
  #endif
public:
 mainmode();
};
mainmode::mainmode() {}

}
