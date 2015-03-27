# Introduction #

Ares uses a specific type of world using the DynWorld plugin. So as such it doesn't work with regular Crystal Space world files.

# Cells #

The world is divided into cells. Typically we will have a few big outdoor cells (for specific ares) and many smaller indoor cells. At this moment the actual 'ground' of each cell has to be made outside of AresEd as it has no support for that kind of editing yet. So typically would make a base-level using Blender. This can be a very simple base-level. For example, for an outdoor cell it could be just a skybox and a terrain. For an indoor cell it could just be the floor. But if you want you can also put more stuff there. The only limitation is that everything that you put in the static base level cannot be manipulated in the game and in the AresEd editor.

# Building Cells #

In AresEd you will be able to place building blocks to make your level complete. These building blocks can be small (like clutter objects, rocks, plants, ...) or big (pieces of walls, roofs, even entire buildings).

When making a building one has to keep in mind that the indoor and outdoor of a building will in most cases be part of different cells. The outdoor part of a building will typically be located in an outdoor cell while the indoor part of a building will be in its own building-specific cell. Due to this the indoor and outdoor of a building are in fact totally independent from each other (and you could in fact make indoors that have different dimensions from how the building looks on the outside, of course this is not recommended for realism).

But this also means that you will not use the same 'wall' blocks to build the outdoor of a building compared to the walls you're going to use to build the indoor of a building. They will be separate pieces. It is even possible that the size of the pieces differ. For example, someone can make a blender model for a full house which is commonly used in the Ares world (but far apart so that it isn't very obvious the same house model is reused). But the indoor of that house can be made out of smaller wall building blocks.

# Blender Tips #

To get the lighting that most corresponds with what Crystal Space supports in addition to support for alpha and z-key alpha there are a few things that you have to do:

  * In the Display/Shading properties you must set shading to GLSL and enable 'Backface culling'
  * In addition to using normal UV mapping for texturing you must add a proper Blender material for every texture you have on the object. Give this material a sensible name as it is that name that will be used for the Crystal Space material. Also note that you can reuse materials for different objects.
  * If the material uses Z-key alpha then you must check the 'Transparency' flag and change the Alpha value to 0.
  * Don't forget to 'Assign' the material to all faces. The easiest way to do this (when converting from a pre-GLSL blender file) is to select one face that has the texture you want to convert and then use Shift-G (Select Similar) and select 'Image'. Then select the right material and 'Assign' it.
  * For the given material you also have to define the texture (and possible normalmap textures) that you want this material to use. This texture should be the same as the one you used for uv-mapping. Also give proper names to these textures.
  * The type of the texture should be 'Image or Movie'
  * In the Mapping section you should set 'Coordinates' to 'UV'
  * If the texture uses transparency then you should enable the 'Alpha' checkbox and enable the 'Binary Alpha' option (if that's what it is) in the Crystal Space section.
