#include "goap/needs_component.hpp"
#include "wallet.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/print_string.hpp>

#include <algorithm>
#include <utility>
#include <vector>

using namespace godot;

void NeedsComponent::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			_sync_values();
			if (!Engine::get_singleton()->is_editor_hint()) {
				set_process(auto_decay);
			}
		} break;
		case NOTIFICATION_PROCESS: {
			if (auto_decay) {
				decay((float)get_process_delta_time());
			}
		} break;
	}
}

void NeedsComponent::set_needs(const TypedArray<Need> &v) {
	needs = v;
	_sync_values();
}

void NeedsComponent::set_auto_decay(bool v) {
	auto_decay = v;
	if (is_inside_tree() && !Engine::get_singleton()->is_editor_hint()) {
		set_process(v);
	}
}

void NeedsComponent::_sync_values() {
	Dictionary seen;
	for (int i = 0; i < needs.size(); ++i) {
		Ref<Need> need = needs[i];
		if (!need.is_valid()) {
			continue;
		}
		const String id = need->get_need_id();
		if (id.is_empty()) {
			continue;
		}
		if (seen.has(id)) {
			print_error("NeedsComponent \"", get_name(), "\" has more than one need with id \"", id, "\". Only the first one is used.");
			continue;
		}
		seen[id] = true;

		if (!values.has(id)) {
			float start = CLAMP(need->get_start_value(), 0.0f, need->get_max_value());
			values[id] = start;
			urgent_flags[id] = start <= need->get_urgent_threshold();
		}
	}

	// Forget values of needs that were removed.
	Array ids = values.keys();
	for (int i = 0; i < ids.size(); ++i) {
		if (!seen.has(ids[i])) {
			values.erase(ids[i]);
			urgent_flags.erase(ids[i]);
			goals_by_need.erase(ids[i]);
		}
	}
}

Ref<Need> NeedsComponent::get_need(const String &need_id) const {
	for (int i = 0; i < needs.size(); ++i) {
		Ref<Need> need = needs[i];
		if (need.is_valid() && need->get_need_id() == need_id) {
			return need;
		}
	}
	return Ref<Need>();
}

float NeedsComponent::get_need_value(const String &need_id) const {
	return values.get(need_id, 0.0f);
}

void NeedsComponent::set_need_value(const String &need_id, float value) {
	Ref<Need> need = get_need(need_id);
	if (!need.is_valid()) {
		print_error("NeedsComponent \"", get_name(), "\" has no need with id \"", need_id, "\".");
		return;
	}
	_apply_value(need, value);
}

bool NeedsComponent::is_need_urgent(const String &need_id) const {
	return urgent_flags.get(need_id, false);
}

void NeedsComponent::_apply_value(const Ref<Need> &need, float value) {
	const String id = need->get_need_id();
	float clamped = CLAMP(value, 0.0f, need->get_max_value());
	float old = values.get(id, -1.0f);
	if (clamped == old) {
		return;
	}
	values[id] = clamped;
	emit_signal("need_changed", id, clamped);

	bool urgent = clamped <= need->get_urgent_threshold();
	bool was_urgent = urgent_flags.get(id, false);
	if (urgent != was_urgent) {
		urgent_flags[id] = urgent;
		if (urgent) {
			emit_signal("need_became_urgent", id);
		} else {
			emit_signal("need_satisfied", id);
		}
	}
}

void NeedsComponent::decay(float delta) {
	if (delta <= 0.0f) {
		return;
	}
	for (int i = 0; i < needs.size(); ++i) {
		Ref<Need> need = needs[i];
		if (!need.is_valid() || need->get_need_id().is_empty()) {
			continue;
		}
		float rate = need->get_decay_per_second();
		if (rate <= 0.0f) {
			continue;
		}
		_apply_value(need, get_need_value(need->get_need_id()) - rate * delta);
	}
	try_auto_consume();
}

bool NeedsComponent::consume_for_need(const String &need_id, Wallet *wallet) {
	Ref<Need> need = get_need(need_id);
	if (!need.is_valid()) {
		print_error("NeedsComponent \"", get_name(), "\" has no need with id \"", need_id, "\".");
		return false;
	}
	if (!wallet) {
		print_error("Cannot consume for need \"", need_id, "\", the wallet is null.");
		return false;
	}
	const String satisfier = need->get_satisfier_item_id();
	if (satisfier.is_empty()) {
		print_error("Need \"", need_id, "\" has no satisfier item.");
		return false;
	}
	if (get_need_value(need_id) >= need->get_max_value()) {
		// Already fully met, consuming would waste the item.
		return false;
	}
	if (!wallet->has_item(satisfier, 1) || !wallet->remove_item(satisfier, 1)) {
		return false;
	}
	_apply_value(need, get_need_value(need_id) + need->get_restore_per_item());
	emit_signal("need_consumed", need_id, satisfier);
	return true;
}

Wallet *NeedsComponent::get_wallet() const {
	return Object::cast_to<Wallet>(ObjectDB::get_instance(wallet_id));
}

void NeedsComponent::set_wallet(Wallet *wallet) {
	wallet_id = wallet ? wallet->get_instance_id() : 0;
}

int NeedsComponent::try_auto_consume() {
	if (!auto_consume) {
		return 0;
	}
	Wallet *wallet = get_wallet();
	if (!wallet) {
		return 0;
	}
	int consumed = 0;
	for (int i = 0; i < needs.size(); ++i) {
		Ref<Need> need = needs[i];
		if (!need.is_valid() || need->get_need_id().is_empty() || need->get_satisfier_item_id().is_empty()) {
			continue;
		}
		if (need->get_restore_per_item() <= 0.0f) {
			continue; // consuming would never raise the value, so it would only waste items
		}
		const String id = need->get_need_id();
		while (get_need_value(id) <= need->get_effective_consume_threshold()
				&& wallet->has_item(need->get_satisfier_item_id(), 1)) {
			if (!consume_for_need(id, wallet)) {
				break;
			}
			++consumed;
		}
	}
	return consumed;
}

Ref<GoapGoal> NeedsComponent::_goal_for(const Ref<Need> &need) {
	const String id = need->get_need_id();
	const String satisfier = need->get_satisfier_item_id();
	if (satisfier.is_empty()) {
		return Ref<GoapGoal>();
	}

	Ref<GoapGoal> goal = goals_by_need.get(id, Variant());
	if (!goal.is_valid()) {
		goal.instantiate();
		goals_by_need[id] = goal;
	}
	// Refreshed every time, so changes to the Need in the inspector are picked up.
	goal->set_goal_name(need->get_display_name().is_empty() ? id : need->get_display_name());
	goal->set_priority(need->get_priority());
	goal->clear_targets();
	goal->set_item_target(satisfier, need->get_goal_amount());
	return goal;
}

Array NeedsComponent::get_active_goals() {
	// (priority, index in needs), sorted by priority, highest first.
	// stable_sort keeps the order of the needs array for equal priorities.
	std::vector<std::pair<int, int>> urgent;
	for (int i = 0; i < needs.size(); ++i) {
		Ref<Need> need = needs[i];
		if (!need.is_valid() || need->get_need_id().is_empty()) {
			continue;
		}
		if (is_need_urgent(need->get_need_id())) {
			urgent.emplace_back(need->get_priority(), i);
		}
	}
	std::stable_sort(urgent.begin(), urgent.end(),
			[](const std::pair<int, int> &a, const std::pair<int, int> &b) { return a.first > b.first; });

	Array result;
	for (const std::pair<int, int> &entry : urgent) {
		Ref<GoapGoal> goal = _goal_for(needs[entry.second]);
		if (goal.is_valid()) {
			result.push_back(goal);
		}
	}
	if (fallback_goal.is_valid()) {
		result.push_back(fallback_goal);
	}
	return result;
}

Ref<GoapGoal> NeedsComponent::get_current_goal() {
	Array goals = get_active_goals();
	if (goals.is_empty()) {
		return Ref<GoapGoal>();
	}
	return goals[0];
}

void NeedsComponent::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_needs"), &NeedsComponent::get_needs);
	ClassDB::bind_method(D_METHOD("set_needs", "needs"), &NeedsComponent::set_needs);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "needs", PROPERTY_HINT_ARRAY_TYPE, "Need"), "set_needs", "get_needs");

	ClassDB::bind_method(D_METHOD("get_fallback_goal"), &NeedsComponent::get_fallback_goal);
	ClassDB::bind_method(D_METHOD("set_fallback_goal", "goal"), &NeedsComponent::set_fallback_goal);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "fallback_goal", PROPERTY_HINT_RESOURCE_TYPE, "GoapGoal"),
				 "set_fallback_goal", "get_fallback_goal");

	ClassDB::bind_method(D_METHOD("get_auto_decay"), &NeedsComponent::get_auto_decay);
	ClassDB::bind_method(D_METHOD("set_auto_decay", "enabled"), &NeedsComponent::set_auto_decay);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_decay"), "set_auto_decay", "get_auto_decay");

	ClassDB::bind_method(D_METHOD("decay", "delta"), &NeedsComponent::decay);
	ClassDB::bind_method(D_METHOD("get_need", "need_id"), &NeedsComponent::get_need);
	ClassDB::bind_method(D_METHOD("get_need_value", "need_id"), &NeedsComponent::get_need_value);
	ClassDB::bind_method(D_METHOD("set_need_value", "need_id", "value"), &NeedsComponent::set_need_value);
	ClassDB::bind_method(D_METHOD("is_need_urgent", "need_id"), &NeedsComponent::is_need_urgent);
	ClassDB::bind_method(D_METHOD("consume_for_need", "need_id", "wallet"), &NeedsComponent::consume_for_need);
	ClassDB::bind_method(D_METHOD("try_auto_consume"), &NeedsComponent::try_auto_consume);

	ClassDB::bind_method(D_METHOD("get_auto_consume"), &NeedsComponent::get_auto_consume);
	ClassDB::bind_method(D_METHOD("set_auto_consume", "enabled"), &NeedsComponent::set_auto_consume);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_consume"), "set_auto_consume", "get_auto_consume");
	ClassDB::bind_method(D_METHOD("get_wallet"), &NeedsComponent::get_wallet);
	ClassDB::bind_method(D_METHOD("set_wallet", "wallet"), &NeedsComponent::set_wallet);
	ClassDB::bind_method(D_METHOD("get_active_goals"), &NeedsComponent::get_active_goals);
	ClassDB::bind_method(D_METHOD("get_current_goal"), &NeedsComponent::get_current_goal);

	ADD_SIGNAL(MethodInfo("need_changed", PropertyInfo(Variant::STRING, "need_id"), PropertyInfo(Variant::FLOAT, "value")));
	ADD_SIGNAL(MethodInfo("need_became_urgent", PropertyInfo(Variant::STRING, "need_id")));
	ADD_SIGNAL(MethodInfo("need_satisfied", PropertyInfo(Variant::STRING, "need_id")));
	ADD_SIGNAL(MethodInfo("need_consumed", PropertyInfo(Variant::STRING, "need_id"), PropertyInfo(Variant::STRING, "item_id")));
}
