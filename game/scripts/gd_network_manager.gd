extends GDNetworkManager

@export var port = 3630
@onready var server_state_label: Label = $ServerStateLabel

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


func _on_network_state_chanded(current_state: int) -> void:
	match current_state:
		0:
			server_state_label.text = "Not Connected"
		1:
			server_state_label.text = "Connecting"
		2:
			server_state_label.text = "Connected"
		3:
			server_state_label.text = "Spurious"
