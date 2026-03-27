# TradeLineLayer.gd
# Attach to a Node2D named "TradeLineLayer" that sits above the world in the scene tree
# No children needed — lines are drawn purely in code

extends Node2D

# Each entry: { from, to, label, timer, duration }
var _lines: Array = []

const DURATION   = 1.2   # seconds the line stays visible
const LINE_COLOR = Color(1.0, 0.9, 0.3, 0.9)
const TEXT_COLOR = Color(1.0, 1.0, 1.0, 1.0)
const LINE_WIDTH = 2.0

func show_trade_line(from: Vector2, to: Vector2, item_id: String, amount: int):
	_lines.append({
		"from":     from,
		"to":       to,
		"label":    "%s x%d" % [item_id, amount],
		"timer":    0.0,
		"duration": DURATION
	})
	queue_redraw()

func _process(delta):
	if _lines.is_empty(): return
	var any_alive = false
	for line in _lines:
		line["timer"] += delta
		if line["timer"] < line["duration"]:
			any_alive = true
	if not any_alive:
		_lines.clear()
	queue_redraw()

func _draw():
	for line in _lines:
		var progress = line["timer"] / line["duration"]
		var alpha    = 1.0 - progress  # fade out over time

		var lc = Color(LINE_COLOR.r, LINE_COLOR.g, LINE_COLOR.b, LINE_COLOR.a * alpha)
		draw_line(line["from"], line["to"], lc, LINE_WIDTH)

		# Small circle at each end
		draw_circle(line["from"], 4.0, lc)
		draw_circle(line["to"],   4.0, lc)

		# Floating label at the midpoint, drifting slightly upward
		var mid = (line["from"] + line["to"]) * 0.5
		mid.y  -= progress * 20.0  # drift up as it fades

		var tc = Color(TEXT_COLOR.r, TEXT_COLOR.g, TEXT_COLOR.b, TEXT_COLOR.a * alpha)
		draw_string(
			ThemeDB.fallback_font,
			mid,
			line["label"],
			HORIZONTAL_ALIGNMENT_CENTER,
			-1,
			14,
			tc
		)
