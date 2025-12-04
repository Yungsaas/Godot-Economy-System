extends ItemList

var econmgr = preload("res://economy_manager.gd")

func  _ready() -> void:
	var currencies = $"../EconomyManager".get_currencies()
	var items = $"../EconomyManager".get_items()
	
	
	add_item("---- Currencies ----")
	set_item_disabled(get_item_count() - 1, true)  
	
	for key in currencies:
		var index = add_item("%s (Value: %s, Currency Symbol: %s)" % [currencies[key].get_name(), currencies[key].get_base_value(), currencies[key].get_symbol()])
	
	add_item("---- Items ----")
	set_item_disabled(get_item_count() - 1, true) 
	
	for key in items:
		var index = add_item("%s (Value: %s, Stack Limit: %s)" % [items[key].get_name(), items[key].get_base_value(), items[key].get_max_stack_size() ])
	
func _on_option_button_item_selected(index: int) -> void:
	if(index == 0):
		visible = true
	else:
		visible = false
