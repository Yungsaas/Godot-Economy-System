#pragma once
#include "trade_rule.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// TraderProfile, shared rule set, assign to multiple Trader nodes for a common archetype
// Profile rules evaluate before per-instance rules
class TraderProfile : public Resource {
	GDCLASS(TraderProfile, Resource)

public:
	TraderProfile() {}
	~TraderProfile() override {}

	String get_profile_name() const { return profile_name; }
	void set_profile_name(const String &v) { profile_name = v; }

	// default accepted lists, Trader instance lists extend these
	Array get_accepted_currencies() const { return accepted_currencies; }
	void set_accepted_currencies(const Array &v) { accepted_currencies = v; }
	Array get_accepted_items() const { return accepted_items; }
	void set_accepted_items(const Array &v) { accepted_items = v; }

	// rules evaluated index-0 first; all matching rules fire
	Array get_rules() const { return rules; }
	void set_rules(const Array &v) { rules = v; }
	void add_rule(const Ref<TradeRule> &rule) { rules.push_back(rule); }
	void remove_rule(const Ref<TradeRule> &rule) { rules.erase(rule); }

protected:
	static void _bind_methods();

private:
	String profile_name = "DefaultProfile";
	Array accepted_currencies;
	Array accepted_items;
	Array rules;
};
