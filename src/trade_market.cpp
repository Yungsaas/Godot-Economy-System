#include "trade_market.hpp"
#include "godot_cpp/core/print_string.hpp"
#include "trader.hpp"
#include "wallet.hpp"

using namespace godot;

void TradeMarket::_ready() {
	add_to_group("trade_market");
}

Trader *TradeMarket::_resolve_trader(const Variant &entry) {
	NodePath path = entry;
	if (path.is_empty())
		return nullptr;
	return get_node<Trader>(path);
}

Trader *TradeMarket::find_best_seller(const String &item_id, const String &currency_id, int amount) {
	Trader *best = nullptr;
	float best_price = FLT_MAX;
	int best_stock = 0;

	for (int i = 0; i < traders.size(); ++i) {
		Trader *t = _resolve_trader(traders[i]);
		if (!t || !t->get_wallet())
			continue;
		if (!t->has_sell_rule_for(item_id))
			continue;
		if (!t->get_wallet()->has_item(item_id, amount))
			continue;

		float price = t->get_sell_price_for(item_id);
		int stock = t->get_wallet()->get_item_balance_of(item_id);

		// lowest price wins, tie-break on most stock
		if (price < best_price || (price == best_price && stock > best_stock)) {
			best = t;
			best_price = price;
			best_stock = stock;
		}
	}
	return best;
}

bool TradeMarket::execute_trade(Trader *buyer, Trader *seller, const String &item_id, const String &currency_id, int amount) {
	if (!buyer || !seller)
		return false;

	Wallet *buyer_wallet = buyer->get_wallet();
	Wallet *seller_wallet = seller->get_wallet();
	if (!buyer_wallet || !seller_wallet)
		return false;

	float price = seller->get_sell_price_for(item_id) * (float)amount;

	if (!buyer_wallet->has_currency(currency_id, price)) {
		print_line("TradeMarket: Buyer cannot afford '", item_id, "'");
		return false;
	}
	// currency buyer -> seller
	if (!buyer_wallet->transfer_currency_to(seller_wallet, currency_id, price))
		return false;
	// item seller -> buyer
	if (!seller_wallet->transfer_item_to(buyer_wallet, item_id, amount)) {
		// rollback currency if item transfer failed
		seller_wallet->transfer_currency_to(buyer_wallet, currency_id, price);
		return false;
	}

	emit_signal("trade_executed", buyer, seller, item_id, amount, price);
	return true;
}

TradeRule::SupplyLevel TradeMarket::get_supply_level(const String &item_id) {
	int sellers = 0;
	int trader_count = traders.size();

	for (int i = 0; i < trader_count; ++i) {
		Trader *t = _resolve_trader(traders[i]);
		if (!t || !t->get_wallet())
			continue;
		if (!t->has_sell_rule_for(item_id))
			continue;
		if (t->get_wallet()->get_item_balance_of(item_id) > 0)
			sellers++;
	}

	if (trader_count == 0 || sellers == 0)
		return TradeRule::SUPPLY_LOW;
	float ratio = (float)sellers / (float)trader_count;
	if (ratio < 0.33f)
		return TradeRule::SUPPLY_LOW;
	if (ratio > 0.66f)
		return TradeRule::SUPPLY_HIGH;
	return TradeRule::SUPPLY_NORMAL;
}

void TradeMarket::_bind_methods() {
	// traders list
	ClassDB::bind_method(D_METHOD("get_traders"), &TradeMarket::get_traders);
	ClassDB::bind_method(D_METHOD("set_traders", "traders"), &TradeMarket::set_traders);
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "traders", PROPERTY_HINT_ARRAY_TYPE, "NodePath"),
				 "set_traders", "get_traders");
	ClassDB::bind_method(D_METHOD("add_trader", "path"), &TradeMarket::add_trader);
	ClassDB::bind_method(D_METHOD("remove_trader", "path"), &TradeMarket::remove_trader);

	// market queries
	ClassDB::bind_method(D_METHOD("find_best_seller", "item_id", "currency_id", "amount"), &TradeMarket::find_best_seller);
	ClassDB::bind_method(D_METHOD("execute_trade", "buyer", "seller", "item_id", "currency_id", "amount"), &TradeMarket::execute_trade);
	ClassDB::bind_method(D_METHOD("get_supply_level", "item_id"), &TradeMarket::get_supply_level);

	ADD_SIGNAL(MethodInfo("trade_executed",
						  PropertyInfo(Variant::OBJECT, "buyer"),
						  PropertyInfo(Variant::OBJECT, "seller"),
						  PropertyInfo(Variant::STRING, "item_id"),
						  PropertyInfo(Variant::INT, "amount"),
						  PropertyInfo(Variant::FLOAT, "price")));
}
