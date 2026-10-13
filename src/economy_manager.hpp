#pragma once
#include "currency.hpp"
#include "economy_item.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

class EconomyManager : public Node {
	GDCLASS(EconomyManager, Node)

public:
	Ref<Currency> create_currency(const String &id, const String &name, int precision = 2, float base_value = 1.0, const String &symbol = "$");
	void register_currency(const Ref<Currency> &currency);
	Ref<Currency> get_currency(const String &id) const;
	bool has_currency(const String &id) const;

	Ref<EconomyItem> create_item(const String &id, const String &name, float base_value = 1.0f, bool stackable = true, int max_stack_size = 999);
	void register_economy_item(const Ref<EconomyItem> &econ_item);
	Ref<EconomyItem> get_item(const String &id) const;
	bool has_item(const String &id) const;

	Dictionary get_currencies() const;
	Dictionary get_items() const;

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	Dictionary currencies;
	Dictionary items;
};
