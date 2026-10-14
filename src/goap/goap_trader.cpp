#include "goap/goap_trader.hpp"
#include "economy_manager.hpp"
#include "goap/economy_amounts.hpp"
#include "profiling.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

using namespace godot;

namespace {
// Stands in for "this goal cannot be reached" when comparing plan costs.
constexpr float UNREACHABLE = 1e30f;
constexpr float COST_EPSILON = 1e-4f;

String cost_text(float cost) {
	return cost >= UNREACHABLE ? String("unreachable") : String::num(cost, 1);
}
} // namespace

// ---------------------------------------------------------------- setup

void GoapTrader::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			if (!market_group.is_empty()) {
				add_to_group(market_group);
			}
		} break;
		case NOTIFICATION_READY: {
			// Done in a notification rather than _ready, so a script that overrides
			// _ready without calling super() does not break the linking.
			_link_children();
			if (!Engine::get_singleton()->is_editor_hint()) {
				set_process(auto_run);
			}
		} break;
		case NOTIFICATION_PROCESS: {
			ZoneScopedN("GoapTrader::process");
			const double delta = get_process_delta_time();
			sim_time += delta;
			if (!auto_run || executing) {
				return;
			}
			if (fresh_plan && current_plan.is_valid() && current_plan->is_found()) {
				// An accepted offer already produced the next plan, no need to search again.
				fresh_plan = false;
				_start_step(0);
				return;
			}
			replan_cooldown -= (float)delta;
			if (replan_cooldown > 0.0f) {
				return;
			}
			replan_cooldown = replan_interval;
			start_plan();
		} break;
	}
}

template <typename T>
T *GoapTrader::_get_child_by_id(uint64_t id) const {
	T *node = Object::cast_to<T>(ObjectDB::get_instance(id));
	if (!node || node->get_parent() != this) {
		return nullptr;
	}
	return node;
}

void GoapTrader::_link_children() {
	wallet_id = 0;
	needs_id = 0;
	planner_id = 0;
	for (int i = 0; i < get_child_count(); ++i) {
		Node *child = get_child(i);
		if (wallet_id == 0 && Object::cast_to<Wallet>(child)) {
			wallet_id = child->get_instance_id();
		} else if (needs_id == 0 && Object::cast_to<NeedsComponent>(child)) {
			needs_id = child->get_instance_id();
		} else if (planner_id == 0 && Object::cast_to<GoapPlanner>(child)) {
			planner_id = child->get_instance_id();
		}
	}

	// In the editor nothing is created, so the edited scene is not changed behind the user's back.
	if (Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (wallet_id == 0) {
		Wallet *wallet = memnew(Wallet);
		wallet->set_name("Wallet");
		add_child(wallet);
		wallet_id = wallet->get_instance_id();
		print_line("GoapTrader \"", get_name(), "\": no Wallet child found, one was created automatically.");
	}
	if (needs_id == 0) {
		NeedsComponent *needs = memnew(NeedsComponent);
		needs->set_name("NeedsComponent");
		add_child(needs);
		needs_id = needs->get_instance_id();
		print_line("GoapTrader \"", get_name(), "\": no NeedsComponent child found, one was created automatically.");
	}
	if (planner_id == 0) {
		GoapPlanner *planner = memnew(GoapPlanner);
		planner->set_name("GoapPlanner");
		add_child(planner);
		planner_id = planner->get_instance_id();
	}

	// The needs consume from this trader's own wallet.
	get_needs()->set_wallet(get_wallet());
}

Wallet *GoapTrader::get_wallet() const {
	return _get_child_by_id<Wallet>(wallet_id);
}

NeedsComponent *GoapTrader::get_needs() const {
	return _get_child_by_id<NeedsComponent>(needs_id);
}

GoapPlanner *GoapTrader::get_planner() const {
	return _get_child_by_id<GoapPlanner>(planner_id);
}

EconomyManager *GoapTrader::_get_manager() const {
	if (!is_inside_tree()) {
		return nullptr;
	}
	return Object::cast_to<EconomyManager>(get_tree()->get_first_node_in_group("economy_manager"));
}

void GoapTrader::add_action(const Ref<GoapAction> &action) {
	if (action.is_valid() && !actions.has(action)) {
		actions.push_back(action);
	}
}

void GoapTrader::remove_action(const Ref<GoapAction> &action) {
	actions.erase(action);
}

void GoapTrader::set_auto_run(bool v) {
	auto_run = v;
	if (is_inside_tree() && !Engine::get_singleton()->is_editor_hint()) {
		set_process(v);
	}
}

void GoapTrader::set_market_group(const String &v) {
	if (is_inside_tree() && !market_group.is_empty()) {
		remove_from_group(market_group);
	}
	market_group = v;
	if (is_inside_tree() && !market_group.is_empty()) {
		add_to_group(market_group);
	}
}

// ---------------------------------------------------------------- planning

Dictionary GoapTrader::_active_penalties() {
	Dictionary active;
	Array keys = refusals.keys();
	for (int i = 0; i < keys.size(); ++i) {
		double expires = refusals[keys[i]];
		if (expires > sim_time) {
			active[keys[i]] = refusal_penalty;
		} else {
			refusals.erase(keys[i]);
		}
	}
	return active;
}

float GoapTrader::get_action_penalty(const Ref<GoapAction> &action) const {
	double expires = refusals.get(action, -1.0);
	return expires > sim_time ? refusal_penalty : 0.0f;
}

Ref<GoapPlan> GoapTrader::make_plan() {
	ZoneScopedN("GoapTrader::make_plan");
	Wallet *wallet = get_wallet();
	NeedsComponent *needs = get_needs();
	GoapPlanner *planner = get_planner();

	Ref<GoapPlan> result;
	if (!wallet || !needs || !planner) {
		result.instantiate();
		result->result = GoapPlan::RESULT_INVALID;
		result->message = "GoapTrader \"" + String(get_name()) + "\" is missing its Wallet, NeedsComponent or GoapPlanner.";
	} else {
		Dictionary penalties = _active_penalties();
		Array goals = _goals_in_order(needs);
		Ref<GoapPlan> first_failure;
		Ref<GoapPlan> first_satisfied;
		for (int i = 0; i < goals.size(); ++i) {
			Ref<GoapPlan> plan = _plan_around_market(goals[i], wallet, planner, penalties);
			if (plan->get_result() == GoapPlan::RESULT_FOUND) {
				result = plan;
				break;
			}
			if (plan->get_result() == GoapPlan::RESULT_ALREADY_SATISFIED) {
				if (first_satisfied.is_null()) {
					first_satisfied = plan;
				}
			} else if (first_failure.is_null()) {
				first_failure = plan;
			}
		}
		if (result.is_null()) {
			result = first_failure.is_valid() ? first_failure : first_satisfied;
		}
		if (result.is_null()) {
			result.instantiate();
			result->result = GoapPlan::RESULT_INVALID;
			result->message = "There are no active goals.";
		}
	}

	// While a step runs, the plan it belongs to must stay in place, so a plan made now is
	// only returned. The trader plans again by itself once the step ends.
	if (executing) {
		return result;
	}
	current_plan = result;
	plan_progress = 0;
	fresh_plan = false;
	emit_signal("plan_changed", current_plan);
	return current_plan;
}

// ---------------------------------------------------------------- executing

bool GoapTrader::start_plan() {
	if (executing) {
		return false;
	}
	Ref<GoapPlan> plan = make_plan();
	if (!plan->is_found()) {
		return false;
	}
	_start_step(0);
	return true;
}

Ref<GoapAction> GoapTrader::get_current_step() const {
	if (!executing || current_plan.is_null() || current_step_index < 0 || current_step_index >= current_plan->get_step_count()) {
		return Ref<GoapAction>();
	}
	return current_plan->get_steps()[current_step_index];
}

GoapTrader *GoapTrader::get_current_partner() const {
	return Object::cast_to<GoapTrader>(ObjectDB::get_instance(current_partner_id));
}

void GoapTrader::_start_step(int index) {
	if (current_plan.is_null() || index < 0 || index >= current_plan->get_step_count()) {
		return;
	}
	Ref<GoapAction> action = current_plan->get_steps()[index];
	executing = true;
	current_step_index = index;
	current_partner_id = 0;
	plan_progress = index;

	GoapTrader *partner = nullptr;
	if (action->get_kind() == GoapAction::KIND_TRADE) {
		String reason;
		partner = _find_partner(action, &reason);
		if (!partner) {
			// Reported as a failed step without step_started, so the developer's code
			// never starts walking towards a trade that cannot happen.
			fail_step(reason);
			return;
		}
		current_partner_id = partner->get_instance_id();
	}

	emit_signal("step_started", action, partner);

	// The developer's code may already have completed or failed the step inside the signal.
	if (instant_steps && executing && current_step_index == index) {
		complete_step();
	}
}

void GoapTrader::complete_step() {
	ZoneScopedN("GoapTrader::complete_step");
	if (!executing) {
		print_error("GoapTrader \"", get_name(), "\": complete_step() was called while no step is running.");
		return;
	}
	Ref<GoapAction> action = get_current_step();
	if (action.is_null()) {
		fail_step("the plan changed while the step was running");
		return;
	}
	EconomyManager *manager = _get_manager();
	Wallet *wallet = get_wallet();
	String problem;

	if (action->get_kind() == GoapAction::KIND_TRADE) {
		GoapTrader *partner = get_current_partner();
		if (!partner) {
			fail_step("the trade partner is gone");
			return;
		}
		Dictionary result = partner->receive_offer(wallet, action->get_consumes(), action->get_produces());
		if (!(bool)result.get("accepted", false)) {
			fail_step(String(partner->get_name()) + " refused: " + String(result.get("reason", "")));
			return;
		}
		emit_signal("trade_completed", partner, action->get_consumes(), action->get_produces());
		_clear_demands_for(action->get_produces());
	} else {
		if (!economy_amounts::has_all(wallet, action->get_requires(), manager, &problem)) {
			fail_step(problem);
			return;
		}
		if (!economy_amounts::apply(wallet, action->get_consumes(), action->get_produces(), manager, &problem)) {
			fail_step(problem);
			return;
		}
	}

	plan_progress = current_step_index + 1;
	emit_signal("step_completed", action);
	_end_step();
}

void GoapTrader::fail_step(const String &reason) {
	if (!executing) {
		print_error("GoapTrader \"", get_name(), "\": fail_step() was called while no step is running.");
		return;
	}
	Ref<GoapAction> action = get_current_step();
	if (action.is_valid()) {
		// Makes the action more expensive for a while, so the next plan avoids it if it can.
		refusals[action] = sim_time + refusal_memory_seconds;
	}
	emit_signal("step_failed", action, reason);
	_end_step();
	// Wait before planning again. Planning on the next frame is right after a success, but
	// after a failure the same plan is often the only option, and retrying it every frame
	// would only fail again at frame rate.
	replan_cooldown = replan_interval;
}

void GoapTrader::_end_step() {
	executing = false;
	current_step_index = -1;
	current_partner_id = 0;
	// Plan again on the next frame, from the state the step left behind.
	replan_cooldown = 0.0f;
}

GoapTrader *GoapTrader::_find_partner(const Ref<GoapAction> &action, String *reason) {
	ZoneScopedN("GoapTrader::_find_partner");
	EconomyManager *manager = _get_manager();
	String problem;
	if (!economy_amounts::has_all(get_wallet(), action->get_consumes(), manager, &problem)) {
		*reason = "this trader " + problem;
		return nullptr;
	}
	if (!is_inside_tree() || market_group.is_empty()) {
		*reason = "this trader is not in a market";
		return nullptr;
	}

	// Candidates are traders in the same market that hold what this trader wants.
	// A script on this trader can order them with _rank_partner(partner, action) -> float,
	// lower first, for example by distance. Without it, tree order is used.
	const bool has_rank = has_method("_rank_partner");
	std::vector<std::pair<float, GoapTrader *>> candidates;
	TypedArray<Node> members = get_tree()->get_nodes_in_group(market_group);
	for (int i = 0; i < members.size(); ++i) {
		GoapTrader *other = Object::cast_to<GoapTrader>(members[i]);
		if (!other || other == this || !other->get_wallet()) {
			continue;
		}
		if (!economy_amounts::has_all(other->get_wallet(), action->get_produces(), manager)) {
			continue;
		}
		float rank = has_rank ? (float)call("_rank_partner", other, action) : (float)i;
		candidates.emplace_back(rank, other);
	}
	if (candidates.empty()) {
		_post_demands_for(action->get_produces());
		*reason = "nobody has " + economy_amounts::to_text(action->get_produces());
		return nullptr;
	}
	std::stable_sort(candidates.begin(), candidates.end(),
			[](const std::pair<float, GoapTrader *> &a, const std::pair<float, GoapTrader *> &b) { return a.first < b.first; });

	// Asked before the developer's code starts walking, so the trader goes to someone who
	// would accept. The partner asks itself again when the trader arrives.
	String last_refusal;
	for (const auto &candidate : candidates) {
		Dictionary result = candidate.second->evaluate_offer(action->get_consumes(), action->get_produces());
		if ((bool)result.get("accepted", false)) {
			return candidate.second;
		}
		last_refusal = String(candidate.second->get_name()) + ": " + String(result.get("reason", ""));
	}
	*reason = "nobody accepts, last answer was " + last_refusal;
	return nullptr;
}

// ---------------------------------------------------------------- offers

Ref<GoapGoal> GoapTrader::_goal_for_offers() const {
	if (current_plan.is_valid() && current_plan->get_goal().is_valid()) {
		return current_plan->get_goal();
	}
	NeedsComponent *needs = get_needs();
	return needs ? needs->get_current_goal() : Ref<GoapGoal>();
}

float GoapTrader::_remaining_plan_cost() const {
	if (current_plan.is_null()) {
		return UNREACHABLE;
	}
	switch (current_plan->get_result()) {
		case GoapPlan::RESULT_FOUND:
			return current_plan->get_cost_from(plan_progress);
		case GoapPlan::RESULT_ALREADY_SATISFIED:
			return 0.0f;
		default:
			return UNREACHABLE;
	}
}

Dictionary GoapTrader::_offer_result(bool accepted, const String &reason, const Ref<GoapPlan> &plan) {
	Dictionary result;
	result["accepted"] = accepted;
	result["reason"] = reason;
	result["plan"] = plan;
	return result;
}

Dictionary GoapTrader::evaluate_offer(const Dictionary &they_give, const Dictionary &they_want) {
	ZoneScopedN("GoapTrader::evaluate_offer");
	EconomyManager *manager = _get_manager();
	Wallet *wallet = get_wallet();
	GoapPlanner *planner = get_planner();
	if (!wallet || !planner) {
		return _offer_result(false, "has no wallet or planner", Ref<GoapPlan>());
	}

	String problem;
	if (!economy_amounts::has_all(wallet, they_want, manager, &problem)) {
		return _offer_result(false, problem, Ref<GoapPlan>());
	}
	if (!economy_amounts::can_receive(wallet, they_give, manager, &problem)) {
		return _offer_result(false, problem, Ref<GoapPlan>());
	}

	const float receive_value = economy_amounts::value_of(they_give, manager);
	const float give_value = economy_amounts::value_of(they_want, manager);
	const String trade_text = economy_amounts::to_text(they_want) + " for " + economy_amounts::to_text(they_give);

	Ref<GoapGoal> goal = _goal_for_offers();
	float cost_now = 0.0f;
	float cost_after = 0.0f;
	Ref<GoapPlan> plan_after;
	String goal_name = "no goal";

	if (goal.is_valid()) {
		goal_name = goal->get_goal_name();
		Dictionary penalties = _active_penalties();

		// What the plan costs without the offer. Always planned fresh, with the same penalties
		// as the plan after the offer, so both costs are measured the same way. Reusing the rest
		// of the current plan was cheaper, but that plan can be stale: it may have been made
		// before a penalty was added, or partly carried out since.
		Ref<GoapPlan> plan_now = planner->plan_for_wallet(goal, wallet, actions, this, penalties);
		cost_now = plan_now->is_found() ? plan_now->get_total_cost()
										: (plan_now->get_result() == GoapPlan::RESULT_ALREADY_SATISFIED ? 0.0f : UNREACHABLE);

		// What the plan would cost after the offer: plan on a copy of the state with the offer applied.
		Dictionary state = planner->build_world_state(wallet);
		Array give_keys = they_want.keys();
		for (int i = 0; i < give_keys.size(); ++i) {
			state[give_keys[i]] = (double)state.get(give_keys[i], 0.0) - (double)they_want[give_keys[i]];
		}
		Array get_keys = they_give.keys();
		for (int i = 0; i < get_keys.size(); ++i) {
			state[get_keys[i]] = (double)state.get(get_keys[i], 0.0) + (double)they_give[get_keys[i]];
		}
		plan_after = planner->plan_from_state(goal, state, actions, this, penalties);
		cost_after = plan_after->is_found() ? plan_after->get_total_cost()
											: (plan_after->get_result() == GoapPlan::RESULT_ALREADY_SATISFIED ? 0.0f : UNREACHABLE);
	}

	if (cost_after < cost_now - COST_EPSILON) {
		String why = cost_now >= UNREACHABLE
				? "makes \"" + goal_name + "\" reachable"
				: "plan for \"" + goal_name + "\" gets cheaper (" + cost_text(cost_now) + " -> " + cost_text(cost_after) + ")";
		return _offer_result(true, trade_text + String(": ") + why, plan_after);
	}
	if (cost_after > cost_now + COST_EPSILON) {
		if (cost_after >= UNREACHABLE) {
			// Something this trader cannot replace is never sold, whatever the price.
			return _offer_result(false, trade_text + String(": would make \"") + goal_name + "\" unreachable", plan_after);
		}
		// The plan still works but needs more work. Accept if the payment covers that work.
		const float extra_work = cost_after - cost_now;
		const float work_price = extra_work * work_value;
		if (work_value > 0.0f && receive_value + COST_EPSILON >= work_price) {
			return _offer_result(true, trade_text + String(": payment ") + String::num(receive_value, 1) +
							" covers the extra work for \"" + goal_name + "\" (" + cost_text(cost_now) + " -> " + cost_text(cost_after) +
							", worth " + String::num(work_price, 1) + ")",
					plan_after);
		}
		return _offer_result(false, trade_text + String(": payment ") + String::num(receive_value, 1) +
						" does not cover the extra work for \"" + goal_name + "\" (" + cost_text(cost_now) + " -> " + cost_text(cost_after) +
						", worth " + String::num(work_price, 1) + ")",
				plan_after);
	}

	// The plan does not change: decide by value, so surplus is sold for a fair price.
	if (receive_value + COST_EPSILON >= give_value) {
		return _offer_result(true, trade_text + String(": fair trade, plan for \"") + goal_name + "\" unchanged", plan_after);
	}
	return _offer_result(false, trade_text + String(": not worth it (gets ") + String::num(receive_value, 1) + ", gives " + String::num(give_value, 1) + ")", plan_after);
}

Dictionary GoapTrader::receive_offer(Wallet *from_wallet, const Dictionary &they_give, const Dictionary &they_want) {
	ZoneScopedN("GoapTrader::receive_offer");
	Dictionary result = evaluate_offer(they_give, they_want);
	bool accepted = result.get("accepted", false);

	if (accepted) {
		String problem;
		if (!economy_amounts::exchange(from_wallet, get_wallet(), they_give, they_want, _get_manager(), &problem)) {
			result = _offer_result(false, "the exchange failed: " + problem, Ref<GoapPlan>());
			accepted = false;
		} else {
			// Whatever this trader was waiting for has arrived.
			_clear_demands_for(they_give);
		}
	}

	if (accepted && !executing) {
		// The plan computed while evaluating already starts from the new state, so the
		// next step can start without searching again.
		Ref<GoapPlan> plan_after = result.get("plan", Variant());
		if (plan_after.is_valid()) {
			current_plan = plan_after;
			plan_progress = 0;
			fresh_plan = plan_after->is_found();
			emit_signal("plan_changed", current_plan);
		}
	}

	const String reason = result.get("reason", "");
	last_offer_reason = (accepted ? "Accepted " : "Refused ") + reason;
	emit_signal("offer_received", from_wallet, they_give, they_want, accepted, reason);
	return result;
}

// ---------------------------------------------------------------- demand

void GoapTrader::post_demand(const String &item_id, float amount) {
	if (item_id.is_empty() || amount <= 0.0f) {
		return;
	}
	const bool is_new = !posted_demands.has(item_id);
	Dictionary entry = posted_demands.get(item_id, Dictionary());
	entry["amount"] = MAX((float)entry.get("amount", 0.0f), amount);
	entry["expires"] = sim_time + demand_duration;
	posted_demands[item_id] = entry;
	if (is_new) {
		emit_signal("demand_posted", item_id, amount);
	}
}

void GoapTrader::clear_demand(const String &item_id) {
	posted_demands.erase(item_id);
}

Dictionary GoapTrader::get_active_demands() {
	Dictionary active;
	Array keys = posted_demands.keys();
	for (int i = 0; i < keys.size(); ++i) {
		Dictionary entry = posted_demands[keys[i]];
		if ((double)entry.get("expires", 0.0) > sim_time) {
			active[keys[i]] = entry.get("amount", 0.0f);
		} else {
			posted_demands.erase(keys[i]);
		}
	}
	return active;
}

void GoapTrader::_post_demands_for(const Dictionary &amounts) {
	Array keys = amounts.keys();
	for (int i = 0; i < keys.size(); ++i) {
		post_demand(keys[i], amounts.get(keys[i], 0.0f));
	}
}

void GoapTrader::_clear_demands_for(const Dictionary &amounts) {
	Array keys = amounts.keys();
	for (int i = 0; i < keys.size(); ++i) {
		clear_demand(keys[i]);
	}
}

bool GoapTrader::_market_has(const Dictionary &amounts) {
	ZoneScopedN("GoapTrader::_market_has");
	if (!is_inside_tree() || market_group.is_empty()) {
		return false;
	}
	EconomyManager *manager = _get_manager();
	TypedArray<Node> members = get_tree()->get_nodes_in_group(market_group);
	for (int i = 0; i < members.size(); ++i) {
		GoapTrader *other = Object::cast_to<GoapTrader>(members[i]);
		if (other && other != this && other->get_wallet() && economy_amounts::has_all(other->get_wallet(), amounts, manager)) {
			return true;
		}
	}
	return false;
}

bool GoapTrader::_can_produce(const String &item_id) const {
	// Only custom actions count. A trader that could only buy the item and sell it on
	// would not add anything to the market, so it does not answer demand.
	for (int i = 0; i < actions.size(); ++i) {
		Ref<GoapAction> action = actions[i];
		if (action.is_valid() && action->get_kind() == GoapAction::KIND_CUSTOM && action->get_produces().has(item_id)) {
			return true;
		}
	}
	return false;
}

Dictionary GoapTrader::_collect_market_demands() {
	ZoneScopedN("GoapTrader::_collect_market_demands");
	Dictionary demands; // item id -> largest amount any other trader asks for
	if (!is_inside_tree() || market_group.is_empty()) {
		return demands;
	}
	TypedArray<Node> members = get_tree()->get_nodes_in_group(market_group);
	for (int i = 0; i < members.size(); ++i) {
		GoapTrader *other = Object::cast_to<GoapTrader>(members[i]);
		// Most traders have no demands most of the time. Checking that first skips building
		// a Dictionary for them, which made this scan the most expensive part of make_plan.
		if (!other || other == this || other->posted_demands.is_empty()) {
			continue;
		}
		Dictionary theirs = other->get_active_demands();
		Array keys = theirs.keys();
		for (int k = 0; k < keys.size(); ++k) {
			demands[keys[k]] = MAX((float)demands.get(keys[k], 0.0f), (float)theirs.get(keys[k], 0.0f));
		}
	}
	return demands;
}

Array GoapTrader::_goals_in_order(NeedsComponent *needs) {
	Array goals = needs->get_active_goals();

	// The fallback goal is last in the list from the NeedsComponent and must stay last.
	Ref<GoapGoal> fallback = needs->get_fallback_goal();
	if (fallback.is_valid() && !goals.is_empty() && Ref<GoapGoal>(goals[goals.size() - 1]) == fallback) {
		goals.remove_at(goals.size() - 1);
	} else {
		fallback = Ref<GoapGoal>();
	}

	// One goal per demand this trader can produce for, placed among the need goals by
	// priority. On equal priority a need goes first.
	Dictionary demands = _collect_market_demands();
	Array ids = demands.keys();
	for (int i = 0; i < ids.size(); ++i) {
		const String id = ids[i];
		if (!_can_produce(id)) {
			continue;
		}
		Ref<GoapGoal> goal = demand_goals.get(id, Variant());
		if (goal.is_null()) {
			goal.instantiate();
			demand_goals[id] = goal;
		}
		goal->set_goal_name(String("Demand: ") + id);
		goal->set_priority(demand_priority);
		goal->clear_targets();
		goal->set_item_target(id, (int)std::ceil((float)demands[id]));

		int insert_at = goals.size();
		for (int g = 0; g < goals.size(); ++g) {
			Ref<GoapGoal> existing = goals[g];
			if (existing.is_valid() && existing->get_priority() < demand_priority) {
				insert_at = g;
				break;
			}
		}
		goals.insert(insert_at, goal);
	}

	if (fallback.is_valid()) {
		goals.push_back(fallback);
	}
	return goals;
}

Ref<GoapPlan> GoapTrader::_plan_around_market(const Ref<GoapGoal> &goal, Wallet *wallet, GoapPlanner *planner, const Dictionary &penalties) {
	ZoneScopedN("GoapTrader::_plan_around_market");
	// The planner only sees this trader's own wallet, so it assumes every trade can happen.
	// Here the first step is checked against the market: if it is a trade nobody can serve
	// right now, a demand is posted and the goal is planned again without that action, so a
	// trader that can make the goods itself does so. Only the first step is checked; later
	// steps depend on what the market looks like by then.
	Array usable = actions.duplicate();
	PackedStringArray waiting_for;
	for (int attempt = 0; attempt <= actions.size(); ++attempt) {
		Ref<GoapPlan> plan = planner->plan_for_wallet(goal, wallet, usable, this, penalties);
		if (!plan->is_found()) {
			if (waiting_for.is_empty()) {
				return plan;
			}
			Ref<GoapPlan> waiting;
			waiting.instantiate();
			waiting->goal = goal;
			waiting->result = GoapPlan::RESULT_NO_PLAN;
			waiting->explored_states = plan->get_explored_states();
			waiting->message = String("Waiting for \"") + goal->get_goal_name() + "\": nobody has " +
					String(", ").join(waiting_for) + " (demand posted)";
			return waiting;
		}
		Ref<GoapAction> first = plan->get_steps()[0];
		if (first->get_kind() != GoapAction::KIND_TRADE || _market_has(first->get_produces())) {
			return plan;
		}
		_post_demands_for(first->get_produces());
		waiting_for.push_back(economy_amounts::to_text(first->get_produces()));
		usable.erase(first);
	}
	// Not reached: every attempt removes an action, so the loop ends with no plan above.
	return planner->plan_for_wallet(goal, wallet, usable, this, penalties);
}

// ---------------------------------------------------------------- bindings

void GoapTrader::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_wallet"), &GoapTrader::get_wallet);
	ClassDB::bind_method(D_METHOD("get_needs"), &GoapTrader::get_needs);
	ClassDB::bind_method(D_METHOD("get_planner"), &GoapTrader::get_planner);

	ClassDB::bind_method(D_METHOD("get_actions"), &GoapTrader::get_actions);
	ClassDB::bind_method(D_METHOD("set_actions", "actions"), &GoapTrader::set_actions);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "actions", PROPERTY_HINT_ARRAY_TYPE, "GoapAction"),
				 "set_actions", "get_actions");
	ClassDB::bind_method(D_METHOD("add_action", "action"), &GoapTrader::add_action);
	ClassDB::bind_method(D_METHOD("remove_action", "action"), &GoapTrader::remove_action);

	ADD_GROUP("Execution", "");
	ClassDB::bind_method(D_METHOD("get_auto_run"), &GoapTrader::get_auto_run);
	ClassDB::bind_method(D_METHOD("set_auto_run", "enabled"), &GoapTrader::set_auto_run);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_run"), "set_auto_run", "get_auto_run");
	ClassDB::bind_method(D_METHOD("get_instant_steps"), &GoapTrader::get_instant_steps);
	ClassDB::bind_method(D_METHOD("set_instant_steps", "enabled"), &GoapTrader::set_instant_steps);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "instant_steps"), "set_instant_steps", "get_instant_steps");
	ClassDB::bind_method(D_METHOD("get_replan_interval"), &GoapTrader::get_replan_interval);
	ClassDB::bind_method(D_METHOD("set_replan_interval", "seconds"), &GoapTrader::set_replan_interval);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "replan_interval", PROPERTY_HINT_RANGE, "0,10,0.05,or_greater"),
				 "set_replan_interval", "get_replan_interval");
	ClassDB::bind_method(D_METHOD("get_market_group"), &GoapTrader::get_market_group);
	ClassDB::bind_method(D_METHOD("set_market_group", "group"), &GoapTrader::set_market_group);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "market_group"), "set_market_group", "get_market_group");

	ADD_GROUP("Trading", "");
	ClassDB::bind_method(D_METHOD("get_work_value"), &GoapTrader::get_work_value);
	ClassDB::bind_method(D_METHOD("set_work_value", "value"), &GoapTrader::set_work_value);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "work_value", PROPERTY_HINT_RANGE, "0,10,0.05,or_greater"),
				 "set_work_value", "get_work_value");

	ADD_GROUP("Demand", "");
	ClassDB::bind_method(D_METHOD("get_demand_duration"), &GoapTrader::get_demand_duration);
	ClassDB::bind_method(D_METHOD("set_demand_duration", "seconds"), &GoapTrader::set_demand_duration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "demand_duration", PROPERTY_HINT_RANGE, "0,120,0.5,or_greater"),
				 "set_demand_duration", "get_demand_duration");
	ClassDB::bind_method(D_METHOD("get_demand_priority"), &GoapTrader::get_demand_priority);
	ClassDB::bind_method(D_METHOD("set_demand_priority", "priority"), &GoapTrader::set_demand_priority);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "demand_priority"), "set_demand_priority", "get_demand_priority");
	ClassDB::bind_method(D_METHOD("post_demand", "item_id", "amount"), &GoapTrader::post_demand);
	ClassDB::bind_method(D_METHOD("clear_demand", "item_id"), &GoapTrader::clear_demand);
	ClassDB::bind_method(D_METHOD("get_active_demands"), &GoapTrader::get_active_demands);

	ADD_GROUP("Refusals", "");
	ClassDB::bind_method(D_METHOD("get_refusal_memory_seconds"), &GoapTrader::get_refusal_memory_seconds);
	ClassDB::bind_method(D_METHOD("set_refusal_memory_seconds", "seconds"), &GoapTrader::set_refusal_memory_seconds);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refusal_memory_seconds", PROPERTY_HINT_RANGE, "0,120,0.1,or_greater"),
				 "set_refusal_memory_seconds", "get_refusal_memory_seconds");
	ClassDB::bind_method(D_METHOD("get_refusal_penalty"), &GoapTrader::get_refusal_penalty);
	ClassDB::bind_method(D_METHOD("set_refusal_penalty", "cost"), &GoapTrader::set_refusal_penalty);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "refusal_penalty", PROPERTY_HINT_RANGE, "0,100,0.1,or_greater"),
				 "set_refusal_penalty", "get_refusal_penalty");

	ClassDB::bind_method(D_METHOD("make_plan"), &GoapTrader::make_plan);
	ClassDB::bind_method(D_METHOD("get_current_plan"), &GoapTrader::get_current_plan);
	ClassDB::bind_method(D_METHOD("start_plan"), &GoapTrader::start_plan);
	ClassDB::bind_method(D_METHOD("complete_step"), &GoapTrader::complete_step);
	ClassDB::bind_method(D_METHOD("fail_step", "reason"), &GoapTrader::fail_step);
	ClassDB::bind_method(D_METHOD("is_executing"), &GoapTrader::is_executing);
	ClassDB::bind_method(D_METHOD("get_current_step"), &GoapTrader::get_current_step);
	ClassDB::bind_method(D_METHOD("get_current_step_index"), &GoapTrader::get_current_step_index);
	ClassDB::bind_method(D_METHOD("get_current_partner"), &GoapTrader::get_current_partner);
	ClassDB::bind_method(D_METHOD("get_action_penalty", "action"), &GoapTrader::get_action_penalty);

	ClassDB::bind_method(D_METHOD("evaluate_offer", "they_give", "they_want"), &GoapTrader::evaluate_offer);
	ClassDB::bind_method(D_METHOD("receive_offer", "from_wallet", "they_give", "they_want"), &GoapTrader::receive_offer);
	ClassDB::bind_method(D_METHOD("get_last_offer_reason"), &GoapTrader::get_last_offer_reason);

	ADD_SIGNAL(MethodInfo("plan_changed", PropertyInfo(Variant::OBJECT, "plan", PROPERTY_HINT_RESOURCE_TYPE, "GoapPlan")));
	ADD_SIGNAL(MethodInfo("demand_posted", PropertyInfo(Variant::STRING, "item_id"), PropertyInfo(Variant::FLOAT, "amount")));
	ADD_SIGNAL(MethodInfo("step_started", PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_RESOURCE_TYPE, "GoapAction"),
			PropertyInfo(Variant::OBJECT, "partner", PROPERTY_HINT_NODE_TYPE, "GoapTrader")));
	ADD_SIGNAL(MethodInfo("step_completed", PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_RESOURCE_TYPE, "GoapAction")));
	ADD_SIGNAL(MethodInfo("step_failed", PropertyInfo(Variant::OBJECT, "action", PROPERTY_HINT_RESOURCE_TYPE, "GoapAction"),
			PropertyInfo(Variant::STRING, "reason")));
	ADD_SIGNAL(MethodInfo("trade_completed", PropertyInfo(Variant::OBJECT, "partner", PROPERTY_HINT_NODE_TYPE, "GoapTrader"),
			PropertyInfo(Variant::DICTIONARY, "gave"), PropertyInfo(Variant::DICTIONARY, "received")));
	ADD_SIGNAL(MethodInfo("offer_received", PropertyInfo(Variant::OBJECT, "from_wallet", PROPERTY_HINT_NODE_TYPE, "Wallet"),
			PropertyInfo(Variant::DICTIONARY, "they_give"), PropertyInfo(Variant::DICTIONARY, "they_want"),
			PropertyInfo(Variant::BOOL, "accepted"), PropertyInfo(Variant::STRING, "reason")));
}
