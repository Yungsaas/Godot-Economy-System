#include "goap/goap_action.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

using namespace godot;

bool GoapAction::is_available_for(Object *trader) {
	if (!has_method("_is_available")) {
		return true;
	}
	Variant result = call("_is_available", trader);
	if (result.get_type() != Variant::BOOL) {
		print_error("GoapAction \"", action_name, "\": _is_available must return a bool. The action is treated as available.");
		return true;
	}
	return result;
}

float GoapAction::get_cost_for(Object *trader) {
	if (!has_method("_get_cost")) {
		return cost;
	}
	Variant result = call("_get_cost", trader);
	if (result.get_type() != Variant::FLOAT && result.get_type() != Variant::INT) {
		print_error("GoapAction \"", action_name, "\": _get_cost must return a number. The cost field is used instead.");
		return cost;
	}
	float value = result;
	return value < MIN_COST ? MIN_COST : value;
}

void GoapAction::_bind_methods() {
	BIND_ENUM_CONSTANT(KIND_CUSTOM);
	BIND_ENUM_CONSTANT(KIND_TRADE);

	ClassDB::bind_method(D_METHOD("get_action_name"), &GoapAction::get_action_name);
	ClassDB::bind_method(D_METHOD("set_action_name", "name"), &GoapAction::set_action_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "action_name"), "set_action_name", "get_action_name");

	ClassDB::bind_method(D_METHOD("get_kind"), &GoapAction::get_kind);
	ClassDB::bind_method(D_METHOD("set_kind", "kind"), &GoapAction::set_kind);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "kind", PROPERTY_HINT_ENUM, "Custom,Trade"), "set_kind", "get_kind");

	ClassDB::bind_method(D_METHOD("get_requires"), &GoapAction::get_requires);
	ClassDB::bind_method(D_METHOD("set_requires", "amounts"), &GoapAction::set_requires);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "requires"), "set_requires", "get_requires");

	ClassDB::bind_method(D_METHOD("get_consumes"), &GoapAction::get_consumes);
	ClassDB::bind_method(D_METHOD("set_consumes", "amounts"), &GoapAction::set_consumes);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "consumes"), "set_consumes", "get_consumes");

	ClassDB::bind_method(D_METHOD("get_produces"), &GoapAction::get_produces);
	ClassDB::bind_method(D_METHOD("set_produces", "amounts"), &GoapAction::set_produces);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "produces"), "set_produces", "get_produces");

	ClassDB::bind_method(D_METHOD("get_cost"), &GoapAction::get_cost);
	ClassDB::bind_method(D_METHOD("set_cost", "cost"), &GoapAction::set_cost);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cost", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater"),
				 "set_cost", "get_cost");

	ClassDB::bind_method(D_METHOD("is_available_for", "trader"), &GoapAction::is_available_for);
	ClassDB::bind_method(D_METHOD("get_cost_for", "trader"), &GoapAction::get_cost_for);
}
