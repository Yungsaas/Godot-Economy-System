# Markets

`market_group` on a `GoapTrader` is the group it joins (default `goap_traders`). Traders only see and trade with traders in the same group:

* Partner search only looks in the trader's own market ([Trading and Offers](Trading-and-Offers)).
* Demand is only seen by traders in the same market ([Demand](Demand)).

Use separate markets for places that should have separate economies, such as two villages. A trader can change market at runtime by setting `market_group`, for example when it travels.

Every plan and every trade scans its market, so smaller markets are faster. At 900 NPCs, markets of 30 were clearly faster than one market for everyone. See [Performance](Performance).

Next: [Debugging](Debugging)
