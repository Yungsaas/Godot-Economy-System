#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// TradeRule, single automated trading decision, authored in the Godot editor
// Sentinel value -1 on any condition field disables that check
class TradeRule : public Resource {
	GDCLASS(TradeRule, Resource)

public:
	enum TradeAction {
		ACTION_BUY = 0,
		ACTION_SELL = 1,
	};

	// maps to supply ratio ranges: LOW < 0.33, NORMAL 0.33–0.66, HIGH > 0.66
	enum SupplyLevel {
		SUPPLY_ANY = 0,
		SUPPLY_LOW = 1,
		SUPPLY_NORMAL = 2,
		SUPPLY_HIGH = 3,
	};

	TradeRule() {}
	~TradeRule() override {}

	String get_rule_name() const { return rule_name; }
	void set_rule_name(const String &v) { rule_name = v; }

	bool get_enabled() const { return enabled; }
	void set_enabled(bool v) { enabled = v; }

	String get_item_id() const { return item_id; }
	void set_item_id(const String &v) { item_id = v; }

	TradeAction get_action() const { return action; }
	void set_action(TradeAction v) { action = v; }

	int get_trade_amount() const { return trade_amount; }
	void set_trade_amount(int v) { trade_amount = v; }

	String get_currency_id() const { return currency_id; }
	void set_currency_id(const String &v) { currency_id = v; }

	// buy fires when market price <= max, sell fires when market price >= min
	float get_buy_price_max() const { return buy_price_max; }
	void set_buy_price_max(float v) { buy_price_max = v; }
	float get_sell_price_min() const { return sell_price_min; }
	void set_sell_price_min(float v) { sell_price_min = v; }

	float get_min_currency_balance() const { return min_currency_balance; }
	void set_min_currency_balance(float v) { min_currency_balance = v; }

	// fires when stock is at or below threshold
	int get_restock_threshold() const { return restock_threshold; }
	void set_restock_threshold(int v) { restock_threshold = v; }

	// fires when stock is at or above threshold
	int get_surplus_threshold() const { return surplus_threshold; }
	void set_surplus_threshold(int v) { surplus_threshold = v; }

	SupplyLevel get_required_supply_level() const { return required_supply_level; }
	void set_required_supply_level(SupplyLevel v) { required_supply_level = v; }

	float get_cooldown_seconds() const { return cooldown_seconds; }
	void set_cooldown_seconds(float v) { cooldown_seconds = v; }

	// runtime only, not exported, managed by Trader
	float last_fired_at = -1.0f;

protected:
	static void _bind_methods();

private:
	String rule_name = "NewTradeRule";
	bool enabled = true;
	String item_id = "";
	TradeAction action = ACTION_BUY;
	int trade_amount = 1;
	String currency_id = "";
	float buy_price_max = -1.0f;
	float sell_price_min = -1.0f;
	float min_currency_balance = -1.0f;
	int restock_threshold = -1;
	int surplus_threshold = -1;
	SupplyLevel required_supply_level = SUPPLY_ANY;
	float cooldown_seconds = 0.0f;
};

VARIANT_ENUM_CAST(TradeRule::TradeAction)
VARIANT_ENUM_CAST(TradeRule::SupplyLevel)
