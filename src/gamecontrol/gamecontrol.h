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

#ifndef __CEL_PF_GAMECONTROL__
#define __CEL_PF_GAMECONTROL__

#include "cstypes.h"
#include "iutil/comp.h"
#include "csutil/scf.h"
#include "physicallayer/propclas.h"
#include "physicallayer/propfact.h"
#include "physicallayer/facttmpl.h"
#include "celtool/stdpcimp.h"
#include "celtool/stdparams.h"
#include "include/igamecontrol.h"

struct iCelEntity;
struct iObjectRegistry;

/**
 * Factory for game controller.
 */
CEL_DECLARE_FACTORY (GameController)

/**
 * This is a game controller property class.
 */
class celPcGameController : public scfImplementationExt1<
	celPcGameController, celPcCommon, iPcGameController>
{
private:
  // For SendMessage parameters.
  static csStringID id_message;
  static csStringID id_timeout;

  // For actions.
  enum actionids
  {
    action_message = 0
  };

  // For properties.
  //enum propids
  //{
    //propid_counter = 0,
    //propid_max
  //};
  static PropertyHolder propinfo;

  //csRef<iMessageDispatcher> dispatcher_print;

public:
  celPcGameController (iObjectRegistry* object_reg);
  virtual ~celPcGameController ();

  virtual void Message (const char* message, float timeout = 2.0f);

  virtual bool PerformActionIndexed (int idx,
      iCelParameterBlock* params, celData& ret);
};

#endif // __CEL_PF_GAMECONTROL__

