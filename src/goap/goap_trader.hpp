#pragma once
#include "goap/goap_action.hpp"
#include "goap/goap_plan.hpp"
#include "goap/goap_planner.hpp"
#include "goap/needs_component.hpp"
#include "wallet.hpp"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

using namespace godot;

class EconomyManager;

// An NPC that trades by planning towards the goals produced by its needs.
//
// Children, found by type among the direct children and created if missing:
//   Wallet          what it owns, and the world state the planner starts from
//   NeedsComponent  what it wants, as goals in order of priority
//   GoapPlanner     the search
//
// The extension decides what to do, the developer decides how it looks. With auto_run on,
// the trader plans by itself and starts the first step of its plan:
//
//   step_started(action, partner)   the developer's code carries the step out, for
//                                   example by walking to the partner or to a field,
//                                   and then calls complete_step() or fail_step(reason)
//
// complete_step() applies the action's effects to the wallets. For a custom action that
// is consumes and produces on this trader's own wallet. For a trade action, the partner
// is asked to accept the offer first, and the goods change hands only if it does.
// After every step the trader plans again from the new state.
//
// With instant_steps on, every step completes the moment it starts, without any
// developer code. Useful for simulations without visuals and for NPCs off screen.
//
// A trader that is offered a trade accepts it if it makes its own plan cheaper, refuses
// it if it makes its plan more expensive, and when the plan cost does not change,
// accepts only if what it receives is worth at least as much as what it gives.
class GoapTrader : public Node {
	GDCLASS(GoapTrader, Node)

public:
	GoapTrader() {}
	~GoapTrader() override {}

	Wallet *get_wallet() const;
	NeedsComponent *get_needs() const;
	GoapPlanner *get_planner() const;

	TypedArray<GoapAction> get_actions() const { return actions; }
	void set_actions(const TypedArray<GoapAction> &v) { actions = v; }
	void add_action(const Ref<GoapAction> &action);
	void remove_action(const Ref<GoapAction> &action);

	// --- Settings
	bool get_auto_run() const { return auto_run; }
	void set_auto_run(bool v);
	bool get_instant_steps() const { return instant_steps; }
	void set_instant_steps(bool v) { instant_steps = v; }
	float get_replan_interval() const { return replan_interval; }
	void set_replan_interval(float v) { replan_interval = v < 0.0f ? 0.0f : v; }
	float get_refusal_memory_seconds() const { return refusal_memory_seconds; }
	void set_refusal_memory_seconds(float v) { refusal_memory_seconds = v < 0.0f ? 0.0f : v; }
	float get_refusal_penalty() const { return refusal_penalty; }
	void set_refusal_penalty(float v) { refusal_penalty = v < 0.0f ? 0.0f : v; }
	// Traders only trade with other traders in the same group.
	String get_market_group() const { return market_group; }
	void set_market_group(const String &v);
	// How much one unit of plan cost is worth to this trader, in the value units of base_value.
	// A trader sells something its plan still needs if the payment covers the extra work of
	// replacing it. 0 means it never does; it then only sells surplus.
	float get_work_value() const { return work_value; }
	void set_work_value(float v) { work_value = v < 0.0f ? 0.0f : v; }

	// --- Demand
	// When nothing in the market can serve the first step of a plan, the trader posts a demand
	// for the missing goods and works on something else. Traders with a custom action that
	// produces those goods see the demand and get a goal to hold them.
	float get_demand_duration() const { return demand_duration; }
	void set_demand_duration(float v) { demand_duration = v < 0.0f ? 0.0f : v; }
	// Priority of the goal this trader gets from another trader's demand, compared with the
	// priorities of its needs. The fallback goal always stays last.
	int get_demand_priority() const { return demand_priority; }
	void set_demand_priority(int v) { demand_priority = v; }
	void post_demand(const String &item_id, float amount);
	void clear_demand(const String &item_id);
	// Demands this trader has posted and that have not expired: item id -> amount.
	Dictionary get_active_demands();

	// --- Planning
	Ref<GoapPlan> make_plan();
	Ref<GoapPlan> get_current_plan() const { return current_plan; }

	// --- Executing
	// Plans and starts the first step. Returns false if there is nothing to do.
	// Called automatically when auto_run is on.
	bool start_plan();
	void complete_step();
	void fail_step(const String &reason);
	bool is_executing() const { return executing; }
	Ref<GoapAction> get_current_step() const;
	int get_current_step_index() const { return current_step_index; }
	GoapTrader *get_current_partner() const;

	// Extra cost the planner currently adds to an action because it failed recently.
	float get_action_penalty(const Ref<GoapAction> &action) const;

	// --- Offers, seen from this trader as the one receiving the offer.
	// they_give: what this trader would receive. they_want: what this trader would give.
	// Returns {accepted: bool, reason: String}. Changes nothing.
	Dictionary evaluate_offer(const Dictionary &they_give, const Dictionary &they_want);
	// Evaluates and, if accepted, exchanges the goods with from_wallet. Works with any
	// wallet, so a player can trade with a GoapTrader too.
	Dictionary receive_offer(Wallet *from_wallet, const Dictionary &they_give, const Dictionary &they_want);
	String get_last_offer_reason() const { return last_offer_reason; }

protected:
	static void _bind_methods();
	void _notification(int p_what);

private:
	TypedArray<GoapAction> actions;
	Ref<GoapPlan> current_plan;

	bool auto_run = true;
	bool instant_steps = false;
	float replan_interval = 1.0f;
	float refusal_memory_seconds = 10.0f;
	float refusal_penalty = 5.0f;
	String market_group = "goap_traders";
	float work_value = 1.0f;
	float demand_duration = 20.0f;
	int demand_priority = 1;
	Dictionary posted_demands; // item id -> Dictionary { amount, expires }
	Dictionary demand_goals; // item id -> Ref<GoapGoal>, so a demand keeps the same goal object

	bool executing = false;
	int current_step_index = -1;
	int plan_progress = 0; // index of the first step of current_plan that has not been completed
	uint64_t current_partner_id = 0;
	bool fresh_plan = false; // current_plan was produced by an accepted offer and has not been started
	float replan_cooldown = 0.0f;
	double sim_time = 0.0; // seconds this trader has processed, scaled by Engine.time_scale

	Dictionary refusals; // GoapAction -> sim_time at which the penalty expires
	String last_offer_reason;

	uint64_t wallet_id = 0;
	uint64_t needs_id = 0;
	uint64_t planner_id = 0;

	void _link_children();
	template <typename T>
	T *_get_child_by_id(uint64_t id) const;
	EconomyManager *_get_manager() const;

	Dictionary _active_penalties();
	void _start_step(int index);
	void _end_step();
	GoapTrader *_find_partner(const Ref<GoapAction> &action, String *reason);
	Ref<GoapGoal> _goal_for_offers() const;
	float _remaining_plan_cost() const;
	Dictionary _offer_result(bool accepted, const String &reason, const Ref<GoapPlan> &plan);

	Array _goals_in_order(NeedsComponent *needs);
	Ref<GoapPlan> _plan_around_market(const Ref<GoapGoal> &goal, Wallet *wallet, GoapPlanner *planner, const Dictionary &penalties);
	bool _market_has(const Dictionary &amounts);
	bool _can_produce(const String &item_id) const;
	Dictionary _collect_market_demands();
	void _post_demands_for(const Dictionary &amounts);
	void _clear_demands_for(const Dictionary &amounts);
};
