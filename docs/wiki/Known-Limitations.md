# Known Limitations

* **Only the first step of a plan is checked against the market.** Later steps can still fail when they start, and the trader then plans again.
* **Demand covers goods nobody has, not goods an NPC cannot afford.** An NPC without money and without anything to sell produces no signal. Credit or barter is not supported.
* **An NPC with money and no needs buys anything offered at a fair price, without limit.** There is no per item upper limit yet.
* **Offers are judged against the trader's current goal only.** An offer that would help a lower priority need is not taken into account.
* **Prices are fixed per action.** They do not respond to supply and demand. See [Balancing an Economy](Balancing-an-Economy).
* **Saving and loading the state of an economy is not built in.** Wallet balances and need values can be read and set through the [Reference](Reference) methods.
