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
static std::map<uint32_t, bool> firstSshThr;
static std::map<uint32_t, bool> firstRtt;
static std::map<uint32_t, bool> firstRto;
static std::map<uint32_t, bool> firstCongState;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> cWndStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> ssThreshStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> rttStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> rtoStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> nextTxStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> nextRxStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> inFlightStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> congStateStream;
static std::map<uint32_t, Ptr<OutputStreamWrapper>> rxDropStream;
static std::map<uint32_t, uint32_t> cWndValue;
static std::map<uint32_t, uint32_t> ssThreshValue;
static bool goodputSetup = false;
static uint32_t goodput = 0;
static uint32_t goodput2 = 0;
static Ptr<OutputStreamWrapper> goodputStream;
static Ptr<OutputStreamWrapper> goodputStream2;
static Ptr<OutputStreamWrapper> goodputStreamDetailed;
static uint32_t throughput;
static Ptr<OutputStreamWrapper> throughputStream;
static Ptr<OutputStreamWrapper> throughputStreamDetailed;
static Ptr<OutputStreamWrapper> flowStatStream;
static Ptr<OutputStreamWrapper> bytesDroppedStream;
static Ptr<OutputStreamWrapper> packetsDroppedStream;

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

  if (goodputSetup)
  {
    *goodputStream2->GetStream () << Simulator::Now().GetSeconds() << "," << goodput2 << std::endl;
  }

  /*if (!firstSshThr[nodeId])
    {
      *ssThreshStream[nodeId]->GetStream ()
          << Simulator::Now ().GetSeconds () << "," << ssThreshValue[nodeId] << std::endl;
    }*/
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
NextTxTracer (std::string context, [[maybe_unused]] SequenceNumber32 old, SequenceNumber32 nextTx)
{
  uint32_t nodeId = GetNodeIdFromContext (context);

  *nextTxStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << nextTx << std::endl;
}

static void
InFlightTracer (std::string context, [[maybe_unused]] uint32_t old, uint32_t inFlight)
{
  uint32_t nodeId = GetNodeIdFromContext (context);

  *inFlightStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << inFlight << std::endl;
}

static void
NextRxTracer (std::string context, [[maybe_unused]] SequenceNumber32 old, SequenceNumber32 nextRx)
{
  uint32_t nodeId = GetNodeIdFromContext (context);

  *nextRxStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << nextRx << std::endl;
}

static void
CongStateTracer (std::string context, TcpSocketState::TcpCongState_t oldval, TcpSocketState::TcpCongState_t newval)
{
  /**CA_OPEN(0),      < Normal state, no dubious events */
  /**CA_DISORDER(1),  < In all the respects it is "Open",
                     *  but requires a bit more attention. It is entered when
                     *  we see some SACKs or dupacks. It is split of "Open" */
  /**CA_CWR(2),       < cWnd was reduced due to some congestion notification
                     *  event, such as ECN, ICMP source quench, local device
                     *  congestion. */
  /**CA_RECOVERY(3),  < CWND was reduced, we are fast-retransmitting. */
  /**CA_LOSS(4),      < CWND was reduced due to RTO timeout or SACK reneging. */

  //NS_LOG_INFO(Simulator::Now().GetSeconds()<<" New congstate value: "<<newval);
  //NS_LOG_INFO(Simulator::Now().GetSeconds()<<" Old congState value: "<<oldval);
  uint32_t nodeId = GetNodeIdFromContext (context);

  if (firstCongState[nodeId])
    {
      *congStateStream[nodeId]->GetStream () << "0.0," << oldval << std::endl;
      firstCongState[nodeId] = false;
    }
  *congStateStream[nodeId]->GetStream () << Simulator::Now ().GetSeconds () << "," << newval << std::endl;
}
static void
rxDropTracer(std::string context, Ptr<const Packet> packet)
{
  //NS_LOG_INFO(Simulator::Now().GetSeconds() << ": Got packet with size: " << packet->GetSize());
  uint32_t nodeId = GetNodeIdFromContext (context);
  *rxDropStream[nodeId]->GetStream () << Simulator::Now().GetSeconds() << "," << packet->GetSize() << std::endl;
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
TraceNextTx (std::string next_tx_seq_file_name, uint32_t nodeId)
{
  //NS_LOG_INFO("NOTICE ME************Reached the TraceNextTx function");
  AsciiTraceHelper ascii;
  nextTxStream[nodeId] = ascii.CreateFileStream (next_tx_seq_file_name.c_str ());
  *nextTxStream[nodeId]->GetStream () << "Time,Sequence number of next packet" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence",
                   MakeCallback (&NextTxTracer));
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
TraceNextRx (std::string next_rx_seq_file_name, uint32_t nodeId)
{
  AsciiTraceHelper ascii;
  nextRxStream[nodeId] = ascii.CreateFileStream (next_rx_seq_file_name.c_str ());
  Config::Connect ("/NodeList/" + std::to_string (nodeId) +
                       "/$ns3::TcpL4Protocol/SocketList/1/RxBuffer/NextRxSequence",
                   MakeCallback (&NextRxTracer));
}

static void
TraceCongState (std::string tr_file_name, uint32_t nodeId)
{
  NS_LOG_LOGIC("Called TraceCwnd");
  AsciiTraceHelper ascii;
  congStateStream[nodeId] = ascii.CreateFileStream (tr_file_name.c_str ());
  *congStateStream[nodeId]->GetStream () << "Time,State" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/CongState",
                   MakeCallback (&CongStateTracer));
  NS_LOG_LOGIC("Finished TraceCwnd");
}

static void
TraceRxDrop (std::string tr_file_name, uint32_t nodeId)
{
  AsciiTraceHelper ascii;
  rxDropStream[nodeId] = ascii.CreateFileStream (tr_file_name.c_str ());
  *rxDropStream[nodeId]->GetStream () << "Time,Size" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/DeviceList/1/$ns3::PointToPointNetDevice/PhyRxDrop",
                   MakeCallback (&rxDropTracer));
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
DetailedThroughputTracer(std::string context, Ptr<const Packet> packet)
{
  //NS_LOG_INFO(Simulator::Now().GetSeconds() << ": Got packet with size: " << packet->GetSize());
  *throughputStreamDetailed->GetStream () << Simulator::Now().GetSeconds() << "," << packet->GetSize() << std::endl;
  throughput = throughput + packet->GetSize();
}

static void
ThroughputTracer()
{
  *throughputStream->GetStream () << Simulator::Now().GetSeconds() << "," << throughput << std::endl;
  throughput = 0;
  Simulator::Schedule(Seconds(1), ThroughputTracer);
}

static void
TraceThroughput(std::string tr_file_dir, uint32_t nodeId)
{
  std::string tr_file_name = tr_file_dir + "throughput.data";
  std::string tr_file_name_detailed = tr_file_dir + "throughput-Detailed.data";
  AsciiTraceHelper ascii;
  throughputStream = ascii.CreateFileStream (tr_file_name.c_str ());
  throughputStreamDetailed = ascii.CreateFileStream (tr_file_name_detailed.c_str ());
  *throughputStream->GetStream () << "Time,throughput" << std::endl;
  *throughputStreamDetailed->GetStream () << "Time,throughput" << std::endl;
  Config::Connect ("/NodeList/" + std::to_string (nodeId) + "/DeviceList/0/$ns3::PointToPointNetDevice/PhyTxBegin",
                   MakeCallback (&DetailedThroughputTracer));
  Simulator::Schedule(Seconds(1), ThroughputTracer);
  Simulator::Schedule(Seconds(stopTime-0.001), ThroughputTracer);
}

static void
RecordFlowStats(ns3::Time Period, Ptr<FlowMonitor> monitor)
{
  //NS_LOG_INFO("Started RecordFlowStats");
  monitor->CheckForLostPackets();
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  auto itr = stats.begin ();
  std::vector<uint64_t> BD = itr->second.bytesDropped;
  std::vector<uint32_t> PD = itr->second.packetsDropped;
  std::string BDString = "";
  std::string PDString = "";
  //NS_LOG_INFO("Size of the BD vector is " << BD.size());
  //NS_LOG_INFO("Size of the PD vector is " << PD.size());
  *flowStatStream->GetStream () << Simulator::Now().GetSeconds() << "," << itr->second.lostPackets << "," << itr->second.rxBytes << "," << itr->second.rxPackets << "," << itr->second.txBytes << "," << itr->second.rxPackets << std::endl;
  for(uint i=0; i<BD.size(); i++)
  {
    BDString = BDString + "," + std::to_string(BD[i]);
  }
  for(uint i=BD.size(); i<9; i++)
  {
    BDString = BDString + ",0";
  }
  for(uint i=0; i<PD.size(); i++)
  {
    PDString = PDString + "," + std::to_string(PD[i]);
  }
  for(uint i=PD.size(); i<9; i++)
  {
    PDString = PDString + ",0";
  }
  *bytesDroppedStream->GetStream () << Simulator::Now().GetSeconds() << BDString << std::endl;
  *packetsDroppedStream->GetStream () << Simulator::Now().GetSeconds() << PDString << std::endl;
  /*if(BD.size() == 5)
  {
    *bytesDroppedStream->GetStream () << Simulator::Now().GetSeconds() << "," << BD[0] << "," << BD[1] << "," << BD[2] << "," << BD[3] << "," << BD[4] << "," << BD[5] << ",0,0,0" << std::endl;
    *packetsDroppedStream->GetStream () << Simulator::Now().GetSeconds() << "," << PD[0] << "," << PD[1] << "," << PD[2] << "," << PD[3] << "," << PD[4] << "," << PD[5] << ",0,0,0" <<  std::endl;
  }*/
  //*bytesDroppedStream->GetStream () << Simulator::Now().GetSeconds() << "," << BD[0] << "," << BD[1] << "," << BD[2] << "," << BD[3] << "," << BD[4] << "," << BD[5] << "," << BD[6] << "," << BD[7] << "," << BD[8] << std::endl;
  //*packetsDroppedStream->GetStream () << Simulator::Now().GetSeconds() << "," << PD[0] << "," << PD[1] << "," << PD[2] << "," << PD[3] << "," << PD[4] << "," << PD[5] << "," << PD[6] << "," << PD[7] << "," << PD[8] << std::endl;
  //Time curTime = Now ();
  //std::ofstream thr (tp_tr_file_name, std::ios::out | std::ios::app);
  //thr <<  curTime << " " << 8 * (itr->second.rxBytes - prev) / (1000 * 1000 * (curTime.GetSeconds () -     prevTime.GetSeconds ())) << std::endl;

  //prevTime = curTime;
  //prev = itr->second.rxBytes;
  Simulator::Schedule (Period, RecordFlowStats, Period, monitor);
}

static void
TraceFlowStats(std::string tr_file_dir, ns3::Time Period, Ptr<FlowMonitor> monitor)
{
  //NS_LOG_INFO("Started TraceFlowStats");
  std::string tr_file_name1 = tr_file_dir + "flowStats.data";
  std::string tr_file_name2 = tr_file_dir + "bytesDropped.data";
  std::string tr_file_name3 = tr_file_dir + "packetsDropped.data";
  AsciiTraceHelper ascii;
  flowStatStream = ascii.CreateFileStream(tr_file_name1.c_str ());
  *flowStatStream->GetStream () << "Time,Packets Lost,rxBytes,rxPackets,txBytes,txPackets" << std::endl;
  bytesDroppedStream = ascii.CreateFileStream(tr_file_name2.c_str ());
  *bytesDroppedStream->GetStream () << "Time,NO_ROUTE,TTL_EXPIRE,BAD_CHECKSUM,QUEUE,QUEUE_DISC,INTERFACE_DOWN,ROUTE_ERROR,FRAGMENT_TIMEOUT,INVALID_REASON" << std::endl;
  packetsDroppedStream = ascii.CreateFileStream(tr_file_name3.c_str ());
  *packetsDroppedStream->GetStream () << "Time,NO_ROUTE,TTL_EXPIRE,BAD_CHECKSUM,QUEUE,QUEUE_DISC,INTERFACE_DOWN,ROUTE_ERROR,FRAGMENT_TIMEOUT,INVALID_REASON" << std::endl;
  //NS_LOG_INFO("Finished TraceFlowStats");
  RecordFlowStats(Period, monitor);
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
  cmd.AddValue ("dropPackets", "Drop packets in queue when route switching happens", dropPackets);
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("switchTime", "Start time of route switching in seconds", switchTime);
  cmd.AddValue ("RSP", "Route Switch Period in seconds", RSP);
  cmd.AddValue ("stopTime", "Time for the simulation to stop in seconds", stopTime);
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
  firstSshThr[0] = true;
  firstRtt[0] = true;
  firstRto[0] = true;

  //Simulator::Schedule (Seconds (startTime + 1.00001), &TraceCwnd, "scratch/P5/Statistics/RoutingTest2-cwnd.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceCwnd, "scratch/P5/Statistics/RoutingTest2/cwnd.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceSsThresh, "scratch/P5/Statistics/RoutingTest2/ssth.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceRtt, "scratch/P5/Statistics/RoutingTest2/rtt.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceRto, "scratch/P5/Statistics/RoutingTest2/rto.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceNextTx, "scratch/P5/Statistics/RoutingTest2/next-tx.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceInFlight, "scratch/P5/Statistics/RoutingTest2/inflight.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceCongState, "scratch/P5/Statistics/RoutingTest2/congState.data", 0);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceRxDrop, "scratch/P5/Statistics/RoutingTest2/DroppedNode2", 2);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceRxDrop, "scratch/P5/Statistics/RoutingTest2/DroppedNode3", 3);
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceGoodput, "scratch/P5/Statistics/RoutingTest2/", dstNode.Get(0)->GetId());
  Simulator::Schedule (Seconds (startTime+0.0001), &TraceThroughput, "scratch/P5/Statistics/RoutingTest2/", srcNode.Get(0)->GetId());
  
  //NS_LOG_INFO("Destination node ID: " << dstNode.Get(0)->GetId());
  //Simulator::Schedule (Seconds (startTime+0.1), &TraceNextRx, "scratch/P5/Statistics/RoutingTest2/next-rx.data", dstNode.Get(0)->GetId());
  //Config::Connect ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongState", MakeCallback (&CongStateTracer));
  //Simulator::Schedule (Seconds(startTime+0.0001), &Config::Connect, "/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongState", MakeCallback (&CongStateTracer));

  AnimationInterface anim("scratch/P5/Animations/RoutingTest2/Animation.xml");
  //anim.EnablePacketMetadata(true);
  anim.EnableIpv4RouteTracking("scratch/P5/Animations/RoutingTest2/Routes.xml", Seconds(startTime), Seconds(stopTime), Seconds(RSP));
  FlowMonitorHelper flowHelper;
  Ptr<FlowMonitor> FM = flowHelper.InstallAll();
  Simulator::Schedule(Seconds(1), &TraceFlowStats, "scratch/P5/Statistics/RoutingTest2/", Seconds(1), FM);
  //NS_LOG_INFO("Scheduled FlowStats");
  Simulator::Stop(Seconds(stopTime));
  Simulator::Run ();
  flowHelper.SerializeToXmlFile("scratch/P5/Animations/RoutingTest2/FlowMonitor.xml", false, false);
  Simulator::Destroy ();

}