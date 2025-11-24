#pragma once
#include "economy_manager.hpp"
#include "godot_cpp/classes/ref.hpp"
#include "godot_cpp/core/print_string.hpp"

using namespace godot;

Ref<Currency> EconomyManager::create_currency(const int &id,const String &name, int precision, float base_value, const String &symbol)
{
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

void EconomyManager::register_currency(const Ref<Currency> &currency)
{
if (!currency.is_valid())
{
    print_error("Unable to register currency, it is invalid.");
    return;
}
    currencies[currency->get_id()] = currency;
    emit_signal("currency_registered", currency);
}

Ref<Currency> EconomyManager::get_currency(const int &id)
{
    auto v = currencies.find_key(id);
    return v;
}

void EconomyManager::_bind_methods() 
{

    /* Currencies */
    ClassDB::bind_method(D_METHOD("create_currency", "id", "name", "divisible", "precision", "base_value", "symbol"),
                         &EconomyManager::create_currency, DEFVAL(true), DEFVAL(2), DEFVAL(1.0), DEFVAL(String("$")));
    ClassDB::bind_method(D_METHOD("register_currency", "currency"), &EconomyManager::register_currency);
    ClassDB::bind_method(D_METHOD("get_currency", "id"), &EconomyManager::get_currency);

}
