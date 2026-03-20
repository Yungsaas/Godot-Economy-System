#pragma once
#include "economy_item.hpp"

using namespace godot;

void EconomyItem::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_id"), &EconomyItem::get_id);
	ClassDB::bind_method(D_METHOD("set_id", "id"), &EconomyItem::set_id);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");

	ClassDB::bind_method(D_METHOD("get_name"), &EconomyItem::get_name);
	ClassDB::bind_method(D_METHOD("set_name", "display_name"), &EconomyItem::set_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_name", "get_name");

	ClassDB::bind_method(D_METHOD("get_stackable"), &EconomyItem::get_stackable);
	ClassDB::bind_method(D_METHOD("set_stackable", "precision"), &EconomyItem::set_stackable);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stackable"), "set_stackable", "get_stackable");

	ClassDB::bind_method(D_METHOD("get_base_value"), &EconomyItem::get_base_value);
	ClassDB::bind_method(D_METHOD("set_base_value", "base_value"), &EconomyItem::set_base_value);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_value"), "set_base_value", "get_base_value");

	ClassDB::bind_method(D_METHOD("get_max_stack_size"), &EconomyItem::get_max_stack_size);
	ClassDB::bind_method(D_METHOD("set_max_stack_size", "max_stack_size"), &EconomyItem::set_max_stack_size);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_stack_size"), "set_max_stack_size", "get_max_stack_size");
}
