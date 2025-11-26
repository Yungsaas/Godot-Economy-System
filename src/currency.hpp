#pragma once
#include <godot_cpp/classes/resource.hpp>

using namespace godot;

class Currency : public Resource
{ 
GDCLASS(Currency, Resource)

public:
    String id;
    String display_name;
    int precision = 2; // decimal places
    float base_value = 1.0;
    String symbol;

    Currency() {}
    ~Currency() override {}

    String get_id() const { return id; }
    void set_id(const String &v) { id = v; }

    String get_name() const { return display_name; }
    void set_name(const String &v) { display_name = v; }

    int get_precision() const { return precision; }
    void set_precision(int v) { precision = v; }

    float get_base_value() const { return base_value; }
    void set_base_value(float v) { base_value = v; }

    String get_symbol() const { return symbol; }
    void set_symbol(const String &v) { symbol = v; }

protected:
	static void _bind_methods();

private:

};
