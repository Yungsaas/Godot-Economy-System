#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// A condition a GoapTrader wants to be true: hold at least a given amount of
// some items and some currencies.
//
// item_targets:     item id     -> minimum amount (int)
// currency_targets: currency id -> minimum amount (float)
//
// Goals are either authored directly (for example the fallback goal on a NeedsComponent)
// or produced by a NeedsComponent from a Need.
class GoapGoal : public Resource {
	GDCLASS(GoapGoal, Resource)

public:
	GoapGoal() {}
	~GoapGoal() override {}

	String get_goal_name() const { return goal_name; }
	void set_goal_name(const String &v) { goal_name = v; }

	// Higher means more important.
	int get_priority() const { return priority; }
	void set_priority(int v) { priority = v; }

	Dictionary get_item_targets() const { return item_targets; }
	void set_item_targets(const Dictionary &v) { item_targets = v; }

	Dictionary get_currency_targets() const { return currency_targets; }
	void set_currency_targets(const Dictionary &v) { currency_targets = v; }

	void set_item_target(const String &item_id, int amount);
	int get_item_target(const String &item_id) const;
	void set_currency_target(const String &currency_id, float amount);
	float get_currency_target(const String &currency_id) const;
	void clear_targets();

protected:
	static void _bind_methods();

private:
	String goal_name = "NewGoal";
	int priority = 0;
	Dictionary item_targets;
	Dictionary currency_targets;
};
