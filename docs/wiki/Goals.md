# Goals

A **GoapGoal** is a set of minimum amounts:

| Field | Meaning |
|---|---|
| `goal_name` | Shown in plans and offer reasons |
| `item_targets` | Item id to minimum amount (int) |
| `currency_targets` | Currency id to minimum amount (float) |
| `priority` | Higher is planned for first |

Needs produce their goals automatically (see [Needs](Needs)). You author one goal yourself: the fallback goal.

## The fallback goal

Set `fallback_goal` on the `NeedsComponent`. It is always active and always planned last, whatever its own `priority` says, so an NPC with no urgent need still has something to work towards and to judge offers against.

This is what makes NPCs produce stock and trade their surplus. A farmer with the fallback goal "hold 2 grain and 4 gold" farms when he has no grain, and sells the grain he has beyond that.

## Choosing targets

* **Keep them reachable.** If the currency targets of all NPCs add up to more money than exists in the economy, no NPC can reach its goal and trade stops.
* **Producers should make more at once than they keep.** A farmer who farms 3 grain and keeps 2 always has 1 to sell.
* **Targets control willingness to trade.** Higher targets make an NPC hold on to more. Lower targets make it sell more readily.

Next: [Carrying out Steps](Carrying-out-Steps)
