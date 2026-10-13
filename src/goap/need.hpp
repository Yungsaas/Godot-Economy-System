#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

// Definition of a need such as thirst or hunger, authored in the inspector.
//
// A Need holds no runtime value. The current value of a need lives on the
// NeedsComponent of each NPC, so the same Need resource can be shared by many NPCs.
//
// The value runs from 0 (completely unmet) to max_value (fully met) and drops by
// decay_per_second. At or below urgent_threshold the need is urgent, and the
// NeedsComponent produces a goal to hold goal_amount of satisfier_item_id.
// Consuming one satisfier item restores restore_per_item. A NeedsComponent with
// auto_consume consumes one by itself once the value drops to consume_threshold.
class Need : public Resource {
	GDCLASS(Need, Resource)

public:
	Need() {}
	~Need() override {}

	String get_need_id() const { return need_id; }
	void set_need_id(const String &v) { need_id = v; }

	String get_display_name() const { return display_name; }
	void set_display_name(const String &v) { display_name = v; }

	float get_max_value() const { return max_value; }
	void set_max_value(float v) { max_value = v > 0.0f ? v : 0.001f; }

	float get_start_value() const { return start_value; }
	void set_start_value(float v) { start_value = v; }

	float get_decay_per_second() const { return decay_per_second; }
	void set_decay_per_second(float v) { decay_per_second = v; }

	float get_urgent_threshold() const { return urgent_threshold; }
	void set_urgent_threshold(float v) { urgent_threshold = v; }

	// Higher means more important. Also used as the priority of the goal this need produces.
	int get_priority() const { return priority; }
	void set_priority(int v) { priority = v; }

	String get_satisfier_item_id() const { return satisfier_item_id; }
	void set_satisfier_item_id(const String &v) { satisfier_item_id = v; }

	float get_restore_per_item() const { return restore_per_item; }
	void set_restore_per_item(float v) { restore_per_item = v; }

	// At or below this value the NeedsComponent consumes a satisfier automatically,
	// if it holds one. A negative value means "use urgent_threshold".
	float get_consume_threshold() const { return consume_threshold; }
	void set_consume_threshold(float v) { consume_threshold = v; }
	float get_effective_consume_threshold() const { return consume_threshold < 0.0f ? urgent_threshold : consume_threshold; }

	int get_goal_amount() const { return goal_amount; }
	void set_goal_amount(int v) { goal_amount = v < 1 ? 1 : v; }

protected:
	static void _bind_methods();

private:
	String need_id = "";
	String display_name = "";
	float max_value = 100.0f;
	float start_value = 100.0f;
	float decay_per_second = 1.0f;
	float urgent_threshold = 30.0f;
	int priority = 1;
	String satisfier_item_id = "";
	float restore_per_item = 50.0f;
	float consume_threshold = -1.0f;
	int goal_amount = 1;
};
