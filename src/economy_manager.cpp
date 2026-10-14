#include "economy_manager.hpp"
#include "profiling.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/dictionary.hpp"

using namespace godot;

void EconomyManager::_notification(int p_what) {
	// Registered on enter tree instead of in _ready, so a script that overrides
	// _ready without calling super() does not silently remove the manager from the group.
	if (p_what == NOTIFICATION_ENTER_TREE) {
		add_to_group("economy_manager");
		if (!Engine::get_singleton()->is_editor_hint()) {
			_register_initial_content();
		}
	}
#ifdef TRACY_ENABLE
	// Godot owns the main loop, so the manager marks the frames for the profiler. There is
	// one manager per scene, which gives exactly one frame mark per frame.
	if (p_what == NOTIFICATION_READY && !Engine::get_singleton()->is_editor_hint()) {
		set_process(true);
	}
	if (p_what == NOTIFICATION_PROCESS) {
		ECONOMY_FRAME_END();
	}
#endif
}

Ref<Currency> EconomyManager::create_currency(const String &id, const String &name, int precision, float base_value, const String &symbol) {
	Ref<Currency> cur;
	cur.instantiate();
	cur->set_id(id);
	cur->set_name(name);
	cur->set_precision(precision);
	cur->set_base_value(base_value);
	cur->set_symbol(symbol);
	register_currency(cur);
	return cur;
}

void EconomyManager::register_currency(const Ref<Currency> &currency) {
	if (!currency.is_valid()) {
		print_error("Unable to register currency, it is invalid.");
		return;
	}
	if (currency->get_id().is_empty()) {
		print_error("Unable to register currency, its id is empty.");
		return;
	}
	if (items.has(currency->get_id())) {
		// Actions and goals refer to items and currencies by id in the same Dictionary,
		// so one id cannot mean both.
		print_error("Unable to register currency \"", currency->get_id(), "\", an item with that id already exists.");
		return;
	}
	if (currencies.has(currency->get_id())) {
		print_line("Currency \"", currency->get_id(), "\" was already registered and has been replaced.");
	}
	currencies[currency->get_id()] = currency;
	emit_signal("currency_registered", currency);
}

Ref<Currency> EconomyManager::get_currency(const String &id) const {
	return currencies.get(id, Variant());
}

bool EconomyManager::has_currency(const String &id) const {
	return currencies.has(id);
}

Ref<EconomyItem> EconomyManager::create_item(const String &id, const String &name, float base_value, bool stackable, int max_stack_size) {
	Ref<EconomyItem> itm;
	itm.instantiate();
	itm->set_id(id);
	itm->set_name(name);
	itm->set_stackable(stackable);
	if (stackable)
		itm->set_max_stack_size(max_stack_size);
	else
		itm->set_max_stack_size(1);
	itm->set_base_value(base_value);

	register_economy_item(itm);
	return itm;
}

void EconomyManager::register_economy_item(const Ref<EconomyItem> &econ_item) {
	if (!econ_item.is_valid()) {
		print_error("Unable to register item, it is invalid.");
		return;
	}
	if (econ_item->get_id().is_empty()) {
		print_error("Unable to register item, its id is empty.");
		return;
	}
	if (currencies.has(econ_item->get_id())) {
		print_error("Unable to register item \"", econ_item->get_id(), "\", a currency with that id already exists.");
		return;
	}
	if (items.has(econ_item->get_id())) {
		print_line("Item \"", econ_item->get_id(), "\" was already registered and has been replaced.");
	}
	items[econ_item->get_id()] = econ_item;
	emit_signal("item_registered", econ_item);
}

Ref<EconomyItem> EconomyManager::get_item(const String &id) const {
	return items.get(id, Variant());
}

bool EconomyManager::has_item(const String &id) const {
	return items.has(id);
}

Dictionary EconomyManager::get_currencies() const {
	// Copy, so callers cannot add or remove registry entries without going through register_currency.
	return currencies.duplicate();
}

Dictionary EconomyManager::get_items() const {
	return items.duplicate();
}

void EconomyManager::_register_initial_content() {
	// Currencies first: an item may not take an id that a currency already uses, so the
	// currencies a designer set up take precedence. A resource that is already registered
	// (for example when the manager re-enters the tree) is skipped.
	for (int i = 0; i < initial_currencies.size(); ++i) {
		Ref<Currency> currency = initial_currencies[i];
		if (currency.is_valid() && Ref<Currency>(currencies.get(currency->get_id(), Variant())) != currency) {
			register_currency(currency);
		}
	}
	for (int i = 0; i < initial_items.size(); ++i) {
		Ref<EconomyItem> item = initial_items[i];
		if (item.is_valid() && Ref<EconomyItem>(items.get(item->get_id(), Variant())) != item) {
			register_economy_item(item);
		}
	}
}

void EconomyManager::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_initial_currencies"), &EconomyManager::get_initial_currencies);
	ClassDB::bind_method(D_METHOD("set_initial_currencies", "currencies"), &EconomyManager::set_initial_currencies);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "initial_currencies", PROPERTY_HINT_ARRAY_TYPE, "Currency"),
				 "set_initial_currencies", "get_initial_currencies");
	ClassDB::bind_method(D_METHOD("get_initial_items"), &EconomyManager::get_initial_items);
	ClassDB::bind_method(D_METHOD("set_initial_items", "items"), &EconomyManager::set_initial_items);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "initial_items", PROPERTY_HINT_ARRAY_TYPE, "EconomyItem"),
				 "set_initial_items", "get_initial_items");
	/* Currencies */
	ClassDB::bind_method(D_METHOD("create_currency", "id", "name", "precision", "base_value", "symbol"),
						 &EconomyManager::create_currency, DEFVAL(2), DEFVAL(1.0f), DEFVAL(String("$")));
	ClassDB::bind_method(D_METHOD("register_currency", "currency"), &EconomyManager::register_currency);
	ClassDB::bind_method(D_METHOD("get_currency", "id"), &EconomyManager::get_currency);
	ClassDB::bind_method(D_METHOD("has_currency", "id"), &EconomyManager::has_currency);
	ClassDB::bind_method(D_METHOD("get_currencies"), &EconomyManager::get_currencies);
	/* items */
	ClassDB::bind_method(D_METHOD("create_item", "id", "name", "base_value", "stackable", "max_stack_size"),
						 &EconomyManager::create_item, DEFVAL(1.0f), DEFVAL(true), DEFVAL(999));
	ClassDB::bind_method(D_METHOD("register_item", "item"), &EconomyManager::register_economy_item);
	ClassDB::bind_method(D_METHOD("get_item", "id"), &EconomyManager::get_item);
	ClassDB::bind_method(D_METHOD("has_item", "id"), &EconomyManager::has_item);
	ClassDB::bind_method(D_METHOD("get_items"), &EconomyManager::get_items);

	/* Signal emitters */
	ADD_SIGNAL(MethodInfo("currency_registered", PropertyInfo(Variant::OBJECT, "currency")));
	ADD_SIGNAL(MethodInfo("item_registered", PropertyInfo(Variant::OBJECT, "item")));
}
