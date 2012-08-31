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

#include "../apparesed.h"
#include "../aresview.h"
#include "messageframe.h"
#include "uimanager.h"

#include <wx/html/htmlwin.h>

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MessageFrame, wxFrame)
  EVT_BUTTON (XRCID("close_Button"), MessageFrame :: OnCloseButton)
  EVT_BUTTON (XRCID("clear_Button"), MessageFrame :: OnClearButton)
  EVT_CLOSE(MessageFrame :: OnClose)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void MessageFrame::OnCloseButton (wxCommandEvent& event)
{
  wxFrame::Show (false);
}

void MessageFrame::OnClose (wxCloseEvent& event)
{
  if (event.CanVeto ())
  {
    wxFrame::Show (false);
    event.Veto ();
  }
  else
  {
    Destroy ();
  }
}

void MessageFrame::UpdateHtml ()
{
  if (IsShown ())
  {
    wxHtmlWindow* message_Html = XRCCTRL (*this, "message_Html", wxHtmlWindow);
    message_Html->SetPage (wxString::FromUTF8 ("<html><body>"+messages+"</body></html>"));
  }
}

void MessageFrame::OnClearButton (wxCommandEvent& event)
{
  messages = "";
  lastMessageSeverity = -1;
  lastMessageID = "";
  UpdateHtml ();
}

void MessageFrame::ReceiveMessage (int severity, const char* msgID, const char* description)
{
  bool popup = true;
  if (severity != lastMessageSeverity || lastMessageID != msgID)
  {
    csString color;
    switch (severity)
    {
      case CS_REPORTER_SEVERITY_DEBUG: color = "green"; break;
      case CS_REPORTER_SEVERITY_BUG: color = "green"; break;
      case CS_REPORTER_SEVERITY_ERROR: color = "red"; break;
      case CS_REPORTER_SEVERITY_WARNING: color = "orange"; break;
      case CS_REPORTER_SEVERITY_NOTIFY: color = "black"; popup = false; break;
      default: color = "green";
    }
    lastMessageSeverity = severity;
    lastMessageID = msgID;
    messages.AppendFmt ("<font color='%s'><b>%s</b></font><br>", color.GetData (), msgID);
  }
  messages.AppendFmt ("&nbsp;&nbsp;%s<br>", description);
  if (popup)
  {
    if (!IsShown ())
    {
      wxFrame::Show (true);
      Raise ();
    }
  }
  UpdateHtml ();
}

void MessageFrame::Show ()
{
  wxFrame::Show (true);
  Raise ();
  UpdateHtml ();
}

MessageFrame::MessageFrame (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadFrame (this, parent, wxT ("MessageFrame"));
  SetSize (600, 300);
  lastMessageSeverity = -1;
}

MessageFrame::~MessageFrame ()
{
}


