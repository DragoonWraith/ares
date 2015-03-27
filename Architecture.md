# Architecture #

This graph gives a high-level description of how CS, CEL and Ares work together:

![http://ares.googlecode.com/svn/wiki/CELAresArchitecture.png](http://ares.googlecode.com/svn/wiki/CELAresArchitecture.png)

# Explanation #

At the top there is Crystal Space. This is the basic 3D engine and provides functionality related to 3D rendering, sound, physics and related. As such Crystal Space has nothing to do with games. It is _just_ a 3D engine.

Crystal Space is very modular (uses a plugin system). Various plugins are:

  * OpenGL: provides for 3D rendering
  * Bullet: interface to the Bullet physics library
  * Landscape engine: well you know what this is, don't you?
  * Particle systems: fire, weapon effects, you name it
  * 3D Sound: using OpenAL or software rendering
  * Skeletal animation: powerful skeletal animation system
  * ...

Below Crystal Space there is CEL (Crystal Entity Layer). CEL makes use of the Crystal Space plugin system to adds the notion of _game entities_ and contains plugins to help make games. 'Entities' are the basic building blocks of a game. The player is an entity, NPC's are entities, a chest is an entity, a door is an entity, a quest is an entity, and so on.

Entities are defined as a collection of _property classes_ (the green boxes). These property classes add various functionalities to the entities. For example the player entity will most likely need a 'camera' property class so that the real player can watch what is going on. A player might also need an inventory property class so that it can hold other entities. A chest will most likely also need an inventory property class. If you need a spawn point (for a creature or perhaps for some valuable object) then you can make an entity that contains the 'spawn' property class. With these basic building blocks you can define your game objects in a very powerful way. There are many more property classes then are shown in this diagram.

Entities are typically created from [entity templates](EntityTemplates.md).

In addition to entities and property classes there are also other plugins:

  * [ELCM](ELCM.md): this is the Entity Life Cycle Manager. It keeps track of which entities are active and which entities can be 'swapped out' because they haven't been touched in a while. This is an important tool to work with very big worlds.
  * DynWorld: this plugin adds the notion of a big dynamic world to CEL. It has the notion of 'cells' (rooms) and allows for a huge number of objects. The second video blog was a demonstration of this plugin (together with the [ELCM](ELCM.md) plugin).
  * [CEL Messaging System](CELMessaging.md): CEL makes heavy usage of a messaging sytem for communication between game entities. This is the basis for the game logic.
  * [Loot Generator](LootGenerator.md): the loot generator works together with the inventory property class. You can define loot packages to randomly generate loot in some inventory depending on various factors (like for example, the level of the player, the time of the day, ...)
  * [Quest System](CELQuests.md): CEL has a powerful quest system. Quests are very general state machines that can be used for simple tasks (like making a glowing cube or a door that you can open and close) but also for big quests that you can see in typical role playing games.
  * [Behaviour Trees](BehaviourTrees.md): these can be used for defining the AI of NPCs.

Finally there is Ares and AresEd which will use the above technology.