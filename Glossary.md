# Glossary #

  * [Ares](AresGame.md): this is one part of this project; the actual game.
  * AresEd: this is one part of this project; the game editor.
  * [Bullet](http://bulletphysics.org/wordpress/): is the physics library that Ares uses.
  * [CEL](http://cel.crystalspace3d.org): Crystal Entity Layer is the game entity layer that Ares uses. Basically a game entity layer is responsible for managing game objects (like the player, NPC's, weapons, tools, ...)
  * [CS](http://www.crystalspace3d.org): Crystal Space is the 3D Engine that is used by this project. It is responsible for drawing the 3D graphics, integrating with the [Physics Library http://bulletphysics.org/wordpress/](Bullet.md) and much more.
  * DynWorld: The Dynamic World plugin is responsible for managing dynamic objects in the vicinity of the camera and making sure persistence works.
  * [ELCM](ELCM.md): The Entity Life Cycle Manager is responsible for managing the activity and lifetime of entities. It is a crucial plugin of CEL and important for Ares as it makes it possible to have lots of entities in the world at the same time.
  * Entity: is an important concept of CEL. It represents a game object. The player, NPC's, objects, doors, ... are all examples of entities.
  * [Entity Templates](EntityTemplates.md): entity templates are objects from which you can create entities. They contain the basic building blocks.
  * Property Class: entities are defined by all the property classes that are attached to an entity. This is the way that an entity can obtain functionality. Examples are the [inventory property class](PCInventory.md), the [spawning property class](PCSpawn.md), and many more...
  * [Quest](CELQuests.md): a quest is a state machine that is defined in CEL. Quests can be used for simple mundane tasks like glowing objects, opening doors, moving platforms, but also for big story-related quests.