extends EconomyManager

func _ready() -> void:
	connect("currency_registered", Callable(self, "_on_currency_registered"))
	create_currency(0, "gold")

func _on_currency_registered(currency):
	print("currency registered: ", currency.get_name(), ", currency id: ", currency.get_id())
