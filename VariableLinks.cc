#include <iostream>
#include <math.h>
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("VariableLinks");

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
//static Ptr<OutputStreamWrapper> PosStream = Create<OutputStreamWrapper> ("scratch/P5/Statistics/VariableLinks1.Pos", std::ios::out);
//static Ptr<OutputStreamWrapper> PosStream2 = Create<OutputStreamWrapper> ("scratch/P5/Statistics/VariableLinks2.Pos", std::ios::out);
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

static double speff_thresholds[] = {0,0.434841,0.490243,0.567805,0.656448,0.789412,0.889135,0.988858,1.088581,1.188304,1.322253,1.487473,1.587196,1.647211,1.713601,1.779991,1.972253,2.10485,2.193247,2.370043,2.458441,2.524739,2.635236,2.637201,2.745734,2.856231,2.966728,3.077225,3.165623,3.289502,3.300184,3.510192,3.620536,3.703295,3.841226,3.951571,4.206428,4.338659,4.603122,4.735354,4.933701,5.06569,5.241514,5.417338,5.593162,5.768987,5.900855};
static double lin_thresholds[] = {1e-10,0.5188000389,0.5821032178,0.6266138647,0.751622894,0.9332543008,1.051961874,1.258925412,1.396368361,1.671090614,2.041737945,2.529297996,2.937649652,2.971666032,3.25836701,3.548133892,3.953666201,4.518559444,4.83058802,5.508076964,6.45654229,6.886522963,6.966265141,7.888601176,8.452788452,9.354056741,10.49542429,11.61448614,12.67651866,12.88249552,14.48771854,14.96235656,16.48162392,18.74994508,20.18366364,23.1206479,25.00345362,30.26913428,35.2370871,38.63669771,45.18559444,49.88844875,52.96634439,64.5654229,72.27698036,76.55966069,90.57326009};
static double SOL = 299792458;             // Speed of light in m/s
static double boltzmann = 1.38e-23;

// Adjustable variables
uint32_t numNodes = 1;
uint32_t srcIndex = 0;
uint32_t dstIndex = 0;
std::string transport_prot = "TcpNewReno";
uint64_t data_mbytes = 0;
uint32_t mtu_bytes = 400;
double error_p = 0.0;
double speed = 200000;
double ISD = 1000;
double IOD = 3000;
//double COD = 5600000;
bool sack = false;
uint32_t initCwnd = 10;
double DSP = 1;
double startTime = 0;
double switchTime = 10.0;
double stopTime = 25.0;

double power = 10;
double frequency = 26e9; // 26GHz is in the K-band
double bandwidth = 500e6; // 500MHz
double aDiameterTx = 0.26;
double aDiameterRx = 0.26;
double noiseFigure = 2;
double noiseTemperature = 290;
double pointingLoss = 0.3;
double efficiency = 0.55;

// Calculated Variables
double fullGain;
double power_dB;
double noise;

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

static double
calculateDatarate(double dist)
{
  double path_loss_dB = 10*log10(pow(4*M_PI*dist*frequency/SOL, 2));
  double SNR = pow(10, (power_dB + fullGain - path_loss_dB - noise)/10);
  std::cout << "\tSNR = pow(10, (" << power_dB << " + " << fullGain << " - " << path_loss_dB << " - " << noise << ")/10) = " << SNR << std::endl;
  int Index = 0;
  for(uint32_t k=0; k<sizeof(lin_thresholds)/sizeof(double); k++)
  {
    if(lin_thresholds[k] <= SNR){Index = k;} else{break;}
  }
  double speff = speff_thresholds[Index];
  NS_LOG_INFO("\t" << lin_thresholds[Index-1] << "\t" << SNR << "\t" << lin_thresholds[Index]);
  double datarate = speff*bandwidth;
  return datarate;
}

static void
variableLinks (ns3::Time Period, NodeContainer srcOrbit, NodeContainer dstOrbit, NetDeviceContainer devCont, RateErrorModel errArr[])
{
  //uint32_t NumNodes =  srcOrbit.GetN();
  std::string linkId;
  double dist;
  for(uint32_t i=0; i<srcOrbit.GetN(); i++)
  {
    for(uint32_t j=0; j<dstOrbit.GetN(); j++)
    {
      std::cout << "Time is: " << Simulator::Now().GetSeconds() << std::endl;
      Ptr<MobilityModel> m1 = srcOrbit.Get(i)->GetObject<MobilityModel>();
      Ptr<MobilityModel> m2 = dstOrbit.Get(j)->GetObject<MobilityModel>();
      dist = m1->GetDistanceFrom(m2);
      int errorIndex = numNodes*i+j;
      ///int devIndex = 2*(NumNodes*i+j);
      int devIndex = 2*errorIndex;
      int32_t intIndex =  srcOrbit.Get(i)->GetObject<Ipv4>()->GetInterfaceForDevice(devCont.Get(devIndex));
      NS_LOG_INFO(Simulator::Now().GetSeconds()<<"s: Link stats between node "<<i<<" and "<<j<<":");
      double datarate = calculateDatarate(dist);
      double delay = dist/SOL;
      NS_LOG_INFO("\tDistance is: " << std::to_string(dist));
      NS_LOG_INFO("\tDataRate is: " << std::to_string(datarate));
      NS_LOG_INFO("\tDelay is: " << std::to_string(delay));
      linkId = std::to_string(srcOrbit.Get(i)->GetId()) + "-" + std::to_string(dstOrbit.Get(j)->GetId());
      if(datarate <= 0)
      {
        srcOrbit.Get(i)->GetObject<Ipv4>()->SetDown(intIndex);
        errArr[errorIndex].SetRate(1.0);
        *linkStatus[linkId]->GetStream () << Simulator::Now().GetSeconds() << "," << std::to_string(datarate) << "," << std::to_string(delay) << "," << std::to_string(dist) << ",0" << std::endl;
        NS_LOG_INFO("Checkpoint1");
      }
      else
      {
        
        srcOrbit.Get(i)->GetObject<Ipv4>()->SetUp(intIndex);
        devCont.Get(devIndex)->SetAttribute("DataRate", StringValue(std::to_string(datarate) + "bps"));
        devCont.Get(devIndex)->GetChannel()->SetAttribute("Delay", StringValue(std::to_string(delay) + "s"));
        errArr[errorIndex].SetRate(error_p);
        // record here the new data rate and delay
        *linkStatus[linkId]->GetStream () << Simulator::Now().GetSeconds() << "," << std::to_string(datarate) << "," << std::to_string(delay) << "," << std::to_string(dist) << ",1" << std::endl;
        NS_LOG_INFO("Checkpoint2");
      }
    }
  }
  Simulator::Schedule(Period, variableLinks, Period, srcOrbit, dstOrbit, devCont, errArr);
}

static void
variableInit(ns3::Time Period, NodeContainer srcOrbit, NodeContainer dstOrbit, NetDeviceContainer devCont, RateErrorModel errArr[], std::string tr_file_dir)
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
    *linkStatus[nodeId]->GetStream () << "Time,Data Rate,Delay,Distance,Is Up" << std::endl;
    }
  }
  variableLinks(Seconds(DSP), srcOrbit, dstOrbit, devCont, errArr);
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
  cmd.AddValue ("data", "Number of Megabytes of data to transmit", data_mbytes);
  cmd.AddValue ("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("speed", "Movement speed of the satellites in orbit (m/s)", speed);
  cmd.AddValue ("ISD", "Inter-satellite distance in meters. Used for satellites in same orbit.", ISD);
  cmd.AddValue ("IOD", "Inter-Orbit distance in meters. Used for distance between orbits.", IOD);
  //cmd.AddValue ("COD", "Cutoff distance for RF links in meters", COD);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("initCwnd", "Initial size of the congestion window in segments", initCwnd);
  cmd.AddValue ("DSP", "Distance Sampling Period", DSP);
  cmd.AddValue ("switchTime", "Time for the route to swtich", switchTime);
  cmd.AddValue ("stopTime", "Time for the simulation to stop", stopTime);
  cmd.AddValue ("power", "Maximum transmission power in W", power);
  cmd.AddValue ("frequency", "Frequency of RF links", frequency);
  cmd.AddValue ("bandwidth", "Maximum bandwidth of RF links", bandwidth);
  cmd.AddValue ("aDiameterTx", "Diameter of transmitting antenna", aDiameterTx);
  cmd.AddValue ("aDiameterRx", "Diameter of receiving antenna", aDiameterRx);
  cmd.AddValue ("noiseFigure", "Noise figure in dB", noiseFigure);
  cmd.AddValue ("noiseTemperature", "Noise temperature in kelvin", noiseTemperature);
  cmd.AddValue ("pointingLoss", "Pointing loss in dB", pointingLoss);
  cmd.AddValue ("efficiency", "The efficiency of the RF links", efficiency);

  cmd.Parse (argc, argv);
  transport_prot = std::string ("ns3::") + transport_prot;
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue(initCwnd));
  Config::SetDefault ("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents", BooleanValue (true));

  LogComponentEnable ("VariableLinks", LOG_LEVEL_ALL);
  //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_ALL);
  //LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  Time::SetResolution (Time::US);

  // Calculate stuff needed for datarate calculations
  power_dB = 10*log10(power);
  double txGain = 10*log10(efficiency*(pow(M_PI*aDiameterTx*frequency/SOL,2)));
  double rxGain = 10*log10(efficiency*(pow(M_PI*aDiameterRx*frequency/SOL,2)));
  fullGain = rxGain + txGain - 2*pointingLoss;
  noise = 10*log10(bandwidth*boltzmann) + noiseFigure + 10*log10(290 + (noiseTemperature - 290)*pow(10, -noiseFigure/10));
  frequency = frequency;
  bandwidth = bandwidth;

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
                                  "MinY", DoubleValue (ISD*numNodes),
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

  double datarate = calculateDatarate(ISD);
  double delay = ISD/SOL;
  NS_LOG_INFO("Initial datarate: " << datarate);
  NS_LOG_INFO("Initial delay: " << delay);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue (std::to_string(datarate) + "bps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue (std::to_string(delay) + "s"));

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
  for(uint32_t i=0; i<numNodes*numNodes; i++)
  {
    errorModels[i].SetRandomVariable (uv);
    errorModels[i].SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
    errorModels[i].SetRate (error_p);
  }
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
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1000000Gbps"));
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

  variableInit(Seconds(DSP), srcOrbit, dstOrbit, crossDevices, errorModels, "scratch/P5/Statistics/VariableLinks/LinkStats/");
  //variableLinks(Seconds(DSP), srcOrbit, dstOrbit, crossDevices, errorModels);
  //dynamicLinks(Seconds(DSP), srcOrbit, dstOrbit, crossDevices);

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
  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("scratch/P5/Statistics/VariableLinks1.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (0), routingStream);
  Ptr<OutputStreamWrapper> routingStream2 = Create<OutputStreamWrapper> ("scratch/P5/Statistics/VariableLinks2.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (switchTime+1), routingStream2);
  //g.PrintRoutingTableAllAt (Seconds (switchTime+0.0001), routingStream2);
  //Ptr<OutputStreamWrapper> routingStreams[int(stopTime)];
  //for(int i=0; i<int(stopTime); i++)
  //{
    //routingStreams[i] = Create<OutputStreamWrapper> ("scratch/P5/Statistics/DynamicLinks/" + std::to_string(i) + ".routes");
  //  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("scratch/P5/Statistics/DynamicLinks/" + std::to_string(i) + ".routes", std::ios::out);
  //  g.PrintRoutingTableAllAt (Seconds (i), routingStream);
  //}

  AsciiTraceHelper ascii;
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("scratch/P5/Traces/VariableLinks.tr"));
  pointToPoint.EnablePcapAll ("scratch/P5/Pcap/VariableLinks/Node-Device");
  firstCwnd[0] = true;
  //Simulator::Schedule (Seconds (startTime + 1.00001), &TraceCwnd, "scratch/P5/Statistics/VariableLinks-cwnd.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceCwnd, "scratch/P5/Statistics/VariableLinks/cwnd.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceSsThresh, "scratch/P5/Statistics/VariableLinks/ssth.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceRtt, "scratch/P5/Statistics/VariableLinks/rtt.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceRto, "scratch/P5/Statistics/VariableLinks/rto.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceInFlight, "scratch/P5/Statistics/VariableLinks/inflight.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceGoodput, "scratch/P5/Statistics/VariableLinks/", dstNode.Get(0)->GetId());
  AnimationInterface anim("scratch/P5/Animations/VariableLinks/Animation.xml");
  anim.SetMobilityPollInterval(Seconds(DSP));
  //anim.SetMobilityPollInterval(Seconds(0.1));
  //anim.EnablePacketMetadata(true);
  anim.EnableIpv4RouteTracking("scratch/P5/Animations/VariableLinks/Routes.xml", Seconds(startTime), Seconds(stopTime), Seconds(DSP));
  FlowMonitorHelper flowHelper;
  flowHelper.InstallAll();
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();
  flowHelper.SerializeToXmlFile("scratch/P5/Animations/VariableLinks/FlowMonitor.xml", true, true);
  Simulator::Destroy ();

}