# Godot Economy System

A GDExtension for Godot 4.4, written in C++, that gives NPCs a working economy. NPCs have needs such as thirst and hunger, plan how to satisfy them with goal oriented action planning (GOAP), and trade with each other without any trade being scripted.

The extension decides **what** an NPC does. Your game decides **how** it looks: walking, animations, timers. The extension then applies the results to the NPC's wallet.

## How it fits together

```
EconomyManager            what exists: currencies and items
NPC
└── GoapTrader            decides what to do, trades with other traders
    ├── Wallet            what this NPC holds
    ├── NeedsComponent    this NPC's needs, which become goals
    └── GoapPlanner       finds the cheapest sequence of actions to a goal
```

Every few moments a trader:

1. Asks its `NeedsComponent` for its goals, most urgent first.
2. Plans the cheapest sequence of its actions that reaches the first goal it can reach.
3. Hands the first step to your game through `step_started`.
4. Applies the step's effects once your game calls `complete_step()`, and plans again.

Everything is addressed by string id (`"gold"`, `"water"`), so the extension works with any content you register.

## Pages

**Getting started**
* [Installation](Installation)
* [Quick Start](Quick-Start)

**Setting up an economy**
* [Economy and Wallets](Economy-and-Wallets)
* [Setting up an NPC](Setting-up-an-NPC)
* [Needs](Needs)
* [Actions](Actions)
* [Goals](Goals)

**In your game**
* [Carrying out Steps](Carrying-out-Steps)
* [Trading and Offers](Trading-and-Offers)
* [Demand](Demand)
* [Markets](Markets)

**Going further**
* [Debugging](Debugging)
* [Performance](Performance)
* [Balancing an Economy](Balancing-an-Economy)
* [Reference](Reference)
* [Known Limitations](Known-Limitations)
