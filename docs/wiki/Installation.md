# Installation

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
| `tracy=yes` | Adds the [Tracy](https://github.com/wolfpld/tracy) profiler client. Without it, the profiling markers compile to nothing. See [Performance](Performance) |

## Installing in your own project

Copy the `test_project/economyextension` folder into your project. It contains the `.gdextension` file and the built libraries. Godot loads the extension on the next start, and the new node and resource types appear in the "Create New Node" and "New Resource" dialogs.

## Demos

Open `test_project` in Godot.

| Scene | Shows |
|---|---|
| `goap_demo/goap_demo.tscn` | Three NPCs (farmer, baker, water carrier) walking and trading. Labels show each NPC's plan, needs, wallet and last offer reason. `npc.gd` is a complete example of the game side |
| `goap_test/goap_test.tscn` | Automated checks for the whole extension, and a text based economy in the Output panel |
| `goap_stress/goap_stress.tscn` | Up to 900 NPCs, with frame time and economy health reports |

Next: [Quick Start](Quick-Start)
