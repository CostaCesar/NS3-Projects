
#include "ns3/core-module.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/applications-module.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/constant-velocity-mobility-model.h"

#include <netinet/in.h>
#include "RelayApp.hpp"

NS_LOG_COMPONENT_DEFINE("Atv3");
using namespace ns3;

RelayApp::RelayApp () : 
    velocity(0),
    is_edge (false),
    sender_socket (0),
    receiver_socket (0)
{
    return;
}

RelayApp::~RelayApp ()
{
    sender_socket = 0;
    receiver_socket = 0;
}

TypeId 
RelayApp::GetTypeId (void)
{
    static TypeId tid = TypeId ("RelayApp")
        .SetParent<Application>()
        .AddConstructor<RelayApp>();
    return tid;
}

void
RelayApp::Setup (int index, Ptr<Node> node, bool is_edge,
    Ipv4Address left_address)
{
    this->index = index;
    this->node = node;
    this->left_address = left_address;
    this->is_edge = is_edge;
    this->velocity = START_VELOCITY;
}

void
RelayApp::StartApplication (void)
{
    Ptr<Socket> receiver_socket = Socket::CreateSocket(
        this->node, TcpSocketFactory::GetTypeId ()
    );
    Ptr<Socket> sender_socket = Socket::CreateSocket(
        this->node, TcpSocketFactory::GetTypeId ()
    );

    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), k_receiver_port);
    if (receiver_socket->Bind(local) == -1)
    {
      NS_FATAL_ERROR("Failed to bind socket");
    }
    receiver_socket->Listen();
    receiver_socket->SetAcceptCallback(
      MakeCallback(&RelayApp::OnConnectionRequested, this),
      MakeCallback(&RelayApp::OnAccept, this)
    );

    this->receiver_socket = receiver_socket;
    this->sender_socket = sender_socket;

    if(this->index != 0){
        Connect(this->left_address);
    }
    if(this->index == 4){
        GenVelocity();
    }

}

void
RelayApp::GenVelocity()
{
    this->velocity = (rand() % 7) + 2;
    NS_LOG_INFO("Nova velocidade: " << this->velocity);
    UpdateVelocity();
    
    SendPacket();
    Simulator::Schedule(Seconds(DELAY_TIME), &RelayApp::GenVelocity, this);
}

void
RelayApp::UpdateVelocity()
{
    this->node->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
        Vector(this->velocity, 0, 0)
    );
}

void
RelayApp::StopApplication(void)
{
    if (this->receiver_socket)
    {
        this->receiver_socket->Close();
        this->receiver_socket = nullptr;
    }

    if (this->sender_socket)
    {
        this->sender_socket->Close();
        this->sender_socket = nullptr;
    }
    NS_LOG_DEBUG("End of app");
}

void
RelayApp::OnAccept(Ptr<Socket> s, const Address& from)
{
    s->SetRecvCallback(MakeCallback(&RelayApp::OnReceive, this));
}

void
RelayApp::OnReceive(Ptr<Socket> socket)
{
    Address from;
    Ptr<Packet> packet;
    int32_t networkOrderNumber;

    this->sender_socket = Socket::CreateSocket(
        this->node, TcpSocketFactory::GetTypeId ()
    );

    while ((packet = socket->RecvFrom(from))) {
        if (packet->GetSize() == 0) {
            break;
        }
        packet->CopyData((uint8_t *)&networkOrderNumber, sizeof(networkOrderNumber));
        this->velocity = ntohl(networkOrderNumber);
        NS_LOG_INFO("[" << this->index << "] Velocidade recebida: " << this->velocity);
        UpdateVelocity();

        if(this->index != 0)
        {
            Connect(this->left_address);
            SendPacket();
            break;
        }
    }
}

void
RelayApp::SendPacket ()
{
    int32_t networkOrderNumber = htonl(this->velocity);
    Ptr<Packet> packet = Create<Packet>((uint8_t *)&networkOrderNumber, sizeof(networkOrderNumber));
    this->sender_socket->Send(packet);
    NS_LOG_DEBUG("Sending " << velocity);
    //this->sender_socket->Close();
}

void
RelayApp::Connect (Ipv4Address neighbor_address)
{
    NS_LOG_DEBUG("Node " << this->index << " trying connection to " << neighbor_address);
    this->sender_socket->SetConnectCallback (
        MakeCallback(&RelayApp::OnConnectionSucceeded, this),
        MakeCallback(&RelayApp::OnConnectionFailed, this)
    );
    InetSocketAddress remote = InetSocketAddress(neighbor_address, this->k_receiver_port);
    this->sender_socket->Connect(remote);
    //NS_LOG_DEBUG("Node "<< this->index << " linked with " << neighbor_address);
}

void
RelayApp::OnConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_DEBUG("Connected");
}

void
RelayApp::OnConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_DEBUG("Failed to connect");
}

bool
RelayApp::OnConnectionRequested(Ptr<Socket> socket, const Address& from) {
    NS_LOG_DEBUG("Connection request by " << from);
    return true;
}
