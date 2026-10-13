#pragma once
#include "goap/goap_goal.hpp"
#include "goap/need.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

class Wallet;

// Holds the needs of one NPC and turns them into goals.
//
// The Need resources are definitions and can be shared between NPCs. The current
// value of each need is stored here, per NPC, keyed by need id.
//
// get_active_goals() returns one goal per urgent need, most important first,
// followed by the fallback goal. The fallback goal is always active and always
// last, whatever its own priority field says, so an NPC always has a goal.
class NeedsComponent : public Node {
	GDCLASS(NeedsComponent, Node)

public:
	NeedsComponent() {}
	~NeedsComponent() override {}

	TypedArray<Need> get_needs() const { return needs; }
	void set_needs(const TypedArray<Need> &v);

	Ref<GoapGoal> get_fallback_goal() const { return fallback_goal; }
	void set_fallback_goal(const Ref<GoapGoal> &v) { fallback_goal = v; }

	// When true, needs decay every frame. Turn it off to drive decay manually
	// through decay(), for example to throttle NPCs that are off screen.
	bool get_auto_decay() const { return auto_decay; }
	void set_auto_decay(bool v);

	void decay(float delta);

	Ref<Need> get_need(const String &need_id) const;
	float get_need_value(const String &need_id) const;
	void set_need_value(const String &need_id, float value);
	bool is_need_urgent(const String &need_id) const;

	// Removes one satisfier item of the need from the wallet and restores the need.
	// Returns false if the need has no satisfier or the wallet does not hold one.
	bool consume_for_need(const String &need_id, Wallet *wallet);

	// When true, a need at or below its consume threshold is satisfied from the linked
	// wallet automatically, every frame and after every manual decay() call.
	bool get_auto_consume() const { return auto_consume; }
	void set_auto_consume(bool v) { auto_consume = v; }

	// The wallet auto_consume takes items from. A GoapTrader links its own wallet.
	Wallet *get_wallet() const;
	void set_wallet(Wallet *wallet);

	// Consumes for every need at or below its consume threshold, while the wallet has a satisfier.
	// Returns how many items were consumed.
	int try_auto_consume();

	Array get_active_goals();
	Ref<GoapGoal> get_current_goal();

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	TypedArray<Need> needs;
	Ref<GoapGoal> fallback_goal;
	bool auto_decay = true;
	bool auto_consume = true;
	uint64_t wallet_id = 0;

	Dictionary values; // need id -> float
	Dictionary urgent_flags; // need id -> bool, used to emit urgency changes once
	Dictionary goals_by_need; // need id -> Ref<GoapGoal>, kept so a need keeps the same goal object

	void _sync_values();
	void _apply_value(const Ref<Need> &need, float value);
	Ref<GoapGoal> _goal_for(const Ref<Need> &need);
};
