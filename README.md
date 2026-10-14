# Godot Economy System

A GDExtension for Godot 4.4, written in C++, that gives NPCs a working economy. NPCs have needs such as thirst and hunger, plan how to satisfy them with goal oriented action planning (GOAP), and trade with each other without any trade being scripted.

The extension decides **what** an NPC does. Your game decides **how** it looks: walking, animations, timers. The extension then applies the results to the NPC's wallet.

* Currencies, items, needs, actions and goals are Resources, authored in the inspector.
* NPCs sell what they do not need to afford what they do, produce what others are waiting for, and accept or refuse offers with a readable reason.
* Tested with up to 900 NPCs in one scene.

## Requirements

* Godot 4.4
* To build: Python with SCons, and a C++17 compiler (MSVC on Windows)

## Building

```
git clone --recursive https://github.com/Yungsaas/Godot-Economy-System.git
cd Godot-Economy-System
scons
```

If the repository was cloned without `--recursive`, run `git submodule update --init --recursive` first.

The build copies the library into `test_project/economyextension/bin/`.

| Option | Effect |
|---|---|
| `target=template_release` | Release build |
| `debug_symbols=yes optimize=speed` | Optimised build with symbols, for profiling |
| `tracy=yes` | Adds the [Tracy](https://github.com/wolfpld/tracy) profiler client. Without it, the profiling markers compile to nothing |

## Installing in your own project

Copy the `test_project/economyextension` folder into your project. It contains the `.gdextension` file and the built libraries. Godot loads the extension on the next start, and the new node and resource types appear in the "Create New Node" and "New Resource" dialogs.

## Quick start

1. Add an **EconomyManager** node to your scene. In the inspector, add a `Currency` to `initial_currencies` (for example id `gold`) and `EconomyItem`s to `initial_items` (for example `water`, base value 2).
2. Add a **GoapTrader** node to each NPC. It creates its own `Wallet`, `NeedsComponent` and `GoapPlanner` if they are missing.
3. On the trader's `NeedsComponent`, add a `Need` (for example `thirst`, satisfied by `water`). On NPCs that should keep stock to sell, set a fallback goal, for example `water: 2`.
4. On the trader, add `GoapAction`s, for example "Buy water" (kind Trade, consumes `gold: 2`, produces `water: 1`) on one NPC, and "Draw water" (kind Custom, produces `water: 2`) on another.
5. Give the NPCs some gold, and respond to `step_started`:

```gdscript
func _ready() -> void:
	$GoapTrader.get_wallet().add_currency("gold", 10.0)
	$GoapTrader.step_started.connect(_on_step_started)

func _on_step_started(action: GoapAction, partner: GoapTrader) -> void:
	# Walk, animate, wait... then report back:
	$GoapTrader.complete_step()
```

To try it without any game code at all, turn on `instant_steps` on the traders.

The full documentation is in the [wiki](https://github.com/Yungsaas/Godot-Economy-System/wiki). The wiki pages are also kept in this repository, in `docs/wiki/`.

## Demos

Open `test_project` in Godot.

| Scene | Shows |
|---|---|
| `goap_demo/goap_demo.tscn` | Three NPCs (farmer, baker, water carrier) walking and trading. Labels show each NPC's plan, needs, wallet and last offer reason. `npc.gd` is a complete example of the game side |
| `goap_test/goap_test.tscn` | Automated checks for the whole extension, and a text based economy in the Output panel |
| `goap_stress/goap_stress.tscn` | Up to 900 NPCs, with frame time and economy health reports |

## Not covered here

The extension also contains a rule based trading system (`RuleTrader`, `TradeMarket`, `TradeRule`, `TraderProfile`), built for a separate project. It is independent of the GOAP system and not documented here.
