#pragma once
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class EconomyManager;
class Wallet;

// Helpers for applying a Dictionary of amounts (id -> amount, where an id is an item id
// or a currency id) to wallets. Internal to the extension, not exposed to GDScript.
//
// Every function that changes wallets is all or nothing: if any part fails, the parts
// that were already done are undone, so no item or currency is created or lost.
namespace economy_amounts {

// True if the wallet holds at least every amount. On false, problem says what is missing.
bool has_all(Wallet *wallet, const Dictionary &amounts, EconomyManager *manager, String *problem = nullptr);

// True if the wallet can take every amount without going over an item's max stack size.
bool can_receive(Wallet *wallet, const Dictionary &amounts, EconomyManager *manager, String *problem = nullptr);

// Removes `remove` from and adds `add` to the same wallet.
bool apply(Wallet *wallet, const Dictionary &remove, const Dictionary &add, EconomyManager *manager, String *problem = nullptr);

// a gives a_gives to b, b gives b_gives to a.
bool exchange(Wallet *a, Wallet *b, const Dictionary &a_gives, const Dictionary &b_gives, EconomyManager *manager, String *problem = nullptr);

// Sum of amount * base value. Ids that are not registered count as 0.
float value_of(const Dictionary &amounts, EconomyManager *manager);

// "2 gold + 1 water"
String to_text(const Dictionary &amounts);

} // namespace economy_amounts
