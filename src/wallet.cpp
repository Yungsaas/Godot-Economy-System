#pragma once
#include "wallet.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/error_macros.hpp"
#include "godot_cpp/core/print_string.hpp"

void Wallet::_ready()
{
    get_tree()->connect("node_added", Callable(this, "_on_node_added"));
}

void Wallet::add_currency(const String &id, float amount)
{
    float bal = balances_currencies[id];
    bal += amount;
    balances_currencies[id] = bal;
    emit_signal("balance_changed", id, bal);
}

bool Wallet::remove_currency(const String &id, float amount)
{
    float bal = balances_currencies[id];
    if(bal<amount)
    {
        print_error("Balance of ", id, " is insufficient. Required: ", amount, " Has: ", bal);
        return false;
    }
    bal -= amount;
    balances_currencies[id] = bal;
    emit_signal("balance_changed", id, bal);
    return true;
}

bool Wallet::has_currency(const String &id, float amount) const
{
    float bal = balances_currencies[id];
    return bal>=amount;
}

bool Wallet::transfer_currency_to(Wallet* other, const String &id, float amount)
{
    if(!other->is_node_ready())
    {
        print_error("Other node was not ready");
        return false;
    }
    if(!remove_currency(id, amount))
    {
        return false;
    }
    other->add_currency(id, amount);
    return true;
}

void Wallet::_on_economy_manager_ready(Object *mgr_obj) {
    EconomyManager *mgr = Object::cast_to<EconomyManager>(mgr_obj);
    if (!mgr) 
    {
        print_error("Could not bind economy manager");
        return;
    }
    economy_manager = mgr;
    print_line("bound mgr");
}

void Wallet::_on_node_added(Node*node)
{
    if (!node) return;
    if(economy_manager) return;
    auto mgr_n = get_tree()->get_first_node_in_group("economy_manager");
    if(EconomyManager *mgr = Object::cast_to<EconomyManager>(mgr_n))
    {
        print_line("Economy manager linked to ", this);
        economy_manager = mgr; return; 
    } else {
        print_error("No economy manager found.");
    }
}

void Wallet::_bind_methods()
{
    /* Currencies */
    ClassDB::bind_method(D_METHOD("add_currency", "currency", "amount"), &Wallet::add_currency);
    ClassDB::bind_method(D_METHOD("remove_currency", "currency", "amount"), &Wallet::remove_currency);
    ClassDB::bind_method(D_METHOD("transfer_currency_to", "other_wallet", "currency", "amount"), &Wallet::transfer_currency_to);
    /* Items */

    /* Signal functions */
    ClassDB::bind_method(D_METHOD("_on_economy_manager_ready", "manager"), &Wallet::_on_economy_manager_ready);
    ClassDB::bind_method(D_METHOD("_on_node_added", "node"), &Wallet::_on_node_added);
    
    /* Signal emitters */
    ADD_SIGNAL(MethodInfo("balance_changed", PropertyInfo(Variant::STRING, "item_id"), PropertyInfo(Variant::FLOAT, "new_amount")));
}
