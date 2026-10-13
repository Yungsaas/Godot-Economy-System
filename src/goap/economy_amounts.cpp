#include "goap/economy_amounts.hpp"
#include "economy_item.hpp"
#include "economy_manager.hpp"
#include "wallet.hpp"

#include <cmath>
#include <vector>

using namespace godot;

namespace {

struct Entry {
	String id;
	bool is_currency = false;
	double amount = 0.0;
};

void set_problem(String *problem, const String &text) {
	if (problem) {
		*problem = text;
	}
}

bool collect(const Dictionary &amounts, EconomyManager *manager, std::vector<Entry> &out, String *problem) {
	Array keys = amounts.keys();
	for (int i = 0; i < keys.size(); ++i) {
		String id = keys[i];
		double amount = amounts.get(keys[i], 0.0);
		if (amount <= 0.0) {
			continue;
		}
		if (!manager) {
			set_problem(problem, "No economy manager found.");
			return false;
		}
		if (manager->has_currency(id)) {
			out.push_back(Entry{ id, true, amount });
		} else if (manager->has_item(id)) {
			out.push_back(Entry{ id, false, (double)std::llround(amount) });
		} else {
			set_problem(problem, "\"" + id + "\" is not a registered item or currency.");
			return false;
		}
	}
	return true;
}

bool add_one(Wallet *wallet, const Entry &e) {
	return e.is_currency ? wallet->add_currency(e.id, (float)e.amount) : wallet->add_item(e.id, (int)e.amount);
}

bool remove_one(Wallet *wallet, const Entry &e) {
	return e.is_currency ? wallet->remove_currency(e.id, (float)e.amount) : wallet->remove_item(e.id, (int)e.amount);
}

bool transfer_one(Wallet *from, Wallet *to, const Entry &e) {
	return e.is_currency ? from->transfer_currency_to(to, e.id, (float)e.amount) : from->transfer_item_to(to, e.id, (int)e.amount);
}

bool holds(Wallet *wallet, const Entry &e) {
	return e.is_currency ? wallet->has_currency(e.id, (float)e.amount) : wallet->has_item(e.id, (int)e.amount);
}

String number_text(double amount) {
	if (amount == std::floor(amount)) {
		return String::num_int64((int64_t)amount);
	}
	return String::num(amount, 2);
}

} // namespace

namespace economy_amounts {

bool has_all(Wallet *wallet, const Dictionary &amounts, EconomyManager *manager, String *problem) {
	if (!wallet) {
		set_problem(problem, "No wallet.");
		return false;
	}
	std::vector<Entry> entries;
	if (!collect(amounts, manager, entries, problem)) {
		return false;
	}
	for (const Entry &e : entries) {
		if (!holds(wallet, e)) {
			set_problem(problem, "does not have " + number_text(e.amount) + " " + e.id);
			return false;
		}
	}
	return true;
}

bool can_receive(Wallet *wallet, const Dictionary &amounts, EconomyManager *manager, String *problem) {
	if (!wallet) {
		set_problem(problem, "No wallet.");
		return false;
	}
	std::vector<Entry> entries;
	if (!collect(amounts, manager, entries, problem)) {
		return false;
	}
	for (const Entry &e : entries) {
		if (e.is_currency) {
			continue;
		}
		Ref<EconomyItem> item = manager->get_item(e.id);
		int max_amount = item->get_stackable() ? item->get_max_stack_size() : 1;
		if (wallet->get_item_balance_of(e.id) + (int)e.amount > max_amount) {
			set_problem(problem, "has no room for " + number_text(e.amount) + " more " + e.id);
			return false;
		}
	}
	return true;
}

bool apply(Wallet *wallet, const Dictionary &remove, const Dictionary &add, EconomyManager *manager, String *problem) {
	std::vector<Entry> removes;
	std::vector<Entry> adds;
	if (!collect(remove, manager, removes, problem) || !collect(add, manager, adds, problem)) {
		return false;
	}
	if (!has_all(wallet, remove, manager, problem) || !can_receive(wallet, add, manager, problem)) {
		return false;
	}

	size_t removed = 0;
	size_t added = 0;
	bool ok = true;
	for (; removed < removes.size(); ++removed) {
		if (!remove_one(wallet, removes[removed])) {
			ok = false;
			break;
		}
	}
	if (ok) {
		for (; added < adds.size(); ++added) {
			if (!add_one(wallet, adds[added])) {
				ok = false;
				break;
			}
		}
	}
	if (!ok) {
		// Undo in reverse order.
		for (size_t i = added; i-- > 0;) {
			remove_one(wallet, adds[i]);
		}
		for (size_t i = removed; i-- > 0;) {
			add_one(wallet, removes[i]);
		}
		set_problem(problem, "the wallet refused the change");
	}
	return ok;
}

bool exchange(Wallet *a, Wallet *b, const Dictionary &a_gives, const Dictionary &b_gives, EconomyManager *manager, String *problem) {
	if (!a || !b || a == b) {
		set_problem(problem, "an exchange needs two different wallets");
		return false;
	}
	std::vector<Entry> from_a;
	std::vector<Entry> from_b;
	if (!collect(a_gives, manager, from_a, problem) || !collect(b_gives, manager, from_b, problem)) {
		return false;
	}
	if (!has_all(a, a_gives, manager, problem) || !has_all(b, b_gives, manager, problem)) {
		return false;
	}
	if (!can_receive(b, a_gives, manager, problem) || !can_receive(a, b_gives, manager, problem)) {
		return false;
	}

	size_t done_a = 0;
	size_t done_b = 0;
	bool ok = true;
	for (; done_a < from_a.size(); ++done_a) {
		if (!transfer_one(a, b, from_a[done_a])) {
			ok = false;
			break;
		}
	}
	if (ok) {
		for (; done_b < from_b.size(); ++done_b) {
			if (!transfer_one(b, a, from_b[done_b])) {
				ok = false;
				break;
			}
		}
	}
	if (!ok) {
		for (size_t i = done_b; i-- > 0;) {
			transfer_one(a, b, from_b[i]);
		}
		for (size_t i = done_a; i-- > 0;) {
			transfer_one(b, a, from_a[i]);
		}
		set_problem(problem, "a wallet refused the transfer");
	}
	return ok;
}

float value_of(const Dictionary &amounts, EconomyManager *manager) {
	if (!manager) {
		return 0.0f;
	}
	double total = 0.0;
	Array keys = amounts.keys();
	for (int i = 0; i < keys.size(); ++i) {
		String id = keys[i];
		double amount = amounts.get(keys[i], 0.0);
		if (Ref<Currency> currency = manager->get_currency(id); currency.is_valid()) {
			total += amount * currency->get_base_value();
		} else if (Ref<EconomyItem> item = manager->get_item(id); item.is_valid()) {
			total += amount * item->get_base_value();
		}
	}
	return (float)total;
}

String to_text(const Dictionary &amounts) {
	PackedStringArray parts;
	Array keys = amounts.keys();
	for (int i = 0; i < keys.size(); ++i) {
		double amount = amounts.get(keys[i], 0.0);
		parts.push_back(number_text(amount) + " " + String(keys[i]));
	}
	return parts.is_empty() ? String("nothing") : String(" + ").join(parts);
}

} // namespace economy_amounts
