#include "goap/goap_plan.hpp"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

String GoapPlan::describe() const {
	if (result != RESULT_FOUND) {
		return message;
	}
	PackedStringArray names;
	for (int i = 0; i < steps.size(); ++i) {
		Ref<GoapAction> step = steps[i];
		names.push_back(step.is_valid() ? step->get_action_name() : String("?"));
	}
	return String(" > ").join(names);
}

float GoapPlan::get_cost_from(int first_step) const {
	float sum = 0.0f;
	for (int i = MAX(first_step, 0); i < step_costs.size(); ++i) {
		sum += step_costs[i];
	}
	return sum;
}

void GoapPlan::_bind_methods() {
	BIND_ENUM_CONSTANT(RESULT_FOUND);
	BIND_ENUM_CONSTANT(RESULT_ALREADY_SATISFIED);
	BIND_ENUM_CONSTANT(RESULT_NO_PLAN);
	BIND_ENUM_CONSTANT(RESULT_SEARCH_LIMIT);
	BIND_ENUM_CONSTANT(RESULT_INVALID);

	ClassDB::bind_method(D_METHOD("get_result"), &GoapPlan::get_result);
	ClassDB::bind_method(D_METHOD("get_goal"), &GoapPlan::get_goal);
	ClassDB::bind_method(D_METHOD("get_steps"), &GoapPlan::get_steps);
	ClassDB::bind_method(D_METHOD("get_step_count"), &GoapPlan::get_step_count);
	ClassDB::bind_method(D_METHOD("get_total_cost"), &GoapPlan::get_total_cost);
	ClassDB::bind_method(D_METHOD("get_explored_states"), &GoapPlan::get_explored_states);
	ClassDB::bind_method(D_METHOD("get_search_usec"), &GoapPlan::get_search_usec);
	ClassDB::bind_method(D_METHOD("get_message"), &GoapPlan::get_message);
	ClassDB::bind_method(D_METHOD("is_found"), &GoapPlan::is_found);
	ClassDB::bind_method(D_METHOD("describe"), &GoapPlan::describe);
	ClassDB::bind_method(D_METHOD("get_step_costs"), &GoapPlan::get_step_costs);
	ClassDB::bind_method(D_METHOD("get_cost_from", "first_step"), &GoapPlan::get_cost_from);
}
