#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-socket.h"
#include "ns3/config.h"
#include "ns3/csma-module.h"
#include "ns3/animation-interface.h"
#include "ns3/pcap-file.h"
#include <iostream>
#include <vector>
using namespace ns3;
using namespace std;

string RESET = "\u001B[0m";
string BLACK = "\u001B[30;1m";
string RED = "\u001B[31m";
string GREEN = "\u001B[32m";
string YELLOW = "\u001B[33m";
string BLUE = "\u001B[34m";
string PURPLE = "\u001B[35m";
string CYAN = "\u001B[36m";
string WHITE = "\u001B[37m";



void log(){
    string in;
    cout << "Enable or disable? ";
    cin >> in;
    if(in == "enable" || in == "Enable"){
        cout << "component name? ";
        cin >> in;
        LogComponentEnable(in.c_str(), LOG_LEVEL_INFO);
    }
    else if(in == "disable" || in == "Disable"){
        cout << "component name? ";
        cin >> in;
        LogComponentDisable(in.c_str(), LOG_LEVEL_INFO);
    }
}

void run(){
    NodeContainer nodes;
    nodes.Create(6);
    Ptr<Node> src = nodes.Get(0);
    Ptr<Node> src_gw = nodes.Get(1);
    Ptr<Node> lnk1 = nodes.Get(2);
    Ptr<Node> lnk2 = nodes.Get(3);
    Ptr<Node> dst_gw = nodes.Get(4);
    Ptr<Node> dst = nodes.Get(5);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4StaticRoutingHelper router;

    //router.PrintRoutingTableAllAt(Time(0), Create<OutputStreamWrapper>(&cout), Time::S);
    router.PrintRoutingTableAt(Time(0), src, Create<OutputStreamWrapper>(&cout), Time::S);

    stack.SetRoutingHelper(router);

    Simulator::Run();
    Simulator::Destroy();
}

int main(int argc, char *argv[]){
    CommandLine(__FILE__).Parse(argc, argv);

    for(string in = ""; in != "exit"; cin >> in){
        if(in == "log") log();
        else if(in == "run") run();

        cout << RESET << "[" << CYAN << "simulator" << RESET << "] > ";
    }
    return 0;
}