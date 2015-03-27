# Introduction #

CEL has a powerful quest system. A quest is basically a state machine which can jump from one state to the other when certain things happen.

# Details #

The basic structure of a quest is that it has a number of states. Every state then has a number of triggers. When a trigger fires it will execute a number of rewards.

In addition a quest also supports sequences which are timed operations that can be used for animation. Some examples of useful sequences are: opening doors, glowing lights, moving platforms, ...

A few example triggers:

  * _message_: a message is received from some source. You can specify the id mask of the messages you want to listen too.
  * _sequencefinish_: a sequence finishes executing.
  * _timeout_: a certain time has elapsed.
  * _meshsel_: a mesh was selected with the mouse.
  * _inventory_: something happened on an inventory.
  * _propertychange_: a property changes.
  * _watch_: wait until a certain entity comes in range.
  * ...

A few example rewards:

  * _debugprint_: for debugging. Print out a message on the console.
  * _changeproperty_: change a property of an entity.
  * _sequence_: start an animation sequence.
  * _createentity_: create a new entity.
  * _destroyentity_: destroy an entity.
  * _inventory_: manipulate an inventory.
  * _newstate_: switch to a new state.
  * ...

# Examples #

Here we show a few quest examples.

## Simple Quest ##

```
<addon plugin="cel.addons.questdef" >
  <quest name="clickerObject">
    <state name="init">
      <trigger type="message">
        <fireon entity="$this" mask="ares.activate" />
        <reward type="debugprint" message="=?counter" />
        <reward type="changeproperty" entity="$this" property="counter" diff="1" />
        <reward type="newstate" state="init" />
      </trigger>
    </state>
  </quest>
</addon>
```

This is a very simple quest that just has a single state (named 'init'). This state has a single trigger that listens to a message with mask 'ares.activate'. This message will be sent by the Ares game whenever an entity is activated. The message is sent to the entity so if you attach this quest to an entity then it will know how to respond to activation.

Whenever the entity is activated there are three rewards to are executed:
  * First a message is printed on the console for debugging purposes. This message will be the value of the 'counter' attribute in the entity (the string '?counter' is an expression that evaluates the attribute 'counter' in the current entity).
  * Secondly the 'counter' property is modified by adding 1 to it.
  * And finally the state of the quest is reset to 'init'. This is needed because otherwise the state isn't reactivated and the quest would simply stop.

This quest never stops. Every time the entity gets the activation message the counter will be increased.

## Proximity Quest ##

```
<addon plugin="cel.addons.questdef" >
  <quest name="watchPlayerQuest">
    <state name="init">
      <trigger type="watch">
        <fireon entity="$this" target="player" checktime="1000" radius="5" />
        <reward type="message" entity="world" id="ares.messagebox">
          <par name="message" string="You enter a very dangerous area!" />
        </reward>
        <reward type="newstate" state="stop" />
      </trigger>
    </state>
    <state name="stop" />
  </quest>
</addon>
```

This quest has two states. In the initial state it will watch out for the player using the 'watch' trigger. It is set up in such a manner that it will look for the player once every second. If the player is closer to this entity then 5 units then the trigger will fire and the two rewards will execute:
  * First a message is sent to the Ares game logic (which is kept in the 'world' entity). This message is the 'ares.messagebox' message and when Ares receives this message it will show a message box to the user with a certain message. The message is given with the 'message' parameter for the reward.
  * Then the state is set to 'stop at which point the quest simply stops executing.


## A Glowing Object ##

```
<addon plugin="cel.addons.questdef" >
  <quest name="glowQuest">
    <state name="init">
      <oninit>
        <reward sequence='animglow' type='sequence' entity='$this' entity_tag='glow' />
      </oninit>
      <trigger type="sequencefinish">
        <fireon entity="$this" sequence="animglow" entity_tag='glow'/>
        <reward type="newstate" state="continue" />
      </trigger>
    </state>
    <state name="continue">
      <oninit>
        <reward type="newstate" state="init" />
      </oninit>
    </state>
    <sequence name="animglow">
      <op entity="$this" type="ambientmesh" duration="$dur" >
        <abscolor red="0" green="0" blue="1" />
      </op>
      <delay time="$dur" />
      <op entity="$this" type="ambientmesh" duration="$dur" >
        <abscolor red="1" green="0" blue="0" />
      </op>
      <delay time="$dur" />
    </sequence>
  </quest>
</addon>
```

This quest introduces a few new concepts:

First this quest defines an 'animglow' sequence which will animate the ambient color of the mesh associated with this entity. A sequence is something that keeps on executing for some time. This is in contrast with a reward that executes once, has some effect and is done.

Secondly this quest also has a parameter. If an entity wants to use this quest it has to set the 'dur' parameter at the time this quest is attached to the entity. Parameters can be used anywhere in the quest. In this case we're using it to control the duration of the animation sequence for the ambient color.

Finally this quest uses 'oninit' blocks to fire rewards as soon as the quest enters a state.

Due to the way sequences work we have to use two states here. We can't switch directly to 'init'. We first have to go to 'continue' and then back to 'init'. This limitation may be fixed in the future.

## A Bigger Example: A Player Quest ##

In this example I demonstrate how you could make a quest that actually operates as a quest for the player. i.e. it involves a story line quest that the player has to solve. Let's call these quests 'Story Quests' to distinguish them from the more mundane quests that are used for managing entity behaviour on a lower level. Nevertheless these quests are going to be made using the same quest system.

```
<addon plugin="cel.addons.questdef" >
  <quest name="findTheSecretNote">

    <state name="init">
      <oninit>
        <reward type="message" entity="world" id="ares.logentry">
          <par name="logid" string="SECNOTE_intro" />
          <par name="message" string="You must go find a secret note. Perhaps look in the basement?" />
        </reward>
        <reward type="newstate" state="watchout" />
      </oninit>
    </state>

    <state name="watchout">
      <trigger type="entersector">
        <fireon entity="player" sector="cellBasement" />
        <reward type="message" entity="world" id="ares.logentry">
          <par name="logid" string="SECNOTE_enterbasement" />
          <par name="message" string="As you enter the basement you get the impression that there was a fight recently. Perhaps you will find a clue here?" />
        </reward>
        <reward type="newstate" state="watchout" />
      </trigger>
      <trigger type="inventory">
        <fireon entity="player" child_entity="SecretNote" />
        <reward type="message" entity="world" id="ares.logentry">
          <par name="logid" string="SECNOTE_gotnote" />
          <par name="message" string="You pick up a note. This looks like the secret note you had to find." />
        </reward>
        <reward type="newstate" state="finish" />
      </trigger>
    </state>

    <state name="finish">
      <oninit>
        <reward type="message" entity="world" id="ares.finishquest" />
      </oninit>
    </state>

  </quest>
</addon>
```

This quest is a bit more complicated and it assumes that Ares has a log message system which is activated by sending a message 'ares.logentry' to the 'world' entity. Ares will add the message to the player log where all important things are logged for the player. This message also has a 'logid' parameter that should identify each log entry with a unique name so that Ares knows that a certain log entry doesn't have to be added again in case it gets the message again (why this is useful is explained below).

This quest also assumes there is an 'ares.finishquest' message. When Ares gets this message it will mark the quest (and associated log entries) as finished.

The quest has three states:
  * _init_: the initial state. This state has no triggers but it has an 'oninit' block that is fired as soon as this state is entered. In the oninit block there is a reward that will send out a log entry to Ares giving some hints to the player about this quest. In this case the player gets a log entry telling him/her to go look in the basement. In addition to that the quest immediatelly switches to the 'watchout' state.
  * _watchout_: this state is interesting as it has two triggers. This state represents the main loop of the quest. As long as the quest is active it will remain in this state. And while it is active it will constantly watch out for two conditions: as soon as the player enters the basement it will log an additional hint to the player log. And as soon as the player picks up an entity called 'SecretNote' another log entry telling the player that he picked up the secret note is added.
  * _finish_: as soon as the player picks up the note the quest will go to this state and the quest is marked as finished.

Note that it is possible that the player will enter the basement multiple times without picking up the note. Every time the 'entersector' trigger will fire. But only the first time will the player actually get a new log entry due to the unique ID.

Also note that this quest is fairly robust in case (for whatever reason) the secret note happens to have moved out of the basement (perhaps another NPC picked it up and moved it elsewhere). The main 'watchout' state doesn't care if the secret note is picked up in the basement. It will work in any case. And if the player never enters the basement and manages to find the secret note elsewhere then the quest will also function correctly.