extends EconomyManager

#this script serves as an example, please visit the wiki for proper documentation

func _ready() -> void:
	add_to_group("economy_manager")
	
	connect("currency_registered", Callable(self, "_on_currency_registered"))
	connect("item_registered", Callable(self, "_on_item_registered"))
	
	create_currency("gold", "Gold Coins")
	create_currency("dia", "Diamond", 1, 20, "D")
	
	create_item("wood", "Wood")
	create_item("rstools", "Rusted Tools", 2.5, false, 25)
	create_item("st", "Stone", 2.5)

func _on_currency_registered(currency):
	print("currency registered: ", currency.get_name(), ", currency id: ", currency.get_id(), ", base value: ", currency.get_base_value())

func _on_item_registered(item):
	print("item registered: ", item.get_name(), ", item id: ", item.get_id(), ", base value: ", item.get_base_value(), ", max stack size: ", item.get_max_stack_size())
