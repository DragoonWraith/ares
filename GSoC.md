On this page various ideas for Google Summer of Code (see http://www.google-melange.com/gsoc/homepage/google/gsoc2013 for more information) are presented. If you have your own ideas you can of course also use these for your own project submission.


# Entity Wizards #

AresEd supports a graphical node-based editor where you can edit entity templates and quests by inserting nodes in a tree. This works fine but there are many patterns that appear again and again. It would be nice if AresEd had some kind of wizard system where common patterns can be easily generated using custom input from the user. These patterns/wizards should be defined in xml files so that it is easy to extend and add new wizards if needed. This task involves the following subtasks:

  * Define an XML syntax to describe wizards
  * Define a user interface in WX where the user can control input for the wizards.
  * Make code to generate the nodes based on this input

# Foliage and Decal Painter #

AresEd already contains a non-functional foliage painting attempt. This task involves making a user interface and system to paint decals and foliage on the world and store that in the data files. With decals you have to keep in mind that there can be decals which are fixed to a location (and thus when objects move in/out of that location you must update the decal accordingly) and decals which are fixed to some object. This task involves the following subtasks:

  * Find out how to store the decal information in the data file.
  * Implement a nice userinterface in WX to implement this foliage and decal painting in realtime.

# Physics based Actor #

AresEd currently already supports a physics based actor (using bullet plugin in CS) but this doesn't work very well in all situations. Recently Crystal Space got a new physics actor built-in into the new bullet2 physics plugin. The idea is to extend AresEd to support this new bullet2 plugin. This task will involve changing the CEL dynworld plugin to use the new bullet2 plugin.

# Sound System #

Make a system to manage sounds in AresEd games. There are basically three types of sounds that should be supported:

  1. Music
  1. Ambient sounds depending on environment
  1. Game related sounds (based on game actions)

Sounds can be generally present or else tied to a specific 3D location or object. This task involves writing the UI for managing sounds in AresEd and adding support for sounds in the game logic itself.

# Game User Interface System #

There is already some support for ingame user interface. Specifically for inventory and context specific cursors. But more support is needed. It should be able to define HUD elements and let them interact with the game logic. This task involves the following subtasks:

  * Define an XML system to define HUD elements.
  * Make a UI in WX to let the user define HUD elements in the editor.
  * Define a game logic system to control the HUD elements from within game logic.

# CSEditor Framework #

Investigate how the AresEd code base can move closer to the CSEditor framework in CS. See where the common areas are and how interfaces can be shared. As one proof of concept integrate the particle system editor from that framework so it becomes usable inside AresEd.

# Shaders and Special Effects #

Work on a shader and special effect editor. Note that this does **not** mean a shader editor. That's another project. This involves at least the following subtasks:

  * Define how special effects can be constructed out of objects, shaders, particle systems, ...
  * Define an XML system to define special effects.
  * Make a UI in WX to let the user define special effects in the editor.
  * Define a game logic system to control the special effects from within game logic.

# Shader Editor #

Write a realtime graphical shader editor in AresEd.

# Python in-game Scripting #

The idea in AresEd is to avoid programming as much as possible while making games. However there will always be situations where this is not possible. Especially if games start getting more powerful. For this situation AresEd should support attaching python scripts to game entities.

# Python AresEd Scripting #

Python could also be used to extend the AresEd editor itself. Think and design a system where it is possible to make AresEd extensions in Python itself.

# Work on an NPC conversation system #

Implement the NPC converstation system in CEL based on [NPCConversation](NPCConversation.md). Make a user interface in WX to edit conversations in AresEd.