#pragma once
#include <godot_cpp/classes/resource.hpp>

using namespace godot;

class EconomyItem : public Resource {
	GDCLASS(EconomyItem, Resource)

public:
	String id;
	String display_name;
	bool stackable = true;
	int max_stack_size = 999; // maximum stack size for materials
	float base_value = 1.0; // how much 1 of this type of material is worth

	EconomyItem() {}
	~EconomyItem() override {}

	String get_id() const { return id; }
	void set_id(const String &v) { id = v; }

	String get_name() const { return display_name; }
	void set_name(const String &v) { display_name = v; }

	bool get_stackable() const { return stackable; }
	void set_stackable(bool v) { stackable = v; }

	int get_max_stack_size() const { return max_stack_size; }
	void set_max_stack_size(int v) { max_stack_size = v; }

	float get_base_value() const { return base_value; }
	void set_base_value(float v) { base_value = v; }

protected:
	static void _bind_methods();

private:
};
