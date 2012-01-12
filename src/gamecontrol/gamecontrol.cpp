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

#include "cssysdef.h"
#include "iutil/objreg.h"
#include "gamecontrol.h"
#include "physicallayer/pl.h"
#include "physicallayer/entity.h"

//---------------------------------------------------------------------------

CEL_IMPLEMENT_FACTORY (GameController, "ares.gamecontrol")

//---------------------------------------------------------------------------

csStringID celPcGameController::id_message = csInvalidStringID;
csStringID celPcGameController::id_timeout = csInvalidStringID;

PropertyHolder celPcGameController::propinfo;

celPcGameController::celPcGameController (iObjectRegistry* object_reg)
	: scfImplementationType (this, object_reg)
{
  // For SendMessage parameters.
  if (id_message == csInvalidStringID)
  {
    id_message = pl->FetchStringID ("message");
    id_timeout = pl->FetchStringID ("timeout");
  }

  propholder = &propinfo;

  // For actions.
  if (!propinfo.actions_done)
  {
    SetActionMask ("ares.controller.");
    AddAction (action_message, "Message");
  }

  // For properties.
  propinfo.SetCount (0);
  //AddProperty (propid_counter, "counter",
	//CEL_DATA_LONG, false, "Print counter.", &counter);
  //AddProperty (propid_max, "max",
	//CEL_DATA_LONG, false, "Max length.", 0);
}

celPcGameController::~celPcGameController ()
{
}

bool celPcGameController::PerformActionIndexed (int idx,
	iCelParameterBlock* params,
	celData& ret)
{
  switch (idx)
  {
    case action_message:
      {
        CEL_FETCH_STRING_PAR (msg,params,id_message);
        if (!p_msg) return false;
        CEL_FETCH_FLOAT_PAR (timeout,params,id_timeout);
        if (!p_timeout) timeout = 2.0f;
        Message (msg, timeout);
        return true;
      }
    default:
      return false;
  }
  return false;
}

void celPcGameController::Message (const char* message, float timeout)
{
  printf ("MSG: %s\n", message);
  fflush (stdout);
}

#if 0
void celPcGameController::Print (const char* msg)
{
  printf ("Print: %s\n", msg);
  fflush (stdout);
  params->GetParameter (0).Set (msg);
  iCelBehaviour* ble = entity->GetBehaviour ();
  if (ble)
  {
    celData ret;
    ble->SendMessage ("pcmisc.test_print", this, ret, params);
  }

  if (!dispatcher_print)
  {
    dispatcher_print = entity->QueryMessageChannel ()->
      CreateMessageDispatcher (this, pl->FetchStringID ("cel.test.print"));
    if (!dispatcher_print) return;
  }
  dispatcher_print->SendMessage (params);

  counter++;
  size_t l = strlen (msg);
  if (l > max) max = l;
}
#endif

//---------------------------------------------------------------------------

