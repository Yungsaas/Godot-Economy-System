extends Node

# Stress test for the GOAP economy (ILO 3).
#
# Spawns N traders in the same three roles as the 2D demo (farmer, baker, water carrier)
# with instant_steps on, so there is no walking: every frame is pure extension work.
# Pick N with the buttons; the scene reports frame time, trades per second and how
# healthy the economy is, every few seconds in the Output panel and on screen.
#
# Run it with a Tracy build (scons tracy=yes) and the Tracy profiler connected to see
# where the time goes. The numbers here say how much; Tracy says why.
#
# "Market" switches between one market for everyone (every trader sees every other
# trader) and markets of MARKET_GROUP_SIZE traders. Partner search and demand scan the
# market, so this shows how the cost depends on market size.

const COUNTS := [9, 30, 90, 300, 900]
const MARKET_GROUP_SIZE := 30
const REPORT_INTERVAL := 5.0
const SAMPLE_INTERVAL := 0.25
const ROLE_NAMES := ["Farmer", "Baker", "Carrier"]

var _economy: EconomyManager
var _traders: Array[GoapTrader] = []
var _role_actions := []  # per role: Array[GoapAction]
var _role_goals := []  # per role: GoapGoal
var _needs: Array[Need] = []
var _split_markets := false
var _rng := RandomNumberGenerator.new()

# Measurements since the last report
var _elapsed := 0.0
var _frames := 0
var _frame_ms_sum := 0.0
var _frame_ms_max := 0.0
var _trades := 0
var _sample_timer := 0.0
var _samples := 0
var _urgent_share_sum := 0.0
var _empty_share_sum := 0.0

var _status: Label
var _market_button: Button


func _ready() -> void:
	# Uncapped frame rate, so frame time shows the work done and not the monitor's refresh rate.
	DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
	Engine.max_fps = 0

	_economy = EconomyManager.new()
	_economy.name = "EconomyManager"
	add_child(_economy)
	_economy.create_currency("gold", "Gold", 2, 1.0, "G")
	_economy.create_item("water", "Water", 2.5)
	_economy.create_item("bread", "Bread", 6.0)
	_economy.create_item("grain", "Grain", 3.0)

	_build_content()
	_build_ui()
	_spawn(COUNTS[0])


# ---------------------------------------------------------------- content (same as the 2D demo)

func _build_content() -> void:
	var thirst := Need.new()
	thirst.need_id = "thirst"
	thirst.display_name = "Thirst"
	thirst.decay_per_second = 1.5
	thirst.priority = 2
	thirst.satisfier_item_id = "water"
	thirst.restore_per_item = 50.0
	var hunger := Need.new()
	hunger.need_id = "hunger"
	hunger.display_name = "Hunger"
	hunger.decay_per_second = 1.0
	hunger.satisfier_item_id = "bread"
	hunger.restore_per_item = 40.0
	_needs = [thirst, hunger]

	var farm := _action("Farm grain", GoapAction.KIND_CUSTOM, {}, {"grain": 3}, 2.0)
	var sell_grain := _action("Sell grain", GoapAction.KIND_TRADE, {"grain": 1}, {"gold": 3.0})
	var buy_grain := _action("Buy grain", GoapAction.KIND_TRADE, {"gold": 3.0}, {"grain": 1})
	var bake := _action("Bake bread", GoapAction.KIND_CUSTOM, {"grain": 1}, {"bread": 1})
	var sell_bread := _action("Sell bread", GoapAction.KIND_TRADE, {"bread": 1}, {"gold": 6.0})
	var buy_bread := _action("Buy bread", GoapAction.KIND_TRADE, {"gold": 6.0}, {"bread": 1})
	var draw := _action("Draw water", GoapAction.KIND_CUSTOM, {}, {"water": 2}, 2.0)
	var sell_water := _action("Sell water", GoapAction.KIND_TRADE, {"water": 1}, {"gold": 2.5})
	var buy_water := _action("Buy water", GoapAction.KIND_TRADE, {"gold": 2.5}, {"water": 1})

	var farmer_actions: Array[GoapAction] = [farm, sell_grain, buy_bread, buy_water]
	var baker_actions: Array[GoapAction] = [buy_grain, bake, sell_bread, buy_water]
	var carrier_actions: Array[GoapAction] = [draw, sell_water, buy_bread]
	_role_actions = [farmer_actions, baker_actions, carrier_actions]
	_role_goals = [
		_goal("Farmer reserves", {"grain": 2}),
		_goal("Baker reserves", {"bread": 1}),
		_goal("Water carrier reserves", {"water": 2}),
	]


func _action(action_name: String, kind: int, consumes: Dictionary, produces: Dictionary, cost := 1.0) -> GoapAction:
	var action := GoapAction.new()
	action.action_name = action_name
	action.kind = kind
	action.consumes = consumes
	action.produces = produces
	action.cost = cost
	return action


func _goal(goal_name: String, items: Dictionary) -> GoapGoal:
	var goal := GoapGoal.new()
	goal.goal_name = goal_name
	goal.item_targets = items
	goal.currency_targets = {"gold": 4.0}
	return goal


# ---------------------------------------------------------------- spawning

func _spawn(count: int) -> void:
	for trader in _traders:
		trader.queue_free()
	_traders.clear()
	_rng.seed = 7  # the same starting needs for every run with the same N

	for i in count:
		var role := i % 3
		var trader := GoapTrader.new()
		trader.name = "%s%d" % [ROLE_NAMES[role], i]
		trader.instant_steps = true
		trader.market_group = "stress_market_%d" % (i / MARKET_GROUP_SIZE) if _split_markets else "stress_market"
		trader.actions = _role_actions[role]

		# Children made here so the trader does not have to create (and announce) them.
		var wallet := Wallet.new()
		wallet.name = "Wallet"
		trader.add_child(wallet)
		var needs := NeedsComponent.new()
		needs.name = "NeedsComponent"
		needs.needs = _needs
		needs.fallback_goal = _role_goals[role]
		trader.add_child(needs)
		var planner := GoapPlanner.new()
		planner.name = "GoapPlanner"
		trader.add_child(planner)

		add_child(trader)
		wallet.add_currency("gold", 10.0)
		# Different starting values, so the NPCs do not all get hungry in the same frame.
		for need in _needs:
			needs.set_need_value(need.need_id, _rng.randf_range(40.0, 100.0))
		trader.trade_completed.connect(_on_trade_completed)
		_traders.append(trader)

	_reset_measurements()
	print("--- %d traders, %s ---" % [count, _market_text()])


func _on_trade_completed(_partner: GoapTrader, _gave: Dictionary, _received: Dictionary) -> void:
	_trades += 1


# ---------------------------------------------------------------- measuring

func _process(delta: float) -> void:
	_elapsed += delta
	_frames += 1
	var frame_ms := delta * 1000.0
	_frame_ms_sum += frame_ms
	_frame_ms_max = maxf(_frame_ms_max, frame_ms)

	_sample_timer -= delta
	if _sample_timer <= 0.0:
		_sample_timer = SAMPLE_INTERVAL
		_sample_needs()

	if _elapsed >= REPORT_INTERVAL:
		_report()
		_reset_measurements()


# Share of traders with at least one urgent need, and with at least one need at 0.
func _sample_needs() -> void:
	if _traders.is_empty():
		return
	var urgent := 0
	var empty := 0
	for trader in _traders:
		var needs := trader.get_needs()
		var is_urgent := false
		var is_empty := false
		for need in _needs:
			if needs.is_need_urgent(need.need_id):
				is_urgent = true
			if needs.get_need_value(need.need_id) <= 0.0:
				is_empty = true
		urgent += int(is_urgent)
		empty += int(is_empty)
	_urgent_share_sum += float(urgent) / _traders.size()
	_empty_share_sum += float(empty) / _traders.size()
	_samples += 1


func _report() -> void:
	var avg_ms := _frame_ms_sum / maxi(_frames, 1)
	var line := "N=%d | %s | frame avg %.2f ms, max %.2f ms (%.0f fps) | trades/s %.1f | urgent %.0f%% | need at 0 %.0f%%" % [
		_traders.size(), _market_text(), avg_ms, _frame_ms_max, 1000.0 / maxf(avg_ms, 0.001),
		_trades / maxf(_elapsed, 0.001),
		100.0 * _urgent_share_sum / maxi(_samples, 1),
		100.0 * _empty_share_sum / maxi(_samples, 1)]
	print(line)
	_status.text = line


func _reset_measurements() -> void:
	_elapsed = 0.0
	_frames = 0
	_frame_ms_sum = 0.0
	_frame_ms_max = 0.0
	_trades = 0
	_samples = 0
	_urgent_share_sum = 0.0
	_empty_share_sum = 0.0


func _market_text() -> String:
	return "markets of %d" % MARKET_GROUP_SIZE if _split_markets else "one market"


# ---------------------------------------------------------------- UI

func _build_ui() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	var box := VBoxContainer.new()
	box.position = Vector2(10, 10)
	layer.add_child(box)

	var bar := HBoxContainer.new()
	box.add_child(bar)
	for count in COUNTS:
		var button := Button.new()
		button.text = "%d NPCs" % count
		button.pressed.connect(func(): _spawn(count))
		bar.add_child(button)
	_market_button = Button.new()
	_market_button.text = "Market: one for everyone"
	_market_button.pressed.connect(_on_market_pressed)
	bar.add_child(_market_button)

	_status = Label.new()
	_status.text = "Measuring... the first report appears after %d seconds." % int(REPORT_INTERVAL)
	box.add_child(_status)


func _on_market_pressed() -> void:
	_split_markets = not _split_markets
	_market_button.text = "Market: groups of %d" % MARKET_GROUP_SIZE if _split_markets else "Market: one for everyone"
	_spawn(_traders.size())
