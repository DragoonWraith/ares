/*
The MIT License

Copyright (c) 2011 by Jorrit Tyberghein

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

#ifndef __aresed_playmode_h
#define __aresed_playmode_h

#include "csutil/csstring.h"
#include "csutil/scfstr.h"
#include "edcommon/editmodes.h"
#include "inature.h"

struct iPcDynamicWorld;
struct iCelEntity;

/**
 * A snapshot of the current objects. This is used to remember the situation
 * before 'Play' is selected.
 */
class DynworldSnapshot
{
private:
  struct Obj
  {
    iDynamicCell* cell;
    iDynamicFactory* fact;
    bool isStatic;
    csReversibleTransform trans;
    csString entityName;
    csString templateName;
    csRef<iCelParameterBlock> params;
    // Indices of connected objects.
    csArray<size_t> connectedObjects;
  };
  csArray<Obj> objects;

  // Find the index of a given object in this cell.
  size_t FindObjIndex (iDynamicCell* cell, iDynamicObject* dynobj);

public:
  DynworldSnapshot (iPcDynamicWorld* dynworld);
  void Restore (iPcDynamicWorld* dynworld);
};


class PlayMode : public scfImplementationExt1<PlayMode, EditingMode, iComponent>
{
private:
  DynworldSnapshot* snapshot;
  csRef<iCelEntity> world;
  csRef<iCelEntity> player;
  csRef<iCelPlLayer> pl;
  csRef<iNature> nature;
  csRef<iVirtualClock> vc;
#if NEW_PHYSICS
  csRef<CS::Physics::iPhysicalSystem> dyn;
#else
  csRef<iDynamics> dyn;
#endif
  iCamera* camera;
  iDynamicCell* oldcell;

  csTicks currentTime;

public:
  PlayMode (iBase* parent);
  virtual ~PlayMode ();

  virtual bool Initialize (iObjectRegistry* object_reg);
  virtual void FramePre ();

  virtual bool IsContextMenuAllowed () { return false; }

  virtual csRef<iString> GetStatusLine ()
  {
    csRef<iString> str;
    str.AttachNew (new scfString ("Playtest mode, press escape to exit"));
    return str;
  }

  virtual void Start ();
  virtual void Stop ();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
};

#endif // __aresed_playmode_h

