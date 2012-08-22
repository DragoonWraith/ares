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
static char const metainfo_gamecontrol[] =
"<?xml version=\"1.0\"?>"
"<!-- gamecontrol.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>cel.pcfactory.ares.gamecontrol</name>"
"        <implementation>celPfGameController</implementation>"
"        <description>Game Controller Property Class</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef celPfGameController_FACTORY_REGISTER_DEFINED 
  #define celPfGameController_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(celPfGameController) 
  #endif

class gamecontrol
{
SCF_REGISTER_STATIC_LIBRARY(gamecontrol,metainfo_gamecontrol)
  #ifndef celPfGameController_FACTORY_REGISTERED 
  #define celPfGameController_FACTORY_REGISTERED 
    celPfGameController_StaticInit celPfGameController_static_init__; 
  #endif
public:
 gamecontrol();
};
gamecontrol::gamecontrol() {}

}
