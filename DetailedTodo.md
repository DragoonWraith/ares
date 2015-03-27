Here is a list of what I think still has to be done. Most likely this list is far from complete:

## Shared between Ares Game and Ares Editor ##

  * Code issues:

  * Landscape:
    * Investigate how to implement a **really** big but accurate terrain. Probably doable with the tools available in CS right now. Still needs someone to actually look at it. This task includes paging as well.
    * 'Clever' foliage on top of landscape. This means grass that doesn't grow on rocks. Scattered rocks but like you would expect them to scatter. Constraints can also be added on a minimal/maximal slope of the terrain. Also keeping in mind the 'really big terrain'.

  * Big World (dynworld plugin):
    * The dynworld plugin already supports 'paging in' dynamic/static objects and their associated rigid bodies. However, due to numerical accuracy problems in physics libraries (bullet in particular but this is true for other physics libraries as well) it would be nice if we could 'localize' the coordinate system so that coordinate systems for the physics simulation are always relatively small even if we are moving along a very big landscape. To support this the dynworld plugin needs to keep track of the physics bodies (it already does this) and from time to time it will have to physically 'move' the colliders so that they are again centered around the camera. That means that all 'active' rigid bodies will always be centered roughly around the camera.
    * Fix the imposters. As it is right now they swap up-side-down sometimes. This is actually a bug in Crystal Space. ''[Mike: If someone can send me a genmesh which shows this behaviour, I'll try and fix it.]''
    * Define 'zones' in the world. Segments are really separate. There is no seamless integration between different zones. Instead you get a loading screen. This is useful for making different separate areas or for making indoor areas where you first have to open a door and then enter another zone in the world.

  * Sky shader:
    * Fix the ugly seams. ''[Mike: I can take a look at this.]''
    * Fix the sharp day/night transition. ''[Mike: And this...]''
    * ''[kickvb: This sky shader would be rad too for the Island demo]'' (see [wiki:"Demo and Test apps#Theisland" here])

  * Water:
    * Fix the seams that are sometimes visible between land and water transition.

  * Trees and nature:
    * See how we can make the environment as natural and nice as possible.
    * Nice trees with leaves that move in the wind and such. ''[Mike: Already supported.. needs a bit of artist work to get it right though.]''
    * Moving grass. ''[Mike: Also already supported. Could probably still be improved, but for now it will produce the desired effect given the right art.]'' ''[kickvb: is it compatible with the recent changes Frank made regarding the instancing of the meshgen? I haven't manage to get it working in the island demo.]''

## Ares Editor ##

  * Shared Code:
    * A general thing we need to do is to discuss and check how we can share as much as possible between all editing tool efforts for Crystal Space. A proposal based on an iModifiable interface is in discussion and could perhaps be considered here.

  * User Interface:

  * Landscape:
    * Allow for real-time editing of the terrain. That includes height but also the texturing. Such an editor has already been started in CS (see 'csterrainedtest').
    * Allow for editing of foliage, eg by painting directly on the meshgen density maps.

  * Big World:
    * See if our current approach to editing works as well for indoor type areas. Possibly we want to hide objects that are obstructing what we want to edit.

  * Asset Management:
    * A Peragro's /[DAMN/](http://wiki.peragro.org/index.php/Tools/DAMN) client can be added in CS. This would greatly help managing the assets.

  * Curved Mesh editing (like the street editor):
    * Fix the bug where the mesh sometimes comes out garbled when first created.
    * _(Assigned to Jorrit):_ Make a better tool to fit the mesh on the landscape.
    * Allow a way to 'paint' a road from a top-down view. Basically draw a line/curve and then let the editor make the best fitting curve. Fitting it automatically to the terrain below would be possible too if it is done on 'vertex' level of the generated mesh.
    * For big streets the curved mesh should be split so that the distant parts can 'fade' out of the view.

  * Undo system:
    * The Undo/Redo system from Peragro's Anvil is probably a good solution to this problem

  * Basic editing (moving/aligning/rotating objects):
    * Precise positioning of an object. This can be made through a dedicated mesh displaying the transform axis of the object + a mouse interaction to move/rotate/scale the object (see eg [the one from Blender](http://www.blendercookie.com/getting-started-with-blender-modeling/), video at 2:50)
    * Use of decals to indicate where new objects will appear (requires support for decals in terrain2).

  * Camera handling:
    * Easier camera navigation ''[kickvb: I'll work on that using the new iCameraManager]''

  * Quest editor:

  * Attribute editor:
    * For NPC's
    * For players.
    * For objects.
    * ''[kickvb: wouldn't it be instead a CEL entity editor where you can add/remove/edit propclasses?]''
    * ''[jorrit: perhaps, but keep in mind that this is not really a generic world/CEL editor. It is a game editor with an associated game. Trying to make things general is nice but in the first place I want to make things easy and not too hard to implement. General propclass editing has the risk that the user might add property classes that don't fit in the rest of the game concepts. But to be discussed]''

  * NPC editor:
    * Edit characteristics (cloths, looks, ...) of NPCs'

  * AI editor:
    * For the AI system (see below in Ares Game section) we need an editor.
    * Selection of behaviours for an NPC.
    * Conversation options for an NPC.

  * Path finding editor:
    * Management of the preprocess tools and files, previsualization of the navigation meshes
    * Tagging of additional information (eg to act on the heuristic of the Astar algorithm)
    * Definition of waypoints, eg for patrolling

  * HUD editor:
    * An editor so that you can describe what attributes from player entity should be visible in-game and define the layout.

  * Sound editor
    * 'viewsound': view/manipulate sound source properties

## Ares Game ##

  * Eye Candy:
    * Ripples where something touches the water.
    * Decals for blood, dirt, footsteps, ...
    * Use ragdoll system for dead NPC's

  * Player movement and camera:
    * Try out and experiment with current CEL camera systems and see if they need improving.
    * When player moves he should interact with physical objects (i.e. push barrels and such) and also be blocked by (static) physical objects.

  * AI system:
    * For NPC's we need an AI system. We should probably use the behaviour trees of CEL for this.
    * Conversation system. All NPC's can potentially react to the player when the player wants to talk. When an NPC wants to react he will suspend his current behaviour and confront the player so a conversation can start. I would propose a system similar to Fallout 3 where there is a limited number of conversation possibilities.
    * Many predefined AI 'behaviours' are possible:
      * Stand: this is a simple behaviour where an NPC just stands somewhere (as a guard for example).
      * Wander: let the NPC wander in a specific bounded area (geometrical bounds).
      * Path: the NPC moves along a certain path. At the completion it stops (an associated quest can then kick in when this happens so that possibly another thing happens like switching to another behaviour).
      * Fight on Sight: NPC fights as soon as it 'sees' the player.
      * Flee on Sight
    * Such predefined behavior bricks can be made through behaviour sub-trees that can be shared to build more complex trees. Something like a template mechanism may be needed to make it easier to use.
    * NPC vision system: define and implement how NPC's can 'see' or 'hear' the player and kick off appropriate messages (for quests) when this happens. The iPcTrigger works well for that, but it should be optimized (it uses brute force), and also improved eg by adding support for cone (for visibility) or sound triggers.

  * NPC attribute system and inventory:
    * NPC's (and player too) need attributes like health, strength, and other. Also skills and progression.
    * Consider using pcrules property class in CEL for making relations between attributes. Like HP = strength\*10 + endurance\*20 or something.
    * Inventory system for player and NPC's.
    * Inventory system for objects.

  * NPC customization:
    * The ability to have a single NPC model with support for different hairstyles, clothes, props, body and facial characteristics
    * Add support for dynamic addition/removal of submeshes to an animesh + hardware skinning pipeline ''[to Anthony](Assigned.md)''
    * Short-term plan: add support for exporting animeshes with in B2.5CS ''[to Sueastside and kickvb](Assigned.md)''
    * Long-term plan: write a CS importer for the models, hairs, clothes, props, physical properties, etc from [MakeHuman](http://makehuman.blogspot.com/), using the [wiki:"Tools plugins" tools guidelines]
    * We can get models and props/clothes eg [here](http://www.blendswap.com/3D-models/category/clothes/)
    * Add support for exporting from the Blender hair system to the CS hair mesh in B2.5CS

  * HUD:
    * Show attributes from player (and possibly other) entities in a manner as defined by the editor.
    * Message system to notify players when it gets a certain quest or some other important thing is occured. These messages are non-modal. They appear briefly on screen but the player can continue playing without pause.
    * Conversation/message system which is modal. The player has to respond or act before the game continues.
    * Game map. See how we can create a map from the area around the player.

  * Weapons/targetting:
    * See how we can define a weapon system with weapon types, weapon extensions, ammo types and so on.
    * Define targetting system. Do we have a 'paused' system similar to Fallout 3 (VATS) if we are allowed to use something similar without getting lawyers on the premises?

  * Sound system:
    * Probably we can use what is in CEL already.

  * Save files:
    * Define saving/loading system.


Back to [MainPage](MainPage.md)