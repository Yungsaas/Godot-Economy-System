extends Node2D

var IDInput : String
var AmountInput : int

func _ready() -> void:
	test_plugin_functionality()
	pass

func test_plugin_functionality()->void:
	#currency test
	print("Currency Test:")
	$Wallet1.add_currency("gold", 20)
	$Wallet2.add_currency("gold", 20)
	
	#item test
	print("Item Test:")
	$Wallet1.add_item("wood", 5)
	
	print()
	print("Plan execution should look like this (currently not automated, needs solution, possibly rule based decision making):")
	$Wallet2.add_item("st", 5)
	$Wallet2.transfer_item_to($Wallet1, "st", 2)
	$Wallet1.transfer_currency_to($Wallet2, "gold", 5)
	pass
