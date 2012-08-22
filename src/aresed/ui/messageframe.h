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

#ifndef __appares_messageframe_h
#define __appares_messageframe_h

#include <crystalspace.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

class UIManager;

class MessageFrame : public wxFrame
{
private:
  UIManager* uiManager;

  void OnCloseButton (wxCommandEvent& event);
  void OnClearButton (wxCommandEvent& event);
  void OnClose (wxCloseEvent& event);

  int lastMessageSeverity;
  csString lastMessageID;

  csString messages;

  void UpdateHtml ();

public:
  MessageFrame (wxWindow* parent, UIManager* uiManager);
  ~MessageFrame ();

  void Show ();

  /**
   * A message was received from the reporter.
   */
  void ReceiveMessage (int severity, const char* msgID, const char* description);

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_messageframe_h

