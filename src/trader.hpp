#pragma once

#include "trade_rule.hpp"
#include "trader_profile.hpp"
#include "wallet.hpp"

#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/node_path.hpp"

using namespace godot;

// forward declaration — breaks circular include with trade_market.hpp
class TradeMarket;

class Trader : public Node {
	GDCLASS(Trader, Node)

public:
	Trader();

	Wallet *get_wallet() const { return linked_wallet; }

	void set_profile(const Ref<TraderProfile> &p);
	Ref<TraderProfile> get_profile() const { return profile; }

	// per-instance rules evaluated after profile rules
	Array get_instance_rules() const { return instance_rules; }
	void set_instance_rules(const Array &v) { instance_rules = v; }
	void add_instance_rule(const Ref<TradeRule> &rule) { instance_rules.push_back(rule); }
	void remove_instance_rule(const Ref<TradeRule> &rule) { instance_rules.erase(rule); }

	// extends (does not replace) profile accepted lists
	Array get_accepted_currencies() const { return accepted_currencies; }
	void set_accepted_currencies(const Array &v) { accepted_currencies = v; }
	Array get_accepted_items() const { return accepted_items; }
	void set_accepted_items(const Array &v) { accepted_items = v; }

	// market this trader belongs to, resolved lazily via NodePath
	void set_market(const NodePath &p) {
		market_path = p;
		trade_market = nullptr;
	}
	NodePath get_market() const { return market_path; }

	// used by TradeMarket during seller search
	bool has_sell_rule_for(const String &item_id) const;
	float get_sell_price_for(const String &item_id) const;

	void evaluate_trade_rules(float delta);

	void _ready() override;

protected:
	static void _bind_methods();

private:
	bool _check_rule_conditions(TradeRule *rule) const;
	bool _execute_rule(TradeRule *rule);
	float _get_market_price(const String &item_id) const;
	static TradeRule::SupplyLevel _supply_ratio_to_level(float ratio);
	bool _accepts_currency(const String &id) const;
	bool _accepts_item(const String &id) const;
	bool _bind_economy_manager();
	bool _bind_trade_market();

	Wallet *linked_wallet = nullptr;
	Ref<TraderProfile> profile;
	Array instance_rules;
	Array accepted_currencies;
	Array accepted_items;
	float elapsed_time = 0.0f; // accumulated for cooldown checks
	EconomyManager *economy_manager = nullptr;
	TradeMarket *trade_market = nullptr;
	NodePath market_path;
};
