# Economy and Wallets

## The EconomyManager

Every scene with an economy needs one **EconomyManager** node. It holds the registry of what exists. Wallets and traders find it by themselves, wherever it sits in the scene tree.

**In the inspector:** add `Currency` resources to `initial_currencies` and `EconomyItem` resources to `initial_items`. They are registered when the manager enters the scene tree, before any wallet or trader is ready.

**In code:**

```gdscript
var economy: EconomyManager = $EconomyManager
economy.create_currency("gold", "Gold", 2, 1.0, "G")      # id, name, decimals, base value, symbol
economy.create_item("water", "Water", 2.5)                # id, name, base value
economy.create_item("sword", "Sword", 50.0, false)        # not stackable: at most 1 per wallet
```

| `EconomyItem` field | Meaning |
|---|---|
| `id` | Used everywhere to refer to the item |
| `display_name` | For your UI |
| `base_value` | What the item is worth. Traders use it to judge whether a trade is fair |
| `stackable`, `max_stack_size` | How many one wallet can hold |

An item and a currency cannot share an id, because actions and goals refer to both in the same way.

## Wallets

A **Wallet** holds the balances of one NPC, or of the player. A `GoapTrader` creates one by itself, but you can add one to anything.

```gdscript
wallet.add_currency("gold", 10.0)
wallet.remove_item("water", 1)
wallet.has_item("water", 2)                     # true if it holds at least 2
wallet.get_item_balance("water")
wallet.transfer_currency_to(other_wallet, "gold", 5.0)
```

Every change returns `false` and changes nothing if it is not allowed: an unknown id, not enough balance, a full stack. Every change of a balance emits `balance_changed(item_id, new_amount)`, which is enough to drive a UI.

Next: [Setting up an NPC](Setting-up-an-NPC)
