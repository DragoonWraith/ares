## List of keys and bindings ##

Mouse:

  * Right mouse button: on a dynamic object this will give a little 'push'. On a static object this will only select it.
  * Left mouse button: on a dynamic object this will select the object and allow you to drag it dynamically. On a static object this will only select it.
  * Ctrl-left mouse button: this will temporarily make the object static and then allow you to drag it accross the plane of the screen (i.e. parallel to the screen).
  * Alt-left mouse button: this will temporarily make the object static and then allow you to drag it horizontally. There is currently a bug that occurs when you drag the object too far in the distance (which is easy to do with this tool) and then it will fade away even if you then move it back. It will then slowly fade back but temporarily the object will be invisible.


Global keys:

  * Esc: quit program (currently without saving or checking, be careful!)
  * '1': toggle debug mode. This will show the physics rigid bodies and their status (i.e. moving/static/...).
  * F2: add 500 ticks to the current time (affects the sky shader).
  * F3: toggle automatic time transition (for sky shader too).
  * 'c': open the camera window. On this window you can toggle gravity and use a few standard positions. A few of the buttons work on the selected objects and you can also remember/restore camera positions.

In normal editing mode:

  * '2': toggle static mode (equivalent to clicking on the 'static' checkbox).
  * 'h': when dragging an object this will create a pivot point at the place where you are holding the object. Releasing the object after this will fix the object to that pivot point. This is currently experimental and the pivot point is not saved in the world data.
  * 'e': spawn a new item where the mouse is pointing. The item that is spawned depends on what you selected in the object selector on the right.
  * Arrow keys: move the selected object horizontally. In the future this movement will occur relative to the camera position. Right now it is relative to the local origin of the object.
  * Shift + Arrow keys: faster movement.
  * Ctrl + Arrow keys: slower movement.
  * '<', '>': move current selected object down or up.
  * Shift + '<' or '>': faster.
  * Ctrl + '<' or '>': slower.

In curve editing mode:

  * 'e': create a new node on the selected curve.

Back to [MainPage](MainPage.md)