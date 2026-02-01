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
    if (udp_socket != INVALID_SOCKET)
    {
#ifdef _WIN32
        closesocket(udp_socket);
#else
        close(udp_socket);
#endif
    }
}

void godot::GDNetworkManager::set_state(NetworkState p_state)
{
    if (current_state == p_state)
        return;
    current_state = p_state;
    UtilityFunctions::print("Network State Changed: ", p_state);
}

void GDNetworkManager::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("bind_port", "port"), &GDNetworkManager::bind_port);
    ClassDB::bind_method(D_METHOD("send_packet", "ip", "port", "data"), &GDNetworkManager::send_packet);
    ClassDB::bind_method(D_METHOD("poll"), &GDNetworkManager::poll);
    ClassDB::bind_method(D_METHOD("close_socket"), &GDNetworkManager::close_socket);

    ADD_SIGNAL(MethodInfo("packet_received",
                          PropertyInfo(Variant::STRING, "sender_ip"),
                          PropertyInfo(Variant::INT, "sender_port"),
                          PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data")));
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
}

void godot::GDNetworkManager::_ready()
{
}

bool godot::GDNetworkManager::bind_port(int port)
{
    UtilityFunctions::print("Start socket to port : " + port);
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

void godot::GDNetworkManager::send_packet(String ip, int port, PackedByteArray data)
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

void godot::GDNetworkManager::connect_socket(String ip, int port)
{
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

        emit_signal("packet_received", String(sender_ip), sender_port, received_data);
    }
}
