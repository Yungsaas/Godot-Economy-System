#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/variant/array.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "trade_rule.hpp"

using namespace godot;

class Trader;

class TradeMarket : public Node {
	GDCLASS(TradeMarket, Node)

public:
	TradeMarket() {}

	Array get_traders() const { return traders; }
	void set_traders(const Array &v) { traders = v; }
	void add_trader(const NodePath &path) { traders.push_back(path); }
	void remove_trader(const NodePath &path) { traders.erase(path); }

	// returns best seller: lowest price, tie-broken by most stock; nullptr if none found
	Trader *find_best_seller(const String &item_id, const String &currency_id, int amount);

	// transfers item seller -> buyer, currency buyer -> seller
	bool execute_trade(Trader *buyer, Trader *seller, const String &item_id, const String &currency_id, int amount);

	// ratio of traders with stock of item_id mapped to a SupplyLevel
	TradeRule::SupplyLevel get_supply_level(const String &item_id);

	void _ready() override;

protected:
	static void _bind_methods();

private:
	Array traders; // Array of NodePaths pointing to Trader nodes

	Trader *_resolve_trader(const Variant &entry);
};
