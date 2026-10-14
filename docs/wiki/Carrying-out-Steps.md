# Carrying out Steps

The trader decides, your game shows it. The contract:

1. The trader emits `step_started(action, partner)`. `partner` is the other `GoapTrader` for a trade, and `null` for a custom action.
2. Your game carries the step out however it wants: walking, animations, a timer.
3. Your game calls `complete_step()` when it is done, or `fail_step(reason)` if it cannot finish.
4. The trader applies the effects (for a trade, the partner judges the offer again first), emits `step_completed` or `step_failed`, and plans again.

```gdscript
@onready var trader: GoapTrader = $GoapTrader
var _action: GoapAction

func _ready() -> void:
	trader.step_started.connect(_on_step_started)

func _on_step_started(action: GoapAction, partner: GoapTrader) -> void:
	_action = action
	var target: Vector2 = partner.get_parent().global_position if partner else $Workplace.global_position
	await walk_to(target)            # your own movement code
	# The step may have ended in the meantime, for example by a timeout.
	if trader.is_executing() and trader.get_current_step() == _action:
		trader.complete_step()

func _on_timeout() -> void:
	if trader.is_executing():
		trader.fail_step("took too long")
```

`goap_demo/npc.gd` in the test project is a complete, commented version with walking, working, a timeout and a debug label.

## What the extension checks for you

* A step is checked again when it completes. A crafting step whose ingredients are gone by then fails and changes nothing.
* A trade with several items either happens completely or not at all.
* After a failed step the trader waits `replan_interval` before planning again, and the failed action is more expensive for a while (see [Trading and Offers](Trading-and-Offers)).

## Without any game code

`instant_steps` completes every step immediately. Use it for NPCs off screen and for testing.

| Setting | Meaning |
|---|---|
| `auto_run` | Plan and start steps by itself. Off: call `start_plan()` yourself |
| `instant_steps` | Complete steps without game code |
| `replan_interval` | Seconds between planning attempts when there is nothing to do, and after a failed step |

Next: [Trading and Offers](Trading-and-Offers)
