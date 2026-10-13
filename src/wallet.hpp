#pragma once
#include "economy_manager.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

class Wallet : public Node {
	GDCLASS(Wallet, Node)

public:
	Wallet() {}
	~Wallet() override {}

	bool add_currency(const String &currency_id, float amount);
	bool remove_currency(const String &currency_id, float amount);
	bool has_currency(const String &currency_id, float amount) const;
	bool transfer_currency_to(Wallet *other, const String &currency_id, float amount);
	float get_currency_balance_of(const String &currency_id) const;
	Dictionary get_currencies() const { return balances_currencies.duplicate(); }

	bool add_item(const String &item_id, int amount);
	bool remove_item(const String &item_id, int amount);
	bool has_item(const String &item_id, int amount) const;
	bool transfer_item_to(Wallet *other, const String &item_id, int amount);
	int get_item_balance_of(const String &item_id) const;
	Dictionary get_items() const { return balances_items.duplicate(); }

protected:
	static void _bind_methods();
	bool bind_economy_manager_from_group();
	EconomyManager *_get_manager();

private:
	Dictionary balances_items;
	Dictionary balances_currencies;
	// Stored as an id rather than a pointer, so a freed manager is detected instead of dereferenced.
	uint64_t economy_manager_id = 0;
};
