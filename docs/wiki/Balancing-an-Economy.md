# Balancing an Economy

The extension keeps the economy consistent. Balancing it is a design job, because it depends on your content and your players, but some rules help.

* **Match base values to prices.** Traders use `base_value` to judge whether a trade is fair. If the prices in your actions differ from the base values, ordinary trades look unfair and are refused.
* **Check income against spending per role.** In a closed economy, every coin one NPC gains, another loses. If one role spends more than it earns, it slowly goes broke and drops out of the economy.
* **Give every role a reason to spend.** An NPC without needs only collects money.
* **Avoid recipes that block.** A recipe needing 2 grain blocks a baker who has 1 grain and no money. A recipe of 1 grain to 1 bread does not.
* **Player pricing is up to your game.** For example, charge the player a markup through `receive_offer`. See [Trading and Offers](Trading-and-Offers).

## Example: the demo's prices

In the demo, per 40 seconds each NPC eats about 1 bread and drinks about 1.2 water. The baker bakes 3 bread from 3 grain, eats one and sells two. Setting income equal to spending for every role gives:

* bread = 2.4 × water
* grain = 1.2 × water

With water at 2.5 that gives grain 3 and bread 6, and no NPC slowly gains or loses gold.

Next: [Reference](Reference)
