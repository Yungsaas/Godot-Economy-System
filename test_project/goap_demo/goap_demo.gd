extends Node2D

# 2D demo of the GOAP economy: three NPCs with needs walk around and trade with each other.
#
#   Farmer:        farms grain on the field and sells it, buys bread and water
#   Baker:         buys grain, bakes bread in the bakery and sells it, buys water
#   Water Carrier: draws water at the well and sells it, buys bread
#
# None of the trades are scripted. Each NPC plans with its GoapTrader, and npc.gd walks it
# to wherever the next step happens. The label above each NPC shows what it is doing, its
# needs, its wallet and the reason for the last offer it answered.
#
# The text based test scene (goap_test) is kept separately for the automated checks.

const STARTING_BALANCES := {
	"Farmer":       {"gold": 10.0},
	"Baker":        {"gold": 10.0},
	"WaterCarrier": {"gold": 10.0},
}
const MAX_LOG_LINES := 12

@onready var economy: EconomyManager = $EconomyManager

var _log_lines: PackedStringArray = []
var _log_label: RichTextLabel
var _pause_button: Button


func _ready() -> void:
	# Children are ready before this runs, so every GoapTrader already has its children.
	# Currencies and items are authored in the inspector on the EconomyManager
	# (initial_currencies, initial_items), so nothing is registered in code here.
	for npc in $NPCs.get_children():
		_set_up_npc(npc)
	_build_ui()
	_log("Ready. Nothing below is scripted: every trade comes from an NPC's own plan.")


# Prices are balanced so no NPC slowly gains or loses gold. Per 40 seconds each NPC eats
# about 1 bread and drinks about 1.2 water; the baker bakes 3 bread from 3 grain, eats one
# and sells two. Income equals spending for everyone when bread = 2.4 x water and
# grain = 1.2 x water. The base values on the items in the EconomyManager match the prices
# in the actions, so the tie rule sees ordinary trades as fair.


func _set_up_npc(npc: Node) -> void:
	var trader: GoapTrader = npc.trader
	var balances: Dictionary = STARTING_BALANCES.get(String(npc.name), {})
	for id in balances:
		if economy.has_currency(id):
			trader.get_wallet().add_currency(id, balances[id])
		else:
			trader.get_wallet().add_item(id, balances[id])

	npc.log_message.connect(_log)
	trader.trade_completed.connect(_on_trade_completed.bind(npc))
	trader.offer_received.connect(_on_offer_received.bind(npc))
	trader.demand_posted.connect(_on_demand_posted.bind(npc))
	trader.get_needs().need_consumed.connect(_on_need_consumed.bind(npc))


# ---------------------------------------------------------------- log

func _on_trade_completed(partner: GoapTrader, gave: Dictionary, received: Dictionary, npc: Node) -> void:
	_log("[color=lightgreen]%s got %s from %s for %s[/color]" % [
		npc.display_name, _amounts_text(received), npc.name_of(partner), _amounts_text(gave)])


func _on_offer_received(_from_wallet: Wallet, _they_give: Dictionary, _they_want: Dictionary,
		accepted: bool, reason: String, npc: Node) -> void:
	# Accepted offers show up as a trade already, so only refusals are logged.
	if not accepted:
		_log("[color=salmon]%s refuses: %s[/color]" % [npc.display_name, reason])


func _on_need_consumed(need_id: String, item_id: String, npc: Node) -> void:
	_log("%s consumes 1 %s (%s)" % [npc.display_name, item_id, need_id])


func _on_demand_posted(item_id: String, amount: float, npc: Node) -> void:
	_log("[color=khaki]%s wants %s %s, but nobody has any[/color]" % [npc.display_name, str(int(amount)), item_id])


func _amounts_text(amounts: Dictionary) -> String:
	var parts: PackedStringArray = []
	for id in amounts:
		var amount = amounts[id]
		parts.append("%s %s" % [str(int(amount)) if float(amount) == floorf(float(amount)) else str(amount), id])
	return " + ".join(parts)


func _log(text: String) -> void:
	_log_lines.append(text)
	while _log_lines.size() > MAX_LOG_LINES:
		_log_lines.remove_at(0)
	if _log_label != null:
		_log_label.text = "\n".join(_log_lines)
	print_rich(text)


# ---------------------------------------------------------------- UI

func _build_ui() -> void:
	var layer := CanvasLayer.new()
	# Keeps working while the tree is paused, so the pause button can resume.
	layer.process_mode = Node.PROCESS_MODE_ALWAYS
	add_child(layer)

	var bar := HBoxContainer.new()
	bar.position = Vector2(10, 10)
	layer.add_child(bar)

	_pause_button = Button.new()
	_pause_button.text = "Pause"
	_pause_button.pressed.connect(_on_pause_pressed)
	bar.add_child(_pause_button)
	for speed in [1.0, 3.0, 10.0]:
		var speed_button := Button.new()
		speed_button.text = "%dx" % int(speed)
		speed_button.pressed.connect(func(): Engine.time_scale = speed)
		bar.add_child(speed_button)

	var panel := PanelContainer.new()
	panel.position = Vector2(10, 50)
	panel.custom_minimum_size = Vector2(440, 0)
	layer.add_child(panel)
	_log_label = RichTextLabel.new()
	_log_label.bbcode_enabled = true
	_log_label.fit_content = true
	_log_label.custom_minimum_size = Vector2(440, 0)
	_log_label.add_theme_font_size_override("normal_font_size", 12)
	panel.add_child(_log_label)
	_log_label.text = "\n".join(_log_lines)


func _on_pause_pressed() -> void:
	get_tree().paused = not get_tree().paused
	_pause_button.text = "Resume" if get_tree().paused else "Pause"
