extends Control

# Demo of two wallets and transfers between them.
#
# Run this scene on its own with F6. Each wallet shows every registered currency and item,
# with a button to give the chosen amount to the other wallet. Transfers are refused when
# the sender does not have enough, or when the receiver would go past the item's maximum
# stack size. A refused transfer changes nothing in either wallet.
#
# Set up to show the refusals:
#   wood has a maximum stack of 10 and B already holds 8, so A giving 5 wood is refused
#   tool is not stackable, so a wallet can hold only 1, and both already hold one
#
# The extension prints an error in the Output panel for every refused transfer. Those red
# lines are expected.

const STARTING_BALANCES := {
	"A": {"gold": 20.0, "bread": 2, "wood": 5, "tool": 1},
	"B": {"gold": 5.0, "water": 3, "wood": 8, "tool": 1},
}

@onready var economy: EconomyManager = $EconomyManager
@onready var wallet_a: Wallet = $Wallets/A
@onready var wallet_b: Wallet = $Wallets/B

var _amount: SpinBox
var _panels := {}  # Wallet -> VBoxContainer holding the rows
var _log: RichTextLabel
var _quiet := false  # true while setting balances, so the log only shows transfers


func _ready() -> void:
	_register_content()
	_build_ui()
	for wallet in [wallet_a, wallet_b]:
		wallet.balance_changed.connect(_on_balance_changed.bind(wallet))
	_set_starting_balances()
	_refresh()
	_log_line("Ready. Pick an amount and give something from one wallet to the other.")


func _register_content() -> void:
	economy.create_currency("gold", "Gold", 2, 1.0, "G")
	economy.create_item("water", "Water", 2.0)
	economy.create_item("bread", "Bread", 4.0)
	economy.create_item("wood", "Wood", 1.0, true, 10)
	economy.create_item("tool", "Tool", 10.0, false)


func _set_starting_balances() -> void:
	_quiet = true
	for wallet in [wallet_a, wallet_b]:
		# Empty the wallet first, so this also works as a reset.
		for id in wallet.get_currencies():
			var bal: float = wallet.get_currency_balance(id)
			if bal > 0.0:
				wallet.remove_currency(id, bal)
		for id in wallet.get_items():
			var count: int = wallet.get_item_balance(id)
			if count > 0:
				wallet.remove_item(id, count)

		var balances: Dictionary = STARTING_BALANCES.get(String(wallet.name), {})
		for id in balances:
			if economy.has_currency(id):
				wallet.add_currency(id, balances[id])
			else:
				wallet.add_item(id, balances[id])
	_quiet = false


# ---------------------------------------------------------------- transfers

func _give(from: Wallet, to: Wallet, id: String) -> void:
	var amount := _amount.value
	var ok: bool
	if economy.has_currency(id):
		ok = from.transfer_currency_to(to, id, amount)
	else:
		ok = from.transfer_item_to(to, id, int(amount))

	if ok:
		_log_line("[color=lightgreen]%s gave %s %s to %s[/color]" % [from.name, _fmt(id, amount), id, to.name])
	else:
		_log_line("[color=orange]%s could not give %s %s to %s: %s. Nothing changed.[/color]" % [
			from.name, _fmt(id, amount), id, to.name, _refusal_reason(from, to, id, amount)])
	_refresh()


# The extension only returns false, so the demo works out why, to show it in the log.
func _refusal_reason(from: Wallet, to: Wallet, id: String, amount: float) -> String:
	if economy.has_currency(id):
		return "%s has only %s" % [from.name, _fmt(id, from.get_currency_balance(id))]
	var have: int = from.get_item_balance(id)
	if have < amount:
		return "%s has only %d" % [from.name, have]
	var item: EconomyItem = economy.get_item(id)
	var max_amount: int = item.get_max_stack_size() if item.get_stackable() else 1
	return "%s can hold at most %d %s and already has %d" % [to.name, max_amount, id, to.get_item_balance(id)]


func _on_balance_changed(id: String, new_amount: float, wallet: Wallet) -> void:
	if not _quiet:
		_log_line("    balance_changed: %s %s -> %s" % [wallet.name, id, _fmt(id, new_amount)])


# ---------------------------------------------------------------- UI

func _build_ui() -> void:
	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	for side in ["left", "right", "top", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 12)
	add_child(margin)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 10)
	margin.add_child(root)

	var top := HBoxContainer.new()
	root.add_child(top)
	var title := Label.new()
	title.text = "Wallet transfer test"
	title.add_theme_font_size_override("font_size", 20)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	top.add_child(title)
	var amount_label := Label.new()
	amount_label.text = "Amount:"
	top.add_child(amount_label)
	_amount = SpinBox.new()
	_amount.min_value = 1
	_amount.max_value = 100
	_amount.step = 1
	_amount.value = 1
	top.add_child(_amount)
	var reset_btn := Button.new()
	reset_btn.text = "Reset wallets"
	reset_btn.pressed.connect(_on_reset_pressed)
	top.add_child(reset_btn)

	var columns := HBoxContainer.new()
	columns.add_theme_constant_override("separation", 10)
	columns.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(columns)
	columns.add_child(_build_wallet_panel(wallet_a))
	columns.add_child(_build_wallet_panel(wallet_b))

	_log = RichTextLabel.new()
	_log.custom_minimum_size = Vector2(0, 220)
	_log.scroll_following = true
	_log.bbcode_enabled = true
	root.add_child(_log)


func _build_wallet_panel(wallet: Wallet) -> Control:
	var panel := PanelContainer.new()
	panel.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	var box := VBoxContainer.new()
	panel.add_child(box)

	var name_label := Label.new()
	name_label.text = "Wallet %s" % wallet.name
	name_label.add_theme_font_size_override("font_size", 18)
	box.add_child(name_label)
	box.add_child(HSeparator.new())

	var rows := VBoxContainer.new()
	box.add_child(rows)
	_panels[wallet] = rows
	return panel


func _refresh() -> void:
	var ids: Array = economy.get_currencies().keys() + economy.get_items().keys()
	for wallet in _panels:
		var other: Wallet = wallet_b if wallet == wallet_a else wallet_a
		var rows: VBoxContainer = _panels[wallet]
		for child in rows.get_children():
			child.queue_free()

		for id in ids:
			var balance: float
			if economy.has_currency(id):
				balance = wallet.get_currency_balance(id)
			else:
				balance = wallet.get_item_balance(id)

			var row := HBoxContainer.new()
			var label := Label.new()
			label.text = "%s: %s" % [id, _fmt(id, balance)]
			label.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			row.add_child(label)
			var btn := Button.new()
			btn.text = "Give to %s" % other.name
			btn.pressed.connect(_give.bind(wallet, other, String(id)))
			row.add_child(btn)
			rows.add_child(row)


func _on_reset_pressed() -> void:
	_set_starting_balances()
	_refresh()
	_log_line("Wallets reset to their starting balances.")


func _fmt(id: String, amount: float) -> String:
	return "%.1f" % amount if economy.has_currency(id) else "%d" % int(amount)


func _log_line(text: String) -> void:
	_log.append_text(text + "\n")
