extends Node2D

# One walking NPC in the GOAP demo.
#
# This script is the "developer's side" of the extension. The GoapTrader child decides what
# to do and emits step_started. This script decides how that looks: it walks to the trade
# partner, or to its work location for custom actions, works there for a moment, and then
# reports back with complete_step(). If the step cannot be finished it calls fail_step().
# The extension applies the effects to the wallets and plans the next step.

signal log_message(text: String)

@export var display_name := "Trader"
@export var color := Color(0.4, 0.7, 1.0)
# Where this NPC carries out its custom actions (farming, baking, drawing water).
@export var work_location: NodePath
@export var speed := 140.0
@export var work_time := 1.2
@export var trade_time := 0.4
# A step that takes longer than this is given up, for example when a partner keeps walking away.
@export var step_timeout := 20.0

const ARRIVE_DISTANCE := 34.0
const LABEL_WIDTH := 220.0
const RADIUS := 16.0

enum State { IDLE, MOVING, WORKING }

@onready var trader: GoapTrader = $GoapTrader

var _state := State.IDLE
var _action: GoapAction
var _partner: GoapTrader
var _work_left := 0.0
var _work_total := 1.0
var _step_time := 0.0
var _home := Vector2.ZERO
var _label: Label
var _label_timer := 0.0


func _ready() -> void:
	_home = position
	# The extension names traders by their node name in its messages, so the trader node
	# carries the NPC's name instead of "GoapTrader".
	trader.name = display_name

	_label = Label.new()
	_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_label.custom_minimum_size = Vector2(LABEL_WIDTH, 0)
	_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_label.add_theme_font_size_override("font_size", 11)
	_label.add_theme_color_override("font_outline_color", Color.BLACK)
	_label.add_theme_constant_override("outline_size", 4)
	add_child(_label)

	trader.step_started.connect(_on_step_started)
	trader.step_completed.connect(_on_step_completed)
	trader.step_failed.connect(_on_step_failed)


# ---------------------------------------------------------------- reacting to the trader

func _on_step_started(action: GoapAction, partner: GoapTrader) -> void:
	_action = action
	_partner = partner
	_state = State.MOVING
	_step_time = 0.0
	if partner != null:
		log_message.emit("%s walks to %s: %s" % [display_name, name_of(partner), action.action_name])
	else:
		log_message.emit("%s goes to work: %s" % [display_name, action.action_name])


func _on_step_completed(_action_done: GoapAction) -> void:
	_state = State.IDLE


func _on_step_failed(action: GoapAction, reason: String) -> void:
	_state = State.IDLE
	var action_name: String = action.action_name if action != null else "step"
	log_message.emit("[color=orange]%s: %s failed, %s[/color]" % [display_name, action_name, reason])


# ---------------------------------------------------------------- carrying steps out

func _process(delta: float) -> void:
	match _state:
		State.MOVING:
			_step_time += delta
			var target = _target_position()
			if target == null:
				_give_up("the partner is gone")
			elif _step_time > step_timeout:
				_give_up("it took too long")
			elif _move_towards(target, delta, ARRIVE_DISTANCE):
				_state = State.WORKING
				_work_total = trade_time if _action.kind == GoapAction.KIND_TRADE else work_time
				_work_left = _work_total
		State.WORKING:
			_work_left -= delta
			if _work_left <= 0.0:
				_finish_step()
		State.IDLE:
			_move_towards(_home, delta * 0.5, 2.0)

	queue_redraw()
	_label_timer -= delta
	if _label_timer <= 0.0:
		_label_timer = 0.15
		_update_label()
	_label.position = Vector2(-LABEL_WIDTH / 2.0, -RADIUS - 8.0 - _label.size.y)


# Where the current step happens: the partner's NPC for a trade, the work location otherwise.
# Returns null when the target no longer exists.
func _target_position():
	if _action == null:
		return null
	if _action.kind == GoapAction.KIND_TRADE:
		if not is_instance_valid(_partner):
			return null
		var partner_npc := _partner.get_parent() as Node2D
		return partner_npc.global_position if partner_npc != null else null
	var location := get_node_or_null(work_location) as Node2D
	return location.global_position if location != null else global_position


# Moves towards target and returns true once within arrive_distance.
func _move_towards(target: Vector2, delta: float, arrive_distance: float) -> bool:
	var to_target := target - global_position
	if to_target.length() <= arrive_distance:
		return true
	global_position += to_target.normalized() * minf(speed * delta, to_target.length())
	return false


func _finish_step() -> void:
	_state = State.IDLE
	# The step may have ended in the meantime, for example by a timeout.
	if trader.is_executing() and trader.get_current_step() == _action:
		trader.complete_step()


func _give_up(reason: String) -> void:
	_state = State.IDLE
	if trader.is_executing():
		trader.fail_step(reason)


# ---------------------------------------------------------------- drawing

func _draw() -> void:
	draw_circle(Vector2.ZERO, RADIUS, color)
	draw_arc(Vector2.ZERO, RADIUS, 0.0, TAU, 32, Color(1, 1, 1, 0.5), 2.0)
	if _state == State.WORKING and _work_total > 0.0:
		var progress := 1.0 - _work_left / _work_total
		draw_arc(Vector2.ZERO, RADIUS + 5.0, -PI / 2.0, -PI / 2.0 + TAU * progress, 32, Color.WHITE, 3.0)


func _update_label() -> void:
	var lines: PackedStringArray = [display_name]

	if _state != State.IDLE and _action != null:
		var partner_text := " with %s" % name_of(_partner) if _partner != null else ""
		lines.append("> %s%s" % [_action.action_name, partner_text])
	else:
		var plan := trader.get_current_plan()
		if plan != null and plan.is_found():
			lines.append("plan: " + plan.describe())
		elif plan != null and plan.get_message().begins_with("Waiting"):
			lines.append(plan.get_message())
		else:
			lines.append("idle")

	var needs_parts: PackedStringArray = []
	for need in trader.get_needs().needs:
		needs_parts.append("%s %d" % [need.display_name, roundi(trader.get_needs().get_need_value(need.need_id))])
	if not needs_parts.is_empty():
		lines.append("  ".join(needs_parts))

	var wallet_parts: PackedStringArray = []
	var currencies := trader.get_wallet().get_currencies()
	for id in currencies:
		wallet_parts.append("%s %.1f" % [id, currencies[id]])
	var items := trader.get_wallet().get_items()
	for id in items:
		if items[id] > 0:
			wallet_parts.append("%s %d" % [id, items[id]])
	lines.append(", ".join(wallet_parts))

	var demands := trader.get_active_demands()
	if not demands.is_empty():
		var wanted: PackedStringArray = []
		for id in demands:
			wanted.append(str(id))
		lines.append("wants: " + ", ".join(wanted))

	var offer := trader.get_last_offer_reason()
	if offer != "":
		lines.append(offer.left(80) + ("..." if offer.length() > 80 else ""))

	_label.text = "\n".join(lines)
	_label.reset_size()


static func name_of(some_trader: GoapTrader) -> String:
	if some_trader == null or not is_instance_valid(some_trader):
		return "?"
	var npc := some_trader.get_parent()
	if npc != null and "display_name" in npc:
		return npc.display_name
	return String(some_trader.name)
