#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// A single thing a GoapTrader can do, authored in the inspector.
//
// An action is described only by what it needs and what it changes, never by the goal
// it serves, so it stays valid for any content registered later:
//
//   requires: id -> amount that must be held, but is not used up (a tool, a licence)
//   consumes: id -> amount that must be held and is removed
//   produces: id -> amount that is added
//
// Ids can be item ids or currency ids. Examples:
//   Farm grain    (custom): produces {grain: 2}
//   Bake bread    (custom): consumes {grain: 2}, produces {bread: 1}
//   Chop wood     (custom): requires {axe: 1},   produces {wood: 1}
//   Buy water     (trade):  consumes {gold: 2},  produces {water: 1}
//
// A trade action needs another trader to give what it produces and receive what it
// consumes. A custom action is carried out by the developer's own code.
//
// Conditions that data cannot express can be added by attaching a script that
// extends GoapAction and defines either of these methods. Both are asked once per
// plan, not for every state the planner explores:
//   func _is_available(trader) -> bool   # false removes the action from planning
//   func _get_cost(trader) -> float      # replaces the cost field
class GoapAction : public Resource {
	GDCLASS(GoapAction, Resource)

public:
	enum Kind {
		KIND_CUSTOM = 0,
		KIND_TRADE = 1,
	};

	GoapAction() {}
	~GoapAction() override {}

	String get_action_name() const { return action_name; }
	void set_action_name(const String &v) { action_name = v; }

	Kind get_kind() const { return kind; }
	void set_kind(Kind v) { kind = v; }

	Dictionary get_requires() const { return required_amounts; }
	void set_requires(const Dictionary &v) { required_amounts = v; }

	Dictionary get_consumes() const { return consumed_amounts; }
	void set_consumes(const Dictionary &v) { consumed_amounts = v; }

	Dictionary get_produces() const { return produced_amounts; }
	void set_produces(const Dictionary &v) { produced_amounts = v; }

	// Planner cost of taking this action. Must be positive for A* to be correct.
	float get_cost() const { return cost; }
	void set_cost(float v) { cost = v < MIN_COST ? MIN_COST : v; }

	// Ask the optional script overrides. Called by the planner once per plan.
	bool is_available_for(Object *trader);
	float get_cost_for(Object *trader);

	static constexpr float MIN_COST = 0.001f;

protected:
	static void _bind_methods();

private:
	String action_name = "NewAction";
	Kind kind = KIND_CUSTOM;
	Dictionary required_amounts;
	Dictionary consumed_amounts;
	Dictionary produced_amounts;
	float cost = 1.0f;
};

VARIANT_ENUM_CAST(GoapAction::Kind)
