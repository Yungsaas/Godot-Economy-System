extends Control

var WalletArray: Array = []
var ILL: ItemList
var ILR: ItemList
var IDInput : String = ""
var AmountInput : int = 0

var left_index : int = 0
var right_index: int = 0

func _ready() -> void:
	ILL = $ItemList_L
	ILR = $ItemList_R

	WalletArray = [
		get_node("../Wallet1"),
		get_node("../Wallet2")
	]

	$OptionButtonW1.clear()
	$OptionButtonW2.clear()
	for wallet in WalletArray:
		$OptionButtonW1.add_item(wallet.name)
		$OptionButtonW2.add_item(wallet.name)

	$OptionButtonW1.select(0)
	$OptionButtonW2.select(0)

	_update_itemlists_from_wallets($OptionButtonW1.get_selected_id(), $OptionButtonW2.get_selected_id())

func _on_option_button_w_1_item_selected(index: int) -> void:
	_update_itemlists_from_wallets(index, $OptionButtonW2.get_selected_id())

func _on_option_button_w_2_item_selected(index: int) -> void:
	_update_itemlists_from_wallets($OptionButtonW1.get_selected_id(), index)

func _update_itemlists_from_wallets(l_index: int, r_index: int) -> void:
	left_index = l_index
	right_index = r_index

	# validate indices
	if left_index < 0 or left_index >= WalletArray.size():
		ILL.clear(); ILR.clear(); return
	if right_index < 0 or right_index >= WalletArray.size():
		ILL.clear(); ILR.clear(); return

	var left_wallet = WalletArray[left_index]
	var right_wallet = WalletArray[right_index]

	ILL.clear()
	for key in left_wallet.get_currencies():
		ILL.add_item("ID: %s Amount: %s" % [str(key), left_wallet.get_currency_balance(key)])
	for key in left_wallet.get_items():
		ILL.add_item("ID: %s Amount: %s" % [str(key), left_wallet.get_item_balance(key)])

	ILR.clear()
	for key in right_wallet.get_currencies():            # = right_wallet (was left_wallet)
		ILR.add_item("ID: %s Amount: %s" % [str(key), right_wallet.get_currency_balance(key)])
	for key in right_wallet.get_items():                 # = right_wallet (was left_wallet)
		ILR.add_item("ID: %s Amount: %s" % [str(key), right_wallet.get_item_balance(key)])

func _on_transfer_button_pressed() -> void:
	# validate indices and inputs
	if left_index == right_index:
		return
	if left_index < 0 or right_index < 0:
		return
	var left_wallet = WalletArray[left_index]
	var right_wallet = WalletArray[right_index]
	if IDInput == "" or AmountInput <= 0:
		return
	# optional: check balances if your API exposes them
	left_wallet.transfer_currency_to(right_wallet, IDInput, AmountInput)
	_update_itemlists_from_wallets(left_index, right_index)

func _on_transfer_material_pressed() -> void:
	if left_index == right_index:
		return
	if IDInput == "" or AmountInput <= 0:
		return
	
	var left_wallet = WalletArray[left_index]
	var right_wallet = WalletArray[right_index]
	left_wallet.transfer_item_to(right_wallet, IDInput, AmountInput)
	
	_update_itemlists_from_wallets(left_index, right_index)

# Connect these to LineEdit signals
func _on_id_input_text_changed() -> void:
	var txt = $IDInput.get_text().strip_edges()
	IDInput = txt
	print(IDInput)

func _on_amount_input_text_changed() -> void:
	var txt = $AmountInput.get_text()
	if txt == "":
		AmountInput = 0
	else:
		AmountInput = int(txt.to_int())
	print(AmountInput)
