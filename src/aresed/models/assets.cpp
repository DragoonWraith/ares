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

#include <crystalspace.h>

#include "assets.h"
#include "edcommon/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"
#include "../aresview.h"

#include "iassetmanager.h"

using namespace Ares;


void AssetsValue::BuildModel ()
{
  dirty = false;
  objectsHash.DeleteAll ();
  ReleaseChildren ();
  iAssetManager* assetManager = app->GetAssetManager ();
  const csRefArray<iAsset>& assets = assetManager->GetAssets ();
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    iAsset* asset = assets[i];
    if ((!writable) || asset->IsWritable ())
    {
      csRef<GenericStringArrayValue<iAsset> > child;
      child.AttachNew (new GenericStringArrayValue<iAsset> (asset));
      csStringArray& array = child->GetArray ();
      array.Push (asset->IsWritable () ? "RW" : "-");
      array.Push (asset->GetNormalizedPath ());
      array.Push (asset->GetFile ());
      array.Push (asset->GetMountPoint ());
      objectsHash.Put (asset, child);
      values.Push (child);
    }
  }
  FireValueChanged ();
}

void AssetsValue::RefreshModel ()
{
  iAssetManager* assetManager = app->GetAssetManager ();
  const csRefArray<iAsset>& assets = assetManager->GetAssets ();
  size_t cnt;
  if (writable)
  {
    cnt = 0;
    for (size_t i = 0 ; i < assets.GetSize () ; i++)
      if (assets[i]->IsWritable ()) cnt++;
  }
  else
    cnt = assets.GetSize ();

  if (cnt != objectsHash.GetSize ())
  {
    // Refresh needed!
    BuildModel ();
    return;
  }

  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    iAsset* asset = assets[i];
    if ((!writable) || asset->IsWritable ())
    {
      StringArrayValue* child = objectsHash.Get (asset, 0);
      if (!child)
      {
        BuildModel ();
        return;
      }
    }
  }
}



