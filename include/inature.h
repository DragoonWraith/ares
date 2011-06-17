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

#ifndef __ARES_NATURE_H__
#define __ARES_NATURE_H__

#include "csutil/scf.h"

class csBox3;
class csVector3;
class csReversibleTransform;

/**
 * Interface to the nature plugin.
 */
struct iNature : public virtual iBase
{
  SCF_INTERFACE(iNature,0,0,1);

  virtual void UpdateTime (csTicks ticks, iCamera* camera) = 0;

  virtual void InitSector (iSector* sector) = 0;

  /// Clean up the nature plugin.
  virtual void CleanUp () = 0;

  /// Set the basic foliage density factor. Default is 1.
  virtual void SetFoliageDensityFactor (float factor) = 0;
  virtual float GetFoliageDensityFactor () const = 0;

  /// Register the name of a foliage density map.
  virtual void RegisterFoliageDensityMap (const char* name, const char* image) = 0;

  /// Get the number of foliage density maps.
  virtual size_t GetFoliageDensityMapCount () const = 0;

  /// Get a specific foliage density map name.
  virtual const char* GetFoliageDensityMapName (size_t idx) const = 0;
  /// Find the index of a foliage density map.
  virtual size_t GetFoliageDensityMapIndex (const char* name) const = 0;

  /// Get a specific foliage density map image.
  virtual iImage* GetFoliageDensityMapImage (size_t idx) = 0;
  virtual iImage* GetFoliageDensityMapImage (const char* name) = 0;
};

#endif // __ARES_NATURE_H__

