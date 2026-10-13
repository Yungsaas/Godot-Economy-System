#include "goap/goap_goal.hpp"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void GoapGoal::set_item_target(const String &item_id, int amount) {
	if (amount <= 0) {
		item_targets.erase(item_id);
		return;
	}
	item_targets[item_id] = amount;
}

int GoapGoal::get_item_target(const String &item_id) const {
	return item_targets.get(item_id, 0);
}

void GoapGoal::set_currency_target(const String &currency_id, float amount) {
	if (amount <= 0.0f) {
		currency_targets.erase(currency_id);
		return;
	}
	currency_targets[currency_id] = amount;
}

float GoapGoal::get_currency_target(const String &currency_id) const {
	return currency_targets.get(currency_id, 0.0f);
}

void GoapGoal::clear_targets() {
	item_targets.clear();
	currency_targets.clear();
}

void GoapGoal::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_goal_name"), &GoapGoal::get_goal_name);
	ClassDB::bind_method(D_METHOD("set_goal_name", "name"), &GoapGoal::set_goal_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "goal_name"), "set_goal_name", "get_goal_name");

	ClassDB::bind_method(D_METHOD("get_priority"), &GoapGoal::get_priority);
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &GoapGoal::set_priority);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");

	ClassDB::bind_method(D_METHOD("get_item_targets"), &GoapGoal::get_item_targets);
	ClassDB::bind_method(D_METHOD("set_item_targets", "targets"), &GoapGoal::set_item_targets);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "item_targets"), "set_item_targets", "get_item_targets");

	ClassDB::bind_method(D_METHOD("get_currency_targets"), &GoapGoal::get_currency_targets);
	ClassDB::bind_method(D_METHOD("set_currency_targets", "targets"), &GoapGoal::set_currency_targets);
	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "currency_targets"), "set_currency_targets", "get_currency_targets");

	ClassDB::bind_method(D_METHOD("set_item_target", "item_id", "amount"), &GoapGoal::set_item_target);
	ClassDB::bind_method(D_METHOD("get_item_target", "item_id"), &GoapGoal::get_item_target);
	ClassDB::bind_method(D_METHOD("set_currency_target", "currency_id", "amount"), &GoapGoal::set_currency_target);
	ClassDB::bind_method(D_METHOD("get_currency_target", "currency_id"), &GoapGoal::get_currency_target);
	ClassDB::bind_method(D_METHOD("clear_targets"), &GoapGoal::clear_targets);
}
