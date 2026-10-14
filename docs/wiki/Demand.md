# Demand

The planner only sees its own trader's wallet, so it assumes every trade can happen. Before a plan starts, the trader checks its first step. If that step is a trade and nobody in the market holds the goods:

1. The trader posts a **demand** for them and emits `demand_posted(item_id, amount)`.
2. It plans the same goal again without that action. If it can make the goods itself, it does. If not, it works on its next goal, and its plan reads "Waiting for ...".
3. Traders with a **Custom** action that produces the goods get a goal "Demand: <item>" and produce them. Traders that could only buy and resell them ignore the demand.

This is how supply chains form by themselves. A hungry farmer posts a demand for bread. The baker has no grain, so he posts a demand for grain. The farmer farms, the baker buys the grain and bakes, and the farmer buys the bread.

| Setting | Meaning |
|---|---|
| `demand_duration` | Seconds a demand lasts unless it is posted again. It is cleared when the goods arrive |
| `demand_priority` | Priority of the demand goal, compared with the NPC's needs. The fallback goal always stays last |

Your game can post demand too, for example for a quest:

```gdscript
trader.post_demand("bread", 5)
```

`get_active_demands()` returns what a trader is currently waiting for.

Next: [Markets](Markets)
