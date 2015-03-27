# Introduction #

Everything related to NPC conversation is here. Note that the NPC conversation system as explained here is not implemented yet.


# Voice Packages #

A voice package represents a certain 'voice' of an NPC. A voice package basically defines the statements a certain group of NPCs can speak together with optional audio associated with it. A voice package is defined as a group of statements. Every statement has an ID and a number of random sentences+audio. Here is an example voice package:

```
<addon template="cel.addons.conversation.loader">
  <voice name="youngFemale">
    <statement id="stGreeting">
      <sentence msg="Hi sir" voice="youngfemale_hisir.wav" validate="=?player.sex=='M'" />
      <sentence msg="Hi lady" voice="youngfemale_hilady.wav" validate="=?player.sex=='F'" />
      <sentence msg="Greetings sir" voice="youngfemale_greetingssir.wav" validate="=?player.sex=='M'" />
      <sentence msg="Greetings lady" voice="youngfemale_greetingslady.wav" validate="=?player.sex=='F'" />
    </statement>
    <statement id="stGoodbye">
      <sentence msg="Goodbye!" voice="youngfemale_goodbye.wav" />
      <sentence msg="Later!" voice="youngfemale_later.wav" />
      <sentence msg="Hmpf..." voice="youngfemale_grunt.wav" validate="=?mood < .3" />
    </statement>
  </package>
</addon>
```

In this example we created a simple voice package for a young female NPC. The 'voice' tags are optional but in this package they are specified so the NPCs using this package will actually speak. This package defines two statement. A greeting statement and a goodbye statement. The greeting statement has four sentences. Two are used in case the player is male and the other two are used in case the player is female. The goodbye statement has three sentences. One of these sentences will only be used when the mood of the NPC is low.

There can be other voice packages which have sentences for the same message ID's but use other words to say the same thing. For example, an alien NPC may do a greeting with a very different sentence. For the conversation packages (see below) it doesn't make a difference. It is the 'stGreeting' statement that the NPC will say. But what is actually said can be very different depending on the style of the NPC.

Voice packages can inherit from one or more other voice packages. This is useful in case you have a more elaborate voice package for another NPC that actually has the same set of conversations as another NPC.

All voice packages inherit from the default voice package. This is a voice package that only has messages and no voice and should contain fallback messages (text only) for all message id's. So if an NPC would say a certain statement but has no specific sentences for that statement a fallback can be picked. If there is also no fallback then the message ID will be shown to the player.

Using this system it is fairly easy to make internationalized packages as well as customized voices.

# Player Voice Package #

While the player is not technically an NPC he or she also has a voice package. This voice package contains no audio but lists all statements that a player can use to talk with NPCs. Which statements are actually possible for a given NPC is defined by the conversation packages (see below).

# Conversation Packages #

While the voice package defines what an NPC can say (and how he or she says it), the conversation packages defines the actual conversation options at any given time. It also defines the options for the player. Conversation packages can be attached to NPCs but also to quests.

```
<addon template="cel.addons.conversation.loader">
  <conversation name="soldierConversation">
    <greeting id="stGreeting" />
    <choice id="playerIntroduction" answer="stIntroduction" />
    <choice id="playerWhereDepot" answer="stExplainDepot" />
    <choice id="playerWhereBarracks" answer="stExplainBarracks" />
    <choice id="playerTrade" answer="stOkLetsTrade" validate="=?mood(ent('player'),$this) > .7" end="true" message="ares.tradeWindow">
      <par name="target" string="$this" />
    </choice>
    <choice id="playerGoodbye" answer="stGoodbye" end="true" />
  </conversation>
  <conversation name="questHiddenKeyClue">
    <choice id="playerWhereIsTheKey" answer="stLookKitchen"
        quest="questHiddenKey" state="looking" />
  </conversation>
</addon>
```

Here you see two conversation packages. The first one is one that can be attached to any human soldier. It allows four standard questions from the player and one optional one that only occurs if the soldier likes the player sufficiently (trading). When the player selects an NPC to start a conversation the NPC will immediatelly say the 'greeting' message (this is an optional tag). And then the player will get 4 or 5 options to choose from depending on how the soldier likes the player. Some of the options immediatelly end the conversation (like 'Goodbye' and 'Trade').

The second conversation package belongs to a quest. This package will be added by the 'questHiddenKey' quest to all NPCs that need it. So at any given time there can be multiple conversation packages active on a given NPC. The list of options for the player will be built from all active conversation packages (depending on 'validate' and other conditions). In this example the player gets an extra 'Where is the key?' question that he or she can ask the soldier. But this question is only present if the 'questHiddenKey' is still in the 'looking' state.