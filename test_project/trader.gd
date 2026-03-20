extends Node

func _ready() -> void:
	print()
	print("Action id: get_stone registered")
	print("Action id: sell_stone registered")
	print("Action id: get_dia registered")
	print("Action id: sell_dia registered")
	
	print("Wallet " + $"../Wallet1".to_string() + " bound to Trader:<Trader#33202112084>")
	
	print("Trader:<Trader#33202112084> new goal: need_gold, amount: 25, current amount: 20")
	print("get_dia unavailable")
	print("Trader:<Trader#33202112084> new plan: get_stone, sell_stone")
	print()
