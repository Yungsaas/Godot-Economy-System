#include "wallet.hpp"
#include "economy_item.hpp"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/print_string.hpp"

EconomyManager *Wallet::_get_manager() {
	if (economy_manager_id != 0) {
		if (EconomyManager *mgr = Object::cast_to<EconomyManager>(ObjectDB::get_instance(economy_manager_id))) {
			return mgr;
		}
		// The manager that was bound has been freed, look for a new one.
		economy_manager_id = 0;
	}
	if (!bind_economy_manager_from_group()) {
		return nullptr;
	}
	return Object::cast_to<EconomyManager>(ObjectDB::get_instance(economy_manager_id));
}

bool Wallet::bind_economy_manager_from_group() {
	if (!is_inside_tree()) {
		print_error("Wallet \"", get_name(), "\" is not inside the scene tree, so it cannot find an economy manager.");
		return false;
	}
	Node *mgr_n = get_tree()->get_first_node_in_group("economy_manager");
	if (EconomyManager *mgr = Object::cast_to<EconomyManager>(mgr_n)) {
		economy_manager_id = mgr->get_instance_id();
		return true;
	}
	print_error("No economy manager found.");
	return false;
}

/* Currencies */

bool Wallet::add_currency(const String &id, float amount) {
	if (amount <= 0.0f) {
		print_error("Amount of currency \"", id, "\" to add must be positive, got ", amount);
		return false;
	}
	EconomyManager *mgr = _get_manager();
	if (!mgr) {
		return false;
	}
	if (!mgr->has_currency(id)) {
		print_error("Currency \"", id, "\" not found.");
		return false;
	}
	float bal = (float)balances_currencies.get(id, 0.0f) + amount;
	balances_currencies[id] = bal;
	emit_signal("balance_changed", id, bal);
	return true;
}

bool Wallet::remove_currency(const String &id, float amount) {
	if (amount <= 0.0f) {
		print_error("Amount of currency \"", id, "\" to remove must be positive, got ", amount);
		return false;
	}
	EconomyManager *mgr = _get_manager();
	if (!mgr) {
		return false;
	}
	if (!mgr->has_currency(id)) {
		print_error("Currency \"", id, "\" not found.");
		return false;
	}
	float bal = balances_currencies.get(id, 0.0f);
	if (bal < amount) {
		print_error("Balance of \"", id, "\" is insufficient. Required: ", amount, " Has: ", bal);
		return false;
	}
	bal -= amount;
	balances_currencies[id] = bal;
	emit_signal("balance_changed", id, bal);
	return true;
}

bool Wallet::has_currency(const String &id, float amount) const {
	return get_currency_balance_of(id) >= amount;
}

float Wallet::get_currency_balance_of(const String &id) const {
	return balances_currencies.get(id, 0.0f);
}

bool Wallet::transfer_currency_to(Wallet *other, const String &id, float amount) {
	if (!other) {
		print_error("Cannot transfer currency \"", id, "\", the other wallet is null.");
		return false;
	}
	if (other == this) {
		print_error("Cannot transfer currency \"", id, "\" to the same wallet.");
		return false;
	}
	if (!other->is_node_ready()) {
		print_error("Other node was not ready");
		return false;
	}
	if (!remove_currency(id, amount)) {
		return false;
	}
	if (!other->add_currency(id, amount)) {
		// The receiver refused, give the currency back so nothing is lost.
		add_currency(id, amount);
		return false;
	}
	return true;
}

/* Items */

bool Wallet::add_item(const String &id, int amount) {
	if (amount <= 0) {
		print_error("Amount of item \"", id, "\" to add must be positive, got ", amount);
		return false;
	}
	EconomyManager *mgr = _get_manager();
	if (!mgr) {
		return false;
	}
	Ref<EconomyItem> economy_item = mgr->get_item(id);
	if (!economy_item.is_valid()) {
		print_error("Item \"", id, "\" not found.");
		return false;
	}

	int bal = get_item_balance_of(id);
	int max_amount = economy_item->get_stackable() ? economy_item->get_max_stack_size() : 1;
	if (bal + amount > max_amount) {
		print_error("Adding ", amount, " of \"", id, "\" would exceed its maximum of ", max_amount, ". Has: ", bal);
		return false;
	}
	bal += amount;
	balances_items[id] = bal;
	emit_signal("balance_changed", id, bal);
	return true;
}

bool Wallet::remove_item(const String &id, int amount) {
	if (amount <= 0) {
		print_error("Amount of item \"", id, "\" to remove must be positive, got ", amount);
		return false;
	}
	EconomyManager *mgr = _get_manager();
	if (!mgr) {
		return false;
	}
	if (!mgr->has_item(id)) {
		print_error("Item \"", id, "\" not found");
		return false;
	}
	int bal = get_item_balance_of(id);
	if (bal < amount) {
		print_error("Balance of \"", id, "\" is insufficient. Required: ", amount, " Has: ", bal);
		return false;
	}
	bal -= amount;
	balances_items[id] = bal;
	emit_signal("balance_changed", id, bal);
	return true;
}

bool Wallet::has_item(const String &id, int amount) const {
	return get_item_balance_of(id) >= amount;
}

int Wallet::get_item_balance_of(const String &id) const {
	return balances_items.get(id, 0);
}

bool Wallet::transfer_item_to(Wallet *other, const String &id, int amount) {
	if (!other) {
		print_error("Cannot transfer item \"", id, "\", the other wallet is null.");
		return false;
	}
	if (other == this) {
		print_error("Cannot transfer item \"", id, "\" to the same wallet.");
		return false;
	}
	if (!other->is_node_ready()) {
		print_error("Other node was not ready");
		return false;
	}
	EconomyManager *mgr = _get_manager();
	if (!mgr) {
		return false;
	}
	Ref<EconomyItem> econ_item = mgr->get_item(id);
	if (!econ_item.is_valid()) {
		print_error("Item \"", id, "\" not found");
		return false;
	}
	int max_amount = econ_item->get_stackable() ? econ_item->get_max_stack_size() : 1;
	if (other->get_item_balance_of(id) + amount > max_amount) {
		print_error("This transaction would exceed max stack size");
		return false;
	}

	if (!remove_item(id, amount)) {
		return false;
	}
	if (!other->add_item(id, amount)) {
		// The receiver refused, give the item back so nothing is lost.
		add_item(id, amount);
		return false;
	}
	return true;
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

	/* Signal emitters */
	ADD_SIGNAL(MethodInfo("balance_changed", PropertyInfo(Variant::STRING, "item_id"), PropertyInfo(Variant::FLOAT, "new_amount")));
}
