import pandas as pd
import matplotlib.pyplot as plt
#import numpy as np
import subprocess

def PlotCWND(Path):
    DF = pd.read_csv(Path)
    print(DF)
    Fig, ax = plt.subplots()
    ax.plot(DF["Time"], DF["Window Size"])
    #ax.scatter(DF["Time"], DF["Window Size"])
    #Fig.show()
    plt.show()

def RoutingTests():
    SubReturn = subprocess.run(["ns3", "run", "scratch/RoutingTest1.cc --datarate=1000Kbps --bottleneckRate=100Kbps --stopTime=2, switchTime=1"])
    PlotCWND("Statistics/RoutingTest1-cwnd.data")
    #print(SubReturn)
    #DF = pd.read_csv("Statistics/RoutingTest1-cwnd.data")
    #print(DF)
    #Fig, ax = plt.subplots()
    #ax.scatter(DF["Time"], DF["Window Size"])
    #Fig.show()
    #plt.show()

def RoutingTest2():
    """
    Program Options:
    --transport_prot:   Transport protocol to use: TcpNewReno, TcpLinuxReno, TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, TcpBic,
                        TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, TcpLp, TcpDctcp, TcpCubic, TcpBbr [TcpNewReno]
    --data:             Number of Megabytes of data to transmit [0]
    --mtu:              Size of IP packets to send in bytes [400]
    --bottleneckRate:   Datarate of bottleneck point to point links [20Kbps]
    --bottleneckDelay:  Delay of bottleneck point to point links [30ms]
    --datarate:         Datarate of point to point links [50Kbps]
    --delay:            Delay of point to point links [10ms]
    --dropPackets:      Drop packets in queue when route swtiching happens [true]
    --error_p:          Packet error rate [0]
    --sack:             Enable or disable SACK option [false]
    --switchTime:       Start time of route switching in seconds [1]
    --RSP:              Route Switch Period in seconds [1]
    --stopTime:         Time for the simulation to stop in seconds [10]
    """
    SubReturn = subprocess.run(["ns3", "run", "scratch/RoutingTest2.cc --transport_prot=TcpLinuxReno --switchTime=5000 --RSP=50 --stopTime=600 --bottleneckRate=50Kbps --bottleneckDelay=10ms"])
    PlotCWND("Statistics/RoutingTest2-cwnd.data")

def DynamicLinks():
    SubReturn = subprocess.run(["ns3", "run", "scratch/DynamicLinks.cc"])
    PlotCWND("Statistics/DynamicLinks-cwnd.data")
    #print(SubReturn)
    #DF = pd.read_csv("Statistics/DynamicLinks-cwnd.data")
    #print(DF)
    #Fig, ax = plt.subplots()
    #ax.scatter(DF["Time"], DF["Window Size"])
    #Fig.show()
    #plt.show()



if __name__=="__main__":
   #DynamicLinks()
   #PlotCWND("Statistics/RoutingTest2-cwnd.data")
    RoutingTest2()
    "scratch/VariableLinks.cc --bandwidth=10000 --speed=1000000 --noiseFigure=49 --numNodes=3"
