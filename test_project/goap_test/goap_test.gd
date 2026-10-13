extends Control

# Test scene for the GOAP traders: planning, carrying out steps, and trading.
#
# Run this scene on its own with F6. The traders run by themselves (auto_run). This
# script plays the part of the developer's code: when a trader starts a step, it waits
# STEP_DURATION seconds, standing in for walking or working, and then calls
# complete_step(). The extension applies the effects and the trader plans again.
#
#   Ada, a farmer:         can farm grain and sell it, has to buy bread and water
#   Bram, a baker:         has to buy grain, can bake bread and sell it, has to buy water
#   Cato, a water carrier: can draw water and sell it, has to buy bread
#
# Ada and Bram share the same Need resources but have their own values.
# Cato has no children in the scene, so its Wallet, NeedsComponent and GoapPlanner are
# created automatically, and it has no needs, so it always plans for the fallback goal.

# Gold targets in the fallback goals are 4 each and there are 30 gold in total, so every
# trader can reach its reserves at the same time. Producers keep 1 of their product and
# produce more than that at once, so they have surplus to sell.
const STARTING_BALANCES := {
	"Ada":  {"gold": 10.0},
	"Bram": {"gold": 10.0, "water": 3},
	"Cato": {"gold": 10.0},
}

# Start values that differ from the Need resource, to show that values are per NPC.
const STARTING_NEED_VALUES := {
	"Ada":  {"thirst": 40.0},
	"Bram": {"hunger": 35.0},
}

const STEP_DURATION := 0.8

@onready var economy: EconomyManager = $EconomyManager

var _traders: Array[GoapTrader] = []
var _views := {}  # GoapTrader -> Dictionary of controls
var _last_plan_text := {}  # GoapTrader -> String, to log only when a plan changes
var _log: RichTextLabel
var _refresh_timer := 0.0
var _paused := false


func _ready() -> void:
	# Children are ready before this runs, so every GoapTrader has its children by now.
	# Content is registered before any wallet is filled.
	_register_content()
	for child in $Traders.get_children():
		if child is GoapTrader:
			_traders.append(child)

	_configure_cato()
	_apply_starting_state()
	_build_ui()
	_connect_signals()
	_log_line("Ready. Each step takes %.1f s (scaled by the speed buttons)." % STEP_DURATION)


func _register_content() -> void:
	economy.create_currency("gold", "Gold", 2, 1.0, "G")
	economy.create_item("water", "Water", 2.0)
	economy.create_item("bread", "Bread", 4.0)
	economy.create_item("grain", "Grain", 2.0)
	economy.create_item("tool", "Tool", 10.0, false)  # not stackable, used by the checks


func _configure_cato() -> void:
	# Cato's NeedsComponent was created automatically and is empty, so his goal is set here.
	var cato := $Traders/Cato as GoapTrader
	var goal := GoapGoal.new()
	goal.goal_name = "Water carrier reserves"
	goal.set_item_target("water", 1)
	goal.set_currency_target("gold", 4.0)
	cato.get_needs().fallback_goal = goal


func _apply_starting_state() -> void:
	for trader in _traders:
		var wallet := trader.get_wallet()
		var balances: Dictionary = STARTING_BALANCES.get(String(trader.name), {})
		for id in balances:
			if economy.has_currency(id):
				wallet.add_currency(id, balances[id])
			else:
				wallet.add_item(id, balances[id])
		var need_values: Dictionary = STARTING_NEED_VALUES.get(String(trader.name), {})
		for need_id in need_values:
			trader.get_needs().set_need_value(need_id, need_values[need_id])


# ---------------------------------------------------------------- the developer's side

func _connect_signals() -> void:
	for trader in _traders:
		trader.step_started.connect(_on_step_started.bind(trader))
		trader.step_failed.connect(_on_step_failed.bind(trader))
		trader.trade_completed.connect(_on_trade_completed.bind(trader))
		trader.offer_received.connect(_on_offer_received.bind(trader))
		trader.plan_changed.connect(_on_plan_changed.bind(trader))
		trader.get_needs().need_consumed.connect(_on_need_consumed.bind(trader))


func _on_step_started(action: GoapAction, partner: GoapTrader, trader: GoapTrader) -> void:
	var where := " with %s" % partner.name if partner != null else ""
	_log_line("%s starts: %s%s" % [trader.name, action.action_name, where])
	# This is where a game would walk to the partner or play an animation.
	# Afterwards it must tell the trader whether the step worked.
	var index := trader.get_current_step_index()
	await get_tree().create_timer(STEP_DURATION).timeout
	if trader.is_executing() and trader.get_current_step_index() == index and trader.get_current_step() == action:
		trader.complete_step()


func _on_step_failed(action: GoapAction, reason: String, trader: GoapTrader) -> void:
	var action_name: String = action.action_name if action != null else "step"
	_log_line("[color=orange]%s: %s failed, %s[/color]" % [trader.name, action_name, reason])


func _on_trade_completed(partner: GoapTrader, gave: Dictionary, received: Dictionary, trader: GoapTrader) -> void:
	_log_line("[color=lightgreen]%s traded %s with %s for %s[/color]" % [
		trader.name, _amounts_text(gave), partner.name, _amounts_text(received)])


func _on_offer_received(_from_wallet: Wallet, _they_give: Dictionary, _they_want: Dictionary,
		accepted: bool, reason: String, trader: GoapTrader) -> void:
	_log_line("  %s %s offer: %s" % [trader.name, "accepts" if accepted else "refuses", reason])


func _on_plan_changed(plan: GoapPlan, trader: GoapTrader) -> void:
	var text := _plan_text(plan)
	if _last_plan_text.get(trader, "") != text:
		_last_plan_text[trader] = text
		_log_line("%s new plan: %s" % [trader.name, text])


func _on_need_consumed(need_id: String, item_id: String, trader: GoapTrader) -> void:
	_log_line("%s consumed 1 %s for %s." % [trader.name, item_id, need_id])


func _plan_text(plan: GoapPlan) -> String:
	if plan == null:
		return "none"
	var goal_name: String = plan.get_goal().goal_name if plan.get_goal() != null else "no goal"
	if plan.is_found():
		return "%s -> %s (cost %.1f)" % [goal_name, plan.describe(), plan.get_total_cost()]
	return plan.get_message()


# ---------------------------------------------------------------- UI

func _build_ui() -> void:
	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	for side in ["left", "right", "top", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 12)
	add_child(margin)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 10)
	margin.add_child(root)

	var top := HBoxContainer.new()
	root.add_child(top)
	var title := Label.new()
	title.text = "GOAP trader test"
	title.add_theme_font_size_override("font_size", 20)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	top.add_child(title)
	var pause_btn := Button.new()
	pause_btn.text = "Pause traders"
	pause_btn.pressed.connect(_on_pause_pressed.bind(pause_btn))
	top.add_child(pause_btn)
	for speed in [1.0, 5.0, 20.0]:
		var speed_btn := Button.new()
		speed_btn.text = "Speed %dx" % int(speed)
		speed_btn.pressed.connect(func(): Engine.time_scale = speed)
		top.add_child(speed_btn)
	var checks_btn := Button.new()
	checks_btn.text = "Run checks"
	checks_btn.pressed.connect(_run_checks)
	top.add_child(checks_btn)

	var columns := HBoxContainer.new()
	columns.add_theme_constant_override("separation", 10)
	columns.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(columns)
	for trader in _traders:
		columns.add_child(_build_trader_panel(trader))

	_log = RichTextLabel.new()
	_log.custom_minimum_size = Vector2(0, 220)
	_log.scroll_following = true
	_log.bbcode_enabled = true
	root.add_child(_log)


func _on_pause_pressed(button: Button) -> void:
	_paused = not _paused
	button.text = "Resume traders" if _paused else "Pause traders"
	for trader in _traders:
		trader.auto_run = not _paused


func _build_trader_panel(trader: GoapTrader) -> Control:
	var panel := PanelContainer.new()
	panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	var box := VBoxContainer.new()
	panel.add_child(box)

	var name_label := Label.new()
	name_label.text = trader.name
	name_label.add_theme_font_size_override("font_size", 18)
	box.add_child(name_label)

	var view := {"bars": {}}
	var needs := trader.get_needs()

	view.goal = _wrapping_label()
	box.add_child(view.goal)
	view.plan = _wrapping_label()
	view.plan.add_theme_color_override("font_color", Color(0.75, 0.9, 1.0))
	box.add_child(view.plan)
	view.step = _wrapping_label()
	view.step.add_theme_color_override("font_color", Color(1.0, 0.9, 0.6))
	box.add_child(view.step)
	view.offer = _wrapping_label()
	view.offer.add_theme_font_size_override("font_size", 12)
	view.offer.add_theme_color_override("font_color", Color(0.75, 0.75, 0.75))
	box.add_child(view.offer)

	box.add_child(HSeparator.new())
	if needs.needs.is_empty():
		var none := Label.new()
		none.text = "No needs, so only the fallback goal."
		box.add_child(none)
	for need in needs.needs:
		var row := HBoxContainer.new()
		var lbl := Label.new()
		lbl.text = need.display_name
		lbl.custom_minimum_size = Vector2(60, 0)
		row.add_child(lbl)
		var bar := ProgressBar.new()
		bar.max_value = need.max_value
		bar.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		row.add_child(bar)
		box.add_child(row)
		view.bars[need.need_id] = bar

	box.add_child(HSeparator.new())
	view.wallet = _wrapping_label()
	box.add_child(view.wallet)
	var give_row := HBoxContainer.new()
	for item_id in ["water", "bread", "grain"]:
		var give_btn := Button.new()
		give_btn.text = "+1 %s" % item_id
		give_btn.pressed.connect(func(): trader.get_wallet().add_item(item_id, 1))
		give_row.add_child(give_btn)
	var gold_btn := Button.new()
	gold_btn.text = "+5 gold"
	gold_btn.pressed.connect(func(): trader.get_wallet().add_currency("gold", 5.0))
	give_row.add_child(gold_btn)
	box.add_child(give_row)

	box.add_child(HSeparator.new())
	var actions := Label.new()
	var lines: PackedStringArray = ["Actions:"]
	for action in trader.actions:
		lines.append("  " + _action_text(action))
	actions.text = "\n".join(lines)
	actions.add_theme_font_size_override("font_size", 12)
	box.add_child(actions)

	_views[trader] = view
	return panel


func _wrapping_label() -> Label:
	var label := Label.new()
	label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	return label


func _action_text(action: GoapAction) -> String:
	var parts: PackedStringArray = []
	if not action.requires.is_empty():
		parts.append("needs " + _amounts_text(action.requires))
	if not action.consumes.is_empty():
		parts.append("uses " + _amounts_text(action.consumes))
	if not action.produces.is_empty():
		parts.append("gives " + _amounts_text(action.produces))
	var kind := "trade" if action.kind == GoapAction.KIND_TRADE else "custom"
	return "%s [%s, cost %.1f]: %s" % [action.action_name, kind, action.cost, ", ".join(parts)]


func _amounts_text(amounts: Dictionary) -> String:
	var parts: PackedStringArray = []
	for id in amounts:
		parts.append("%s %s" % [str(amounts[id]), id])
	return " + ".join(parts)


func _process(delta: float) -> void:
	_refresh_timer -= delta
	if _refresh_timer <= 0.0:
		_refresh_timer = 0.1
		for trader in _traders:
			_refresh_view(trader)


func _refresh_view(trader: GoapTrader) -> void:
	var view: Dictionary = _views[trader]
	var needs := trader.get_needs()

	view.goal.text = "Current goal: " + _goal_text(needs.get_current_goal())
	var plan := trader.get_current_plan()
	view.plan.text = "Plan: " + _plan_text(plan)

	var step := trader.get_current_step()
	if step != null:
		var partner := trader.get_current_partner()
		view.step.text = "Doing: %s%s" % [step.action_name, " with %s" % partner.name if partner != null else ""]
	else:
		view.step.text = "Doing: nothing"
	var offer := trader.get_last_offer_reason()
	view.offer.text = "Last offer: " + (offer if offer != "" else "none yet")

	for need_id in view.bars:
		var bar: ProgressBar = view.bars[need_id]
		bar.value = needs.get_need_value(need_id)
		bar.modulate = Color(1, 0.45, 0.45) if needs.is_need_urgent(need_id) else Color.WHITE

	var wallet := trader.get_wallet()
	var parts: PackedStringArray = []
	var currencies := wallet.get_currencies()
	for id in currencies:
		parts.append("%s %.1f" % [id, currencies[id]])
	var items := wallet.get_items()
	for id in items:
		parts.append("%s %d" % [id, items[id]])
	view.wallet.text = "Wallet: " + (", ".join(parts) if not parts.is_empty() else "empty")


func _goal_text(goal: GoapGoal) -> String:
	if goal == null:
		return "none"
	var parts: PackedStringArray = []
	for id in goal.item_targets:
		parts.append("%s >= %d" % [id, goal.item_targets[id]])
	for id in goal.currency_targets:
		parts.append("%s >= %.1f" % [id, goal.currency_targets[id]])
	return "%s [%s]" % [goal.goal_name, ", ".join(parts)]


var _last_log_text := ""
var _log_repeats := 0


func _log_line(text: String) -> void:
	# Identical consecutive lines are counted instead of printed, so a loop that keeps
	# failing shows up as one line with a count rather than flooding the output.
	if text == _last_log_text:
		_log_repeats += 1
		return
	if _log_repeats > 0:
		var note := "  (previous line repeated %d more times)" % _log_repeats
		_log.append_text(note + "\n")
		print_rich(note)
	_last_log_text = text
	_log_repeats = 0
	_log.append_text(text + "\n")
	print_rich(text)


# ---------------------------------------------------------------- check helpers

var _passed := 0
var _failed := 0
var _temp_nodes: Array[Node] = []


func _check(description: String, condition: bool) -> void:
	if condition:
		_passed += 1
		_log_line("[color=green]PASS[/color] " + description)
	else:
		_failed += 1
		_log_line("[color=red]FAIL[/color] " + description)


func _make_action(action_name: String, kind: int, consumes: Dictionary = {}, produces: Dictionary = {},
		requires: Dictionary = {}, cost: float = 1.0) -> GoapAction:
	var action := GoapAction.new()
	action.action_name = action_name
	action.kind = kind
	action.consumes = consumes
	action.produces = produces
	action.requires = requires
	action.cost = cost
	return action


func _make_goal(goal_name: String, item_targets: Dictionary = {}, currency_targets: Dictionary = {}) -> GoapGoal:
	var goal := GoapGoal.new()
	goal.goal_name = goal_name
	goal.item_targets = item_targets
	goal.currency_targets = currency_targets
	return goal


# A trader for the checks: it does not run by itself and only trades inside its own market,
# so it never interferes with Ada, Bram and Cato or with other checks.
func _make_trader(trader_name: String, market: String, actions: Array = [], goal: GoapGoal = null,
		balances: Dictionary = {}) -> GoapTrader:
	var trader := GoapTrader.new()
	trader.name = trader_name
	trader.auto_run = false
	trader.market_group = market
	add_child(trader)
	_temp_nodes.append(trader)
	for action in actions:
		trader.add_action(action)
	if goal != null:
		trader.get_needs().fallback_goal = goal
	for id in balances:
		if economy.has_currency(id):
			trader.get_wallet().add_currency(id, balances[id])
		else:
			trader.get_wallet().add_item(id, balances[id])
	return trader


func _step_names(plan: GoapPlan) -> Array:
	var names := []
	for step in plan.get_steps():
		names.append(step.action_name)
	return names


# ---------------------------------------------------------------- checks

func _run_checks() -> void:
	_passed = 0
	_failed = 0
	_log_line("--- Running checks. Some of them trigger errors on purpose, so red lines in the Output panel during the checks are expected. ---")
	_check_registry_and_wallet()
	_check_data_model()
	_check_planner()
	_check_trader_integration()
	_check_execution()
	_check_offers()
	_check_demand()
	for node in _temp_nodes:
		node.queue_free()
	_temp_nodes.clear()
	_log_line("--- Checks done: %d passed, %d failed ---" % [_passed, _failed])


func _check_registry_and_wallet() -> void:
	var w1 := Wallet.new()
	var w2 := Wallet.new()
	add_child(w1)
	add_child(w2)
	_temp_nodes.append(w1)
	_temp_nodes.append(w2)

	var gold = economy.get_currency("gold")
	_check("get_currency returns the registered currency", gold != null and gold.get_id() == "gold")
	var water = economy.get_item("water")
	_check("get_item returns the registered item", water != null and water.get_id() == "water")

	var item_count := economy.get_items().size()
	w1.add_item("watr", 1)
	_check("a misspelled item id does not add an entry to the registry", economy.get_items().size() == item_count)
	_check("get_item for an unknown id returns null", economy.get_item("watr") == null)

	var registry_copy := economy.get_items()
	registry_copy.erase("water")
	_check("changing the result of get_items does not change the registry", economy.has_item("water"))

	economy.create_item("gold", "Gold item")
	_check("an item cannot be registered with the id of a currency", not economy.has_item("gold"))

	w1.add_item("tool", 1)
	w2.add_item("tool", 1)
	_check("transfer of a non stackable item to a wallet that already has one is refused",
		not w1.transfer_item_to(w2, "tool", 1))
	_check("after the refused transfer the sender still has the item", w1.get_item_balance("tool") == 1)
	_check("after the refused transfer the receiver still has exactly one", w2.get_item_balance("tool") == 1)

	w1.add_currency("gold", 5.0)
	_check("transfer of more currency than the balance is refused", not w1.transfer_currency_to(w2, "gold", 50.0))
	_check("after the refused currency transfer both balances are unchanged",
		is_equal_approx(w1.get_currency_balance("gold"), 5.0) and is_equal_approx(w2.get_currency_balance("gold"), 0.0))
	_check("a valid currency transfer moves the amount",
		w1.transfer_currency_to(w2, "gold", 2.0) and is_equal_approx(w2.get_currency_balance("gold"), 2.0))
	_check("adding a negative amount is refused", not w1.add_item("water", -3))

	var wallet_copy := w1.get_items()
	wallet_copy["tool"] = 50
	_check("changing the result of wallet.get_items does not change the balance", w1.get_item_balance("tool") == 1)


func _check_data_model() -> void:
	var ada := $Traders/Ada as GoapTrader
	var bram := $Traders/Bram as GoapTrader
	var cato := $Traders/Cato as GoapTrader
	var thirst: Need = ada.get_needs().get_need("thirst")
	var hunger: Need = ada.get_needs().get_need("hunger")

	var action := GoapAction.new()
	action.cost = -5.0
	_check("an action cost below the minimum is clamped to a positive value", action.cost > 0.0)

	var goal := GoapGoal.new()
	goal.set_item_target("water", 2)
	_check("set_item_target stores the target", goal.get_item_target("water") == 2)
	goal.set_item_target("water", 0)
	_check("setting a target to 0 removes it", goal.item_targets.is_empty())

	# A temporary NeedsComponent that is not in the tree and has no wallet.
	var nc := NeedsComponent.new()
	var need_list: Array[Need] = [thirst, hunger]
	nc.needs = need_list
	var fallback := GoapGoal.new()
	fallback.goal_name = "Fallback"
	fallback.priority = 99  # higher than every need, must still come last
	nc.fallback_goal = fallback

	_check("a new need starts at its start value", is_equal_approx(nc.get_need_value("thirst"), thirst.start_value))
	nc.set_need_value("thirst", 500.0)
	_check("a need value is clamped to max_value", is_equal_approx(nc.get_need_value("thirst"), thirst.max_value))
	nc.set_need_value("thirst", -5.0)
	_check("a need value is clamped to 0", is_equal_approx(nc.get_need_value("thirst"), 0.0))

	nc.set_need_value("thirst", 0.0)
	nc.set_need_value("hunger", 0.0)
	var goals := nc.get_active_goals()
	_check("two urgent needs and the fallback give three active goals", goals.size() == 3)
	if goals.size() == 3:
		_check("the higher priority need comes first", goals[0].goal_name == "Thirst")
		_check("the lower priority need comes second", goals[1].goal_name == "Hunger")
		_check("the fallback goal is last even with the highest priority", goals[2] == fallback)
		_check("a need goal asks for its satisfier item", goals[0].get_item_target("water") == 1)

	nc.set_need_value("thirst", 100.0)
	var after := nc.get_active_goals()
	_check("a need above its threshold produces no goal", after.size() == 2 and after[0].goal_name == "Hunger")

	var w := Wallet.new()
	add_child(w)
	_temp_nodes.append(w)
	nc.set_need_value("hunger", 10.0)
	w.add_item("bread", 1)
	_check("consuming with the satisfier in the wallet succeeds", nc.consume_for_need("hunger", w))
	_check("consuming restores the need by restore_per_item",
		is_equal_approx(nc.get_need_value("hunger"), 10.0 + hunger.restore_per_item))
	_check("consuming removes one satisfier from the wallet", w.get_item_balance("bread") == 0)
	_check("consuming without the satisfier is refused", not nc.consume_for_need("hunger", w))
	nc.set_need_value("hunger", hunger.max_value)
	w.add_item("bread", 1)
	_check("consuming when the need is full is refused", not nc.consume_for_need("hunger", w))
	_check("a refused consume keeps the item", w.get_item_balance("bread") == 1)

	nc.set_wallet(w)
	nc.set_need_value("thirst", 10.0)
	w.add_item("water", 1)
	_check("auto consume uses a satisfier when the need is at its threshold", nc.try_auto_consume() == 1)
	_check("auto consume removed the water", w.get_item_balance("water") == 0)
	_check("auto consume restored thirst", is_equal_approx(nc.get_need_value("thirst"), 10.0 + thirst.restore_per_item))
	nc.set_need_value("thirst", 90.0)
	w.add_item("water", 1)
	_check("auto consume does nothing above the threshold", nc.try_auto_consume() == 0 and w.get_item_balance("water") == 1)
	nc.auto_consume = false
	nc.set_need_value("thirst", 5.0)
	_check("auto consume does nothing when turned off", nc.try_auto_consume() == 0)
	nc.free()

	_check("a GoapTrader without children gets a Wallet automatically", cato.get_wallet() != null)
	_check("a GoapTrader without children gets a NeedsComponent automatically", cato.get_needs() != null)
	_check("a GoapTrader without children gets a GoapPlanner automatically", cato.get_planner() != null)
	_check("a GoapTrader links its wallet to its needs", ada.get_needs().get_wallet() == ada.get_wallet())
	_check("Ada and Bram share the same Thirst resource",
		ada.get_needs().get_need("thirst") == bram.get_needs().get_need("thirst"))
	var ada_thirst := ada.get_needs().get_need_value("thirst")
	var bram_thirst := bram.get_needs().get_need_value("thirst")
	ada.get_needs().set_need_value("thirst", 77.0)
	_check("changing Ada's thirst does not change Bram's",
		is_equal_approx(bram.get_needs().get_need_value("thirst"), bram_thirst))
	ada.get_needs().set_need_value("thirst", ada_thirst)


func _check_planner() -> void:
	# A planner that is not in the tree, so it does not apply stack limits.
	var planner := GoapPlanner.new()
	var TRADE := GoapAction.KIND_TRADE
	var CUSTOM := GoapAction.KIND_CUSTOM

	var farm := _make_action("Farm grain", CUSTOM, {}, {"grain": 2}, {}, 2.0)
	var sell_grain := _make_action("Sell grain", TRADE, {"grain": 1}, {"gold": 2.0})
	var buy_water := _make_action("Buy water", TRADE, {"gold": 2.0}, {"water": 1})
	var draw_water := _make_action("Draw water", CUSTOM, {}, {"water": 1}, {}, 2.0)
	var buy_grain := _make_action("Buy grain", TRADE, {"gold": 2.0}, {"grain": 1})
	var bake := _make_action("Bake bread", CUSTOM, {"grain": 2}, {"bread": 1})
	var water_goal := _make_goal("Water", {"water": 1})
	var bread_goal := _make_goal("Bread", {"bread": 1})

	var plan := planner.plan_from_state(water_goal, {"water": 1}, [buy_water])
	_check("a goal that already holds gives ALREADY_SATISFIED with no steps",
		plan.get_result() == GoapPlan.RESULT_ALREADY_SATISFIED and plan.get_step_count() == 0)

	plan = planner.plan_from_state(water_goal, {"gold": 5.0}, [buy_water])
	_check("with enough gold the plan is to buy water", _step_names(plan) == ["Buy water"])

	plan = planner.plan_from_state(water_goal, {"gold": 1.0}, [farm, sell_grain, buy_water])
	_check("without enough gold the trader farms and sells before it buys",
		_step_names(plan) == ["Farm grain", "Sell grain", "Buy water"])
	_check("the plan cost is the sum of the step costs", is_equal_approx(plan.get_total_cost(), 4.0))
	_check("get_cost_from(1) is the cost of the remaining steps", is_equal_approx(plan.get_cost_from(1), 2.0))

	plan = planner.plan_from_state(bread_goal, {"gold": 5.0}, [buy_grain, bake])
	_check("a baker buys grain twice and bakes", _step_names(plan) == ["Buy grain", "Buy grain", "Bake bread"])

	plan = planner.plan_from_state(water_goal, {"gold": 5.0}, [draw_water, buy_water])
	_check("the cheaper of two ways is chosen (buy at cost 1 over draw at cost 2)", _step_names(plan) == ["Buy water"])
	var expensive_buy := _make_action("Buy water", TRADE, {"gold": 2.0}, {"water": 1}, {}, 5.0)
	plan = planner.plan_from_state(water_goal, {"gold": 5.0}, [draw_water, expensive_buy])
	_check("when buying costs more than drawing, drawing is chosen", _step_names(plan) == ["Draw water"])
	plan = planner.plan_from_state(water_goal, {"gold": 5.0}, [draw_water, buy_water], null, {buy_water: 5.0})
	_check("extra_costs are added to an action's cost", _step_names(plan) == ["Draw water"])

	var chop := _make_action("Chop wood", CUSTOM, {}, {"wood": 1}, {"axe": 1})
	plan = planner.plan_from_state(_make_goal("Wood", {"wood": 2}), {"axe": 1}, [chop])
	_check("a required item is not used up, so the axe can chop twice", _step_names(plan) == ["Chop wood", "Chop wood"])
	plan = planner.plan_from_state(_make_goal("Wood", {"wood": 1}), {}, [chop])
	_check("without the required item there is no plan", plan.get_result() == GoapPlan.RESULT_NO_PLAN)

	plan = planner.plan_from_state(bread_goal, {"gold": 50.0}, [buy_water])
	_check("a goal nothing produces gives NO_PLAN without searching",
		plan.get_result() == GoapPlan.RESULT_NO_PLAN and plan.get_explored_states() == 0)

	planner.max_depth = 2
	plan = planner.plan_from_state(water_goal, {"gold": 1.0}, [farm, sell_grain, buy_water])
	_check("a plan longer than max_depth is not found", plan.get_result() == GoapPlan.RESULT_NO_PLAN)
	planner.max_depth = 8

	planner.max_explored_states = 3
	plan = planner.plan_from_state(water_goal, {"gold": 1.0}, [farm, sell_grain, buy_water])
	_check("the search stops at max_explored_states", plan.get_result() == GoapPlan.RESULT_SEARCH_LIMIT)
	planner.max_explored_states = 2000

	var sell_scrap := _make_action("Sell scrap", TRADE, {"scrap": 1}, {"gold": 0.35})
	plan = planner.plan_from_state(_make_goal("Savings", {}, {"gold": 1.0}), {"gold": 0.3, "scrap": 2}, [sell_scrap])
	_check("currency with decimals adds up exactly (0.3 + 0.35 + 0.35 = 1.0)", _step_names(plan) == ["Sell scrap", "Sell scrap"])

	plan = planner.plan_from_state(_make_goal("Riches", {}, {"gold": 1000.0}), {"grain": 0}, [farm, sell_grain])
	_check("an unreachable goal within the limits ends instead of searching forever",
		plan.get_result() == GoapPlan.RESULT_NO_PLAN or plan.get_result() == GoapPlan.RESULT_SEARCH_LIMIT)

	var unavailable_script := GDScript.new()
	unavailable_script.source_code = "extends GoapAction\nfunc _is_available(_trader) -> bool:\n\treturn false\n"
	unavailable_script.reload()
	var blocked_buy := _make_action("Buy water", TRADE, {"gold": 2.0}, {"water": 1})
	blocked_buy.set_script(unavailable_script)
	plan = planner.plan_from_state(water_goal, {"gold": 5.0}, [blocked_buy])
	_check("an action whose script says _is_available is false is left out", plan.get_result() == GoapPlan.RESULT_NO_PLAN)

	var costly_script := GDScript.new()
	costly_script.source_code = "extends GoapAction\nfunc _get_cost(_trader) -> float:\n\treturn 10.0\n"
	costly_script.reload()
	var costly_buy := _make_action("Buy water", TRADE, {"gold": 2.0}, {"water": 1})
	costly_buy.set_script(costly_script)
	plan = planner.plan_from_state(water_goal, {"gold": 5.0}, [draw_water, costly_buy])
	_check("a script _get_cost replaces the cost field", _step_names(plan) == ["Draw water"])

	planner.free()


func _check_trader_integration() -> void:
	var buy_water := _make_action("Buy water", GoapAction.KIND_TRADE, {"gold": 2.0}, {"water": 1})
	# Someone in the market holds water, so the trader can plan to buy it.
	_make_trader("IntegrationSeller", "market_integration", [], null, {"water": 3})
	var trader := _make_trader("Integration", "market_integration", [buy_water], _make_goal("Water", {"water": 1}), {"gold": 5.0})

	var signalled := [false]
	trader.plan_changed.connect(func(_plan): signalled[0] = true)

	var plan := trader.make_plan()
	_check("make_plan uses the wallet: with gold the trader plans to buy water", _step_names(plan) == ["Buy water"])
	_check("make_plan stores the plan as the current plan", trader.get_current_plan() == plan)
	_check("make_plan emits plan_changed", signalled[0])

	trader.get_wallet().add_item("water", 1)
	plan = trader.make_plan()
	_check("when the only goal is already met the result is ALREADY_SATISFIED",
		plan.get_result() == GoapPlan.RESULT_ALREADY_SATISFIED)

	var make_tool := _make_action("Make tool", GoapAction.KIND_CUSTOM, {}, {"tool": 1})
	plan = trader.get_planner().plan_from_state(_make_goal("Two tools", {"tool": 2}), {"tool": 1}, [make_tool])
	_check("a planner in the tree respects max stack size (a tool does not stack)",
		plan.get_result() == GoapPlan.RESULT_NO_PLAN)

	var cato := $Traders/Cato as GoapTrader
	_check("Cato has no needs, so he plans for the fallback goal",
		cato.get_current_plan() != null and cato.get_current_plan().get_goal() == cato.get_needs().fallback_goal)


func _check_execution() -> void:
	var CUSTOM := GoapAction.KIND_CUSTOM
	var TRADE := GoapAction.KIND_TRADE
	var farm := _make_action("Farm grain", CUSTOM, {}, {"grain": 2})
	var bake := _make_action("Bake bread", CUSTOM, {"grain": 2}, {"bread": 1})

	# Custom step: started, then completed by "the developer".
	var farmer := _make_trader("Farmer", "market_custom", [farm], _make_goal("Grain", {"grain": 2}))
	var started := []
	farmer.step_started.connect(func(action, partner): started.append([action, partner]))
	_check("start_plan starts a step when there is a plan", farmer.start_plan())
	_check("the trader is executing after start_plan", farmer.is_executing())
	_check("step_started names the action and no partner for a custom step",
		started.size() == 1 and started[0][0] == farm and started[0][1] == null)
	var other_plan := farmer.make_plan()
	_check("make_plan during a step does not replace the running plan",
		farmer.get_current_plan() != other_plan and farmer.get_current_step() == farm)
	farmer.complete_step()
	_check("completing a custom step applies what it produces", farmer.get_wallet().get_item_balance("grain") == 2)
	_check("the trader is idle after completing the step", not farmer.is_executing())

	var baker := _make_trader("Baker", "market_custom", [bake], _make_goal("Bread", {"bread": 1}), {"grain": 2})
	baker.start_plan()
	baker.complete_step()
	_check("completing a crafting step removes what it consumes and adds what it produces",
		baker.get_wallet().get_item_balance("grain") == 0 and baker.get_wallet().get_item_balance("bread") == 1)

	var baker2 := _make_trader("Baker2", "market_custom", [bake], _make_goal("Bread", {"bread": 1}), {"grain": 2})
	var failures := []
	baker2.step_failed.connect(func(action, reason): failures.append([action, reason]))
	baker2.start_plan()
	baker2.get_wallet().remove_item("grain", 2)  # the grain disappears while the step runs
	baker2.complete_step()
	_check("a step whose ingredients are gone at completion fails", failures.size() == 1 and failures[0][0] == bake)
	_check("a failed crafting step changes nothing", baker2.get_wallet().get_item_balance("bread") == 0)
	_check("a failed step makes its action more expensive for a while", baker2.get_action_penalty(bake) > 0.0)

	var instant := _make_trader("Instant", "market_custom", [farm], _make_goal("Grain", {"grain": 2}))
	instant.instant_steps = true
	instant.start_plan()
	_check("with instant_steps the step completes without developer code",
		instant.get_wallet().get_item_balance("grain") == 2 and not instant.is_executing())

	# A trade nobody can serve: the trader plans around it and posts a demand.
	var buy_water := _make_action("Buy water", TRADE, {"gold": 2.0}, {"water": 1})
	var draw_water := _make_action("Draw water", CUSTOM, {}, {"water": 1}, {}, 2.0)
	var lonely := _make_trader("Lonely", "market_lonely", [buy_water, draw_water], _make_goal("Water", {"water": 1}), {"gold": 5.0})
	_check("when nobody has water, the trader plans around the trade and draws water itself",
		_step_names(lonely.make_plan()) == ["Draw water"])
	_check("a trade nobody can serve posts a demand", lonely.get_active_demands().has("water"))
	_make_trader("LonelySeller", "market_lonely", [], null, {"water": 3})
	_check("once someone has water, buying is chosen again because it is cheaper",
		_step_names(lonely.make_plan()) == ["Buy water"])


func _check_offers() -> void:
	var TRADE := GoapAction.KIND_TRADE
	var buy_water := _make_action("Buy water", TRADE, {"gold": 2.0}, {"water": 1})
	var water_goal := _make_goal("Water", {"water": 1})

	# A successful trade.
	var seller := _make_trader("Seller", "market_a", [], null, {"water": 3})
	var buyer := _make_trader("Buyer", "market_a", [buy_water], water_goal, {"gold": 5.0})
	var traded := []
	buyer.trade_completed.connect(func(partner, gave, received): traded.append(partner))
	buyer.start_plan()
	_check("a trade step picks the trader that has the goods", buyer.get_current_partner() == seller)
	buyer.complete_step()
	_check("after the trade the buyer has the water and paid for it",
		buyer.get_wallet().get_item_balance("water") == 1 and is_equal_approx(buyer.get_wallet().get_currency_balance("gold"), 3.0))
	_check("after the trade the seller has the gold and gave the water",
		seller.get_wallet().get_item_balance("water") == 2 and is_equal_approx(seller.get_wallet().get_currency_balance("gold"), 2.0))
	_check("trade_completed names the partner", traded.size() == 1 and traded[0] == seller)
	_check("the seller records why it accepted", seller.get_last_offer_reason().begins_with("Accepted"))

	# A seller refuses a trade that works against its own goal.
	var keeper := _make_trader("Keeper", "market_b", [], _make_goal("Keep water", {"water": 3}), {"water": 3})
	var buyer_b := _make_trader("BuyerB", "market_b", [buy_water], water_goal, {"gold": 5.0})
	var evaluation := keeper.evaluate_offer({"gold": 2.0}, {"water": 1})
	_check("a trader refuses an offer that makes its own goal unreachable", not evaluation.accepted)
	buyer_b.start_plan()
	_check("the buyer's step fails when nobody accepts", not buyer_b.is_executing() and buyer_b.get_wallet().get_item_balance("water") == 0)
	_check("evaluating an offer changes nothing", keeper.get_wallet().get_item_balance("water") == 3)

	# The tie rule: the plan does not change, so the value decides.
	var stock := _make_trader("Stock", "market_c", [], null, {"water": 5})
	_check("an idle trader accepts a fair offer (2 gold for water worth 2)",
		stock.evaluate_offer({"gold": 2.0}, {"water": 1}).accepted)
	_check("an idle trader refuses an offer worth less than what it gives (1 gold for water worth 2)",
		not stock.evaluate_offer({"gold": 1.0}, {"water": 1}).accepted)

	# The plan beats the value: an unfair offer is accepted if it makes the goal reachable.
	var saver := _make_trader("Saver", "market_d", [], _make_goal("Savings", {}, {"gold": 4.0}), {"gold": 2.0, "bread": 1})
	_check("a trader accepts a low price if it reaches its goal with it (2 gold for bread worth 4)",
		saver.evaluate_offer({"gold": 2.0}, {"bread": 1}).accepted)

	# work_value: a producer sells what it still needs if the payment covers replacing it.
	# The carrier holds exactly its reserve of 1 water, and drawing more costs 2 work.
	var draw := _make_action("Draw water", GoapAction.KIND_CUSTOM, {}, {"water": 2}, {}, 2.0)
	var carrier := _make_trader("Carrier", "market_i", [draw], _make_goal("Reserve", {"water": 1}), {"water": 1})
	_check("a producer sells its last item if the payment covers the work to replace it (2 gold for 2 work)",
		carrier.evaluate_offer({"gold": 2.0}, {"water": 1}).accepted)
	_check("a producer refuses if the payment does not cover the work (1 gold for 2 work)",
		not carrier.evaluate_offer({"gold": 1.0}, {"water": 1}).accepted)
	carrier.work_value = 0.0
	_check("with work_value 0 a producer only sells surplus",
		not carrier.evaluate_offer({"gold": 2.0}, {"water": 1}).accepted)

	# The goods are gone by the time the buyer arrives.
	var last_one := _make_trader("LastOne", "market_e", [], null, {"water": 1})
	var late_buyer := _make_trader("LateBuyer", "market_e", [buy_water], water_goal, {"gold": 5.0})
	late_buyer.start_plan()
	_check("the buyer heads to the only trader with water", late_buyer.get_current_partner() == last_one)
	last_one.get_wallet().remove_item("water", 1)  # someone else bought it first
	late_buyer.complete_step()
	_check("when the goods are gone on arrival the step fails and no gold is paid",
		not late_buyer.is_executing() and is_equal_approx(late_buyer.get_wallet().get_currency_balance("gold"), 5.0))

	# Capacity: a trader cannot receive more of a non stackable item.
	var tool_owner := _make_trader("ToolOwner", "market_f", [], null, {"tool": 1})
	_check("an offer of an item the trader has no room for is refused",
		not tool_owner.evaluate_offer({"tool": 1}, {}).accepted)

	# A plain wallet, for example the player's, can trade with a GoapTrader.
	var shop := _make_trader("Shop", "market_g", [], null, {"water": 3})
	var player_wallet := Wallet.new()
	add_child(player_wallet)
	_temp_nodes.append(player_wallet)
	player_wallet.add_currency("gold", 5.0)
	var result := shop.receive_offer(player_wallet, {"gold": 2.0}, {"water": 1})
	_check("a plain wallet can buy from a GoapTrader with receive_offer",
		result.accepted and player_wallet.get_item_balance("water") == 1 and shop.get_wallet().get_item_balance("water") == 2)

	# _rank_partner lets a script choose between partners, for example by distance.
	var near := _make_trader("Near", "market_h", [], null, {"water": 3})
	var far := _make_trader("Far", "market_h", [], null, {"water": 3})
	var picky := _make_trader("Picky", "market_h", [buy_water], water_goal, {"gold": 5.0})
	var rank_script := GDScript.new()
	rank_script.source_code = "extends GoapTrader\nvar preferred: Node\nfunc _rank_partner(partner, _action) -> float:\n\treturn 0.0 if partner == preferred else 1.0\n"
	rank_script.reload()
	picky.set_script(rank_script)
	picky.set("preferred", near)
	picky.start_plan()
	_check("_rank_partner decides which partner is tried first", picky.get_current_partner() == near)
	picky.fail_step("cancelled by the check")
	picky.set("preferred", far)
	picky.start_plan()
	_check("changing the ranking changes the partner", picky.get_current_partner() == far)


func _check_demand() -> void:
	var TRADE := GoapAction.KIND_TRADE
	var CUSTOM := GoapAction.KIND_CUSTOM
	var buy_bread := _make_action("Buy bread", TRADE, {"gold": 4.0}, {"bread": 1})
	var bake := _make_action("Bake bread", CUSTOM, {"grain": 2}, {"bread": 2})

	# A customer who can only buy bread, in a market where nobody has any.
	var customer := _make_trader("Customer", "market_demand_a", [buy_bread], _make_goal("Bread", {"bread": 1}), {"gold": 10.0})
	var waiting := customer.make_plan()
	_check("a goal that only a trade nobody can serve reaches is waiting, not started",
		waiting.get_result() == GoapPlan.RESULT_NO_PLAN and waiting.get_message().begins_with("Waiting"))
	_check("the waiting trader has posted a demand for the missing goods", customer.get_active_demands().has("bread"))

	# A baker who can make bread answers the demand.
	var baker := _make_trader("DemandBaker", "market_demand_a", [bake], null, {"grain": 2})
	var baker_plan := baker.make_plan()
	_check("a producer gets a goal from another trader's demand",
		baker_plan.get_goal() != null and baker_plan.get_goal().goal_name == "Demand: bread")
	_check("the producer plans to make what was demanded", _step_names(baker_plan) == ["Bake bread"])

	# Someone who could only resell bread does not answer the demand.
	var reseller := _make_trader("Reseller", "market_demand_a", [buy_bread], null, {"gold": 10.0})
	var reseller_plan := reseller.make_plan()
	_check("a trader without a custom action that makes the goods ignores the demand",
		reseller_plan.get_goal() == null or not reseller_plan.get_goal().goal_name.begins_with("Demand"))

	# The baker bakes, the customer buys, and the demand is gone.
	baker.start_plan()
	baker.complete_step()
	_check("once the producer holds the goods, the customer plans to buy them", _step_names(customer.make_plan()) == ["Buy bread"])
	customer.start_plan()
	customer.complete_step()
	_check("the customer has the bread", customer.get_wallet().get_item_balance("bread") == 1)
	_check("getting the goods clears the demand", not customer.get_active_demands().has("bread"))

	# A demand passes down the chain: no bread means the baker needs grain, so the farmer farms.
	var buy_grain := _make_action("Buy grain", TRADE, {"gold": 2.0}, {"grain": 1})
	var farm := _make_action("Farm grain", CUSTOM, {}, {"grain": 3}, {}, 2.0)
	var hungry := _make_trader("Hungry", "market_demand_b", [buy_bread], _make_goal("Bread", {"bread": 1}), {"gold": 10.0})
	var chain_baker := _make_trader("ChainBaker", "market_demand_b", [buy_grain, bake], null, {"gold": 10.0})
	var chain_farmer := _make_trader("ChainFarmer", "market_demand_b", [farm], null)
	hungry.make_plan()
	var chain_baker_plan := chain_baker.make_plan()
	_check("the baker cannot bake without grain, so it waits and posts a demand for grain",
		chain_baker_plan.get_message().begins_with("Waiting") and chain_baker.get_active_demands().has("grain"))
	var farmer_plan := chain_farmer.make_plan()
	_check("the farmer answers the grain demand by farming",
		farmer_plan.get_goal() != null and farmer_plan.get_goal().goal_name == "Demand: grain" and _step_names(farmer_plan) == ["Farm grain"])

	# Demands expire.
	var forgetful := _make_trader("Forgetful", "market_demand_c", [], null)
	forgetful.demand_duration = 0.0
	forgetful.post_demand("bread", 1.0)
	_check("a demand with no duration has expired at once", forgetful.get_active_demands().is_empty())
