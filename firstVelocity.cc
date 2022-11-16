/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <iostream>
#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"


// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("firstVelocity");

/**
 * Function called when there is a course change
 * \param context event context
 * \param mobility a pointer to the mobility model
 */
static void 
CourseChange (std::string context, Ptr<const MobilityModel> mobility)
{
  NS_LOG_INFO(Simulator::Now () << ": Running CourseChange Function");
  Vector pos = mobility->GetPosition ();
  Vector vel = mobility->GetVelocity ();
  std::cout << "Context=" << context << std::endl;
  std::cout << Simulator::Now () << ", model=" << mobility << ", POS: x=" << pos.x << ", y=" << pos.y
            << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
            << ", z=" << vel.z << std::endl;
}

static void ChangeCourse(Ptr<Node> n, Vector Vel)
{
  n->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vel);
}

int
main (int argc, char *argv[])
{
  uint32_t nPackets = 1;
  CommandLine cmd (__FILE__);
  cmd.AddValue("nPackets", "Number of packets to echo", nPackets);
  cmd.Parse (argc, argv);

  NS_LOG_INFO("PeePeePooPoo");
  Time::SetResolution (Time::NS);
  LogComponentEnable ("firstVelocity", LOG_LEVEL_ALL);
  //LogComponentEnable ("ConstantVelocityHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO("Creating Topology");
  NodeContainer nodes;
  nodes.Create (2);

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (20),
                                 "MinY", DoubleValue (20),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (0),
                                 "GridWidth", UintegerValue (2),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mobility.Install(nodes);
  nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0,1,0));
  //Simulator::Schedule (Seconds(5), nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->&SetVelocity, Vector(0,-1,0));
  //Simulator::Schedule (Seconds(5), &ChangeCourse, nodes.Get(1), Vector(0,-1,0));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (.0));

  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeCallback (&CourseChange));
  
  AsciiTraceHelper ascii;
  mobility.EnableAscii(ascii.CreateFileStream ("scratch/firstVelocityMobility.tr"), 1);
  pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("scratch/firstVel.tr"));
  pointToPoint.EnablePcapAll ("scratch/firstVel");
  AnimationInterface anim("firstVelAnimation.xml");
  Simulator::Stop(Seconds(10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}