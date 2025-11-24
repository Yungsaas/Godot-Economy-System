extends EconomyManager

#this script serves as an example, please visit the wiki for proper documentation

func _ready() -> void:
	connect("currency_registered", Callable(self, "_on_currency_registered"))
	create_currency("gold", "Gold Coin")

func _on_currency_registered(currency):
	print("currency registered: ", currency.get_name(), ", currency id: ", currency.get_id(), ", default value: ", currency.get_base_value())
