#include "ns3/socket.h"
#include "ns3/application.h"

using namespace ns3;
#define NUM_NODES (5)
#define START_TIME_S (1.0)
#define END_TIME_S (5.0)


//NS_LOG_COMPONENT_DEFINE("RelayNode");

class RelayApp : public Application
{
private:
    const uint16_t k_receiver_port = 8080;

    int index;
    Ptr<Node> node;
    //bool is_running;

    bool is_edge;
    Ptr<Socket> sender_socket;
    Ptr<Socket> receiver_socket;
    Ipv4Address right_address;
    Ipv4Address left_address;
    

    void OnAccept (Ptr<Socket> s, const Address& from);
    void OnReceive (Ptr<Socket> socket);
    void OnConnectionSucceeded(Ptr<Socket> socket);
    void OnConnectionFailed(Ptr<Socket> socket);
    bool OnConnectionRequested(Ptr<Socket> socket, const Address& from);

    void Connect (Ipv4Address neighbor_address);
    void SendPacket (int32_t number);

public:
    static TypeId GetTypeId (void);

    void Setup (int index, Ptr<Node> node, bool is_edge,
        Ipv4Address right_address, Ipv4Address left_address
    );
    void StartApplication() override;
    void StopApplication() override;

    RelayApp();
    virtual ~RelayApp();
};