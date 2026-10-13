@tool
extends Node2D

# A place where an NPC carries out its custom actions, such as a field or a well.
# Only drawn, it has no logic. @tool makes it visible in the editor as well.

@export var label_text := "Location":
	set(value):
		label_text = value
		queue_redraw()
@export var color := Color(0.35, 0.55, 0.3):
	set(value):
		color = value
		queue_redraw()
@export var size := Vector2(130, 80):
	set(value):
		size = value
		queue_redraw()


func _draw() -> void:
	var rect := Rect2(-size / 2.0, size)
	draw_rect(rect, color)
	draw_rect(rect, Color(1, 1, 1, 0.35), false, 2.0)
	draw_string(ThemeDB.fallback_font, Vector2(-size.x / 2.0, size.y / 2.0 + 18.0), label_text,
		HORIZONTAL_ALIGNMENT_CENTER, size.x, 14, Color(1, 1, 1, 0.85))
