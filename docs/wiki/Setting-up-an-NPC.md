# Setting up an NPC

Add a **GoapTrader** to the NPC. It looks for a `Wallet`, a `NeedsComponent` and a `GoapPlanner` among its children by type, and creates any that are missing, so a single node is enough to start.

```
NPC (your own node, for example a CharacterBody2D)
└── GoapTrader
    ├── Wallet            (created if missing)
    ├── NeedsComponent    (created if missing)
    └── GoapPlanner       (created if missing)
```

Then configure it in the inspector:

* `actions` on the trader: what this NPC can do. See [Actions](Actions).
* `needs` on its `NeedsComponent`: what this NPC needs. See [Needs](Needs).
* `fallback_goal` on its `NeedsComponent`: what it works towards when no need is urgent. See [Goals](Goals).
* `market_group` on the trader: which traders it trades with. See [Markets](Markets).

The trader links its wallet to its `NeedsComponent`, so needs are satisfied from the NPC's own wallet.

Next: [Needs](Needs)
