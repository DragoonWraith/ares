[MakeHuman](http://www.makehuman.org/) will be used in order to get access to a huge panel
of morph targets for the different parts of the human body, allowing a
complete in-game customization of the characters.

# CS import plugin #

A first task would be to write a CS plugin that would read the data used by the MakeHuman software, and convert it on the fly into a CS animesh.

Some features requested/desired:
  * import of the vertices, triangles, weight map and skeleton of the MakeHuman model
  * adaptation of the animesh's skeleton to the size of the model
  * blending of the skin textures (through procedural textures?) and import of the materials/shaders
  * the animeshes have an optimization of the morph system using a subdivision of the vertices. This subdivision should probably be made here in some preprocessing of the MakeHuman model, or using the MakeHuman vertex groups.
  * import of the clothes
  * threaded loading of the morph targets, clothes and other data
  * cache of render buffers and textures, shared among all active characters
  * subdivision of the MakeHuman model, in order to deactivate the parts of the body that are hidden by the clothes
  * import from the MakeHuman hair system to the CS one
  * LOD management using the MakeHuman proxies as submeshes that are enabled/disabled depending on the LOD

## More technical information ##

There are several file types to be parsed:
  * .obj files, for the base model, the clothes and the hairs
  * .target files, for the morph targets
  * .rig files, for the skeleton and the weight map

The main data file used is in 'makehuman/data/3dobjs/base.obj', this is the base model, without any morph targets applied. This file is in the Wavefront file format and has to be loaded with the exact same order of vertices than MakeHuman, in order to be able to apply the morph targets.

So, the key is to copy the exact same behavior of MakeHuman by adapting in C++ their Python code. The code doing that is sit at 'makehuman/core/files3d.py'.