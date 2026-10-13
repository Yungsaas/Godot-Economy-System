#include "currency.hpp"
#include "economy_item.hpp"
#include "economy_manager.hpp"
#include "wallet.hpp"

// Rule based trading (separate system)
#include "rule_trader.hpp"
#include "trade_market.hpp"
#include "trade_rule.hpp"
#include "trader_profile.hpp"

// GOAP trading
#include "goap/goap_action.hpp"
#include "goap/goap_goal.hpp"
#include "goap/goap_plan.hpp"
#include "goap/goap_planner.hpp"
#include "goap/goap_trader.hpp"
#include "goap/need.hpp"
#include "goap/needs_component.hpp"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;

	GDREGISTER_CLASS(Currency)
	GDREGISTER_CLASS(EconomyItem)
	GDREGISTER_CLASS(EconomyManager)
	GDREGISTER_CLASS(Wallet)

	// Rule based trading (separate system)
	GDREGISTER_CLASS(TradeRule)
	GDREGISTER_CLASS(TraderProfile)
	GDREGISTER_CLASS(RuleTrader)
	GDREGISTER_CLASS(TradeMarket)

	// GOAP trading
	GDREGISTER_CLASS(GoapAction)
	GDREGISTER_CLASS(GoapGoal)
	GDREGISTER_CLASS(GoapPlan)
	GDREGISTER_CLASS(Need)
	GDREGISTER_CLASS(NeedsComponent)
	GDREGISTER_CLASS(GoapPlanner)
	GDREGISTER_CLASS(GoapTrader)
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
		return;
}

extern "C" {
GDExtensionBool GDE_EXPORT economyextension_init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_gdextension_types);
	init_obj.register_terminator(uninitialize_gdextension_types);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}
}
