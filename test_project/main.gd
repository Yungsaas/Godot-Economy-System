extends Node2D

func _ready():
	var economy = $EconomyManager
	economy.create_currency("gold", "Gold", 2, 1.0, "G")
	economy.create_item("wheat", "Wheat", 2.0, true, 99)
	economy.create_item("iron",  "Iron",  5.0, true, 99)
	economy.create_item("gems",  "Gems",  12.0, true, 99)
	$TradeMarket.trade_executed.connect(_on_trade_executed)

func _on_trade_executed(buyer, seller, item_id, amount, price):
	$UI.add_log_entry(buyer.name, seller.name, item_id, amount, price)
	$TradeLineLayer.show_trade_line(buyer.get_parent().global_position, seller.get_parent().global_position, item_id, amount)
