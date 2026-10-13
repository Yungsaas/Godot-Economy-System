#pragma once
#include "goap/goap_action.hpp"
#include "goap/goap_goal.hpp"
#include "goap/goap_plan.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

class Wallet;

// Finds the cheapest sequence of actions that reaches a goal, using A* forward search.
//
// World state: an amount per id, where an id is an item id or a currency id. Needs are
// not part of the world state; they only decide which goal is planned for.
//
// At the API boundary the world state is a Dictionary (id -> amount), so it can be
// built from a wallet, inspected from GDScript, or copied and changed to ask
// "what would my plan be if I accepted this offer".
//
// The search always ends: it is limited both in plan length (max_depth) and in the
// number of states it explores (max_explored_states).
class GoapPlanner : public Node {
	GDCLASS(GoapPlanner, Node)

public:
	GoapPlanner() {}
	~GoapPlanner() override {}

	int get_max_depth() const { return max_depth; }
	void set_max_depth(int v) { max_depth = v < 1 ? 1 : v; }

	int get_max_explored_states() const { return max_explored_states; }
	void set_max_explored_states(int v) { max_explored_states = v < 1 ? 1 : v; }

	// Combines the wallet's currencies and items into one Dictionary.
	Dictionary build_world_state(Wallet *wallet) const;

	Ref<GoapPlan> plan_for_wallet(const Ref<GoapGoal> &goal, Wallet *wallet, const Array &actions, Object *trader = nullptr, const Dictionary &extra_costs = Dictionary());
	// extra_costs: GoapAction -> cost added on top of the action's own cost, for example
	// a temporary penalty after the action failed.
	Ref<GoapPlan> plan_from_state(const Ref<GoapGoal> &goal, const Dictionary &state, const Array &actions, Object *trader = nullptr, const Dictionary &extra_costs = Dictionary());

protected:
	static void _bind_methods();

private:
	int max_depth = 8;
	int max_explored_states = 2000;
};
