# Needs

A **Need** is a definition, so one `thirst` resource can be shared by every NPC. The current value of a need lives on each NPC's `NeedsComponent`.

| Field | Meaning |
|---|---|
| `need_id` | Id used in code and signals |
| `max_value`, `start_value` | The value runs from 0 (unmet) to `max_value` (fully met) |
| `decay_per_second` | How fast the value drops |
| `urgent_threshold` | At or below this, the need is urgent and produces a goal |
| `priority` | Higher is planned for first |
| `satisfier_item_id` | The item that satisfies the need |
| `restore_per_item` | How much one item restores |
| `goal_amount` | How many satisfier items the goal asks for |
| `consume_threshold` | When `auto_consume` eats or drinks. `-1` means the same as `urgent_threshold` |

## How a need is satisfied

1. The value drops by `decay_per_second`.
2. Once it reaches `urgent_threshold`, the need becomes a goal: "hold `goal_amount` of the satisfier item". The trader plans to get one, for example by buying it.
3. With `auto_consume` on, the `NeedsComponent` consumes the item from the wallet as soon as the value reaches `consume_threshold`, and the value goes up by `restore_per_item`.

Several urgent needs are planned for in order of `priority`. If the most important one cannot be reached right now, the trader works on the next one.

## Signals

On the `NeedsComponent`:

| Signal | When |
|---|---|
| `need_changed(need_id, value)` | Every change, including decay every frame |
| `need_became_urgent(need_id)` | The need reaches its urgent threshold |
| `need_satisfied(need_id)` | The need rises above its urgent threshold again |
| `need_consumed(need_id, item_id)` | An item was consumed for the need |

## Driving decay yourself

Turn off `auto_decay` and call `decay(delta)` when you want, for example less often for NPCs far away. Auto consume still runs after each call. See [Performance](Performance).

Next: [Actions](Actions)
