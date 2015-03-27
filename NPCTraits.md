# Introduction #

To define NPC's in Ares I propose a system of 'traits'. A trait is basically a set of attributes, properties and other stuff which is all collected in a single object. An NPC can be defined using different traits. Also at runtime you can associate and change the traits which are coupled to an NPC. To clarify this idea let us make it more concrete with a few examples which could be relevant to the Project Ares game. Note that the definitions below are just examples. It will most likely not be like this in the real game:

# Examples #

First let us make a few traits for the different races we have:

  * The 'Human' trait:
    * Race attribute: 'human'
    * Standard health: 100
    * Enemy with: race 'alien'
    * Neutral with: race 'animal'
    * Clothes: 'casual clothes'
    * AI: watchfull
    * ...

  * The 'Alien' trait:
    * Race attribute: 'alien'
    * Standard health: 90
    * Enemy with: race 'human'
    * Enemy with: race 'animal'
    * AI: watchfull
    * ...

  * The 'Dangerous Creature' trait:
    * Race attribute: 'animal'
    * Standard health: 120
    * Enemy with: race 'human'
    * Enemy with: race 'alien'
    * AI: hunting
    * ...

  * The 'Docile Creature' trait:
    * Race attribute: 'animal'
    * Standard health: 110
    * AI: wandering
    * ...

Then we have traits for different factions:

  * The 'Favor Alien Domination' trait:
    * Friendly with: race 'alien'

  * The 'Favor Diplomatic Solution' trait:
    * Friendly with: race 'human'
    * Friendly with: race 'alien'

  * The 'Favor Human Victory' trait:
    * Friendly with: race 'human'

Then make traits for a specific occupations:

  * The 'Human Soldier' trait:
    * Standard health: 110
    * Clothes: 'human soldier set'
    * Trait: 'Favor Human Victory'
    * AI: engage

So in this example we have a human. However it is also a soldier so the default health of 100 is changed to 110.

  * The 'Merchant' trait:
    * Standard health: 98
    * AI: wandering

When you define an NPC you first select the base model and then you associate traits to that model. The order of the traits is important. Later traits override attributes defined in earlier traits. For example, a specific human soldier NPC would look like this:

  * Model 'Human Male':
    * Trait: 'Human'
    * Trait: 'Human Soldier'
    * Trait: NPC specific
      * Attribute name: 'Mark'
      * Attribute hair: 'Blond'
      * Attribute eyes: 'Blue'

Let's make another example:

  * Model 'Human Male':
    * Trait: 'Human'
    * Trait: 'Human Soldier'
    * Trait: 'Favor Diplomatic Solution'
    * Trait: NPC specific
      * Attribute name: 'Peter'
      * Attribute hair: 'Brown'
      * Attribute eyes: 'Black'

This is also a soldier but the default 'Favor Human Victory' trait which is defined in 'Human Soldier' is now changed to 'Favor Diplomatic Solution'. So this is a non-typical soldier who happens to be in the army but would like a diplomatic solution anyway.

Here is another example of a tiger which is tamed and trained to attack only aliens:

  * Model 'Tiger':
    * Trait: 'Dangerous Creature'
    * Trait: NPC specific
      * Attribute name: 'Tom'
      * Neutral with: race 'human'
      * Friendly with: npc 'Mark'

So this tiger is neutral to humans but still enemy with aliens (since that is defined in the 'Dangerous Creature' trait). He is also friendly with Mark who is the owner of this tiger.


# The UI In AresEd #

In AresEd this would require three things:
  * A trait editor window for editing a single trait.
  * A window where all the existing traits are listed and where one can add new traits.
  * Then the NPC editor where one can add previously created traits and also get access to the 'NPC specific' trait (editing this would go to the the 'trait editor').