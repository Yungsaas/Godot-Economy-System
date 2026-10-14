#include "goap/goap_planner.hpp"
#include "economy_manager.hpp"
#include "profiling.hpp"
#include "wallet.hpp"

#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace godot;

namespace {

// Amounts are stored as integers in thousandths. Currencies with decimals stay exact,
// and two states with the same amounts always hash and compare equal.
using Amount = int64_t;
constexpr Amount SCALE = 1000;
constexpr Amount UNLIMITED = std::numeric_limits<Amount>::max();

Amount to_amount(const Variant &v) {
	return (Amount)std::llround((double)v * (double)SCALE);
}

using State = std::vector<Amount>;

struct StateHash {
	size_t operator()(const State &s) const {
		size_t h = 1469598103934665603ULL;
		for (Amount a : s) {
			h ^= std::hash<Amount>()(a) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		}
		return h;
	}
};

// Gives every id that the goal or an action mentions a slot in the State vector.
// Ids that nothing mentions are left out, so they do not slow the search down.
struct SymbolTable {
	std::vector<String> ids;
	Dictionary indices;

	int index_of(const String &id) {
		int found = indices.get(id, -1);
		if (found >= 0) {
			return found;
		}
		int index = (int)ids.size();
		ids.push_back(id);
		indices[id] = index;
		return index;
	}
};

// An action translated into slot indices, so the search does not touch Dictionaries.
struct CompiledAction {
	int source_index = 0; // index in the actions array passed in
	float cost = 1.0f;
	std::vector<std::pair<int, Amount>> needs; // slot -> minimum amount that must be held
	std::vector<std::pair<int, Amount>> deltas; // slot -> change when the action is applied
};

struct SearchNode {
	State amounts;
	int parent = -1;
	int action = -1;
	float g = 0.0f;
	int depth = 0;
};

struct OpenEntry {
	float f;
	float h;
	int node;
};

// Lowest f first. On equal f, lowest h first, which prefers states closer to the goal.
struct OpenCompare {
	bool operator()(const OpenEntry &a, const OpenEntry &b) const {
		if (a.f != b.f) {
			return a.f > b.f;
		}
		return a.h > b.h;
	}
};

template <typename F>
void for_each_amount(const Dictionary &amounts, F &&f) {
	Array keys = amounts.keys();
	for (int i = 0; i < keys.size(); ++i) {
		String id = keys[i];
		Amount amount = to_amount(amounts.get(keys[i], 0));
		if (id.is_empty() || amount <= 0) {
			continue;
		}
		f(id, amount);
	}
}

} // namespace

Dictionary GoapPlanner::build_world_state(Wallet *wallet) const {
	Dictionary state;
	if (!wallet) {
		return state;
	}
	state.merge(wallet->get_currencies());
	state.merge(wallet->get_items());
	return state;
}

Ref<GoapPlan> GoapPlanner::plan_for_wallet(const Ref<GoapGoal> &goal, Wallet *wallet, const Array &actions, Object *trader, const Dictionary &extra_costs) {
	if (!wallet) {
		Ref<GoapPlan> plan;
		plan.instantiate();
		plan->goal = goal;
		plan->result = GoapPlan::RESULT_INVALID;
		plan->message = "No wallet to plan from.";
		return plan;
	}
	return plan_from_state(goal, build_world_state(wallet), actions, trader, extra_costs);
}

Ref<GoapPlan> GoapPlanner::plan_from_state(const Ref<GoapGoal> &goal, const Dictionary &state, const Array &actions, Object *trader, const Dictionary &extra_costs) {
	ZoneScopedN("GoapPlanner::plan_from_state");
	ECONOMY_COUNT_SEARCH();
	const uint64_t start_usec = Time::get_singleton()->get_ticks_usec();

	Ref<GoapPlan> plan;
	plan.instantiate();
	plan->goal = goal;

	auto finish = [&](GoapPlan::Result result, const String &message) {
		plan->result = result;
		plan->message = message;
		plan->search_usec = (int64_t)(Time::get_singleton()->get_ticks_usec() - start_usec);
		return plan;
	};

	if (!goal.is_valid()) {
		return finish(GoapPlan::RESULT_INVALID, "No goal to plan for.");
	}

	// --- Goal targets
	SymbolTable symbols;
	std::unordered_map<int, Amount> target_map;
	auto add_target = [&](const String &id, Amount amount) {
		int slot = symbols.index_of(id);
		target_map[slot] = std::max(target_map[slot], amount);
	};
	for_each_amount(goal->get_item_targets(), add_target);
	for_each_amount(goal->get_currency_targets(), add_target);

	// --- Actions. Script overrides are asked here, once per plan.
	std::vector<CompiledAction> compiled;
	float min_cost = std::numeric_limits<float>::max();
	for (int i = 0; i < actions.size(); ++i) {
		Ref<GoapAction> action = actions[i];
		if (!action.is_valid() || !action->is_available_for(trader)) {
			continue;
		}
		std::unordered_map<int, Amount> needs;
		std::unordered_map<int, Amount> deltas;
		for_each_amount(action->get_requires(), [&](const String &id, Amount amount) {
			int slot = symbols.index_of(id);
			needs[slot] = std::max(needs[slot], amount);
		});
		for_each_amount(action->get_consumes(), [&](const String &id, Amount amount) {
			int slot = symbols.index_of(id);
			needs[slot] = std::max(needs[slot], amount);
			deltas[slot] -= amount;
		});
		for_each_amount(action->get_produces(), [&](const String &id, Amount amount) {
			deltas[symbols.index_of(id)] += amount;
		});

		CompiledAction entry;
		entry.source_index = i;
		entry.cost = action->get_cost_for(trader) + (float)extra_costs.get(action, 0.0f);
		for (const auto &n : needs) {
			entry.needs.emplace_back(n.first, n.second);
		}
		for (const auto &d : deltas) {
			if (d.second != 0) {
				entry.deltas.emplace_back(d.first, d.second);
			}
		}
		if (entry.deltas.empty()) {
			continue; // changes nothing, can never help
		}
		min_cost = std::min(min_cost, entry.cost);
		compiled.push_back(std::move(entry));
	}

	const int slot_count = (int)symbols.ids.size();
	std::vector<std::pair<int, Amount>> targets(target_map.begin(), target_map.end());

	// --- Start state
	State start(slot_count, 0);
	for (int i = 0; i < slot_count; ++i) {
		start[i] = to_amount(state.get(symbols.ids[i], 0));
	}

	auto satisfied = [&](const State &s) {
		for (const auto &t : targets) {
			if (s[t.first] < t.second) {
				return false;
			}
		}
		return true;
	};

	if (satisfied(start)) {
		return finish(GoapPlan::RESULT_ALREADY_SATISFIED, "Goal \"" + goal->get_goal_name() + "\" is already met.");
	}
	if (compiled.empty()) {
		return finish(GoapPlan::RESULT_NO_PLAN, "No actions are available.");
	}

	// --- Stack limits. Items cannot exceed their max stack size, currencies are unlimited.
	State caps(slot_count, UNLIMITED);
	if (is_inside_tree()) {
		EconomyManager *manager = Object::cast_to<EconomyManager>(get_tree()->get_first_node_in_group("economy_manager"));
		if (manager) {
			for (int i = 0; i < slot_count; ++i) {
				Ref<EconomyItem> item = manager->get_item(symbols.ids[i]);
				if (item.is_valid()) {
					Amount max_stack = item->get_stackable() ? item->get_max_stack_size() : 1;
					caps[i] = max_stack * SCALE;
				}
			}
		}
	}

	// --- Heuristic
	// For every unmet target: deficit / (largest amount any single action produces of it)
	// is a lower bound on how many actions are still needed, and every action costs at
	// least min_cost. The largest of these bounds never overestimates, so A* stays optimal.
	std::vector<Amount> max_produced(slot_count, 0);
	for (const CompiledAction &a : compiled) {
		for (const auto &d : a.deltas) {
			max_produced[d.first] = std::max(max_produced[d.first], d.second);
		}
	}
	for (const auto &t : targets) {
		if (start[t.first] < t.second && max_produced[t.first] <= 0) {
			return finish(GoapPlan::RESULT_NO_PLAN, "None of the available actions produces \"" + symbols.ids[t.first] + "\".");
		}
	}
	// Returns a negative value when the goal has become unreachable from this state.
	auto heuristic = [&](const State &s) -> float {
		Amount steps = 0;
		for (const auto &t : targets) {
			Amount deficit = t.second - s[t.first];
			if (deficit <= 0) {
				continue;
			}
			Amount per_action = max_produced[t.first];
			if (per_action <= 0) {
				return -1.0f;
			}
			steps = std::max(steps, (deficit + per_action - 1) / per_action);
		}
		return (float)steps * min_cost;
	};

	// --- A*
	std::vector<SearchNode> nodes;
	std::priority_queue<OpenEntry, std::vector<OpenEntry>, OpenCompare> open;
	std::unordered_map<State, float, StateHash> best_g;

	nodes.push_back(SearchNode{ start, -1, -1, 0.0f, 0 });
	best_g[start] = 0.0f;
	open.push(OpenEntry{ heuristic(start), heuristic(start), 0 });

	int explored = 0;
	bool depth_limited = false;
	int found_node = -1;

	while (!open.empty()) {
		OpenEntry entry = open.top();
		open.pop();

		// Copied out, because pushing new nodes can move the vector.
		const State current = nodes[entry.node].amounts;
		const float g = nodes[entry.node].g;
		const int depth = nodes[entry.node].depth;

		auto best = best_g.find(current);
		if (best != best_g.end() && g > best->second + 1e-6f) {
			continue; // a cheaper way to this state was found after this entry was queued
		}

		++explored;
		if (satisfied(current)) {
			found_node = entry.node;
			break;
		}
		if (explored >= max_explored_states) {
			break;
		}
		if (depth >= max_depth) {
			depth_limited = true;
			continue;
		}

		for (int a = 0; a < (int)compiled.size(); ++a) {
			const CompiledAction &action = compiled[a];

			bool usable = true;
			for (const auto &need : action.needs) {
				if (current[need.first] < need.second) {
					usable = false;
					break;
				}
			}
			if (!usable) {
				continue;
			}

			State next = current;
			for (const auto &delta : action.deltas) {
				Amount value = next[delta.first] + delta.second;
				if (value < 0 || (delta.second > 0 && value > caps[delta.first])) {
					usable = false;
					break;
				}
				next[delta.first] = value;
			}
			if (!usable) {
				continue;
			}

			float h = heuristic(next);
			if (h < 0.0f) {
				continue;
			}
			float next_g = g + action.cost;
			auto known = best_g.find(next);
			if (known != best_g.end() && known->second <= next_g + 1e-6f) {
				continue;
			}
			best_g[next] = next_g;
			nodes.push_back(SearchNode{ std::move(next), entry.node, a, next_g, depth + 1 });
			open.push(OpenEntry{ next_g + h, h, (int)nodes.size() - 1 });
		}
	}

	plan->explored_states = explored;
	TracyPlot("explored states per search", (int64_t)explored);

	if (found_node < 0) {
		if (explored >= max_explored_states) {
			return finish(GoapPlan::RESULT_SEARCH_LIMIT,
					"Stopped after exploring " + String::num_int64(explored) + " states without reaching \"" + goal->get_goal_name() + "\".");
		}
		if (depth_limited) {
			return finish(GoapPlan::RESULT_NO_PLAN,
					"No plan for \"" + goal->get_goal_name() + "\" within " + String::num_int64(max_depth) + " steps.");
		}
		return finish(GoapPlan::RESULT_NO_PLAN, "No sequence of available actions reaches \"" + goal->get_goal_name() + "\".");
	}

	// --- Walk back from the goal to the start to recover the steps.
	std::vector<int> step_indices;
	std::vector<float> step_costs;
	for (int n = found_node; nodes[n].parent >= 0; n = nodes[n].parent) {
		const CompiledAction &used = compiled[nodes[n].action];
		step_indices.push_back(used.source_index);
		step_costs.push_back(used.cost);
	}
	std::reverse(step_indices.begin(), step_indices.end());
	std::reverse(step_costs.begin(), step_costs.end());
	for (size_t i = 0; i < step_indices.size(); ++i) {
		plan->steps.push_back(actions[step_indices[i]]);
		plan->step_costs.push_back(step_costs[i]);
	}
	plan->total_cost = nodes[found_node].g;
	return finish(GoapPlan::RESULT_FOUND, "Plan found for \"" + goal->get_goal_name() + "\".");
}

void GoapPlanner::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_max_depth"), &GoapPlanner::get_max_depth);
	ClassDB::bind_method(D_METHOD("set_max_depth", "depth"), &GoapPlanner::set_max_depth);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_depth", PROPERTY_HINT_RANGE, "1,64,1,or_greater"), "set_max_depth", "get_max_depth");

	ClassDB::bind_method(D_METHOD("get_max_explored_states"), &GoapPlanner::get_max_explored_states);
	ClassDB::bind_method(D_METHOD("set_max_explored_states", "count"), &GoapPlanner::set_max_explored_states);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_explored_states", PROPERTY_HINT_RANGE, "1,100000,1,or_greater"),
				 "set_max_explored_states", "get_max_explored_states");

	ClassDB::bind_method(D_METHOD("build_world_state", "wallet"), &GoapPlanner::build_world_state);
	ClassDB::bind_method(D_METHOD("plan_for_wallet", "goal", "wallet", "actions", "trader", "extra_costs"), &GoapPlanner::plan_for_wallet, DEFVAL(Variant()), DEFVAL(Dictionary()));
	ClassDB::bind_method(D_METHOD("plan_from_state", "goal", "state", "actions", "trader", "extra_costs"), &GoapPlanner::plan_from_state, DEFVAL(Variant()), DEFVAL(Dictionary()));
}
