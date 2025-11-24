#pragma once
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include <godot_cpp/variant/typed_array.hpp> 
#include <godot_cpp/classes/node.hpp> 
#include <godot_cpp/classes/packed_scene.hpp> 
#include <godot_cpp/classes/resource.hpp> 
#include "currency.hpp"

using namespace godot;

class EconomyManager : public Node
{ 
	GDCLASS(EconomyManager, Node)

public:

    Ref<Currency> create_currency(const String &id,const String &name, int precision = 2, float base_value = 1.0, const String &symbol = "$");
    void register_currency(const Ref<Currency> &currency);
    Ref<Currency> get_currency(const String &id);

protected:
	static void _bind_methods();

private:
Dictionary currencies;
Dictionary items;

};
