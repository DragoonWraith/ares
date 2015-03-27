# Introduction #

Entity templates are blue-prints for creating entities. They are a very important thing in CEL and Ares because most property classes and plugins (like the [inventory](PCInventory.md), the [loot generator](LootGenerator.md), the [spawning property class](PCSpawn.md), the [dynamic world plugin](DynWorld.md), ...) work with entity templates from which they will create the actual entities.

# Examples #

Here is an example of a door entity template. This door will teleport the player to another cell as soon as it is activated. To do that it makes use of the [wire property class](PCWire.md):

```
<addon plugin="cel.addons.celentitytpl" entityname="Door" >
  <template name="Base" />
  <propclass name="pclogic.wire">
    <action name="AddInput">
      <par name="mask" string="ares.activate" />
    </action>
    <action name="AddOutput">
      <par name="msgid" string="ares.teleport" />
      <par name="entity" string="world" />
      <par name="cell" string="$cell" />
      <par name="pos" vector3="$pos" />
    </action>
  </propclass>
  <characteristic name="weight" value="50" />
</addon>
```

Let's go over this example step-by-step:
  * _template_: with this statement the entity template is defined to depend on another entity template. This is useful so that you can group basic stuff that everything needs in a parent entity template. It can also be used to make specialized entity templates that are almost the same as the parent but have some specific changes.
  * _propclass_: then a [wire property class](PCWire.md) is added. This property class will listen to the 'ares.activate' message (using the 'AddInput' action which is specific to the wire property class) and when it receives this message it will send an 'ares.teleport' message to the 'world' entity (using the 'AddOutput' action). This will cause the player to be teleported to another cell.
  * This entity template has two parameters that have to be set at the time an entity is created from this template. The first parameter is the name of the 'cell' and the second parameter is the position to teleport too in the destination cell.
  * Finally we define a characteristic which can be used by the inventory system. In this case we say that the weight of this door is 50.