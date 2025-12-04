#pragma once
#include "wallet.hpp"
#include "economy_item.hpp"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/print_string.hpp"

void Wallet::_ready() {
	get_tree()->connect("node_added", Callable(this, "_on_node_added"));
}

void Wallet::add_currency(const String &id, float amount) {
	if (!economy_manager)
		bind_economy_manager_from_group();
	float bal = balances_currencies[id];
	bal += amount;
	balances_currencies[id] = bal;
	emit_signal("balance_changed", id, bal);
}

bool Wallet::remove_currency(const String &id, float amount) {
	if (!economy_manager)
		bind_economy_manager_from_group();
	float bal = balances_currencies[id];
	if (bal < amount) {
		print_error("Balance of ", id, " is insufficient. Required: ", amount, " Has: ", bal);
		return false;
	}
	bal -= amount;
	balances_currencies[id] = bal;
	emit_signal("balance_changed", id, bal);
	return true;
}

bool Wallet::has_currency(const String &id, float amount) const {
	float bal = balances_currencies[id];
	return bal >= amount;
}

bool Wallet::transfer_currency_to(Wallet *other, const String &id, float amount) {
	if (!economy_manager)
		bind_economy_manager_from_group();

	if (!other->is_node_ready()) {
		print_error("Other node was not ready");
		return false;
	}
	if (!remove_currency(id, amount)) {
		return false;
	}
	other->add_currency(id, amount);
	return true;
}

bool Wallet::add_item(const String &id, int amount) {
	if (!economy_manager)
		bind_economy_manager_from_group();

	int bal = balances_items[id];
	auto economy_variant = economy_manager->get_items()[id];

	auto economy_item = Object::cast_to<EconomyItem>(economy_variant);

	if (economy_item->get_stackable()) {
		if (economy_item->get_max_stack_size() < (bal + amount)) {
			print_error("Tried to add economy item amount above max stack size");
			return false;
		}
		bal += amount;
		balances_items[id] = bal;
		emit_signal("balance_changed", id, bal);
		return true;
	} else {
		if (bal == 0 && amount == 1) {
			balances_items[id] = 1;
			emit_signal("balance_changed", id, bal);
			return true;
		}
	}
	return false;
}

bool Wallet::remove_item(const String &id, int amount) {
	if (!economy_manager)
		bind_economy_manager_from_group();
	int bal = balances_items[id];
    bal -=amount;
	if (bal < 0) {
		return false;
	}
    balances_items[id] = bal;
    emit_signal("balance_changed", id, bal);
	return true;
}

bool Wallet::has_item(const String &id, int amount) const {
	int bal = balances_items[id];
	return bal >= amount;
}

bool Wallet::transfer_item_to(Wallet *other, const String &id, int amount) {
    if(!economy_manager) bind_economy_manager_from_group();

	auto econ_item_var = economy_manager->get_items()[id];
	auto econ_item = Object::cast_to<EconomyItem>(econ_item_var);
	if (!other->is_node_ready()) {
		print_error("Other node was not ready");
		return false;
	}
	if (econ_item->get_max_stack_size() < other->get_item_balance_of(id) + amount) {
		print_error("This transaction would exceed max stack size");
		return false;
	}

	if (!remove_item(id, amount)) {
		return false;
	}
	other->add_item(id, amount);
	return true;
}

void Wallet::_on_economy_manager_ready(Object *mgr_obj) {
	EconomyManager *mgr = Object::cast_to<EconomyManager>(mgr_obj);
	if (!mgr) {
		print_error("Could not bind economy manager");
		return;
	}
	economy_manager = mgr;
	print_line("bound mgr");
}

void Wallet::_on_node_added(Node *node) {
	if (!node)
		return;
	if (!economy_manager)
		bind_economy_manager_from_group();
}

bool Wallet::bind_economy_manager_from_group() {
	auto mgr_n = get_tree()->get_first_node_in_group("economy_manager");
	if (EconomyManager *mgr = Object::cast_to<EconomyManager>(mgr_n)) {
		print_line("Economy manager linked to ", this);
		economy_manager = mgr;
		return true;
	} else {
		print_error("No economy manager found.");
		return false;
	}
}

float Wallet::get_currency_balance_of(const String &currency_id) {
	return balances_currencies[currency_id];
}

int Wallet::get_item_balance_of(const String &item_id) {
	return balances_items[item_id];
}

void Wallet::_bind_methods() {
	/* Currencies */
	ClassDB::bind_method(D_METHOD("add_currency", "currency", "amount"), &Wallet::add_currency);
	ClassDB::bind_method(D_METHOD("remove_currency", "currency", "amount"), &Wallet::remove_currency);
	ClassDB::bind_method(D_METHOD("transfer_currency_to", "other_wallet", "currency", "amount"), &Wallet::transfer_currency_to);
	ClassDB::bind_method(D_METHOD("has_currency", "currency", "amount"), &Wallet::has_currency);
	ClassDB::bind_method(D_METHOD("get_currencies"), &Wallet::get_currencies);
	ClassDB::bind_method(D_METHOD("get_currency_balance", "currency"), &Wallet::get_currency_balance_of);
	
	/* Items */
	ClassDB::bind_method(D_METHOD("add_item", "item", "amount"), &Wallet::add_item);
	ClassDB::bind_method(D_METHOD("remove_item", "item", "amount"), &Wallet::remove_item);
	ClassDB::bind_method(D_METHOD("transfer_item_to", "other_wallet", "item", "amount"), &Wallet::transfer_item_to);
	ClassDB::bind_method(D_METHOD("has_item", "item", "amount"), &Wallet::has_item);
	ClassDB::bind_method(D_METHOD("get_items"), &Wallet::get_items);
	ClassDB::bind_method(D_METHOD("get_item_balance", "item"), &Wallet::get_item_balance_of);

	/* Signal functions */
	ClassDB::bind_method(D_METHOD("_on_economy_manager_ready", "manager"), &Wallet::_on_economy_manager_ready);
	ClassDB::bind_method(D_METHOD("_on_node_added", "node"), &Wallet::_on_node_added);

	/* Signal emitters */
	ADD_SIGNAL(MethodInfo("balance_changed", PropertyInfo(Variant::STRING, "item_id"), PropertyInfo(Variant::FLOAT, "new_amount")));
}
