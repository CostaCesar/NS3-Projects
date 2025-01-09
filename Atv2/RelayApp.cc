
#include "ns3/core-module.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/applications-module.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/double.h"
#include "ns3/log.h"

#include <netinet/in.h>
#include "RelayApp.hpp"

NS_LOG_COMPONENT_DEFINE("Atv2");
using namespace ns3;

RelayApp::RelayApp () : 
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
    Ipv4Address right_address, Ipv4Address left_address)
{
    this->index = index;
    this->node = node;
    this->right_address = right_address;
    this->left_address = left_address;
    this->is_edge = is_edge;
}

void
RelayApp::StartApplication (void)
{
    Time dropTime = Seconds(30.0);
    Simulator::Schedule(dropTime, &RelayApp::StopApplication, this); 

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

    if(this->index == 0){
        int32_t number =  1 + rand() % 100;
        Connect(this->left_address);
        SendPacket(number);
    }

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
    NS_LOG_UNCOND("Aplicação encerrada");
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
    int32_t receivedNumber = 0;

    this->sender_socket = Socket::CreateSocket(
        this->node, TcpSocketFactory::GetTypeId ()
    );

    while ((packet = socket->RecvFrom(from))) {
        if (packet->GetSize() == 0) {
            break;
        }
        InetSocketAddress inetFrom = InetSocketAddress::ConvertFrom(from);
        packet->CopyData((uint8_t *)&networkOrderNumber, sizeof(networkOrderNumber));
        receivedNumber = ntohl(networkOrderNumber);
        NS_LOG_INFO("[" << this->index << "] Numero Recebido: " << receivedNumber);

        if (this->is_edge) {
            receivedNumber = 1 + rand() % 100;
            Connect(this->left_address);
        }
        else {
            if (this->right_address == inetFrom.GetIpv4()) {
                Connect(this->left_address);
            }
            else {
                Connect(this->right_address);
            }
        }
        
        SendPacket(receivedNumber);
    } 
}

void
RelayApp::SendPacket (int32_t number)
{
    int32_t networkOrderNumber = htonl(number);
    Ptr<Packet> packet = Create<Packet>((uint8_t *)&networkOrderNumber, sizeof(networkOrderNumber));
    this->sender_socket->Send(packet);
    NS_LOG_DEBUG("Sending " << number);
    sender_socket->Close();
}

void
RelayApp::Connect (Ipv4Address neighbor_address)
{
    this->sender_socket->SetConnectCallback (
        MakeCallback(&RelayApp::OnConnectionSucceeded, this),
        MakeCallback(&RelayApp::OnConnectionFailed, this)
    );
    InetSocketAddress remote = InetSocketAddress(neighbor_address, this->k_receiver_port);
    this->sender_socket->Connect(remote);
    NS_LOG_DEBUG("Node "<< this->index << " linked with " << neighbor_address);
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
