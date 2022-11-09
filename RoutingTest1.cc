#include <iostream>
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RoutingTest");

int main(int argc, char *argv[])
{
    CommandLine cmd (__FILE__);
    cmd.Parse (argc, argv);

    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
    //LogComponentEnable ("TcpServer", LOG_LEVEL_INFO);
    //LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_ALL);
    //LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
    Time::SetResolution (Time::NS);
    Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (250));
    Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("5kb/s"));

    NS_LOG_INFO("Creating Topology");
    NodeContainer nodes;
    nodes.Create (4);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                    "MinX", DoubleValue (5),
                                    "MinY", DoubleValue (5),
                                    "DeltaX", DoubleValue (5.0),
                                    "DeltaY", DoubleValue (5.0),
                                    "GridWidth", UintegerValue (2),
                                    "LayoutType", StringValue ("RowFirst"));
    //mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
    pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

    NetDeviceContainer devices;
    NetDeviceContainer Temp11 = pointToPoint.Install (nodes.Get(0), nodes.Get(1));
    NetDeviceContainer Temp12 = pointToPoint.Install (nodes.Get(0), nodes.Get(2));
    NetDeviceContainer Temp31 = pointToPoint.Install (nodes.Get(3), nodes.Get(1));
    NetDeviceContainer Temp32 = pointToPoint.Install (nodes.Get(3), nodes.Get(2));
    devices.Add(Temp11);devices.Add(Temp12);devices.Add(Temp31);devices.Add(Temp32);

    InternetStackHelper stack;
    stack.Install (nodes);

    Ipv4AddressHelper address;
    address.SetBase ("10.1.0.0", "255.255.0.255");

    Ipv4InterfaceContainer interfaces = address.Assign (devices);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::cout << interfaces.GetN() << std::endl;

    UdpEchoServerHelper echoServer (9);

    ApplicationContainer serverApps = echoServer.Install (nodes.Get (3));
    serverApps.Start (Seconds (1.0));
    serverApps.Stop (Seconds (10.0));

    /*UdpEchoClientHelper echoClient (interfaces.GetAddress (6), 9);
    //Ptr<Ipv4InterfaceContainer> ClientAddress = nodes.Get(3)->GetObject<Ipv4InterfaceContainer>();
    //Address Add3 = ClientAddress->GetAddress(0);
    //UdpEchoClientHelper echoClient (nodes.Get(3)->GetObject<Ipv4InterfaceContainer>()->GetAddress(0), 9);
    echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
    echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
    clientApps.Start (Seconds (2.0));
    clientApps.Stop (Seconds (10.0));*/

    //
    // Create an OnOff application to send UDP datagrams from node zero to node 1.
    //
    NS_LOG_INFO ("Create Applications.");
    uint16_t port = 9;   // Discard port (RFC 863)

    // Create an optional packet sink to receive these packets
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                            Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
    //ApplicationContainer app;
    ApplicationContainer sinkApp;
    sinkApp = sink.Install (nodes.Get (3));
    sinkApp.Start (Seconds (0.0));
    sinkApp.Stop(Seconds(10.0));

    // 
    // Create a similar flow from n3 to n0, starting at time 1.1 seconds
    //
    OnOffHelper onoff ("ns3::TcpSocketFactory", Address());
    onoff.SetConstantRate (DataRate ("500kb/s"));

    onoff.SetAttribute ("Remote", 
                        AddressValue (InetSocketAddress (Ipv4Address ("10.1.0.7"), port)));
    onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
    onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.8]"));
    ApplicationContainer app;
    app = onoff.Install (nodes.Get (0));
    app.Start (Seconds (1.0));
    app.Stop (Seconds (10.0));

    //app = sink.Install (nodes.Get (0));
    //app.Start (Seconds (0.0));
    
    // Trace routing tables 
    Ipv4GlobalRoutingHelper g;
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("RoutingTest1.routes", std::ios::out);
    g.PrintRoutingTableAllAt (Seconds (1), routingStream);

    AsciiTraceHelper ascii;
    pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("scratch/RoutingTest1.tr"));
    pointToPoint.EnablePcapAll ("scratch/RoutingTest1");
    AnimationInterface anim("RoutingTest1.xml");
    Simulator::Run ();
    Simulator::Stop(Seconds(10.0));
    Simulator::Destroy ();

}