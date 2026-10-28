extends Node

# Showcase scenarios on top of goap_demo, for recording short clips of one behaviour each.
#
# The normal demo shows everything eventually, but at random moments. Each button here
# puts the NPCs in a known starting situation, so the behaviour shows up within seconds.
# A scenario only sets wallets and need values. Every decision after that is still made by
# the NPCs' own GoapTraders; nothing about the trading is scripted.
#
#   1  Sell first     the Farmer is hungry, has no gold and 3 grain; the Baker has bread
#   2  Offers         a Player wallet makes offers to the Water Carrier and the Baker
#   3  Demand chain   nobody has bread or grain, and the Farmer becomes hungry
#
# Keys 1, 2 and 3 do the same as the buttons. A step that is running when a scenario starts
# is ended with fail_step, so an orange "failed, scenario reset" line in the log is expected.

const FULL := 100.0
const HUNGRY := 25.0  # below the urgent threshold of 30

@onready var demo: Node2D = $"../GoapDemo"

var farmer: Node
var baker: Node
var carrier: Node
var player: Wallet

var _fallbacks := {}  # npc -> GoapGoal, so scenario 3 can switch them off and the others back on
var _info: Label
var _player_label: Label
var _offer_box: VBoxContainer


func _ready() -> void:
	farmer = demo.get_node("NPCs/Farmer")
	baker = demo.get_node("NPCs/Baker")
	carrier = demo.get_node("NPCs/WaterCarrier")
	for npc in _npcs():
		_fallbacks[npc] = npc.trader.get_needs().fallback_goal

	# A plain wallet, not a trader: receive_offer works with any wallet, so this stands in
	# for a player trading with the NPCs.
	player = Wallet.new()
	player.name = "Player"
	add_child(player)
	player.balance_changed.connect(func(_id, _amount): _update_player_label())

	for npc in _npcs():
		npc.trader.offer_received.connect(_on_offer_received.bind(npc))

	_build_ui()
	_set_info("Pick a scenario above (or press 1, 2 or 3).")


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		match event.keycode:
			KEY_1: _scenario_sell_first()
			KEY_2: _scenario_offers()
			KEY_3: _scenario_demand_chain()


# ---------------------------------------------------------------- scenarios

func _scenario_sell_first() -> void:
	_reset({
		"Farmer": {"gold": 0.0, "grain": 3},
		"Baker": {"gold": 10.0, "bread": 2},
		"WaterCarrier": {"gold": 10.0, "water": 2},
	}, true)
	farmer.trader.get_needs().set_need_value("hunger", HUNGRY)
	_offer_box.visible = false
	_set_info("Sell first: the Farmer is hungry but has 0 gold. Bread costs 6, so it has to sell grain to the Baker before it can buy bread.")
	demo._log("[color=cyan]Scenario: the Farmer is hungry with 0 gold and 3 grain.[/color]")


func _scenario_offers() -> void:
	_reset({
		"Farmer": {"gold": 10.0, "grain": 2},
		"Baker": {"gold": 10.0, "bread": 1},
		"WaterCarrier": {"gold": 10.0, "water": 3},
	}, true)
	_empty(player)
	_fill(player, {"gold": 30.0, "grain": 3})
	_offer_box.visible = true
	_set_info("Offers: use the buttons above to make offers from a Player wallet. The reason for each answer appears above the NPC and in the log.")
	demo._log("[color=cyan]Scenario: the Player makes offers. The Water Carrier holds 3 water, 1 more than its reserve of 2.[/color]")


func _scenario_demand_chain() -> void:
	# Fallback goals off, so the Farmer farms because of the Baker's demand and not because
	# its own reserves ask for grain.
	_reset({
		"Farmer": {"gold": 10.0},
		"Baker": {"gold": 10.0},
		"WaterCarrier": {"gold": 10.0},
	}, false)
	farmer.trader.get_needs().set_need_value("hunger", HUNGRY)
	_offer_box.visible = false
	_set_info("Demand chain: nobody has bread or grain. The hungry Farmer asks for bread, the Baker asks for grain, and the Farmer farms it.")
	demo._log("[color=cyan]Scenario: nobody has bread or grain, the Farmer is hungry. Reserves are off.[/color]")


# ---------------------------------------------------------------- offers from the player

func _offer(npc: Node, player_gives: Dictionary, player_wants: Dictionary) -> void:
	# receive_offer is seen from the trader: they_give is what the trader receives.
	npc.trader.receive_offer(player, player_gives, player_wants)


func _on_offer_received(from_wallet: Wallet, they_give: Dictionary, they_want: Dictionary,
		accepted: bool, reason: String, npc: Node) -> void:
	if from_wallet != player:
		return
	# goap_demo already logs refusals, so only accepted player offers are logged here.
	if accepted:
		demo._log("[color=lightgreen]%s accepts the Player's offer (%s for %s): %s[/color]" % [
			npc.display_name, demo._amounts_text(they_give), demo._amounts_text(they_want), reason])


# ---------------------------------------------------------------- helpers

func _npcs() -> Array:
	return [farmer, baker, carrier]


func _reset(balances: Dictionary, use_fallback: bool) -> void:
	for npc in _npcs():
		var trader: GoapTrader = npc.trader
		if trader.is_executing():
			trader.fail_step("scenario reset")
		for id in trader.get_active_demands():
			trader.clear_demand(id)
		_empty(trader.get_wallet())
		_fill(trader.get_wallet(), balances.get(String(npc.name), {}))
		var needs: NeedsComponent = trader.get_needs()
		for need in needs.needs:
			needs.set_need_value(need.need_id, FULL)
		needs.fallback_goal = _fallbacks[npc] if use_fallback else null


func _empty(wallet: Wallet) -> void:
	for id in wallet.get_currencies():
		var bal: float = wallet.get_currency_balance(id)
		if bal > 0.0:
			wallet.remove_currency(id, bal)
	for id in wallet.get_items():
		var count: int = wallet.get_item_balance(id)
		if count > 0:
			wallet.remove_item(id, count)


func _fill(wallet: Wallet, balances: Dictionary) -> void:
	for id in balances:
		# The wallet rejects amounts of 0, so an empty balance is simply not added.
		if float(balances[id]) <= 0.0:
			continue
		if demo.economy.has_currency(id):
			wallet.add_currency(id, balances[id])
		else:
			wallet.add_item(id, balances[id])


# ---------------------------------------------------------------- UI

func _build_ui() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)

	var panel := PanelContainer.new()
	panel.anchor_left = 1.0
	panel.anchor_right = 1.0
	panel.offset_left = -270.0
	panel.offset_right = -10.0
	panel.offset_top = 10.0
	panel.grow_horizontal = Control.GROW_DIRECTION_BEGIN
	layer.add_child(panel)

	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 6)
	panel.add_child(box)

	var title := Label.new()
	title.text = "Showcase scenarios"
	title.add_theme_font_size_override("font_size", 14)
	box.add_child(title)

	box.add_child(_button("1  Sell first to afford", _scenario_sell_first))
	box.add_child(_button("2  Offers from the player", _scenario_offers))
	box.add_child(_button("3  Demand chain", _scenario_demand_chain))

	_offer_box = VBoxContainer.new()
	_offer_box.visible = false
	box.add_child(_offer_box)
	_offer_box.add_child(HSeparator.new())
	_player_label = Label.new()
	_player_label.add_theme_font_size_override("font_size", 11)
	_offer_box.add_child(_player_label)
	_offer_box.add_child(_button("Water Carrier: 1 gold for 1 water",
			_offer.bind(carrier, {"gold": 1.0}, {"water": 1})))
	_offer_box.add_child(_button("Water Carrier: 2.5 gold for 1 water",
			_offer.bind(carrier, {"gold": 2.5}, {"water": 1})))
	_offer_box.add_child(_button("Baker: sell 1 grain for 3 gold",
			_offer.bind(baker, {"grain": 1}, {"gold": 3.0})))
	_update_player_label()

	box.add_child(HSeparator.new())
	_info = Label.new()
	_info.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_info.custom_minimum_size = Vector2(250, 0)
	_info.add_theme_font_size_override("font_size", 11)
	box.add_child(_info)


func _button(text: String, callback: Callable) -> Button:
	var button := Button.new()
	button.text = text
	button.alignment = HORIZONTAL_ALIGNMENT_LEFT
	button.pressed.connect(callback)
	return button


func _set_info(text: String) -> void:
	_info.text = text


func _update_player_label() -> void:
	if _player_label == null:
		return
	var parts: PackedStringArray = []
	var currencies := player.get_currencies()
	for id in currencies:
		parts.append("%s %.1f" % [id, currencies[id]])
	var items := player.get_items()
	for id in items:
		if items[id] > 0:
			parts.append("%s %d" % [id, items[id]])
	_player_label.text = "Player wallet: " + (", ".join(parts) if not parts.is_empty() else "empty")
