#include "goap/need.hpp"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void Need::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_need_id"), &Need::get_need_id);
	ClassDB::bind_method(D_METHOD("set_need_id", "need_id"), &Need::set_need_id);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "need_id"), "set_need_id", "get_need_id");

	ClassDB::bind_method(D_METHOD("get_display_name"), &Need::get_display_name);
	ClassDB::bind_method(D_METHOD("set_display_name", "display_name"), &Need::set_display_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_display_name", "get_display_name");

	ADD_GROUP("Value", "");
	ClassDB::bind_method(D_METHOD("get_max_value"), &Need::get_max_value);
	ClassDB::bind_method(D_METHOD("set_max_value", "value"), &Need::set_max_value);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_value", PROPERTY_HINT_RANGE, "0.001,1000,0.01,or_greater"),
				 "set_max_value", "get_max_value");

	ClassDB::bind_method(D_METHOD("get_start_value"), &Need::get_start_value);
	ClassDB::bind_method(D_METHOD("set_start_value", "value"), &Need::set_start_value);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "start_value", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater"),
				 "set_start_value", "get_start_value");

	ClassDB::bind_method(D_METHOD("get_decay_per_second"), &Need::get_decay_per_second);
	ClassDB::bind_method(D_METHOD("set_decay_per_second", "rate"), &Need::set_decay_per_second);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "decay_per_second", PROPERTY_HINT_RANGE, "0,100,0.01,or_greater"),
				 "set_decay_per_second", "get_decay_per_second");

	ClassDB::bind_method(D_METHOD("get_urgent_threshold"), &Need::get_urgent_threshold);
	ClassDB::bind_method(D_METHOD("set_urgent_threshold", "threshold"), &Need::set_urgent_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "urgent_threshold", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater"),
				 "set_urgent_threshold", "get_urgent_threshold");

	ClassDB::bind_method(D_METHOD("get_priority"), &Need::get_priority);
	ClassDB::bind_method(D_METHOD("set_priority", "priority"), &Need::set_priority);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "priority"), "set_priority", "get_priority");

	ADD_GROUP("Satisfaction", "");
	ClassDB::bind_method(D_METHOD("get_satisfier_item_id"), &Need::get_satisfier_item_id);
	ClassDB::bind_method(D_METHOD("set_satisfier_item_id", "item_id"), &Need::set_satisfier_item_id);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "satisfier_item_id"), "set_satisfier_item_id", "get_satisfier_item_id");

	ClassDB::bind_method(D_METHOD("get_restore_per_item"), &Need::get_restore_per_item);
	ClassDB::bind_method(D_METHOD("set_restore_per_item", "amount"), &Need::set_restore_per_item);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "restore_per_item", PROPERTY_HINT_RANGE, "0,1000,0.01,or_greater"),
				 "set_restore_per_item", "get_restore_per_item");

	ClassDB::bind_method(D_METHOD("get_consume_threshold"), &Need::get_consume_threshold);
	ClassDB::bind_method(D_METHOD("set_consume_threshold", "threshold"), &Need::set_consume_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "consume_threshold", PROPERTY_HINT_RANGE, "-1,1000,0.01,or_greater"),
				 "set_consume_threshold", "get_consume_threshold");
	ClassDB::bind_method(D_METHOD("get_effective_consume_threshold"), &Need::get_effective_consume_threshold);

	ClassDB::bind_method(D_METHOD("get_goal_amount"), &Need::get_goal_amount);
	ClassDB::bind_method(D_METHOD("set_goal_amount", "amount"), &Need::set_goal_amount);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "goal_amount", PROPERTY_HINT_RANGE, "1,99,1,or_greater"),
				 "set_goal_amount", "get_goal_amount");
}
