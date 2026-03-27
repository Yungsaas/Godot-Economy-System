extends Wallet
func _ready() -> void:
	
	connect("balance_changed", Callable(self, "_on_balance_change"))
	
	
func _on_balance_change(item_id: String, new_amount: float) -> void:
	print("Balance of: ", item_id ," changed in ", self, " to: ", new_amount)
	pass
