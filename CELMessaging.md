# Introduction #

CEL makes heavy usage of a message system for communcation between property classes. Using these messages everything is connected and game logic is defined.

# Message Channels #

The CEL message system is based on **message channels**. Message producers can send messages to channels and messages consumers can listen to specific channels.

Messages are attributed with **message id**. This is a string indicating what kind of message it is. A few id's which are already used in CEL are:
  * _cel.input.mouselook_
  * _cel.input.forward.up_
  * _ares.controller.Examine_

When a message receiver subscribes to a message channel it can specify a mask. This is a substring of an id. For example, if you want to listen to all messages produced by a channel that come from CEL itself you could use _cel._ as a mask. If you only want to listen to messages produced by the input property class then listen to _cel.input._. A message receiver can subscribe to the same channel multiple times using different masks.

Every entity in CEL implements its own message channel. This is the most commonly found type of message channel although it is possible to make your own message channels that are not tied to an entity.

# Property Classes #

In the previous section we listed three example message masks.

The first two are from CEL. These messages are sent by the input property class whenever it receives that kind of input from the mouse or keyboard. Some other property classes (like for example the movement property classes) will listen to these messages.

The last example is implemented by the Ares game controller and can be used to examine an object.

# Wiring Messages #

Many property send out messages and also listen to them. By wiring these messages together in some way you define how the game works. This is **game logic**.

There are three basic ways to wire messages:

  * The default wiring. Many property classes already know what messages they are interested in. For example, the movement property classes already listen to specific messages which are produced by the input property class. For these you don't have to do anything. Wiring is default.
  * The [Wire Property Class](PCWire.md). Using this property class you can listen to a mask for a given entity and produce another message for another channel (or same channel). Using this property class you can make connections between messages which are not connected by default. A common example is to listen to the _ares.controller.Activate_ message which is sent by the Ares game controller whenever the user actives some object. You can then use a wire property class on a given object to wire this message to whatever needs to be done when this object is activated. This could again be another message to the Ares game controller. For example to show a note to the player.
  * The [Quest Property Class](PCQuest.md). This is the most powerful way to wire messages together. See the quest page for more information on how quests work but quests can also listen to messages (using triggers) and send out messages (in rewards).