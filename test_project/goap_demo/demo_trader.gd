extends GoapTrader

# Ranks trade partners by distance, so an NPC walks to the nearest trader that has what it
# wants. The extension calls _rank_partner when a trade step starts and tries the lowest
# value first. It knows nothing about positions itself; this is the game's job.
#
# The GoapTrader is a child of the NPC's Node2D, so positions are read from the parents.


func _rank_partner(partner: Node, _action: GoapAction) -> float:
	var me := get_parent() as Node2D
	var other := partner.get_parent() as Node2D
	if me == null or other == null:
		return 0.0
	return me.global_position.distance_to(other.global_position)
