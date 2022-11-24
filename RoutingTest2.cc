#include <iostream>
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/error-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RoutingTest2");

// Static variables
static std::map<uint32_t, bool> firstCwnd;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> cWndStream;
static std::map<uint32_t, uint32_t> cWndValue;

// Adjustable variables
std::string transport_prot = "TcpNewReno";
uint64_t data_mbytes = 0;
uint32_t mtu_bytes = 400;
std::string bottleneckRate = "20Kbps";
std::string bottleneckDelay = "30ms";
std::string datarate = "50Kbps";
std::string delay = "10ms";
bool dropPackets = true;
double error_p = 0.0;
bool sack = false;
double RSP = 1;
double startTime = 0;
double switchTime = 1.0;
double stopTime = 10.0;

/*
// Configure the error model
// Here we use RateErrorModel with packet error rate
Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
//uv->SetStream (50);
RateErrorModel error_model;
error_model.SetRandomVariable (uv);
error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
error_model.SetRate (1.0);
*/
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

static void
RouteSwitch(ns3::Time Period, Ptr<Ipv4> defaultIpv4, uint32_t defaultInterface, Ptr<Ipv4> bottleneckIpv4, uint32_t bottleneckInterface)
{
  //NS_LOG_INFO("Calling RouteSwitch at time " << Simulator::Now().GetSeconds());
  if(defaultIpv4->IsUp(defaultInterface))
  {
    //defaultIpv4->GetNetDevice(defaultInterface)->GetObject<Queue>();
    defaultIpv4->SetDown(defaultInterface);
    bottleneckIpv4->SetUp(bottleneckInterface);

  }
  else
  {
    defaultIpv4->SetUp(defaultInterface);
    bottleneckIpv4->SetDown(bottleneckInterface);
  }
  Simulator::Schedule(Period, RouteSwitch, Period, defaultIpv4, defaultInterface, bottleneckIpv4, bottleneckInterface);
}

static void
RouteSwitch2(ns3::Time Period, Ptr<Ipv4> forkIpv4, uint32_t defaultInterface, uint32_t bottleneckInterface)
{
  //NS_LOG_INFO("Calling RouteSwitch at time " << Simulator::Now().GetSeconds());
  if(forkIpv4->IsUp(defaultInterface))
  {
    //defaultIpv4->GetNetDevice(defaultInterface)->GetObject<Queue>();
    forkIpv4->SetDown(defaultInterface);
    forkIpv4->GetNetDevice(defaultInterface)->GetChannel()->SetAttribute("Delay", StringValue("1000ms"));
    forkIpv4->SetUp(bottleneckInterface);
    forkIpv4->GetNetDevice(bottleneckInterface)->GetChannel()->SetAttribute("Delay", StringValue(bottleneckDelay));
  }
  else
  {
    forkIpv4->SetUp(defaultInterface);
    forkIpv4->GetNetDevice(defaultInterface)->GetChannel()->SetAttribute("Delay", StringValue(delay));
    forkIpv4->SetDown(bottleneckInterface);
    forkIpv4->GetNetDevice(bottleneckInterface)->GetChannel()->SetAttribute("Delay", StringValue("1000ms"));
  }
  Simulator::Schedule(Period, RouteSwitch2, Period, forkIpv4, defaultInterface, bottleneckInterface);
}

static void
RouteSwitch3(ns3::Time Period, Ptr<Ipv4> forkIpv4, uint32_t defaultInterface, uint32_t bottleneckInterface, Ptr<RateErrorModel> err1, Ptr<RateErrorModel> err2)
{
  //NS_LOG_INFO("Calling RouteSwitch at time " << Simulator::Now().GetSeconds());
  //NS_LOG_INFO("\tRouteSwitch Checkpoint1");
  if(forkIpv4->IsUp(defaultInterface))
  {
    forkIpv4->SetDown(defaultInterface);
    //forkIpv4->GetNetDevice(defaultInterface)->SetAttribute("ReceiveErrorModel", PointerValue(&full_error_model));
    //forkIpv4->GetNetDevice(defaultInterface)->GetObject<RateErrorModel>()->SetRate(1.0);
    //Ptr<ErrorModel> Ting = forkIpv4->GetNetDevice(defaultInterface)->GetObject<ErrorModel>();
    //NS_LOG_INFO("\tGot pointer: " << Ting);
    err1->SetRate(1.0);
    //NS_LOG_INFO("\tRouteSwitch Checkpoint2");
    forkIpv4->SetUp(bottleneckInterface);
    //forkIpv4->GetNetDevice(bottleneckInterface)->SetAttribute("ReceiveErrorModel", PointerValue(&zero_error_model));
    //forkIpv4->GetNetDevice(bottleneckInterface)->GetObject<RateErrorModel>()->SetRate(error_p);
    err2->SetRate(error_p);
    //NS_LOG_INFO("\tRouteSwitch Checkpoint4");
  }
  else
  {
    forkIpv4->SetUp(defaultInterface);
    //forkIpv4->GetNetDevice(defaultInterface)->SetAttribute("ReceiveErrorModel", PointerValue(&zero_error_model));
    //forkIpv4->GetNetDevice(defaultInterface)->GetObject<RateErrorModel>()->SetRate(error_p);
    err1->SetRate(error_p);
    //NS_LOG_INFO("\tRouteSwitch Checkpoint3");
    forkIpv4->SetDown(bottleneckInterface);
    //forkIpv4->GetNetDevice(bottleneckInterface)->SetAttribute("ReceiveErrorModel", PointerValue(&full_error_model));
    //forkIpv4->GetNetDevice(bottleneckInterface)->GetObject<RateErrorModel>()->SetRate(1.0);
    err2->SetRate(1.0);
    //NS_LOG_INFO("\tRouteSwitch Checkpoint5");
  }
  Simulator::Schedule(Period, RouteSwitch3, Period, forkIpv4, defaultInterface, bottleneckInterface, err1, err2);
  //NS_LOG_INFO("\tRouteSwitch Checkpoint6");
}

int main(int argc, char *argv[])
{
  /*std::string transport_prot = "TcpNewReno";
  uint64_t data_mbytes = 0;
  uint32_t mtu_bytes = 400;
  std::string bottleneckRate = "20Kbps";
  std::string bottleneckDelay = "30ms";
  std::string datarate = "50Kbps";
  std::string delay = "10ms";
  bool sack = false;
  double RSP = 1;
  double startTime = 0;
  double switchTime = 1.0;
  double stopTime = 10.0;*/


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
  cmd.AddValue ("dropPackets", "Drop packets in queue when route swtiching happens", dropPackets);
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("switchTime", "Start time of route switching in seconds", switchTime);
  cmd.AddValue ("RSP", "Route Switch Period in seconds", RSP);
  cmd.AddValue ("stopTime", "Time for the simulation to stop in seconds", stopTime);
  cmd.Parse (argc, argv);
  transport_prot = std::string ("ns3::") + transport_prot;
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

  LogComponentEnable ("RoutingTest2", LOG_LEVEL_ALL);
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

  // Creatinge nodes and positions
  NS_LOG_INFO("Creating Topology");
  NodeContainer nodes, srcNode, dstNode;
  srcNode.Create (1);
  nodes.Create (4);
  dstNode.Create (1);
  NodeContainer allNodes(srcNode, nodes, dstNode);

  ListPositionAllocator posAloc;
  posAloc.Add(Vector(0,10,0));posAloc.Add(Vector(5,10,0));posAloc.Add(Vector(10,5,0));posAloc.Add(Vector(10,15,0));posAloc.Add(Vector(15,10,0));posAloc.Add(Vector(20,10,0));

  MobilityHelper mobility;
  mobility.SetPositionAllocator(&posAloc);
  mobility.Install(allNodes);

  // Creating the error model
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel errorModel;
  errorModel.SetRandomVariable (uv);
  errorModel.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  errorModel.SetRate (error_p);

  RateErrorModel bottleneckErrorModel;
  bottleneckErrorModel.SetRandomVariable (uv);
  bottleneckErrorModel.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  bottleneckErrorModel.SetRate (error_p);

  // Creating links and net devices
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (datarate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));

  NetDeviceContainer devices;
  NetDeviceContainer Device01 = pointToPoint.Install (srcNode.Get(0), nodes.Get(0));
  NetDeviceContainer Device24 = pointToPoint.Install (nodes.Get(3), nodes.Get(1));
  NetDeviceContainer Device45 = pointToPoint.Install (nodes.Get(3), dstNode.Get(0));
  
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (bottleneckRate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (bottleneckDelay));
  NetDeviceContainer Device34 = pointToPoint.Install (nodes.Get(3), nodes.Get(2));
  
  pointToPoint.SetDeviceAttribute ("ReceiveErrorModel", PointerValue(&bottleneckErrorModel));
  NetDeviceContainer Device13 = pointToPoint.Install (nodes.Get(0), nodes.Get(2));

  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (datarate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));
  pointToPoint.SetDeviceAttribute ("ReceiveErrorModel", PointerValue(&errorModel));
  NetDeviceContainer Device12 = pointToPoint.Install (nodes.Get(0), nodes.Get(1));

  devices.Add(Device01);devices.Add(Device12);devices.Add(Device13);devices.Add(Device24);devices.Add(Device34); devices.Add(Device45);

  // Set queue and queue discipline here if needed

  // Installing internet stack and setting ip addresses
  InternetStackHelper stack;
  stack.Install (allNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.0.255");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // Shutting down bottleneck interface
  //nodes.Get(0)->GetObject<Ipv4>()->SetDown(3);
  nodes.Get(0)->GetObject<Ipv4>()->SetDown(nodes.Get(0)->GetObject<Ipv4>()->GetInterfaceForDevice(Device13.Get(0)));
  //Device13.Get(0)->SetAttribute("DataRate", StringValue("0bps"));
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  
  //
  // Create an application to send TCP Packets from source node to destination node.
  //
  NS_LOG_INFO ("Create Applications.");
  uint16_t port = 9;   // Discard port (RFC 863)

  // Create an packet sink to receive these packets
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                          Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
  ApplicationContainer sinkApp;
  sinkApp = sink.Install (dstNode.Get(0));
  sinkApp.Start (Seconds (startTime));
  sinkApp.Stop(Seconds(stopTime));

  //
  // Create BulktSend application
  //
  
  //NS_LOG_INFO("NInterface of node is: " << dstNode.Get(0)->GetObject<Ipv4>()->GetNInterfaces());
  //NS_LOG_INFO("NAddress of node is: " << dstNode.Get(0)->GetObject<Ipv4>()->GetNAddresses(1));
  NS_LOG_INFO("Address of node is: " << dstNode.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetAddress());
  AddressValue remoteAddress (InetSocketAddress (dstNode.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetAddress(), port));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  ftp.SetAttribute ("Remote", remoteAddress);
  ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  ftp.SetAttribute ("MaxBytes", UintegerValue (data_mbytes * 1000000));

  ApplicationContainer sourceApp = ftp.Install (srcNode.Get (0));
  sourceApp.Start (Seconds (startTime));
  sourceApp.Stop (Seconds (stopTime));
  
  //Simulator::Schedule (Seconds(switchTime), RouteSwitch, Seconds(RSP), nodes.Get(0)->GetObject<Ipv4>(), 2, nodes.Get(0)->GetObject<Ipv4>(), 3);
  NS_LOG_INFO("Interface to node 2:" << nodes.Get(0)->GetObject<Ipv4>()->GetInterfaceForDevice(Device12.Get(0)));
  NS_LOG_INFO("Interface to node 3:" << nodes.Get(0)->GetObject<Ipv4>()->GetInterfaceForDevice(Device13.Get(0)));
  if(dropPackets){
    Simulator::Schedule (Seconds(switchTime), RouteSwitch3, Seconds(RSP), nodes.Get(0)->GetObject<Ipv4>(),
                        nodes.Get(0)->GetObject<Ipv4>()->GetInterfaceForDevice(Device12.Get(0)),
                        nodes.Get(0)->GetObject<Ipv4>()->GetInterfaceForDevice(Device13.Get(0)),
                        PointerValue(&errorModel), PointerValue(&bottleneckErrorModel));
  }else{
    Simulator::Schedule (Seconds(switchTime), RouteSwitch2, Seconds(RSP), nodes.Get(0)->GetObject<Ipv4>(),
                        nodes.Get(0)->GetObject<Ipv4>()->GetInterfaceForDevice(Device12.Get(0)),
                        nodes.Get(0)->GetObject<Ipv4>()->GetInterfaceForDevice(Device13.Get(0)));
  }
  //Simulator::Schedule (Seconds(switchTime), &RouteSwitch, 2, 3);

  // Trace routing tables 
  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("scratch/P5/Statistics/RoutingTest2.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (0), routingStream);
  Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper> ("scratch/P5/Statistics/RoutingTest2-2.routes", std::ios::out);
  //g.PrintRoutingTableAllAt (Seconds (switchTime+1), routingStream2);
  g.PrintRoutingTableAllAt (Seconds (switchTime+0.0001), routingStream2);
  //Ptr<OutputStreamWrapper> routingStream3 = Create<OutputStreamWrapper> ("scratch/P5/Statistics/RoutingTest2-3.routes", std::ios::out);
  //g.PrintRoutingTableAllAt (Seconds (switchTime+1.0001), routingStream3);

  //Ptr<Ipv4> Ting = nodes.Get(0)->GetObject<Ipv4>();
  
  // Trace other things 
  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("scratch/P5/Traces/RoutingTest2.tr"));
  pointToPoint.EnablePcapAll ("scratch/P5/Pcap/RoutingTest2/Node");
  firstCwnd[0] = true;
  //Simulator::Schedule (Seconds (startTime + 1.00001), &TraceCwnd, "scratch/P5/Statistics/RoutingTest2-cwnd.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceCwnd, "scratch/P5/Statistics/RoutingTest2-cwnd.data", 0);
  AnimationInterface anim("scratch/P5/Animations/RoutingTest2/Animation.xml");
  //anim.EnablePacketMetadata(true);
  anim.EnableIpv4RouteTracking("scratch/P5/Animations/RoutingTest2/Routes.xml", Seconds(startTime), Seconds(stopTime), Seconds(RSP));
  FlowMonitorHelper flowHelper;
  flowHelper.InstallAll();
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();
  flowHelper.SerializeToXmlFile("scratch/P5/Animations/RoutingTest2/FlowMonitor.xml", false, false);
  Simulator::Destroy ();

}