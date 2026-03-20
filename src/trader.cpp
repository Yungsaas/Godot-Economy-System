#include "trader.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/core/print_string.hpp"

using namespace godot;

Trader::Trader() {}

void Trader::_ready() {
	_bind_economy_manager();
	linked_wallet = Object::cast_to<Wallet>(find_child("Wallet"));
    if (!linked_wallet) {
        // no Wallet child found, create one automatically
        linked_wallet = memnew(Wallet);
        linked_wallet->set_name("Wallet");
        add_child(linked_wallet);
        print_line("Trader: No Wallet child found, one was created automatically.");
    }
}

bool Trader::_bind_economy_manager() {
	if (economy_manager)
		return true;
	auto *mgr_node = get_tree()->get_first_node_in_group("economy_manager");
	if (auto *mgr = Object::cast_to<EconomyManager>(mgr_node)) {
		economy_manager = mgr;
		return true;
	}
	print_error("Trader: No EconomyManager found in group 'economy_manager'.");
	return false;
}

void Trader::set_profile(const Ref<TraderProfile> &p) {
	profile = p;
}

bool Trader::_accepts_currency(const String &id) const {
	// instance list takes priority
	for (int i = 0; i < accepted_currencies.size(); ++i) {
		if (accepted_currencies[i].operator String() == id)
			return true;
	}
	// fall back to profile list
	if (profile.is_valid()) {
		Array prof_currencies = profile->get_accepted_currencies();
		for (int i = 0; i < prof_currencies.size(); ++i) {
			if (prof_currencies[i].operator String() == id)
				return true;
		}
	}
	return false;
}

bool Trader::_accepts_item(const String &id) const {
	for (int i = 0; i < accepted_items.size(); ++i) {
		if (accepted_items[i].operator String() == id)
			return true;
	}
	if (profile.is_valid()) {
		Array prof_items = profile->get_accepted_items();
		for (int i = 0; i < prof_items.size(); ++i) {
			if (prof_items[i].operator String() == id)
				return true;
		}
	}
	return false;
}

float Trader::_get_market_price(const String &item_id) const {
	if (!economy_manager)
		return 0.0f;
	Variant item_var = economy_manager->get_items()[item_id];
	if (item_var.get_type() == Variant::NIL)
		return 0.0f;
	// falls back to base_value until dynamic pricing is added to EconomyManager
	if (EconomyItem *item = Object::cast_to<EconomyItem>(item_var)) {
		return item->get_base_value();
	}
	return 0.0f;
}

TradeRule::SupplyLevel Trader::_supply_ratio_to_level(float ratio) {
	if (ratio < 0.33f)
		return TradeRule::SUPPLY_LOW;
	if (ratio > 0.66f)
		return TradeRule::SUPPLY_HIGH;
	return TradeRule::SUPPLY_NORMAL;
}

bool Trader::_check_rule_conditions(TradeRule *rule) const {
	if (!rule || !rule->get_enabled())
		return false;
	if (!linked_wallet)
		return false;

	const String item_id = rule->get_item_id();
	const String currency_id = rule->get_currency_id();

	// reject if item or currency isn't accepted by this trader
	if (!item_id.is_empty() && !_accepts_item(item_id))
		return false;
	if (!currency_id.is_empty() && !_accepts_currency(currency_id))
		return false;

	// price threshold, sentinel -1 skips the check
	float market_price = _get_market_price(item_id);
	if (rule->get_action() == TradeRule::ACTION_BUY) {
		float max_price = rule->get_buy_price_max();
		if (max_price >= 0.0f && market_price > max_price)
			return false;
	} else {
		float min_price = rule->get_sell_price_min();
		if (min_price >= 0.0f && market_price < min_price)
			return false;
	}

	// minimum currency balance, sentinel -1 skips
	float min_balance = rule->get_min_currency_balance();
	if (min_balance >= 0.0f) {
		if (!linked_wallet->has_currency(currency_id, min_balance))
			return false;
	}

	// restock / surplus thresholds, sentinel -1 skips
	int current_stock = linked_wallet->get_item_balance_of(item_id);
	int restock_thresh = rule->get_restock_threshold();
	if (restock_thresh >= 0 && current_stock > restock_thresh)
		return false;
	int surplus_thresh = rule->get_surplus_threshold();
	if (surplus_thresh >= 0 && current_stock < surplus_thresh)
		return false;

	// supply level, stub until EconomyManager exposes get_item_supply_ratio()
	// TradeRule::SupplyLevel required = rule->get_required_supply_level();
	// if (required != TradeRule::SUPPLY_ANY && economy_manager) {
	//     float ratio = economy_manager->get_item_supply_ratio(item_id);
	//     if (_supply_ratio_to_level(ratio) != required) return false;
	// }

	// cooldown, skip if never fired or cooldown is zero
	float cooldown = rule->get_cooldown_seconds();
	if (cooldown > 0.0f && rule->last_fired_at >= 0.0f) {
		if ((elapsed_time - rule->last_fired_at) < cooldown)
			return false;
	}

	return true;
}

bool Trader::_execute_rule(TradeRule *rule) {
	if (!linked_wallet)
		return false;

	const String item_id = rule->get_item_id();
	const String currency_id = rule->get_currency_id();
	int amount = rule->get_trade_amount();
	float price = _get_market_price(item_id) * (float)amount;

	if (rule->get_action() == TradeRule::ACTION_BUY) {
		if (!linked_wallet->has_currency(currency_id, price)) {
			print_line("Trader: Insufficient funds for rule '", rule->get_rule_name(), "'");
			return false;
		}
		if (!linked_wallet->remove_currency(currency_id, price))
			return false;
		if (!linked_wallet->add_item(item_id, amount)) {
			// rollback, item add failed (e.g. stack full)
			linked_wallet->add_currency(currency_id, price);
			return false;
		}
		emit_signal("rule_executed", rule->get_rule_name(), Variant(TradeRule::ACTION_BUY), item_id, amount);
	} else {
		if (!linked_wallet->has_item(item_id, amount)) {
			print_line("Trader: Insufficient stock for rule '", rule->get_rule_name(), "'");
			return false;
		}
		if (!linked_wallet->remove_item(item_id, amount))
			return false;
		linked_wallet->add_currency(currency_id, price);
		emit_signal("rule_executed", rule->get_rule_name(), Variant(TradeRule::ACTION_SELL), item_id, amount);
	}

	rule->last_fired_at = elapsed_time;
	return true;
}

void Trader::evaluate_trade_rules(float delta) {
	if (!_bind_economy_manager())
		return;

	elapsed_time += delta;

	auto eval_array = [&](const Array &rules) {
		for (int i = 0; i < rules.size(); ++i) {
			Ref<TradeRule> rule_ref = rules[i];
			if (!rule_ref.is_valid())
				continue;
			TradeRule *rule = rule_ref.ptr();
			if (_check_rule_conditions(rule))
				_execute_rule(rule);
		}
	};

	// profile rules first, instance rules on top
	if (profile.is_valid())
		eval_array(profile->get_rules());
	eval_array(instance_rules);
}

void Trader::_bind_methods() {
	// wallet
	ClassDB::bind_method(D_METHOD("get_wallet"), &Trader::get_wallet);

	// profile
	ClassDB::bind_method(D_METHOD("set_profile", "profile"), &Trader::set_profile);
	ClassDB::bind_method(D_METHOD("get_profile"), &Trader::get_profile);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "profile", PROPERTY_HINT_RESOURCE_TYPE, "TraderProfile"),
				 "set_profile", "get_profile");

	// instance rules
	ClassDB::bind_method(D_METHOD("get_instance_rules"), &Trader::get_instance_rules);
	ClassDB::bind_method(D_METHOD("set_instance_rules", "rules"), &Trader::set_instance_rules);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "instance_rules", PROPERTY_HINT_ARRAY_TYPE, "TradeRule"),
				 "set_instance_rules", "get_instance_rules");
	ClassDB::bind_method(D_METHOD("add_instance_rule", "rule"), &Trader::add_instance_rule);
	ClassDB::bind_method(D_METHOD("remove_instance_rule", "rule"), &Trader::remove_instance_rule);

	// accepted lists
	ClassDB::bind_method(D_METHOD("get_accepted_currencies"), &Trader::get_accepted_currencies);
	ClassDB::bind_method(D_METHOD("set_accepted_currencies", "currencies"), &Trader::set_accepted_currencies);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "accepted_currencies", PROPERTY_HINT_ARRAY_TYPE, "String"),
				 "set_accepted_currencies", "get_accepted_currencies");
	ClassDB::bind_method(D_METHOD("get_accepted_items"), &Trader::get_accepted_items);
	ClassDB::bind_method(D_METHOD("set_accepted_items", "items"), &Trader::set_accepted_items);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "accepted_items", PROPERTY_HINT_ARRAY_TYPE, "String"),
				 "set_accepted_items", "get_accepted_items");

	// rule engine
	ClassDB::bind_method(D_METHOD("evaluate_trade_rules", "delta"), &Trader::evaluate_trade_rules);

	ADD_SIGNAL(MethodInfo("rule_executed",
						  PropertyInfo(Variant::STRING, "rule_name"),
						  PropertyInfo(Variant::INT, "action"),
						  PropertyInfo(Variant::STRING, "item_id"),
						  PropertyInfo(Variant::INT, "amount")));
}
