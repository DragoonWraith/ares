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

#ifndef __transformtools_h
#define __transformtools_h

#include "aresextern.h"

struct iSelection;
class csVector3;

class ARES_EXPORT TransformTools
{
public:
  /**
   * Align all selected objects based on the first selected object.
   */
  static void AlignSelectedObjects (iSelection* selection);

  /**
   * Stack all selected objects on top of each other.
   */
  static void StackSelectedObjects (iSelection* selection);

  /**
   * Move all selected objects to the same heigh.
   */
  static void SameYSelectedObjects (iSelection* selection);

  /**
   * Set the position of the selected objects so that they
   * are adjacent to each other along the best matching axis.
   */
  static void SetPosSelectedObjects (iSelection* selection);

  /**
   * Reset the rotation of all selected objects.
   */
  static void RotResetSelectedObjects (iSelection* selection);

  /**
   * Rotate the selected objects with the given base angle.
   */
  static void Rotate (iSelection* selection, float baseAngle,
      bool slow, bool fast);

  /**
   * Rotate the selected objects with the given base angle.
   */
  static void Rotate (iSelection* selection, float baseAngle);

  /**
   * Move the selection with the given base vector.
   */
  static void Move (iSelection* selection, const csVector3& baseVector,
      bool slow, bool fast);

  /**
   * Return the center of all selected objects.
   */
  static csVector3 GetCenterSelected (iSelection* selection);

  /**
   * Return the combined world space box of all selected objects.
   */
  static csBox3 GetBoxSelected (iSelection* selection);

  /**
   * Return the combined world space sphere of all selected objects.
   */
  static csSphere GetSphereSelected (iSelection* selection);
};

#endif // __transformtools_h

