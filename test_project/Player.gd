# Player.gd
# Attach to a CharacterBody2D named "Player"
# Add to group "player" in the Inspector
# Children needed:
#   - Trader (your Trader node, so the player can buy/sell)
#   - Wallet  (child of Trader)
#   - CollisionShape2D (small capsule or circle)
#
# In the Inspector on the Trader node:
#   - accepted_currencies: ["gold"]
#   - accepted_items: ["wheat","iron","gems"]
#   - market: path to TradeMarket
#   Leave rules empty — the player trades manually via the shop UI

extends CharacterBody2D

@export var speed: float = 150.0
@export var color: Color = Color(1.0, 0.85, 0.2)

var _nearby_npc = null  # holds the NPC node the player is standing near

func _ready():
	add_to_group("player")
	await get_tree().process_frame
	await get_tree().process_frame
	# Give the player a starting balance so they can actually buy things
	$Trader/Wallet.add_currency("gold", 100.0)
	queue_redraw()

func _draw():
	draw_circle(Vector2.ZERO, 18, color)
	draw_arc(Vector2.ZERO, 18, 0, TAU, 32, Color(1,1,1,0.5), 2.0)

func _physics_process(_delta):
	var dir = Vector2.ZERO
	if Input.is_action_pressed("player_right"): dir.x += 1
	if Input.is_action_pressed("player_left"):  dir.x -= 1
	if Input.is_action_pressed("player_down"):  dir.y += 1
	if Input.is_action_pressed("player_up"):    dir.y -= 1

	velocity = dir.normalized() * speed
	move_and_slide()

func _input(event):
	if event.is_action_pressed("ui_input") and _nearby_npc:
		_nearby_npc.open_shop()

func set_nearby_npc(npc):
	_nearby_npc = npc

func get_trader() -> Trader:
	return $Trader
