# Debugging

Every decision the extension makes comes with a reason you can read.

* `get_current_plan()` returns a `GoapPlan`. `describe()` gives a readable summary, and `get_message()` says why a plan was or was not found, for example *"None of the available actions produces "water"."*
* `get_last_offer_reason()` returns the reason for the last offer this trader answered.
* `get_active_demands()` returns what this trader is waiting for.
* The signals `plan_changed`, `step_failed`, `offer_received` and `demand_posted` give everything a debug label or a log needs. `goap_demo.gd` and `npc.gd` in the test project show both.

## Plan results

| `GoapPlan` result | Meaning |
|---|---|
| `RESULT_FOUND` | Steps lead to the goal |
| `RESULT_ALREADY_SATISFIED` | The goal already holds |
| `RESULT_NO_PLAN` | No sequence of available actions reaches the goal within `max_depth`, or the trader is waiting for a demand |
| `RESULT_SEARCH_LIMIT` | The search stopped at `max_explored_states`. A plan may still exist |
| `RESULT_INVALID` | Missing input, for example no goal |

## Common problems

| Symptom | Likely cause |
|---|---|
| Nobody trades at all | Goal targets are higher than what exists in the economy. See [Goals](Goals) |
| Ordinary trades are refused as "not worth it" | Prices in the actions do not match the items' `base_value`. See [Balancing an Economy](Balancing-an-Economy) |
| A producer never sells | `work_value` is 0, or its price does not cover the cost of making more. See [Trading and Offers](Trading-and-Offers) |
| "nobody has ..." | Nobody in the same [market](Markets) holds the item, and nobody has a custom action that makes it |
| An NPC slowly goes broke | It spends more than it earns. See [Balancing an Economy](Balancing-an-Economy) |

Next: [Performance](Performance)
