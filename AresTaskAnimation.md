## Ares Task: Animation System ##

### Description ###

The animation support that I would like to see in CEL/Ares needs at least the following features:
  * Modular animation system. It should be possible to define animations in different files. So that for example, if a someone makes a mod he can replace the ‘idle’ animation(s) of a given model and doesn’t have to touch the mesh and/or the other animations.
  * Animations should work together (blending). For example, there are various animations for walking around (stand, walk, run, strafe, …) and there are also animations for drawing a weapon, aiming, slashing, … These animations should work together so that you can aim a weapon and still walk, run, strafe, …
  * Preferably animation files should be reusable for our different game models. If we have different races represented by different meshes they should be able to reuse the same standard set of animations. Of course, non-humanoid models will need different animations.
  * We need to define ‘enter’, ‘loop’, and ‘exit’ animations. For example, the animation for having your weapon ready needs to be done in three parts:
    * An ‘enter’ animation where the player draws his weapon.
    * A ‘loop’ animation where the player holds his weapon ready for attack.
    * An ‘exit’ animation where the player puts his weapon back.

The CS animesh plugin supports most of this already. We just need to put support for this in CEL so that it interacts nicely with the movement system and the (as yet not existing) weapon system.