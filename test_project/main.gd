extends Node2D

func _ready() -> void:
	test_plugin_functionality()
	pass

func test_plugin_functionality()->void:
	$Wallet1.add_currency("gold", 20)
	$Wallet2.add_currency("dia", 20)
	
	$Wallet1.transfer_currency_to($Wallet2, "gold", 15)
	pass
