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
static char const metainfo_helpframe[] =
"<?xml version=\"1.0\"?>"
"<!-- helpframe.csplugin -->"
"<plugin>"
"  <scf>"
"    <classes>"
"      <class>"
"        <name>ares.editor.helpframe</name>"
"        <implementation>HelpFrame</implementation>"
"        <description>AresEd Help Frame</description>"
"      </class>"
"    </classes>"
"  </scf>"
"</plugin>"
;
  #ifndef HelpFrame_FACTORY_REGISTER_DEFINED 
  #define HelpFrame_FACTORY_REGISTER_DEFINED 
    SCF_DEFINE_FACTORY_FUNC_REGISTRATION(HelpFrame) 
  #endif

class helpframe
{
SCF_REGISTER_STATIC_LIBRARY(helpframe,metainfo_helpframe)
  #ifndef HelpFrame_FACTORY_REGISTERED 
  #define HelpFrame_FACTORY_REGISTERED 
    HelpFrame_StaticInit HelpFrame_static_init__; 
  #endif
public:
 helpframe();
};
helpframe::helpframe() {}

}
