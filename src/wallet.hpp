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

	void add_currency(const String &currency_id, float amount);
	bool remove_currency(const String &currency_id, float amount);
	bool has_currency(const String &currency_id, float amount) const;
	bool transfer_currency_to(Wallet *other, const String &currency_id, float amount);
	float get_currency_balance_of(const String &currency_id);
	Dictionary get_currencies() {
		return balances_currencies;
	}

	bool add_item(const String &item_id, int amount);
	bool remove_item(const String &item_id, int amount);
	bool has_item(const String &item_id, int amount) const;
	bool transfer_item_to(Wallet *other, const String &item_id, int amount);
	int get_item_balance_of(const String &item_id);
	Dictionary get_items() {
		return balances_items;
	}
	void _ready() override;

	void _on_economy_manager_ready(Object *mgr_object);

	void _on_node_added(Node *node);

protected:
	static void _bind_methods();
	bool bind_economy_manager_from_group();

private:
	Dictionary balances_items;
	Dictionary balances_currencies;
	EconomyManager *economy_manager = nullptr;
};
