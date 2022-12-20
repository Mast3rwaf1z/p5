#include <iostream>
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DynamicLinks");

static std::map<uint32_t, bool> firstCwnd;
static std::map<uint32_t, bool> firstSshThr;
static std::map<uint32_t, bool> firstRtt;
static std::map<uint32_t, bool> firstRto;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> cWndStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> ssThreshStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> rttStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> rtoStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> nextTxStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> nextRxStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> inFlightStream;
static std::map<uint32_t, uint32_t> cWndValue;
static std::map<uint32_t, uint32_t> ssThreshValue;
static std::map<std::string, Ptr<OutputStreamWrapper>> linkStatus;
static bool goodputSetup = false;
static uint32_t goodput = 0;
static uint32_t goodput2 = 0;
static Ptr<OutputStreamWrapper> goodputStream;
static Ptr<OutputStreamWrapper> goodputStream2;
static Ptr<OutputStreamWrapper> goodputStreamDetailed;
static uint32_t throughput;
static Ptr<OutputStreamWrapper> throughputStream;
static Ptr<OutputStreamWrapper> throughputStreamDetailed;
static Ptr<OutputStreamWrapper> PosStream = Create<OutputStreamWrapper> ("scratch/P5/Statistics/DynamicLinks1.Pos", std::ios::out);
static Ptr<OutputStreamWrapper> PosStream2 = Create<OutputStreamWrapper> ("scratch/P5/Statistics/DynamicLinks2.Pos", std::ios::out);

uint32_t numNodes = 3;
uint32_t srcIndex = 0;
uint32_t dstIndex = 0;
std::string transport_prot = "TcpNewReno";
uint64_t data_mbytes = 0;
uint32_t mtu_bytes = 400;
std::string datarate = "50Kbps";
std::string delay = "10ms";
double error_p = 0.0;
double speed = 1;
double ISD = 5;
double IOD = 5;
double COD = 31.25;
bool sack = false;
double DSP = 1;
double startTime = 0;
double stopTime = 25.0;

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

  if (goodputSetup)
  {
    *goodputStream2->GetStream () << Simulator::Now().GetSeconds() << "," << goodput2 << std::endl;
  }
}

static void
SsThreshTracer (std::string context, uint32_t oldval, uint32_t newval)
{
  uint32_t nodeId = GetNodeIdFromContext (context);

  if (firstSshThr[nodeId])
    {
      *ssThreshStream[nodeId]->GetStream () << "0.0," << oldval << std::endl;
      firstSshThr[nodeId] = false;
    }
  *ssThreshStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << newval << std::endl;
  ssThreshValue[nodeId] = newval;

  /*if (!firstCwnd[nodeId])
    {
      *cWndStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << cWndValue[nodeId] << std::endl;
    }*/
}

static void
RttTracer (std::string context, Time oldval, Time newval)
{
  uint32_t nodeId = GetNodeIdFromContext (context);

  if (firstRtt[nodeId])
    {
      *rttStream[nodeId]->GetStream () << "0.0," << oldval.GetSeconds () << std::endl;
      firstRtt[nodeId] = false;
    }
  *rttStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << newval.GetSeconds () << std::endl;
}

static void
RtoTracer (std::string context, Time oldval, Time newval)
{
  uint32_t nodeId = GetNodeIdFromContext (context);

  if (firstRto[nodeId])
    {
      *rtoStream[nodeId]->GetStream () << "0.0," << oldval.GetSeconds () << std::endl;
      firstRto[nodeId] = false;
    }
  *rtoStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << newval.GetSeconds () << std::endl;
}

static void
InFlightTracer (std::string context, [[maybe_unused]] uint32_t old, uint32_t inFlight)
{
  uint32_t nodeId = GetNodeIdFromContext (context);

  *inFlightStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << inFlight << std::endl;
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
TraceSsThresh (std::string ssthresh_tr_file_name, uint32_t nodeId)
{
  AsciiTraceHelper ascii;
  ssThreshStream[nodeId] = ascii.CreateFileStream (ssthresh_tr_file_name.c_str ());
  *ssThreshStream[nodeId]->GetStream () << "Time,Threshold" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold",
                   MakeCallback (&SsThreshTracer));
}

static void
TraceRtt (std::string rtt_tr_file_name, uint32_t nodeId)
{
  AsciiTraceHelper ascii;
  rttStream[nodeId] = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  *rttStream[nodeId]->GetStream () << "Time,RTT" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/RTT",
                   MakeCallback (&RttTracer));
}

static void
TraceRto (std::string rto_tr_file_name, uint32_t nodeId)
{
  AsciiTraceHelper ascii;
  rtoStream[nodeId] = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  *rtoStream[nodeId]->GetStream () << "Time,RTO" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/RTO",
                   MakeCallback (&RtoTracer));
}

static void
TraceInFlight (std::string in_flight_file_name, uint32_t nodeId)
{
  AsciiTraceHelper ascii;
  inFlightStream[nodeId] = ascii.CreateFileStream (in_flight_file_name.c_str ());
  *inFlightStream[nodeId]->GetStream () << "Time,Bytes" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight",
                   MakeCallback (&InFlightTracer));
}

static void
DetailedGoodputTracer(std::string context, Ptr<const Packet> packet)
{
  //NS_LOG_INFO(Simulator::Now().GetSeconds() << ": Got packet with size: " << packet->GetSize());
  *goodputStreamDetailed->GetStream () << Simulator::Now().GetSeconds() << "," << packet->GetSize() << std::endl;
  goodput = goodput + packet->GetSize();
  goodput2 = goodput2 + packet->GetSize();
}

static void
GoodputTracer()
{
  *goodputStream->GetStream () << Simulator::Now().GetSeconds() << "," << goodput << std::endl;
  goodput = 0;
  Simulator::Schedule(Seconds(1), GoodputTracer);
}

static void
TraceGoodput(std::string tr_file_dir, uint32_t nodeId)
{
  std::string tr_file_name = tr_file_dir + "goodput.data";
  std::string tr_file_name2 = tr_file_dir + "goodput2.data";
  std::string tr_file_name_detailed = tr_file_dir + "goodput-Detailed.data";
  AsciiTraceHelper ascii;
  goodputStream = ascii.CreateFileStream (tr_file_name.c_str ());
  goodputStream2 = ascii.CreateFileStream (tr_file_name2.c_str ());
  goodputStreamDetailed = ascii.CreateFileStream (tr_file_name_detailed.c_str ());
  *goodputStream->GetStream () << "Time,Goodput" << std::endl;
  *goodputStream2->GetStream () << "Time,Goodput" << std::endl;
  *goodputStream2->GetStream () << "0,0" << std::endl;
  *goodputStreamDetailed->GetStream () << "Time,Goodput" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/DeviceList/0/$ns3::PointToPointNetDevice/PhyRxEnd",
                   MakeCallback (&DetailedGoodputTracer));
  Simulator::Schedule(Seconds(1), GoodputTracer);
  Simulator::Schedule(Seconds(stopTime-0.001), GoodputTracer);
  goodputSetup = true;
}

static void
dynamicLinks (ns3::Time Period, NodeContainer srcOrbit, NodeContainer dstOrbit, NetDeviceContainer devCont, RateErrorModel errArr[])
{
  //uint32_t NumNodes =  srcOrbit.GetN();
  std::string linkId;
  double dist;
  for(uint32_t i=0; i<srcOrbit.GetN(); i++)
  {
    for(uint32_t j=0; j<dstOrbit.GetN(); j++)
    {
      //dist = ns3::MobilityHelper::GetDistanceSquaredBetween(srcOrbit.Get(i), dstOrbit.Get(j));
      int errorIndex = numNodes*i+j;
      Ptr<MobilityModel> m1 = srcOrbit.Get(i)->GetObject<MobilityModel>();
      Ptr<MobilityModel> m2 = dstOrbit.Get(j)->GetObject<MobilityModel>();
      dist = m1->GetDistanceFrom(m2);
      ///int devIndex = 2*(NumNodes*i+j);
      int devIndex = 2*errorIndex;
      int32_t intIndex =  srcOrbit.Get(i)->GetObject<Ipv4>()->GetInterfaceForDevice(devCont.Get(devIndex));
      linkId = std::to_string(srcOrbit.Get(i)->GetId()) + "-" + std::to_string(dstOrbit.Get(j)->GetId());
      //if(dist>31.25)
      if(dist>COD)
      {
        srcOrbit.Get(i)->GetObject<Ipv4>()->SetDown(intIndex);
        errArr[errorIndex].SetRate(1.0);
        //NS_LOG_INFO(Simulator::Now().GetSeconds()<<": Route between nodes "<<i<<" and "<<j<<" has been cut with distance squared: "<<dist);
        *linkStatus[linkId]->GetStream () << Simulator::Now().GetSeconds() << "," << std::to_string(dist) << ",0" << std::endl;
      }
      else
      {
        srcOrbit.Get(i)->GetObject<Ipv4>()->SetUp(intIndex);
        errArr[errorIndex].SetRate(error_p);
        //NS_LOG_INFO(Simulator::Now().GetSeconds()<<": Route between nodes "<<i<<" and "<<j<<" has been established with distance squared: "<<dist);
        *linkStatus[linkId]->GetStream () << Simulator::Now().GetSeconds() << "," << std::to_string(dist) << ",1" << std::endl;
      }
      
    }
  }
  Simulator::Schedule(Period, dynamicLinks, Period, srcOrbit, dstOrbit, devCont, errArr);
}

static void
dynamicInit(ns3::Time Period, NodeContainer srcOrbit, NodeContainer dstOrbit, NetDeviceContainer devCont, RateErrorModel errArr[], std::string tr_file_dir)
{
  uint32_t srcId = 0;
  uint32_t dstId = 0;
  std::string nodeId;
  std::string tr_file_name;
  for(uint32_t i=0; i<srcOrbit.GetN(); i++)
  {
    for(uint32_t j=0; j<dstOrbit.GetN(); j++)
    {
    srcId = srcOrbit.Get(i)->GetId();
    dstId = dstOrbit.Get(j)->GetId();
    nodeId = std::to_string(srcId) + "-" + std::to_string(dstId);
    tr_file_name = tr_file_dir + nodeId;
    NS_LOG_INFO("nodeId value is: " << nodeId);
    NS_LOG_INFO("File name is: " + tr_file_name);
    AsciiTraceHelper ascii;
    linkStatus[nodeId] = ascii.CreateFileStream (tr_file_name.c_str ());
    *linkStatus[nodeId]->GetStream () << "Time,Distance,Is Up" << std::endl;
    }
  }
  dynamicLinks(Seconds(DSP), srcOrbit, dstOrbit, devCont, errArr);
}

static void
PosTracer (std::string context, Ptr<const Packet> Pck, const Address &a)
{
  uint32_t nodeId = GetNodeIdFromContext (context);
  *PosStream->GetStream() << Simulator::Now().GetSeconds() << "," << NodeList::GetNode(nodeId)->GetObject<MobilityModel>()->GetPosition() << std::endl;
}

static void
PosTracer2 (ns3::Time Period)
{
  uint32_t NumNodes =  NodeList::GetNNodes();
  //NS_LOG_INFO("Number of nodes is: " << NumNodes);
  double Distances[NumNodes][NumNodes];
  std::string strDistances = "";
  for(uint32_t i=0; i<NumNodes; i++)
  {
    for(uint32_t j=0; j<NumNodes; j++)
    {
      //NS_LOG_INFO("i="<<i<<" j="<<j);
      Ptr<MobilityModel> m1 = NodeList::GetNode(i)->GetObject<MobilityModel>();
      Ptr<MobilityModel> m2 = NodeList::GetNode(j)->GetObject<MobilityModel>();
      Distances[i][j] = m1->GetDistanceFrom(m2);
      strDistances.append("\t" + std::to_string(Distances[i][j]));
    }
    strDistances.append("\n");
  }
  *PosStream2->GetStream() << Simulator::Now().GetSeconds() << ": \n" + strDistances << std::endl;
  //*PosStream2->GetStream() << Simulator::Now().GetSeconds() << ": \n" << Distances << std::endl;
  Simulator::Schedule(Period, PosTracer2, Period);
}

static void
PrintPos(Ptr<const MobilityModel> mob)
{
  std::cout << "Position is: " << mob->GetPosition() << std::endl;
}

int main(int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.AddValue ("numNodes", "Number of nodes in each orbit", numNodes);
  cmd.AddValue ("srcIndex", "Index of the sending node in the source orbit", srcIndex);
  cmd.AddValue ("dstIndex", "Index of the recieving node in the destination orbit", dstIndex);
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, TcpLinuxReno, "
              "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
              "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
              "TcpLp, TcpDctcp, TcpCubic, TcpBbr", transport_prot);
  cmd.AddValue ("data", "Number of Megabytes of data to transmit (0 means unlimited)", data_mbytes);
  cmd.AddValue ("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue ("datarate", "Datarate of point to point links", datarate);
  cmd.AddValue ("delay", "Delay of point to point links", delay);
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("speed", "Movement speed of the satellites in orbit (m/s)", speed);
  cmd.AddValue ("ISD", "Inter-satellite distance in meters. Used for satellites in same orbit.", ISD);
  cmd.AddValue ("IOD", "Inter-Orbit distance in meters. Used for distance between orbits.", IOD);
  cmd.AddValue ("COD", "Cutoff distance for RF links in meters", COD);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("DSP", "Distance Sampling Period", DSP);
  cmd.AddValue ("stopTime", "Time for the simulation to stop", stopTime);
  cmd.Parse (argc, argv);
  transport_prot = std::string ("ns3::") + transport_prot;
  TypeId tcpTid;
  // Select TCP variant
  if (transport_prot.compare ("ns3::TcpWestwoodPlus") == 0)
  { 
    // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
    // the default protocol type in ns3::TcpWestwood is WESTWOOD
    Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
  }
  else
  {
    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));
  }
  //NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
  //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

  LogComponentEnable ("DynamicLinks", LOG_LEVEL_ALL);
  //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_FUNCTION);
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
  NodeContainer srcOrbit, dstOrbit, srcNode, dstNode;
  srcNode.Create (1); dstNode.Create (1);
  srcOrbit.Create (numNodes);
  dstOrbit.Create (numNodes);
  NodeContainer allNodes(srcNode, dstNode, srcOrbit, dstOrbit);
  

  MobilityHelper mobility;
  ListPositionAllocator posAloc;
  posAloc.Add(Vector(0,(ISD*(numNodes+srcIndex)),0)); mobility.SetPositionAllocator(&posAloc); mobility.Install(srcNode.Get(0));
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (IOD),
                                  "MinY", DoubleValue (ISD*(numNodes)),
                                  "DeltaX", DoubleValue (IOD),
                                  "DeltaY", DoubleValue (ISD),
                                  "GridWidth", UintegerValue (1),
                                  "LayoutType", StringValue ("RowFirst"));
  mobility.Install(srcOrbit);
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (2*IOD),
                                  "MinY", DoubleValue (ISD),
                                  "DeltaX", DoubleValue (IOD),
                                  "DeltaY", DoubleValue (ISD),
                                  "GridWidth", UintegerValue (1),
                                  "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mobility.Install(dstOrbit); mobility.Install(dstNode);
  dstNode.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(3*IOD,ISD*(1+dstIndex),0));
  dstNode.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0,speed,0));
  for(uint32_t i=0; i<numNodes; i++)
  {
    dstOrbit.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0,speed,0));
  }

  /*MobilityHelper mobility;
  ListPositionAllocator posAloc;
  posAloc.Add(Vector(0,(5*(numNodes+srcIndex)),0)); mobility.SetPositionAllocator(&posAloc); mobility.Install(srcNode.Get(0));
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (5),
                                  "MinY", DoubleValue (5*numNodes),
                                  "DeltaX", DoubleValue (5.0),
                                  "DeltaY", DoubleValue (5.0),
                                  "GridWidth", UintegerValue (1),
                                  "LayoutType", StringValue ("RowFirst"));
  mobility.Install(srcOrbit);
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue (10),
                                  "MinY", DoubleValue (5),
                                  "DeltaX", DoubleValue (5.0),
                                  "DeltaY", DoubleValue (5.0),
                                  "GridWidth", UintegerValue (1),
                                  "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mobility.Install(dstOrbit); mobility.Install(dstNode);
  dstNode.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(15,5*(1+dstIndex),0));
  dstNode.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0,1,0));
  for(uint32_t i=0; i<numNodes; i++)
  {
    dstOrbit.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0,1,0));
  }*/

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (datarate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (delay));

  NetDeviceContainer srcDevices;
  NetDeviceContainer dstDevices;
  for(uint32_t i=0; i<numNodes-1; i++)
  {
    NetDeviceContainer srcTemp = pointToPoint.Install(srcOrbit.Get(i), srcOrbit.Get(i+1));
    NetDeviceContainer dstTemp = pointToPoint.Install(dstOrbit.Get(i), dstOrbit.Get(i+1));
    srcDevices.Add(srcTemp); dstDevices.Add(dstTemp);
  }
  
  // Creating the error models
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel errorModels[numNodes*numNodes];
  for(int i=0; i<numNodes*numNodes; i++)
  {
    errorModels[i].SetRandomVariable (uv);
    errorModels[i].SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
    errorModels[i].SetRate (error_p);
  }
  // Creating cross orbits links
  NetDeviceContainer crossDevices;
  for(uint32_t i=0; i<numNodes; i++)
  {
    for(uint32_t j=0; j<numNodes; j++)
    {
      pointToPoint.SetDeviceAttribute ("ReceiveErrorModel", PointerValue(&errorModels[i*numNodes+j]));
      NetDeviceContainer Temp = pointToPoint.Install(srcOrbit.Get(i), dstOrbit.Get(j));
      crossDevices.Add(Temp);
    }
  }
  
  RateErrorModel noError;
  noError.SetRandomVariable (uv);
  noError.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  noError.SetRate (0.0);
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100"+datarate));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("0ms"));
  pointToPoint.SetDeviceAttribute ("ReceiveErrorModel", PointerValue(&noError));
  NetDeviceContainer srcDevice = pointToPoint.Install(srcNode.Get(0), srcOrbit.Get(srcIndex));
  NetDeviceContainer dstDevice = pointToPoint.Install(dstNode.Get(0), dstOrbit.Get(dstIndex));

  NetDeviceContainer devices;
  devices.Add(srcDevice); devices.Add(dstDevice); devices.Add(srcDevices); devices.Add(dstDevices); devices.Add(crossDevices);
  
  InternetStackHelper stack;
  stack.Install (allNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.0.255");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  dynamicInit(Seconds(DSP), srcOrbit, dstOrbit, crossDevices, errorModels, "scratch/P5/Statistics/DynamicLinks/LinkStats/");
  //dynamicLinks(Seconds(DSP), srcOrbit, dstOrbit, crossDevices, errorModels);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //
  // Create an application to send TCP Packets from node 0 to node 3.
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
  // Create BulkSend application
  //
  
  NS_LOG_INFO("Address of destination is: " << dstNode.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetAddress());
  AddressValue remoteAddress (InetSocketAddress (dstNode.Get(0)->GetObject<Ipv4>()->GetAddress(1,0).GetAddress(), port));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  ftp.SetAttribute ("Remote", remoteAddress); 
  ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
  ftp.SetAttribute ("MaxBytes", UintegerValue (data_mbytes * 1000000));

  ApplicationContainer sourceApp = ftp.Install (srcNode.Get (0));
  sourceApp.Start (Seconds (startTime));
  sourceApp.Stop (Seconds (stopTime));

  // Trace routing tables 
  //Ipv4GlobalRoutingHelper g;
  //Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("scratch/P5/Statistics/DynamicLinks1.routes", std::ios::out);
  //g.PrintRoutingTableAllAt (Seconds (0), routingStream);
  //Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper> ("scratch/P5/Statistics/DynamicLink2.routes", std::ios::out);
  //g.PrintRoutingTableAllAt (Seconds (switchTime+1), routingStream2);
  //g.PrintRoutingTableAllAt (Seconds (switchTime+0.0001), routingStream2);
  //Ptr<OutputStreamWrapper> routingStreams[int(stopTime)];
  //for(int i=0; i<int(stopTime); i++)
  //{
    //routingStreams[i] = Create<OutputStreamWrapper> ("scratch/P5/Statistics/DynamicLinks/" + std::to_string(i) + ".routes");
  //  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("scratch/P5/Statistics/DynamicLinks/" + std::to_string(i) + ".routes", std::ios::out);
  //  g.PrintRoutingTableAllAt (Seconds (i), routingStream);
  //}

  //Simulator::Schedule(Seconds(5), PrintPos, dstNodes.Get(0)->GetObject<MobilityModel>());
  //Config::Connect ("/NodeList/1/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback (&PosTracer));
  //std::cout << "Num applications: " << NodeList::GetNode(1)->GetNApplications() << std::endl;
  //Simulator::Schedule(Seconds(startTime), PosTracer2, Seconds(DSP));
  

  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("scratch/P5/Traces/DynamicLinks.tr"));
  pointToPoint.EnablePcapAll ("scratch/P5/Pcap/DynamicLinks/Node-Device");
  firstCwnd[0] = true;
  //Simulator::Schedule (Seconds (startTime + 1.00001), &TraceCwnd, "scratch/P5/Statistics/DynamicLinks-cwnd.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceCwnd, "scratch/P5/Statistics/DynamicLinks/cwnd.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceSsThresh, "scratch/P5/Statistics/DynamicLinks/ssth.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceRtt, "scratch/P5/Statistics/DynamicLinks/rtt.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceRto, "scratch/P5/Statistics/DynamicLinks/rto.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceInFlight, "scratch/P5/Statistics/DynamicLinks/inflight.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceGoodput, "scratch/P5/Statistics/DynamicLinks/", dstNode.Get(0)->GetId());
  AnimationInterface anim("scratch/P5/Animations/DynamicLinks/Animation.xml");
  anim.SetMobilityPollInterval(Seconds(DSP));
  //anim.EnablePacketMetadata(true);
  anim.EnableIpv4RouteTracking("scratch/P5/Animations/DynamicLinks/Routes.xml", Seconds(startTime), Seconds(stopTime), Seconds(DSP));
  FlowMonitorHelper flowHelper;
  flowHelper.InstallAll();
  //anim.AddSourceDestination(0, "10.1.0.3");
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();
  //flowHelper.SerializeToXmlFile("scratch/P5/Animations/DynamicLinks/FlowMonitor.xml", true, true);
  flowHelper.SerializeToXmlFile("scratch/P5/Animations/DynamicLinks/FlowMonitor.xml", false, false);
  Simulator::Destroy ();

}