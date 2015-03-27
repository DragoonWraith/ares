# Introduction #

  * Message handling property class
  * _pclogic.wire_
  * _iPcWire_

# Details #

CEL is heavily based on a message system. Various property classes send out messages (for example, [input property class](PCInput.md) sends out messages when it recognizes input, [spawning property class](PCSpawn.md) sends a message when it spawns an entity, [inventory property class](PCInventory.md) sends out messages when an item is added or removed, and so on). Also the Ares game logic will send out messages and react to messages. This way the connection between the user interface and the property classes can be managed (like opening the inventory when the player activates some object can be done by containers listening to 'ares.activate' message).

Messages have string identifiers. CEL messages typically start with 'cel.'.

The wire property class allows an entity to listen to certain types of messages and when it receives them send out a new message. That way you can define an entity that listens to some message (like it listens to when an inventory gets some object) and then it reacts by sending out another message (like to the game to open that inventory).

# Example #

In the section on [entity templates](EntityTemplates.md) you can find an example on how to use this property class.