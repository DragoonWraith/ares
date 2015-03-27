# Introduction #

  * Manages inventory for player and other containers
  * _pctools.inventory_
  * _iPcInventory_

# Details #

The inventory property can be used for any type of container where you want to keep other entities. It has the following features:

  * Support for entities
  * Support for entity templates (with additional 'amount')
  * Support for [loot generator](LootGenerator.md)
  * Support for specifying restrictions on size, weight, and other various user-defined object characteristics.
  * Support for inventories inside inventories

The inventory property class supports both entities as [entity templates](EntityTemplates.md). The latter is useful (and more optimal) when you want to add entities that have no state anyway. You can also combine this with an amount. So you can easily have 1000 Gold items in your inventory (which would internally be stored as a single Gold entity template with amount 1000) and then when you drop the gold it would become an entity (with your choice if the entities are expanded to 1000 or if the entity also gets an 'amount' attribute associated with it).

In Ares this distinction will be important. As long as an entity has no real 'state' (in other words, it is still equal to its state as it was when it was created from the template (we call this 'baseline')) then when the entity is added to an inventory it will be destroyed and the entity template is added instead (possibly increasing an amount if the template was already in the inventory). However, if the entity does have state then it is added as such.