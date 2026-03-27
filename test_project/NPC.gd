# NPC.gd
# Attach to a Node2D named after the trader e.g. "Trader_A"
# Children needed:
#   - Trader (your Trader node)
#   - Wallet  (your Wallet node, child of Trader)
#   - Label   (shows NPC name, centered below the circle)
#   - InteractArea (Area2D with a small CollisionShape2D)
#
# The circle is drawn via _draw(), no sprite needed.
#
# In the Inspector on the Trader node, set:
#   - profile or instance_rules (TradeRule resources)
#   - accepted_currencies: ["gold"]
#   - accepted_items: e.g. ["wheat","iron"]
#   - market: path to the TradeMarket node

extends Node2D

@export var color: Color = Color(0.3, 0.6, 1.0)
@export var trader_label: String = "Trader"

# How often (seconds) the trader evaluates its rules automatically
@export var tick_rate: float = 2.5

@export var starting_gold = 0
@export var starting_wheat = 0
@export var starting_iron = 0
@export var starting_gems = 0

var _tick_timer: float = 0.0
var _is_player_nearby: bool = false

func _ready():
	$Label.text = trader_label
	$Trader/Wallet.add_currency("gold", starting_gold)
	$Trader/Wallet.add_item("wheat", starting_wheat)
	$Trader/Wallet.add_item("iron", starting_iron)
	$Trader/Wallet.add_item("gems", starting_gems)
	$Trader.get_wallet().balance_changed.connect(_on_balance_changed)
	queue_redraw()

func _draw():
	draw_circle(Vector2.ZERO, 24, color)
	draw_arc(Vector2.ZERO, 24, 0, TAU, 32, Color(1,1,1,0.3), 2.0)

func _process(delta):
	_tick_timer += delta
	if _tick_timer >= tick_rate:
		_tick_timer = 0.0
		$Trader.evaluate_trade_rules(tick_rate)

func _on_balance_changed(_id, _amount):
	# Tell the UI to refresh this trader's wallet display
	# We use a group call so NPC does not need a direct reference to UI
	get_tree().call_group("ui", "refresh_wallet_display")

func get_trader() -> Trader:
	return $Trader

# Called by Player when nearby and pressing E
func open_shop():
	get_tree().call_group("ui", "open_shop", $Trader)

func _on_interact_area_body_entered(body):
	if body.is_in_group("player"):
		_is_player_nearby = true
		get_tree().call_group("ui", "show_interact_hint", true)

func _on_interact_area_body_exited(body):
	if body.is_in_group("player"):
		_is_player_nearby = false
		get_tree().call_group("ui", "show_interact_hint", false)
		get_tree().call_group("ui", "close_shop")
