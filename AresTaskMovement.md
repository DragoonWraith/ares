## Ares Task: Movement and Camera system ##

### Description ###

Ares needs both a first person camera as a third person one. In first person view the center of the screen is what is going to be activated when the player presses the 'activate' key. In third person view we need a valid solution for that as well.

### What we have ###

Over the years CEL has accumulated various property classes for movement and camera handling. This has caused the situation to become a bit confusing. It would be nice to unify these camera systems and try to come up with a single camera/movement system that is configurable for every purposes that is currently handled by all the different implementations. Here is a list of current property classes in CEL that are related to this:

Movement systems:

  * _cel.pcfactory.move.linear_ (plugins/propclass/move): this property class is typically required in addition to the movement property class. It handles the actual moving of the mesh.
  * _cel.pcfactory.move.actor.standard_ (plugins/propclass/actormove): this plugin is a commonly used movement class (by the CEL demos). Until now, this is the only motion propclass able to manage animesh animations.
  * _cel.pcfactory.move.npc_ (plugins/propclass/actormove): this is a variant of the above which is supposed to work for NPC's. Have to see how usable this is in reality (i.e. considering path finding and other stuff).
  * _cel.pcfactory.move.actor.wasd_ (plugins/propclass/wasdmove): an alternative for actormove. Not sure how it works exactly.
  * _cel.pcfactory.move.analogmotion_ (plugins/propclass/analogmotion): no idea what this does?
  * _cel.pcfactory.move.actor.dynamic_ (plugins/propclass/dynmove): this is supposed to be a movement class that handles physics as well. We have to see how mature this is. In any case, Ares is going to require a movement system that is physics aware (so that if you bump into an object it will topple or slide). The 'actor' physical object from the 2011 GSOC may be used for this.
  * _cel.pcfactory.move.grab_ (plugins/propclass/grab): this plugin was made to support grabbing of ledges (think Thombraider). Not sure how well it works though.
  * _cel.pcfactory.move.jump_ (plugins/propclass/jump): support for jumping.
  * _cel.pcfactory.mover_ (plugins/propclass/mover): this class supports movement for NPC's and works in combination with actormove and linmove.

Camera systems:

  * _cel.pcfactory.camera.mode.tracking_ (plugins/propclass/cameras/tracking): a tracking camera. Not exactly sure what it does and how it works.
  * _cel.pcfactory.camera.old_ (plugins/propclass/defcam): the 'old' camera system. Still being used though.
  * _cel.pcfactory.camera.delegate_ (plugins/propclass/delegcam): no idea what this does?
  * _cel.pcfactory.camera.standard_ (plugins/propclass/newcamera): the 'new' camera.
  * _CS::Utility::iCameraManager_ A CS specific camera manager.