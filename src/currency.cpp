#pragma once
#include "currency.hpp"

using namespace godot;

void Currency::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_id"), &Currency::get_id);
    ClassDB::bind_method(D_METHOD("set_id", "id"), &Currency::set_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "id"), "set_id", "get_id");

    ClassDB::bind_method(D_METHOD("get_name"), &Currency::get_name);
    ClassDB::bind_method(D_METHOD("set_name", "display_name"), &Currency::set_name);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "display_name"), "set_name", "get_name");

    ClassDB::bind_method(D_METHOD("get_precision"), &Currency::get_precision);
    ClassDB::bind_method(D_METHOD("set_precision", "precision"), &Currency::set_precision);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "precision"), "set_precision", "get_precision");

    ClassDB::bind_method(D_METHOD("get_base_value"), &Currency::get_base_value);
    ClassDB::bind_method(D_METHOD("set_base_value", "base_value"), &Currency::set_base_value);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "base_value"), "set_base_value", "get_base_value");

    ClassDB::bind_method(D_METHOD("get_symbol"), &Currency::get_symbol);
    ClassDB::bind_method(D_METHOD("set_symbol", "symbol"), &Currency::set_symbol);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "symbol"), "set_symbol", "get_symbol");
}
