# Quick Start

Two NPCs: one draws water and sells it, the other gets thirsty and buys it.

1. Add an **EconomyManager** node to your scene. In the inspector, add a `Currency` to `initial_currencies` (id `gold`) and an `EconomyItem` to `initial_items` (id `water`, base value 2).
2. Add a **GoapTrader** node to each NPC. It creates its own `Wallet`, `NeedsComponent` and `GoapPlanner` if they are missing.
3. On the buyer's `NeedsComponent`, add a `Need` (id `thirst`, satisfier `water`). On the seller's `NeedsComponent`, set a fallback goal with the item target `water: 2`, so he keeps water in stock. See [Goals](Goals).
4. Add actions:
   * Buyer: "Buy water", kind **Trade**, consumes `gold: 2`, produces `water: 1`
   * Seller: "Draw water", kind **Custom**, produces `water: 2`, and "Sell water", kind **Trade**, consumes `water: 1`, produces `gold: 2`
5. Give the NPCs some gold, and respond to `step_started`:

```gdscript
func _ready() -> void:
	$GoapTrader.get_wallet().add_currency("gold", 10.0)
	$GoapTrader.step_started.connect(_on_step_started)

func _on_step_started(action: GoapAction, partner: GoapTrader) -> void:
	# Walk, animate, wait... then report back:
	$GoapTrader.complete_step()
```

To try it without any game code at all, turn on `instant_steps` on both traders.

Next: [Economy and Wallets](Economy-and-Wallets)
