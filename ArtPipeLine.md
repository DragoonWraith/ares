# Introduction #

If you are an artist then this page is for you. In addition to the information here you can find general Crystal Space related information [on this page](http://www.crystalspace3d.org/main/Creating_Art).


# Static and Animated Models #

The easiest way to make models is using [Blender](http://www.blender3d.org). The reason is that we have very good support for exporting Blender models to Crystal Space format.

If you use Blender 2.49 (or Blender < 2.5) then you should use [blender2crystal](http://leapingcat.org/blender2crystal/index.php/Main_Page). This tool is very powerful and has a lot of support for Crystal Space features. If you plan to use animated models, then be sure to use the trunk version of blender2crystal.

For Blender 2.6 there is a new exporter included with recent versions of Crystal Space. You can find more information [in the CS manual](http://www.crystalspace3d.org/docs/online/manual-2.0/Blender.html). It is not yet as powerful as the original blender2crystal but it does the job.

# Characters #

We're going to use [MakeHuman](http://www.makehuman.org/) for the characters. This would allow complete in-game customization of the models. See the [dedicated page](AresTaskMakeHuman.md).

Those characters would still need secondary tasks for clothes, hairs, new morph targets, and animations.

## Clothes ##

New clothes can be defined in Blender. See the [dedicated documentation page](http://sites.google.com/site/makehumandocs/blender-export-and-mhx/making-clothes).

## Hairs ##

CS has a dedicated hair system, but it is not (yet?) compatible with the one from MakeHuman. That's still a technical TODO.

Documentation on the MakeHuman wiki [here](http://sites.google.com/site/makehumandocs/blender-export-and-mhx/hair).

## Morph Targets ##

New morph targets can be defined for the MakeHuman model, eg to create new faces for the Alien species. This would need to be made in Blender then exported into MakeHuman, but information on the exact process would still need to be found before.

## Animations ##

Morph animations are not yet really supported by CS. Still a technical TODO. They should however be defined in Blender, then exported using the Blender export script.

Skeletal animations should also be defined in Blender, but here you will have to choose a skeleton, and the animations you will define will be valid only for the models using the same limb proportions of the skeleton. We can probably use the default proportions from MakeHuman, and see later for the other skeletons (this would be managed partially through animation retargeting tools).

Motion capture animations are supported, mainly through the BVH file format. A database of such animations can be setup for future re-use.

# Textures #

You can make textures in any kind of image editor. [GIMP](http://www.gimp.org/) is a good choice if you want a powerful and free Open Source image editor.

# Sound #

For small sound effects we support WAV files. Music is typically done with OGG files. Crystal Space does **not** support MP3.