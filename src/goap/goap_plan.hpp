#pragma once
#include "goap/goap_action.hpp"
#include "goap/goap_goal.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

// The result of one planner search.
//
// The steps are only a proposal. The planner does not carry anything out; the
// GoapTrader hands the steps to the developer's code one at a time.
class GoapPlan : public RefCounted {
	GDCLASS(GoapPlan, RefCounted)

public:
	enum Result {
		RESULT_FOUND = 0, // steps lead to the goal
		RESULT_ALREADY_SATISFIED = 1, // the goal already holds, there are no steps
		RESULT_NO_PLAN = 2, // no sequence of available actions reaches the goal within the depth limit
		RESULT_SEARCH_LIMIT = 3, // the search stopped after max_explored_states, a plan may still exist
		RESULT_INVALID = 4, // the input was unusable, for example no goal
	};

	GoapPlan() {}
	~GoapPlan() override {}

	Result get_result() const { return result; }
	Ref<GoapGoal> get_goal() const { return goal; }
	TypedArray<GoapAction> get_steps() const { return steps; }
	int get_step_count() const { return steps.size(); }
	float get_total_cost() const { return total_cost; }
	int get_explored_states() const { return explored_states; }
	int64_t get_search_usec() const { return search_usec; }
	String get_message() const { return message; }

	bool is_found() const { return result == RESULT_FOUND; }
	String describe() const;

	PackedFloat32Array get_step_costs() const { return step_costs; }
	// Sum of the costs of the steps from first_step to the end.
	float get_cost_from(int first_step) const;

	// Filled in by GoapPlanner.
	Result result = RESULT_INVALID;
	Ref<GoapGoal> goal;
	TypedArray<GoapAction> steps;
	PackedFloat32Array step_costs; // cost of each step as the planner counted it, including penalties
	float total_cost = 0.0f;
	int explored_states = 0;
	int64_t search_usec = 0;
	String message;

protected:
	static void _bind_methods();
};

VARIANT_ENUM_CAST(GoapPlan::Result)
