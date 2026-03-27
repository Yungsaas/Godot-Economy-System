extends CanvasLayer

const MAX_LOG = 12
const ITEMS   = ["wheat", "iron", "gems"]
const CURRENCIES = ["gold"]

var _npcs: Array = []
var _player = null

func _ready():
	print("UI: _ready called")
	$ShopPanel.hide()
	$InteractHint.hide()
	$ShopPanel/VBox/CloseBtn.pressed.connect(close_shop)

	await get_tree().create_timer(0.2).timeout
	_collect_npcs()
	_build_wallet_headers()
	refresh_wallet_display()
	print("UI: setup complete, npcs found: ", _npcs.size())

func _collect_npcs():
	_npcs = get_tree().get_nodes_in_group("npc")
	_player = get_tree().get_first_node_in_group("player")

func _build_wallet_headers():
	var grid = $WalletPanel/VBox/WalletGrid
	grid.columns = 1 + CURRENCIES.size() + ITEMS.size()
	_add_header(grid, "Trader")
	for c in CURRENCIES: _add_header(grid, c.capitalize())
	for i in ITEMS:      _add_header(grid, i.capitalize())

func _add_header(grid: GridContainer, text: String):
	var lbl = Label.new()
	lbl.text = text
	lbl.add_theme_color_override("font_color", Color(0.7, 0.85, 1.0))
	lbl.add_theme_font_size_override("font_size", 13)
	grid.add_child(lbl)

func refresh_wallet_display():
	if _npcs.is_empty():
		_collect_npcs()
	var grid = $WalletPanel/VBox/WalletGrid
	var col_count = grid.columns
	while grid.get_child_count() > col_count:
		grid.get_child(grid.get_child_count() - 1).queue_free()
	var all_traders = _npcs.duplicate()
	if _player: all_traders.append(_player)
	for node in all_traders:
		var trader = node.get_trader() if node.has_method("get_trader") else null
		if not trader: continue
		var wallet = trader.get_wallet()
		if not wallet: continue
		_add_cell(grid, node.name)
		for c in CURRENCIES:
			_add_cell(grid, "%.1f" % wallet.get_currency_balance(c))
		for i in ITEMS:
			_add_cell(grid, str(wallet.get_item_balance(i)))

func _add_cell(grid: GridContainer, text: String):
	var lbl = Label.new()
	lbl.text = text
	lbl.add_theme_font_size_override("font_size", 12)
	grid.add_child(lbl)

func add_log_entry(buyer_name: String, seller_name: String, item_id: String, amount: int, price: float):
	print("UI: add_log_entry called — ", buyer_name, " bought ", amount, " ", item_id, " from ", seller_name)
	var list = $LogPanel/VBox/LogScroll/LogList
	print("UI: LogList node = ", list)
	var lbl = Label.new()
	lbl.text = "%s bought %d %s from %s for %.1fG" % [buyer_name, amount, item_id, seller_name, price]
	lbl.add_theme_font_size_override("font_size", 11)
	lbl.add_theme_color_override("font_color", Color(0.9, 0.95, 1.0))
	list.add_child(lbl)
	print("UI: label added, LogList child count = ", list.get_child_count())
	while list.get_child_count() > MAX_LOG:
		list.get_child(0).queue_free()

func open_shop(seller_trader):
	if not _player: return
	var items_c = $ShopPanel/VBox/ShopItems
	for child in items_c.get_children():
		child.queue_free()
	$ShopPanel/VBox/ShopTitle.text = "Shop: " + seller_trader.get_parent().name
	for item_id in ITEMS:
		if not seller_trader.has_sell_rule_for(item_id): continue
		var price = seller_trader.get_sell_price_for(item_id)
		var stock = seller_trader.get_wallet().get_item_balance(item_id)
		if stock <= 0: continue
		var row = HBoxContainer.new()
		var lbl = Label.new()
		lbl.text = "%s  %.1fG  (stock: %d)" % [item_id.capitalize(), price, stock]
		lbl.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		lbl.add_theme_font_size_override("font_size", 13)
		var btn = Button.new()
		btn.text = "Buy 1"
		btn.pressed.connect(_on_buy_pressed.bind(seller_trader, item_id, 1))
		row.add_child(lbl)
		row.add_child(btn)
		items_c.add_child(row)
	if items_c.get_child_count() == 0:
		var lbl = Label.new()
		lbl.text = "Nothing for sale right now."
		items_c.add_child(lbl)
	$ShopPanel.show()

func _on_buy_pressed(seller_trader, item_id: String, amount: int):
	var market = get_tree().get_first_node_in_group("trade_market")
	if not market: return
	market.execute_trade(_player.get_trader(), seller_trader, item_id, "gold", amount)
	open_shop(seller_trader)
	refresh_wallet_display()

func close_shop():
	$ShopPanel.hide()

func show_interact_hint(should_show: bool):
	$InteractHint.visible = should_show
