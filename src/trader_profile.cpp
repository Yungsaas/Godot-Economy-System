#include "trader_profile.hpp"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void TraderProfile::_bind_methods() {
	// Identity
	ClassDB::bind_method(D_METHOD("get_profile_name"), &TraderProfile::get_profile_name);
	ClassDB::bind_method(D_METHOD("set_profile_name", "name"), &TraderProfile::set_profile_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "profile_name"),
				 "set_profile_name", "get_profile_name");

	// Accepted currencies
	ClassDB::bind_method(D_METHOD("get_accepted_currencies"), &TraderProfile::get_accepted_currencies);
	ClassDB::bind_method(D_METHOD("set_accepted_currencies", "currencies"), &TraderProfile::set_accepted_currencies);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "accepted_currencies",
							  PROPERTY_HINT_ARRAY_TYPE, "String"),
				 "set_accepted_currencies", "get_accepted_currencies");

	// Accepted items
	ClassDB::bind_method(D_METHOD("get_accepted_items"), &TraderProfile::get_accepted_items);
	ClassDB::bind_method(D_METHOD("set_accepted_items", "items"), &TraderProfile::set_accepted_items);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "accepted_items",
							  PROPERTY_HINT_ARRAY_TYPE, "String"),
				 "set_accepted_items", "get_accepted_items");

	// Rules
	ClassDB::bind_method(D_METHOD("get_rules"), &TraderProfile::get_rules);
	ClassDB::bind_method(D_METHOD("set_rules", "rules"), &TraderProfile::set_rules);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "rules",
							  PROPERTY_HINT_ARRAY_TYPE, "TradeRule"),
				 "set_rules", "get_rules");

	ClassDB::bind_method(D_METHOD("add_rule", "rule"), &TraderProfile::add_rule);
	ClassDB::bind_method(D_METHOD("remove_rule", "rule"), &TraderProfile::remove_rule);
}
