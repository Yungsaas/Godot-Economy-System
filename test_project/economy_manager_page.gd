extends Control

var selected_type:int
var id_input:String
var name_input:String
var value_input:float

var stackable:bool
var max_stack_size:int

var symbol_input:String
var precision_input:int

func _on_option_button_item_selected(index: int) -> void:
	if(index == 0):
		visible = true
		$ItemList.reload()
	else:
		visible = false


func _on_option_button_econ_m_item_selected(index: int) -> void:
	selected_type = index
	if(index == 0):
		$Currency.visible = true
		$Item.visible = false
	if(index == 1):
		$Item.visible = true
		$Currency.visible = false
	pass # Replace with function body.


func _on_add_econ_button_pressed() -> void:
	
	if (selected_type == 0):
		#currency
		$"../EconomyManager".create_currency(id_input,name_input,precision_input,value_input,symbol_input)
		pass
	if(selected_type == 1):
		#item
		$"../EconomyManager".create_item(id_input,name_input,value_input,stackable,max_stack_size)
		pass
	$ItemList.reload()
	pass # Replace with function body.


func _on_id_input_text_changed() -> void:
	var txt = $IDInput.get_text().strip_edges()
	id_input = txt


func _on_name_input_text_changed() -> void:
	var txt = $NameInput.get_text().strip_edges()
	name_input = txt


func _on_value_input_text_changed() -> void:
	var txt = $ValueInput.get_text()
	if txt == "":
		value_input = 0
	else:
		value_input = int(txt.to_float())


func _on_precision_input_text_changed() -> void:
	var txt = $Currency/PrecisionInput.get_text()
	if txt == "":
		precision_input = 0
	else:
		precision_input = int(txt.to_int())


func _on_symbol_input_text_changed() -> void:
	var txt = $Currency/SymbolInput.get_text().strip_edges()
	symbol_input = txt


func _on_max_stack_size_text_changed() -> void:
	var txt = $Item/MaxStackSize.get_text()
	if txt == "":
		max_stack_size = 0
	else:
		max_stack_size = int(txt.to_int())


func _on_check_box_pressed() -> void:
	if(stackable):
		stackable = false
	else:
		stackable = true
	pass # Replace with function body.
