#pragma once

#include <godot_cpp/classes/node.hpp>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET socket_t;
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace godot
{
    enum NetworkState
    {
        NOT_CONNECTED,
        CONNECTING,
        CONNECTED,
        SPURIOUS
    };

    enum MessageType : uint8_t
    {
        MSG_HELO = 0,
        MSG_HSK = 1,
        MSG_PING = 2
    };

    struct GD_WSASockInitializer
    {
        GD_WSASockInitializer()
        {
            WSADATA wsaData;
            WORD wVersionRequested = MAKEWORD(2, 2);
            int wsaerr = WSAStartup(wVersionRequested, &wsaData);
            if (wsaerr != 0)
            {
                UtilityFunctions::printerr("WSAStartup failed: %d\n", wsaerr);
            }
        }

        ~GD_WSASockInitializer()
        {
            WSACleanup();
        }
    };

    struct GDReplicatedNode
    {
        ObjectID node_id;
        std::vector<StringName> properties;

        // Check if node is still valid
        bool is_valid() const
        {
            return Object::cast_to<Node>(ObjectDB::get_instance(node_id)) != nullptr;
        }

        Node *get_node() const
        {
            return Object::cast_to<Node>(ObjectDB::get_instance(node_id));
        }
    };

    class GDNetworkManager : public Node
    {
        GDCLASS(GDNetworkManager, Node)
    private:
        socket_t udp_socket = INVALID_SOCKET;

        // Internal helper to set socket to non-blocking mode
        void _set_non_blocking(socket_t sock);

        std::vector<GDReplicatedNode> replicated_nodes;

        NetworkState current_state = NOT_CONNECTED;
    void set_state(NetworkState p_state);
    protected:
        static void _bind_methods();

    public:
        GDNetworkManager();
        ~GDNetworkManager();

        void _process(double delta) override;
        void _ready() override;

        // Bind to a port to receive data (Server or P2P Peer)
        bool bind_port(int port);

        // Send a packet to a specific IP/Port
        void send_packet(String ip, int port, PackedByteArray data);
        void connect_socket(String ip, int port);

        // Internal helper to close socket safely on all platforms
        void close_socket();

        // Check for incoming packets (Call this in _process)
        void poll();
    };
}