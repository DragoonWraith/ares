%module pythonint
%import "bindings/cspace.i"

%{
#include "crystalspace.h"
#include "include/isimgui.h"
%}

INTERFACE_PRE(iSimpleGUI);
INTERFACE_PRE(iSimpleGUICallback);
%include "isimgui.h"
INTERFACE_POST(iSimpleGUI);
INTERFACE_POST(iSimpleGUICallback);

