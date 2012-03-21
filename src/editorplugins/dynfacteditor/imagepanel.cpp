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

#include "imagepanel.h"

 
BEGIN_EVENT_TABLE(ImagePanel, wxPanel)
  EVT_PAINT(ImagePanel::PaintEvent)
END_EVENT_TABLE()
 
ImagePanel::ImagePanel (wxWindow* parent) : wxPanel (parent)
{
  image = wxNullBitmap;
}
 
void ImagePanel::PaintEvent (wxPaintEvent& evt)
{
  // Depending on your system you may need to look at double-buffered dcs
  wxPaintDC dc (this);
  Render (dc);
}
 
void ImagePanel::PaintNow ()
{
  // Depending on your system you may need to look at double-buffered dcs
  wxClientDC dc (this);
  Render (dc);
}
 
void ImagePanel::Render (wxDC& dc)
{
  if (!image.IsOk ()) return;
  dc.DrawBitmap (image, 0, 0, false);
}
 
