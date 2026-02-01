extends GDNetworkManager

@export var port = 3630

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var args = OS.get_cmdline_args()
	var is_server = false
	for arg in args:
		if arg == "--server":
			is_server = true
	if (is_server): 
		if (start_server(port)):
			print("Network Manager")
	else:
		connect_socket("127.0.0.1", port)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass


func _on_packet_received(sender_ip: String, sender_port: int, data: PackedByteArray) -> void:
	print(data.get_string_from_utf8())


func _on_game_tree_exiting() -> void:
	close_socket()


func _on_timer_timeout() -> void:
	on_state_timeout()
