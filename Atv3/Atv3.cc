#include "RelayApp.hpp"
#include "ns3/log.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/netanim-module.h"

using namespace ns3;
//NS_LOG_COMPONENT_DEFINE("Atv2");

int main(int argc, char const *argv[])
{
    Time::SetResolution(Time::NS);
    
    // Node Creation
    NodeContainer nodes;
    nodes.Create(NUM_NODES);

    // Wifi Config
    WifiHelper wifi;
    YansWifiChannelHelper wifiChannel = ns3::YansWifiChannelHelper::Default();
    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetChannel (wifiChannel.Create ());
    WifiMacHelper wifiMac;
    wifiMac.SetType ("ns3::AdhocWifiMac"); // Set it to adhoc mode
    NetDeviceContainer devices;
    devices = wifi.Install (wifiPhy, wifiMac, nodes);

    // Mobility Config
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));
    positionAlloc->Add(Vector(20.0, 0.0, 0.0));
    positionAlloc->Add(Vector(30.0, 0.0, 0.0));
    positionAlloc->Add(Vector(40.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);

    // Internet (ipv4) config
    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // Application
    std::cout << "HERE" << std::endl;
    for(int i = 0; i < NUM_NODES; i++)
    {
        Ptr<RelayApp> app = CreateObject<RelayApp>();
        if(i == NUM_NODES - 1) // End Node (NUM_NODES - 1)
        {
            app->Setup(i, nodes.Get(i), true,
                interfaces.GetAddress(i - 1)
            );
            app->SetStartTime(Seconds(START_TIME_S));
            app->SetStopTime(Seconds(END_TIME_S));
            nodes.Get(i)->AddApplication(app);
            continue;
        }
        else // Middle Nodes
        {
            app->Setup(i, nodes.Get(i), false,
                i > 0 ? interfaces.GetAddress(i - 1) : Ipv4Address("0.0.0.0")
            );
            app->SetStartTime(Seconds(START_TIME_S));
            app->SetStopTime(Seconds(END_TIME_S));
            nodes.Get(i)->AddApplication(app);
        }
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    Time stop_time = Seconds(END_TIME_S);
    AnimationInterface anim("atividade3.xml");
    anim.EnableIpv4RouteTracking (
        "atividade3_routes.xml", 
        Seconds(0.5),
        Seconds(29.0)
    );
    anim.SetMaxPktsPerTraceFile(10000000);
    
    Simulator::Stop(stop_time);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
