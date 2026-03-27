extends ItemList

var econmgr = preload("res://economy_manager.gd")

func  _ready() -> void:
	reload()

func reload() -> void:
	#Remove all existing items first
	clear()

	# Get data from EconomyManager node (adjust path if needed)
	var manager = $"../../EconomyManager"
	var currencies = manager.get_currencies()
	var items = manager.get_items()

	# Add currencies header
	add_item("---- Currencies ----")
	set_item_disabled(get_item_count() - 1, true)

	# Populate currencies
	for key in currencies:
		var cur = currencies[key]
		add_item("%s (Value: %s, Currency Symbol: %s)" %
			[cur.get_name(), str(cur.get_base_value()), cur.get_symbol()])

	# Add items header
	add_item("---- Items ----")
	set_item_disabled(get_item_count() - 1, true)

	# Populate items
	for key in items:
		var it = items[key]
		add_item("%s (Value: %s, Stack Limit: %s)" %
			[it.get_name(), str(it.get_base_value()), str(it.get_max_stack_size())])
