# Reference

All names are as used in GDScript.

## EconomyManager (Node)

| | |
|---|---|
| Properties | `initial_currencies: Array[Currency]`, `initial_items: Array[EconomyItem]` |
| Methods | `create_currency(id, name, precision = 2, base_value = 1.0, symbol = "$")`, `register_currency(currency)`, `get_currency(id)`, `has_currency(id)`, `get_currencies()`, `create_item(id, name, base_value = 1.0, stackable = true, max_stack_size = 999)`, `register_item(item)`, `get_item(id)`, `has_item(id)`, `get_items()` |
| Signals | `currency_registered(currency)`, `item_registered(item)` |

## Currency (Resource)

`id`, `display_name`, `precision`, `base_value`, `symbol`

## EconomyItem (Resource)

`id`, `display_name`, `base_value`, `stackable`, `max_stack_size`

## Wallet (Node)

| | |
|---|---|
| Methods | `add_currency(id, amount)`, `remove_currency(id, amount)`, `has_currency(id, amount)`, `get_currency_balance(id)`, `get_currencies()`, `transfer_currency_to(other, id, amount)`, `add_item(id, amount)`, `remove_item(id, amount)`, `has_item(id, amount)`, `get_item_balance(id)`, `get_items()`, `transfer_item_to(other, id, amount)` |
| Signals | `balance_changed(item_id, new_amount)` |

## Need (Resource)

`need_id`, `display_name`, `max_value`, `start_value`, `decay_per_second`, `urgent_threshold`, `priority`, `satisfier_item_id`, `restore_per_item`, `consume_threshold`, `goal_amount`. See [Needs](Needs).

## NeedsComponent (Node)

| | |
|---|---|
| Properties | `needs: Array[Need]`, `fallback_goal: GoapGoal`, `auto_decay`, `auto_consume` |
| Methods | `get_need(id)`, `get_need_value(id)`, `set_need_value(id, value)`, `is_need_urgent(id)`, `decay(delta)`, `consume_for_need(id, wallet)`, `try_auto_consume()`, `get_active_goals()`, `get_current_goal()`, `set_wallet(wallet)`, `get_wallet()` |
| Signals | `need_changed(need_id, value)`, `need_became_urgent(need_id)`, `need_satisfied(need_id)`, `need_consumed(need_id, item_id)` |

## GoapAction (Resource)

| | |
|---|---|
| Properties | `action_name`, `kind` (`KIND_CUSTOM`, `KIND_TRADE`), `requires`, `consumes`, `produces`, `cost` |
| Script overrides | `_is_available(trader) -> bool`, `_get_cost(trader) -> float` |

## GoapGoal (Resource)

| | |
|---|---|
| Properties | `goal_name`, `priority`, `item_targets`, `currency_targets` |
| Methods | `set_item_target(id, amount)`, `get_item_target(id)`, `set_currency_target(id, amount)`, `get_currency_target(id)`, `clear_targets()` |

## GoapPlanner (Node)

| | |
|---|---|
| Properties | `max_depth` (default 8), `max_explored_states` (default 2000) |
| Methods | `build_world_state(wallet)`, `plan_for_wallet(goal, wallet, actions, trader = null, extra_costs = {})`, `plan_from_state(goal, state, actions, trader = null, extra_costs = {})` |

`plan_from_state` plans from any Dictionary of id to amount, so you can ask "what would this NPC do if...?" without changing its wallet.

## GoapPlan (RefCounted)

`get_result()`, `is_found()`, `get_goal()`, `get_steps()`, `get_step_count()`, `get_step_costs()`, `get_total_cost()`, `get_cost_from(first_step)`, `get_explored_states()`, `get_search_usec()`, `get_message()`, `describe()`. Results are listed in [Debugging](Debugging).

## GoapTrader (Node)

| | |
|---|---|
| Properties | `actions: Array[GoapAction]`, `auto_run`, `instant_steps`, `replan_interval`, `market_group`, `work_value`, `demand_duration`, `demand_priority`, `refusal_memory_seconds`, `refusal_penalty` |
| Planning | `make_plan()`, `start_plan()`, `get_current_plan()`, `add_action(action)`, `remove_action(action)`, `get_action_penalty(action)` |
| Steps | `complete_step()`, `fail_step(reason)`, `is_executing()`, `get_current_step()`, `get_current_step_index()`, `get_current_partner()` |
| Offers | `evaluate_offer(they_give, they_want)`, `receive_offer(from_wallet, they_give, they_want)`, `get_last_offer_reason()` |
| Demand | `post_demand(item_id, amount)`, `clear_demand(item_id)`, `get_active_demands()` |
| Children | `get_wallet()`, `get_needs()`, `get_planner()` |
| Script override | `_rank_partner(partner, action) -> float` |
| Signals | `plan_changed(plan)`, `step_started(action, partner)`, `step_completed(action)`, `step_failed(action, reason)`, `trade_completed(partner, gave, received)`, `offer_received(from_wallet, they_give, they_want, accepted, reason)`, `demand_posted(item_id, amount)` |

`evaluate_offer` and `receive_offer` return `{accepted: bool, reason: String, plan: GoapPlan}`. `they_give` is what the other side gives this trader, and `they_want` is what it asks for in return.
