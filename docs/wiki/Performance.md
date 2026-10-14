# Performance

Measured with Tracy, a frame with 900 NPCs takes about 2.8 ms. The planner itself is cheap, at about 10 µs per search. Most of the cost is per NPC per frame (needs decaying) and in scans over the market (planning and partner search).

## Tips

* **Keep markets small.** At 900 NPCs, markets of 30 were clearly faster than one market for everyone, and removed the frame spikes. See [Markets](Markets).
* **Level of detail for NPCs off screen:**
  * `instant_steps` on the trader, so no movement or animation is needed
  * a longer `replan_interval`
  * `auto_decay` off on the `NeedsComponent`, and `decay(delta)` called less often
* **Keep action lists short.** The search grows with the number of actions per NPC. `max_depth` (default 8) and `max_explored_states` (default 2000) on the `GoapPlanner` cap the worst case.
* **Avoid spawning many traders in the same frame** if you can. Their replan timers then stay in step, and many of them plan in the same frame.

## Profiling

The trader, the planner and the needs are marked for the [Tracy](https://github.com/wolfpld/tracy) profiler.

1. Build with `scons tracy=yes debug_symbols=yes optimize=speed`.
2. Start the Tracy profiler. It must be the same version as the `thirdparty/tracy` submodule.
3. Run your scene and connect.

Two plots are included: explored states per search, and planner searches per frame. Without `tracy=yes` all markers compile to nothing.

`goap_stress/goap_stress.tscn` in the test project spawns up to 900 traders with `instant_steps`, and reports frame time and economy health every 5 seconds.

Next: [Balancing an Economy](Balancing-an-Economy)
