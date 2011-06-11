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

#ifndef __ARES_ROOMS_H__
#define __ARES_ROOMS_H__

#include "csutil/scf.h"

class csVector3;
struct iMeshWrapper;

struct iRoomFactory : public virtual iBase
{
  SCF_INTERFACE(iRoomFactory,0,0,1);

  virtual const char* GetName () const = 0;

  /**
   * Get the mesh factory.
   */
  virtual iMeshFactoryWrapper* GetFactory () = 0;

  /**
   * Setup the material to use for this room factory.
   */
  virtual void SetMaterial (const char* materialName) = 0;

  /**
   * Add a room.
   * Return the index of this room.
   */
  virtual size_t AddRoom (const csVector3& tl, const csVector3& br) = 0;

  /**
   * Delete a room.
   */
  virtual void DeleteRoom (size_t idx) = 0;

  /**
   * Get the number of rooms.
   */
  virtual size_t GetRoomCount () const = 0;
};

/**
 * A template for creating room factories with default settings.
 */
struct iRoomFactoryTemplate : public virtual iBase
{
  SCF_INTERFACE(iRoomFactoryTemplate,0,0,1);

  virtual const char* GetName () const = 0;

  /**
   * Generic attribute system.
   */
  virtual void SetAttribute (const char* name, const char* value) = 0;

  /**
   * Get an attribute.
   */
  virtual const char* GetAttribute (const char* name) const = 0;

  /**
   * Setup the material to use for this room factory.
   */
  virtual void SetMaterial (const char* materialName) = 0;

  /**
   * Add a room to the mesh. These rooms will be used as defaults
   * for creating the factories.
   * Return the index of this room.
   */
  virtual size_t AddRoom (const csVector3& tl, const csVector3& br) = 0;
};

/**
 * Interface to the room mesh plugin.
 */
struct iRoomMeshCreator : public virtual iBase
{
  SCF_INTERFACE(iRoomMeshCreator,0,0,1);

  //-----------------------------------------------------------------------------

  /**
   * Create a room factory template.
   */
  virtual iRoomFactoryTemplate* AddRoomFactoryTemplate (const char* name) = 0;

  /**
   * Get the number of factory templates.
   */
  virtual size_t GetRoomFactoryTemplateCount () const = 0;
  /**
   * Get a room template.
   */
  virtual iRoomFactoryTemplate* GetRoomFactoryTemplate (size_t idx) const = 0;
  /// Delete all factory templates.
  virtual void DeleteRoomFactoryTemplates () = 0;

  //-----------------------------------------------------------------------------

  /**
   * Create a new room mesh factory.
   */
  virtual iRoomFactory* AddRoomFactory (const char* name,
      const char* templatename) = 0;

  /**
   * Get the number of factories.
   */
  virtual size_t GetRoomFactoryCount () const = 0;
  /**
   * Get a factory.
   */
  virtual iRoomFactory* GetRoomFactory (size_t idx) const = 0;
  /**
   * Get a factory by name.
   */
  virtual iRoomFactory* GetRoomFactory (const char* name) const = 0;

  /**
   * Delete all factories.
   */
  virtual void DeleteFactories () = 0;

  //-----------------------------------------------------------------------------

  /**
   * Save the room mesh factories to XML.
   */
  virtual void Save (iDocumentNode* node) = 0;

  /**
   * Load the room mesh factories from XML.
   * Return 0 on success or otherwise a string with the error.
   */
  virtual csRef<iString> Load (iDocumentNode* node) = 0;
};

#endif // __ARES_ROOMS_H__

