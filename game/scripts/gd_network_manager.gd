extends GDNetworkManager

@export var port = 3630

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	if (bind_port(port)):
		print("Network Manager")
		
	var data = "Feur".to_utf8_buffer()
	send_packet("127.0.0.1", port, data)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	poll()


func _on_packet_received(sender_ip: String, sender_port: int, data: PackedByteArray) -> void:
	print(data.get_string_from_utf8())


func _on_game_tree_exiting() -> void:
	close_socket()
