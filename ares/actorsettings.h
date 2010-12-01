/*
The MIT License

Copyright (c) 2010 by Jorrit Tyberghein

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

#ifndef __ACTORSETTINGS_H__
#define __ACTORSETTINGS_H__

#include <stdarg.h>

// CEL Includes
#include "physicallayer/pl.h"
#include "physicallayer/entity.h"
#include "physicallayer/messaging.h"

class ActorSettings;

/**
 * Listen to messages.
 */
class CelTestMessageReceiver : public scfImplementation1<CelTestMessageReceiver,
  	iMessageReceiver>
{
private:
  ActorSettings* as;

public:
  CelTestMessageReceiver (ActorSettings* as) :
    scfImplementationType (this), as (as)
  { }
  virtual ~CelTestMessageReceiver () { }
  virtual bool ReceiveMessage (csStringID msg_id, iMessageSender* sender,
      celData& ret, iCelParameterBlock* params);
};

/**
 * Class to control settings for the actor.
 */
class ActorSettings
{
private:
  csRef<iCelPlLayer> pl;
  csRef<iCelEntity> setting_bar;
  csRef<iCelEntity> entity_cam;

  csRef<CelTestMessageReceiver> receiver;
  csStringID id_toggle_setting_bar;
  csStringID id_next_setting;
  csStringID id_prev_setting;
  csStringID id_decrease_setting;
  csStringID id_increase_setting;
  csStringID id_decrease_setting_slow;
  csStringID id_increase_setting_slow;
  csStringID id_decrease_setting_fast;
  csStringID id_increase_setting_fast;
  csStringID id_next_setting_repeat;
  csStringID id_prev_setting_repeat;
  csStringID id_decrease_setting_repeat;
  csStringID id_increase_setting_repeat;
  csStringID id_decrease_setting_slow_repeat;
  csStringID id_increase_setting_slow_repeat;
  csStringID id_decrease_setting_fast_repeat;
  csStringID id_increase_setting_fast_repeat;
  int current_setting;
  void UpdateSetting ();
  void ChangeSetting (float dir);
 
public:
  ActorSettings () { }
  ~ActorSettings () { }

  void Initialize (iCelPlLayer* pl);
  bool ReceiveMessage (csStringID msg_id, iCelParameterBlock* params);
};

#endif // __CELTEST_H__

