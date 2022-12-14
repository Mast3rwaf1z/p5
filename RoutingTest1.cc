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

static std::map<uint32_t, bool> firstCwnd;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> cWndStream;
static std::map<uint32_t, uint32_t> cWndValue;

static uint32_t
GetNodeIdFromContext (std::string context)
{
  std::size_t const n1 = context.find_first_of ("/", 1);
  std::size_t const n2 = context.find_first_of ("/", n1 + 1);
  return std::stoul (context.substr (n1 + 1, n2 - n1 - 1));
}

static void
CwndTracer (std::string context,  uint32_t oldval, uint32_t newval)
{
  //NS_LOG_LOGIC("Called CwndTracer");
  uint32_t nodeId = GetNodeIdFromContext (context);

  if (firstCwnd[nodeId])
    {
      *cWndStream[nodeId]->GetStream () << "Time,Window Size" << std::endl;
      *cWndStream[nodeId]->GetStream () << "0.0," << oldval << std::endl;
      firstCwnd[nodeId] = false;
    }
  *cWndStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << newval << std::endl;
  cWndValue[nodeId] = newval;
  //NS_LOG_LOGIC("Finished CwndTracer");
}

static void
TraceCwnd (std::string cwnd_tr_file_name, uint32_t nodeId)
{
  NS_LOG_LOGIC("Called TraceCwnd");
  AsciiTraceHelper ascii;
  cWndStream[nodeId] = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                   MakeCallback (&CwndTracer));
  NS_LOG_LOGIC("Finished TraceCwnd");
}


int main(int argc, char *argv[])
{
  std::string transport_prot = "TcpNewReno";
  uint64_t data_mbytes = 0;
  uint32_t mtu_bytes = 400;
  std::string bottleneckRate = "2Kbps";
  std::string bottleneckDelay = "10ms";
  std::string datarate = "5Kbps";
  std::string delay = "2ms";
  bool sack = false;
  double startTime = 0;
  double switchTime = 10.0;
  double stopTime = 25.0;


  CommandLine cmd (__FILE__);
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, TcpLinuxReno, "
              "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
              "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
              "TcpLp, TcpDctcp, TcpCubic, TcpBbr", transport_prot);
  cmd.AddValue ("data", "Number of Megabytes of data to transmit", data_mbytes);
  cmd.AddValue ("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue ("bottleneckRate", "Datarate of bottleneck point to point links", bottleneckRate);
  cmd.AddValue ("bottleneckDelay", "Delay of bottleneck point to point links", bottleneckDelay);
  cmd.AddValue ("datarate", "Datarate of point to point links", datarate);
  cmd.AddValue ("delay", "Delay of point to point links", delay);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("switchTime", "Time for the route to swtich", switchTime);
  cmd.AddValue ("stopTime", "Time for the simulation to stop", stopTime);
  cmd.Parse (argc, argv);
  transport_prot = std::string ("ns3::") + transport_prot;
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

  LogComponentEnable ("RoutingTest", LOG_LEVEL_ALL);
  //LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("TcpServer", LOG_LEVEL_INFO);
  //LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_ALL);
  //LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  Time::SetResolution (Time::US);
  //Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (250));
  //Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("50kb/s"));

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);


  NS_LOG_INFO("Creating Topology");
  NodeContainer nodes, srcNode, dstNode;
  srcNode.Create (1);
  nodes.Create (4);
  dstNode.Create (1);
  NodeContainer allNodes(srcNode, nodes, dstNode);

  ListPositionAllocator posAloc;
  posAloc.Add(Vector(0,10,0));posAloc.Add(Vector(5,10,0));posAloc.Add(Vector(10,5,0));posAloc.Add(Vector(10,15,0));posAloc.Add(Vector(15,10,0));posAloc.Add(Vector(20,10,0));

  MobilityHelper mobility;
  /*mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (5),
                                  "MinY", DoubleValue (5),
                                  "DeltaX", DoubleValue (5.0),
                                  "DeltaY", DoubleValue (5.0),
                                  "GridWidth", UintegerValue (2),
                                  "LayoutType", StringValue ("RowFirst"));
  //mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");*/
  mobility.SetPositionAllocator(&posAloc);
  mobility.Install(allNodes);


  PointToPointHelper pointToPoint;
  //pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (datarate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));

  NetDeviceContainer devices;
  NetDeviceContainer Temp01 = pointToPoint.Install (srcNode.Get(0), nodes.Get(0));
  NetDeviceContainer Temp12 = pointToPoint.Install (nodes.Get(0), nodes.Get(1));
  NetDeviceContainer Temp24 = pointToPoint.Install (nodes.Get(3), nodes.Get(1));
  NetDeviceContainer Temp45 = pointToPoint.Install (nodes.Get(3), dstNode.Get(0));
  
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bottleneckRate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));
  NetDeviceContainer Temp13 = pointToPoint.Install (nodes.Get(0), nodes.Get(2));
  NetDeviceContainer Temp34 = pointToPoint.Install (nodes.Get(3), nodes.Get(2));

  devices.Add(Temp01);devices.Add(Temp12);devices.Add(Temp13);devices.Add(Temp24);devices.Add(Temp34); devices.Add(Temp45);

  InternetStackHelper stack;
  stack.Install (allNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.0.255");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  //Simulator::Schedule (Seconds (0.0001),&Ipv4::SetDown,nodes.Get(0)->GetObject<Ipv4>(), 3);
  //Ipv4::SetDown(nodes.Get(0)->GetObject<Ipv4>(), 3);
  nodes.Get(0)->GetObject<Ipv4>()->SetDown(3);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  

  std::cout << interfaces.GetN() << std::endl;

  /*UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (3));
  serverApps.Start (Seconds (startTime + 1.0));
  serverApps.Stop (Seconds (stopTime));*/

  /*UdpEchoClientHelper echoClient (interfaces.GetAddress (6), 9);
  //Ptr<Ipv4InterfaceContainer> ClientAddress = nodes.Get(3)->GetObject<Ipv4InterfaceContainer>();
  //Address Add3 = ClientAddress->GetAddress(0);
  //UdpEchoClientHelper echoClient (nodes.Get(3)->GetObject<Ipv4InterfaceContainer>()->GetAddress(0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (startTime + 2.0));
  clientApps.Stop (Seconds (stopTime));*/

  //
  // Create an application to send TCP Packets from node 0 to node 3.
  //
  NS_LOG_INFO ("Create Applications.");
  uint16_t port = 9;   // Discard port (RFC 863)

  // Create an packet sink to receive these packets
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                          Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  //ApplicationContainer app;
  ApplicationContainer sinkApp;
  sinkApp = sink.Install (dstNode.Get(0));
  //sinkApp = sink.Install (nodes.Get(1));
  sinkApp.Start (Seconds (startTime));
  sinkApp.Stop(Seconds(stopTime));

  //
  // Create BulktSend application
  //
  
  //NS_LOG_INFO("NInterface of node is: " << dstNode.Get(0)->GetObject<Ipv4>()->GetNInterfaces());
  //NS_LOG_INFO("NAddress of node is: " << dstNode.Get(0)->GetObject<Ipv4>()->GetNAddresses(1));
  NS_LOG_INFO("Address of node is: " << dstNode.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetAddress());
  //AddressValue remoteAddress (InetSocketAddress (Ipv4Address ("10.1.0.12"), port));
  AddressValue remoteAddress (InetSocketAddress (dstNode.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetAddress(), port));
  //AddressValue remoteAddress (InetSocketAddress (Ipv4Address ("10.1.0.4"), port));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  ftp.SetAttribute ("Remote", remoteAddress);
  ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  ftp.SetAttribute ("MaxBytes", UintegerValue (data_mbytes * 1000000));

  ApplicationContainer sourceApp = ftp.Install (srcNode.Get (0));
  //sourceApp.Start (Seconds (startTime + 1.0));
  sourceApp.Start (Seconds (startTime));
  sourceApp.Stop (Seconds (stopTime));

  // 
  // Create a similar flow from n3 to n0, starting at time 1.1 seconds
  //
  /*OnOffHelper onoff ("ns3::TcpSocketFactory", Address());
  onoff.SetConstantRate (DataRate ("500kb/s"));

  onoff.SetAttribute ("Remote", 
                      AddressValue (InetSocketAddress (Ipv4Address ("10.1.0.7"), port)));
  onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.8]"));
  ApplicationContainer app;
  app = onoff.Install (nodes.Get (0));
  app.Start (Seconds (startTime + 1.0));
  app.Stop (Seconds (stopTime));*/

  //app = sink.Install (nodes.Get (0));
  //app.Start (Seconds (startTime));
  
  Simulator::Schedule (Seconds (switchTime),&Ipv4::SetDown,nodes.Get(0)->GetObject<Ipv4>(), 2);
  Simulator::Schedule (Seconds (switchTime),&Ipv4::SetUp,nodes.Get(0)->GetObject<Ipv4>(), 3);
  //Simulator::Schedule (Seconds (switchTime+1), ns3::Ipv4GlobalRoutingHelper::PopulateRoutingTables);

  // Trace routing tables 
  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("scratch/P5/Statistics/RoutingTest1.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (0), routingStream);
  Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper> ("scratch/P5/Statistics/RoutingTest12.routes", std::ios::out);
  //g.PrintRoutingTableAllAt (Seconds (switchTime+1), routingStream2);
  g.PrintRoutingTableAllAt (Seconds (switchTime+0.0001), routingStream2);
  //Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper> ("scratch/P5/Statistics/RoutingTest13.routes", std::ios::out);
  //g.PrintRoutingTableAllAt (Seconds (switchTime+1.0001), routingStream3);

  

  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("scratch/P5/Traces/RoutingTest1.tr"));
  pointToPoint.EnablePcapAll ("scratch/P5/Pcap/RoutingTest1");
  firstCwnd[0] = true;
  //Simulator::Schedule (Seconds (startTime + 1.00001), &TraceCwnd, "scratch/P5/Statistics/RoutingTest1-cwnd.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceCwnd, "scratch/P5/Statistics/RoutingTest1-cwnd.data", 0);
  AnimationInterface anim("scratch/P5/Animations/RoutingTest1.xml");
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();
  Simulator::Destroy ();

}