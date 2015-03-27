# Introduction #

  * Manages spawning point for creatures and other objects
  * _pclogic.spawn_
  * _iPcSpawn_

# Details #

The spawning property class can be used to define spawning points for creatures or other objects. It has the following features:

  * Support for multiple spawning points and random or sequential selection of those points.
  * Support for keeping track of the spawned entity and respawning as soon as that spawned entity is removed/destroyed.
  * Support for spawning multiple entities (upto some limit) in addition to keeping track of them.

# Example #

This is an example of a money spawning entity that spawns up to 5 money items. As soon as one of the spawned items is destroyed it will spawn another one:

```
<addon plugin="cel.addons.celentitytpl" entityname="MoneySpawn" >
  <template name="Base" />
  <propclass name="pclogic.spawn">
    <action name="AddEntityTemplateType">
      <par name="template" string="Money" />
    </action>
    <action name="SetTiming">
      <par name="repeat" bool="true" />
      <par name="random" bool="true" />
      <par name="mindelay" long="1000" />
      <par name="maxdelay" long="5000" />
    </action>
    <action name="AddSpawnPosition">
      <par name="sector" string="$sector" />
      <par name="position" vector3="$pos" />
    </action>
    <action name="Inhibit">
      <par name="count" long="5" />
    </action>
    <property name="spawnunique" bool="true" />
    <property name="namecounter" bool="false" />
  </propclass>
</addon>
```