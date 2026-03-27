# Main.gd
# Attach to a Node2D root node named "Main"
# Children needed in the scene tree:
#   - EconomyManager (your EconomyManager node)
#   - TradeMarket (your TradeMarket node)
#   - World (Node2D)
#   - UI (CanvasLayer) -> see UI.gd
#   - TradeLineLayer (Node2D, for drawing trade lines on top)

extends Node2D

func _ready():
	# Register currencies
	var economy = $EconomyManager
	economy.create_currency("gold", "Gold", 2, 1.0, "G")

	# Register items
	economy.create_item("wheat", "Wheat", 2.0, true, 99)
	economy.create_item("iron",  "Iron",  5.0, true, 99)
	economy.create_item("gems",  "Gems",  12.0, true, 99)

	# Connect TradeMarket signal to UI log
	$TradeMarket.trade_executed.connect(_on_trade_executed)

func _on_trade_executed(buyer, seller, item_id, amount, price):
	$UI.add_log_entry(buyer.name, seller.name, item_id, amount, price)
	$TradeLineLayer.show_trade_line(buyer.global_position, seller.global_position, item_id, amount)
