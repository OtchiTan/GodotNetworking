#include "gn_network_manager.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void godot::GDNetworkManager::_set_non_blocking(socket_t sock)
{
#ifdef _WIN32
    unsigned long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | 0_NONBLOCK);
#endif
}

void godot::GDNetworkManager::close_socket()
{
    UtilityFunctions::print("Disconnect");
    set_state(NOT_CONNECTED);
    if (udp_socket != INVALID_SOCKET)
    {
        _has_authority = false;
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
    }
}

void godot::GDNetworkManager::on_state_timeout()
{
    if (_has_authority)
        return;

    switch (current_state)
    {
    case NOT_CONNECTED:
        _send_helo();
        break;
    case CONNECTING:
        _send_hsk();
        break;
    case CONNECTED:
        UtilityFunctions::print("Connected");
        break;
    case SPURIOUS:
        if (!_is_ping_sent)
            _send_ping();
        else
            close_socket();
        break;
    default:
        break;
    }
}

void godot::GDNetworkManager::set_state(NetworkState p_state)
{
    if (current_state == p_state)
        return;
    current_state = p_state;
    _is_ping_sent = false;
    on_state_timeout();
    emit_signal("network_state_chanded", current_state);
}

void godot::GDNetworkManager::_parse_packet(char sender_ip[INET_ADDRSTRLEN], int sender_port, const PackedByteArray &data)
{
    emit_signal("packet_received", String(sender_ip), sender_port, data);

    if (!_has_authority)
        _time_since_last_data = 0.f;

    MessageType message_type = (MessageType)data[0];
    PackedByteArray data_to_send;
    switch (message_type)
    {
    case MSG_HELO:
        UtilityFunctions::print("Helo");
        if (_has_authority)
            _send_helo(sender_ip, sender_port);
        else
            set_state(CONNECTING);
        break;
    case MSG_HSK:
        UtilityFunctions::print("Handshake");
        if (_has_authority)
        {
            _register_client(ObjectID(), sender_ip, sender_port);
            _send_hsk(sender_ip, sender_port);
        }
        else
            set_state(CONNECTED);
        break;
    case MSG_PING:
        if (_has_authority)
            _send_pong(sender_ip, sender_port);
        else
            set_state(CONNECTED);
        break;
    case MSG_DATA:
        break;
    default:
        UtilityFunctions::print("Unrecognized packet");
        break;
    }
}

void GDNetworkManager::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("start_server", "port"), &GDNetworkManager::start_server);
    ClassDB::bind_method(D_METHOD("poll"), &GDNetworkManager::poll);
    ClassDB::bind_method(D_METHOD("close_socket"), &GDNetworkManager::close_socket);
    ClassDB::bind_method(D_METHOD("connect_socket"), &GDNetworkManager::connect_socket);
    ClassDB::bind_method(D_METHOD("on_state_timeout"), &GDNetworkManager::on_state_timeout);

    ADD_SIGNAL(MethodInfo("packet_received",
                          PropertyInfo(Variant::STRING, "sender_ip"),
                          PropertyInfo(Variant::INT, "sender_port"),
                          PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data")));
    ADD_SIGNAL(MethodInfo("network_state_chanded", PropertyInfo(Variant::INT, "current_state")));
}

GDNetworkManager::GDNetworkManager()
{
#ifdef _WIN32

    static GD_WSASockInitializer sock_initializer{};
#endif
}

GDNetworkManager::~GDNetworkManager()
{
    // Add your cleanup here.
}

void GDNetworkManager::_process(double delta)
{
    if (_has_authority)
    {
        for (const GDConnectedSocket &socket : _connected_sockets)
        {
            _replicate_data(socket.ip, socket.port);
        }
    }
    else if (current_state == CONNECTED)
    {
        _time_since_last_data += delta;
        if (_time_since_last_data > 2.f)
        {
            set_state(SPURIOUS);
            _send_ping();
        }
    }
    poll();
}

void godot::GDNetworkManager::_ready()
{
}

bool godot::GDNetworkManager::start_server(int port)
{
    _has_authority = true;
    return _bind_port(port);
}

bool godot::GDNetworkManager::_bind_port(int port)
{
    close_socket();

    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket == INVALID_SOCKET)
    {
        UtilityFunctions::printerr("create socket failed: " + WSAGetLastError());
        return false;
    }

    _set_non_blocking(udp_socket);

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(port);
    if (bind(udp_socket, (struct sockaddr *)&service, sizeof(service)) == SOCKET_ERROR)
    {
        UtilityFunctions::printerr("bind failed with error: " + WSAGetLastError());
        close_socket();
        return false;
    }

    UtilityFunctions::print("Socket bound to port : " + port);
    return true;
}

void godot::GDNetworkManager::_send_helo()
{
    _send_helo(_server_ip, _server_port);
}

void godot::GDNetworkManager::_send_helo(String ip, int port)
{
    PackedByteArray data;
    _send_message(ip, port, MSG_HELO, data);
}

void godot::GDNetworkManager::_send_hsk()
{
    _send_hsk(_server_ip, _server_port);
}

void godot::GDNetworkManager::_send_hsk(String ip, int port)
{
    PackedByteArray data;
    _send_message(ip, port, MSG_HSK, data);
}

void godot::GDNetworkManager::_send_ping()
{
    _is_ping_sent = true;
    PackedByteArray data;
    _send_message(_server_ip, _server_port, MSG_PING, data);
}

void godot::GDNetworkManager::_send_pong(String ip, int port)
{
    PackedByteArray data;
    _send_message(ip, port, MSG_PING, data);
}

void godot::GDNetworkManager::_replicate_data(String ip, int port)
{
    if (!_has_authority)
        return;
    PackedByteArray data;
    _send_message(ip, port, MSG_DATA, data);
}

void godot::GDNetworkManager::_register_client(ObjectID id, String ip, int port)
{
    GDConnectedSocket client_socket = GDConnectedSocket();
    client_socket.user_id = ObjectID();
    client_socket.ip = ip;
    client_socket.port = port;
    _connected_sockets.push_back(client_socket);
}

void godot::GDNetworkManager::_send_packet(String ip, int port, const PackedByteArray &data)
{
    if (udp_socket == INVALID_SOCKET)
    {
        UtilityFunctions::printerr("Le socket il est pt");
        return;
    }

    sockaddr_in dest_addr{};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    inet_pton(AF_INET, ip.utf8().get_data(), &dest_addr.sin_addr);
    int send_bytes = sendto(udp_socket, (const char *)data.ptr(), data.size(), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (send_bytes == SOCKET_ERROR)
    {
        UtilityFunctions::printerr("Ça à rien envoyer envie de vècre fort là");
        return;
    }
}

void godot::GDNetworkManager::_send_message(String ip, int port, MessageType message_type, const PackedByteArray &data)
{
    PackedByteArray total_data;
    total_data.append(message_type);
    total_data.append(ObjectID());
    total_data.append_array(data);

    _send_packet(ip, port, total_data);
}

void godot::GDNetworkManager::connect_socket(String ip, int port)
{
    _bind_port(0);
    _server_ip = ip;
    _server_port = port;
    _send_helo();
}

void godot::GDNetworkManager::poll()
{
    if (udp_socket == INVALID_SOCKET)
        return;

    char buffer[65535];
    sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    while (true)
    {
        int len = recvfrom(udp_socket, buffer, sizeof(buffer), 0, (struct sockaddr *)&sender_addr, &sender_len);

        if (len < 0)
        {
            break;
        }

        PackedByteArray received_data;
        received_data.resize(len);
        memcpy(received_data.ptrw(), buffer, len);

        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);
        int sender_port = ntohs(sender_addr.sin_port);

        _parse_packet(sender_ip, sender_port, received_data);
    }
}

void godot::GDNetworkManager::_notification(int p_what)
{
    switch (p_what)
    {
    case NOTIFICATION_READY:
        _ready();
        break;
    case NOTIFICATION_PROCESS:
        _process(get_process_delta_time());
        break;
    }
}