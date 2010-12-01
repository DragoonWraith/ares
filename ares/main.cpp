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

#include "ares.h"
#include "appares.h"
#include <csutil/sysfunc.h> // Provides csPrintf()

CS_IMPLEMENT_APPLICATION

int main(int argc, char** argv)
{
  csPrintf ("This application is a place holder and will in the future run the actual game made by aresed.\n");
  exit (0);
  // Open the main system. This will open all the previously loaded plug-ins.
  csPrintf ("ares version 0.1 by Jorrit Tyberghein.\n");

  /* Runs the application.  
   *
   * csApplicationRunner<> cares about creating an application instance 
   * which will perform initialization and event handling for the entire game. 
   *
   * The underlying csApplicationFramework also performs some core 
   * initialization.  It will set up the configuration manager, event queue, 
   * object registry, and much more.  The object registry is very important, 
   * and it is stored in your main application class (again, by 
   * csApplicationFramework). 
   */
  return csApplicationRunner<AppAres>::Run (argc, argv);
}
