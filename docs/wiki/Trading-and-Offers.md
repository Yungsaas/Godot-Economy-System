# Trading and Offers

## Finding a partner

When a trade step starts, the trader looks for traders in its [market](Markets) that hold what it wants, and asks each one in turn whether it would accept. It only emits `step_started` for a partner that would accept, so your NPC never walks to someone who will refuse.

To choose the order, for example nearest first, add a script to the trader. Lower values are tried first:

```gdscript
extends GoapTrader

func _rank_partner(partner: Node, _action: GoapAction) -> float:
	return get_parent().global_position.distance_to(partner.get_parent().global_position)
```

If nobody holds what the trader wants, it posts a demand instead. See [Demand](Demand).

## How a trader judges an offer

It plans its current goal twice, without and with the offer, and compares the cost of both plans:

| Result | Decision |
|---|---|
| The plan gets cheaper, or the goal becomes reachable | Accept |
| The plan stays the same | Accept if what it receives is worth at least what it gives, by `base_value` |
| The plan gets more expensive but still works | Accept if the payment is at least the extra cost × `work_value` |
| The goal becomes unreachable | Refuse, whatever the price |

`work_value` is how much one unit of plan cost is worth to this NPC. It lets producers sell stock they still need, as long as the price pays for making more. A high value makes the NPC reluctant to sell; 0 means it only sells surplus.

Every answer comes with a reason, for example:

> 1 water for 2.5 gold: payment 2.5 covers the extra work for "Water carrier reserves" (0.0 -> 2.0, worth 2.0)

Read it with `get_last_offer_reason()` or the `offer_received` signal.

## Failed trades

A failed action becomes `refusal_penalty` more expensive for `refusal_memory_seconds`, so the next plan avoids it if there is another way.

## Trading with the player

There is no shop node, because the existing methods already cover it. Your shop UI sends the player's offer to an NPC with `receive_offer`, and the NPC judges it like any other offer:

```gdscript
# The player offers gold for 1 bread. The price is the game's choice, here with a 50% markup.
var price := 6.0 * 1.5
var result: Dictionary = npc_trader.receive_offer(player_wallet, {"gold": price}, {"bread": 1})
if result.accepted:
	show_message("Thanks!")
else:
	show_message(result.reason)
```

* `receive_offer(from_wallet, they_give, they_want)`: `they_give` is what the player gives, `they_want` is what the player asks for. If the NPC accepts, the goods are exchanged at once and the NPC plans around the result.
* `evaluate_offer(they_give, they_want)` gives the same answer without trading, for example to grey out a button.
* For trades that should always happen, such as quest rewards or loot, use the [wallet](Economy-and-Wallets) methods directly.

Both methods return `{accepted: bool, reason: String, plan: GoapPlan}`.

Next: [Demand](Demand)
