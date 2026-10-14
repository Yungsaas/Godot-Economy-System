# Actions

A **GoapAction** describes only what it needs and what it changes, never which goal it serves, so it stays correct for content added later.

| Field | Meaning |
|---|---|
| `kind` | **Custom**: your game carries it out (farming, crafting). **Trade**: another trader gives what it produces and receives what it consumes |
| `requires` | Must be held but is not used up, for example a tool |
| `consumes` | Must be held and is removed |
| `produces` | Is added |
| `cost` | How much the planner dislikes doing it. The planner picks the cheapest sequence |

Each of `requires`, `consumes` and `produces` is a Dictionary of id to amount, and can mix items and currencies.

## Examples

| Action | Kind | Fields |
|---|---|---|
| Farm grain | Custom | produces `{grain: 3}`, cost 2 |
| Bake bread | Custom | consumes `{grain: 1}`, produces `{bread: 1}` |
| Chop wood | Custom | requires `{axe: 1}`, produces `{wood: 1}` |
| Buy water | Trade | consumes `{gold: 2.5}`, produces `{water: 1}` |
| Sell bread | Trade | consumes `{bread: 1}`, produces `{gold: 6}` |

A trade action sets the price: "Buy water" offers 2.5 gold for 1 water. Whether the other trader accepts is described in [Trading and Offers](Trading-and-Offers).

Giving an NPC both a way to make something and a way to buy it lets the planner choose. It buys when that is cheaper, and makes it itself when buying is expensive or nobody sells.

## Conditions the fields cannot express

Attach a script that extends `GoapAction`. Both methods are asked once per plan, not for every state the planner explores.

```gdscript
extends GoapAction

func _is_available(trader) -> bool:
	return trader.get_parent().is_daytime   # false removes the action from planning

func _get_cost(trader) -> float:
	return trader.get_parent().distance_to_field() / 100.0   # replaces the cost field
```

Designers keep making plain actions in the inspector, programmers add script based ones, and both go in the same `actions` list.

Next: [Goals](Goals)
