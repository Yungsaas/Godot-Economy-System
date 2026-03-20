#include "trade_rule.hpp"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void TradeRule::_bind_methods() {
	// enums
	BIND_ENUM_CONSTANT(ACTION_BUY);
	BIND_ENUM_CONSTANT(ACTION_SELL);
	BIND_ENUM_CONSTANT(SUPPLY_ANY);
	BIND_ENUM_CONSTANT(SUPPLY_LOW);
	BIND_ENUM_CONSTANT(SUPPLY_NORMAL);
	BIND_ENUM_CONSTANT(SUPPLY_HIGH);

	// identity
	ClassDB::bind_method(D_METHOD("get_rule_name"), &TradeRule::get_rule_name);
	ClassDB::bind_method(D_METHOD("set_rule_name", "name"), &TradeRule::set_rule_name);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "rule_name"), "set_rule_name", "get_rule_name");

	ClassDB::bind_method(D_METHOD("get_enabled"), &TradeRule::get_enabled);
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &TradeRule::set_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");

	// what to trade
	ClassDB::bind_method(D_METHOD("get_item_id"), &TradeRule::get_item_id);
	ClassDB::bind_method(D_METHOD("set_item_id", "item_id"), &TradeRule::set_item_id);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "item_id"), "set_item_id", "get_item_id");

	ClassDB::bind_method(D_METHOD("get_action"), &TradeRule::get_action);
	ClassDB::bind_method(D_METHOD("set_action", "action"), &TradeRule::set_action);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "action", PROPERTY_HINT_ENUM, "Buy,Sell"), "set_action", "get_action");

	ClassDB::bind_method(D_METHOD("get_trade_amount"), &TradeRule::get_trade_amount);
	ClassDB::bind_method(D_METHOD("set_trade_amount", "amount"), &TradeRule::set_trade_amount);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "trade_amount"), "set_trade_amount", "get_trade_amount");

	ClassDB::bind_method(D_METHOD("get_currency_id"), &TradeRule::get_currency_id);
	ClassDB::bind_method(D_METHOD("set_currency_id", "currency_id"), &TradeRule::set_currency_id);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "currency_id"), "set_currency_id", "get_currency_id");

	// price thresholds
	ClassDB::bind_method(D_METHOD("get_buy_price_max"), &TradeRule::get_buy_price_max);
	ClassDB::bind_method(D_METHOD("set_buy_price_max", "price"), &TradeRule::set_buy_price_max);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "buy_price_max", PROPERTY_HINT_RANGE, "-1,99999,0.01"),
				 "set_buy_price_max", "get_buy_price_max");

	ClassDB::bind_method(D_METHOD("get_sell_price_min"), &TradeRule::get_sell_price_min);
	ClassDB::bind_method(D_METHOD("set_sell_price_min", "price"), &TradeRule::set_sell_price_min);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "sell_price_min", PROPERTY_HINT_RANGE, "-1,99999,0.01"),
				 "set_sell_price_min", "get_sell_price_min");

	// wallet / inventory conditions
	ClassDB::bind_method(D_METHOD("get_min_currency_balance"), &TradeRule::get_min_currency_balance);
	ClassDB::bind_method(D_METHOD("set_min_currency_balance", "balance"), &TradeRule::set_min_currency_balance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_currency_balance", PROPERTY_HINT_RANGE, "-1,99999,0.01"),
				 "set_min_currency_balance", "get_min_currency_balance");

	ClassDB::bind_method(D_METHOD("get_restock_threshold"), &TradeRule::get_restock_threshold);
	ClassDB::bind_method(D_METHOD("set_restock_threshold", "threshold"), &TradeRule::set_restock_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "restock_threshold", PROPERTY_HINT_RANGE, "-1,9999,1"),
				 "set_restock_threshold", "get_restock_threshold");

	ClassDB::bind_method(D_METHOD("get_surplus_threshold"), &TradeRule::get_surplus_threshold);
	ClassDB::bind_method(D_METHOD("set_surplus_threshold", "threshold"), &TradeRule::set_surplus_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "surplus_threshold", PROPERTY_HINT_RANGE, "-1,9999,1"),
				 "set_surplus_threshold", "get_surplus_threshold");

	// supply level
	ClassDB::bind_method(D_METHOD("get_required_supply_level"), &TradeRule::get_required_supply_level);
	ClassDB::bind_method(D_METHOD("set_required_supply_level", "level"), &TradeRule::set_required_supply_level);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "required_supply_level", PROPERTY_HINT_ENUM, "Any,Low,Normal,High"),
				 "set_required_supply_level", "get_required_supply_level");

	// cooldown
	ClassDB::bind_method(D_METHOD("get_cooldown_seconds"), &TradeRule::get_cooldown_seconds);
	ClassDB::bind_method(D_METHOD("set_cooldown_seconds", "seconds"), &TradeRule::set_cooldown_seconds);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cooldown_seconds", PROPERTY_HINT_RANGE, "0,3600,0.1"),
				 "set_cooldown_seconds", "get_cooldown_seconds");
}
