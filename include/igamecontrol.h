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

#ifndef __ARES_GAMECONTROL_H__
#define __ARES_GAMECONTROL_H__

#include "csutil/scf.h"

/**
 * Interface to the game control property class plugin.
 *
 * This property class can send out the following messages to entities:
 * - 'ares.controller.activate': entity is activated.
 *
 * This property class supports the following actions (
 * add prefix 'ares.controller.' if you want to access this action through a
 * message):
 * - Message: parameters 'message' (string), 'timeout' (float, default 2 seconds).
 *   Show a message to the user for the specified amount of time.
 * - StartDrag: start dragging the object in the centre of the screen.
 * - StopDrag: stop dragging.
 *
 * This property class supports the following properties:
 */
struct iPcGameController : public virtual iBase
{
  SCF_INTERFACE(iPcGameController,0,0,1);

  /**
   * Show a message to the user for a given amount of time.
   */
  virtual void Message (const char* message, float timeout = 2.0f) = 0;

  /**
   * Examine the object which the player is currently focusing on.
   * This will see if the object has the 'ares.info' class and if so
   * it will get the 'ares.info' property to use as a message.
   */
  virtual void Examine () = 0;

  /**
   * Start dragging the object in the center of the screen (if possible).
   * Returns false if not possible.
   */
  virtual bool StartDrag () = 0;

  /**
   * Stop dragging (if dragging).
   */
  virtual void StopDrag () = 0;
};

#endif // __ARES_GAMECONTROL_H__

