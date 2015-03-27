# Introduction #

The dynamic world plugin is a plugin in CEL that supports very big worlds where objects around the camera are automatically made visible while objects further away are invisible.

It has the following features:

  * Cells: for indoor areas or other big outdoor areas you can define cells.
  * Physics: all objects managed by the DynWorld plugin are physics enabled (using the bullet physics library)
  * Efficient mesh cache. Objects that are too far from the camera are _removed_ and can possibly be reused for other objects that are near to the camera. That way the number of meshes loaded in the engine can be kept low for optimal performance.
  * Efficient savefile mechanism so that only objects that are touched or modified have to be saved.
  * Support for management of inventories and objects with inventories inside other inventories and so on.