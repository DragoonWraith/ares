# Introduction #

The loot generator is a plugin in CEL that adds the concept of a 'loot package'. The inventory property class works closely together with the loot generator. You can set up inventory with a loot package so that as soon as the inventory is opened the loot package is generated and the actual objects are added to the inventory.

# Details #

In XML a loot package is defined as follows:

```
<addon plugin="cel.addons.lootloader">
    <package name="ores" minloot="2" maxloot="6">
        <item template="Gold" chance="0.1" minamount="1" maxamount="10" />
        <item template="Diamond" chance="0.02" minamount="1" maxamount="5" validate="=?player.level > 3" />
        <item template="Silver" chance="0.2" minamount="1" maxamount="10" />
        <item template="Ruby" chance="0.04" minamount="1" maxamount="7" />
        <item template="Emerald" chance="0.04" minamount="1" maxamount="7" />
        <item template="Iron" chance="0.4" minamount="1" maxamount="20" />
        <item template="Copper" chance="0.4" minamount="1" maxamount="20" />
    </package>
</addon>
```

In this package you can see different [entity templates](EntityTemplates.md) that are selected based on a 'chance'. The minimum and maximum amount is also specified. It is also possible to specify an expression that will be evaluated to decide if the template will be selected or not. In the example above we only generate diamonds if the player level is greater then 3.

In addition to loot packages there are also loot selectors. A loot selector can be defined as follows:

```
<addon plugin="cel.addons.lootloader">
    <selector name="simpleboxloot">
        <generator name="ores" chance="0.5" validate="=?player.level > 2" />
        <generator name="food" chance="1" />
    </selector>
</addon>
```

A loot selector is basically a way to choose between different loot packages depending on various criteria. You can use a 'chance' again (like in the example above) or you can use an expression. So you could for example define different loot packages depending on the level of the player and then use a loot selector to select the right package. In the example above we only allow the 'ores' package to be selected if the level of the player is greater then 2. This is done by the expression after 'validate'.

For the inventory property class, loot packages and loot selectors are equivalent. It can work with both.