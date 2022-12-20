import pandas as pd
import matplotlib.pyplot as plt
#import numpy as np
import subprocess

def PlotCWND(Path, Datarate=None, axes=None, Label="", Color="tab:blue", SwitchTimes=[], DF=None):
    if DF is None: DF = pd.read_csv(Path)
    #print(DF)
    Fig, ax = plt.subplots()
    if axes is not None:
        ax = axes
    #ax.plot(DF["Time"], DF["Window Size"])
    if not Datarate is None:
        ax.axhline(Datarate, color="r")
    #ax.scatter(DF["Time"], DF["Window Size"])
    ax.step(DF["Time"], DF["Window Size"], where='post', label=Label, color=Color)
    if axes is None:
        ax.set_title("Congestion Window Size")
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Size (bytes)")
    for i in SwitchTimes:
        ax.axvline(i, color="tab:red", alpha=0.5)
    #Fig.show()
    #plt.show()
    #plt.figure()
    return Fig, ax

def PlotSSThresh(Path, yLim=None, SwitchTimes=[], DF=None):
    if DF is None: DF = pd.read_csv(Path)
    #print(DF)
    Fig, ax = plt.subplots()
    ax.step(DF.iloc[:,0], DF.iloc[:,1], where="post")
    if not yLim is None:
        ax.set_ylim(0,yLim)
    ax.set_title("Slow Start Threshold")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Threshold (bytes)")
    for i in SwitchTimes:
        ax.axvline(i, color="tab:red", alpha=0.5)
    #plt.show()
    #plt.figure()
    return Fig,ax

def PlotInFlight(Path, axes=None, Label="", Color="tab:blue", DF=None):
    if DF is None: DF = pd.read_csv(Path)
    #print(DF)
    Fig, ax = plt.subplots()
    if axes is not None:
        ax = axes
    ax.step(DF.iloc[:,0], DF.iloc[:,1], where="post", label = Label)
    if axes is None:
        ax.set_title("Bytes in flight")
        ax.set_xlabel("Time (s)")
        ax.set_ylabel("Bytes")
    #plt.show()
    #plt.figure()
    return Fig,ax

def PlotGoodput(Path, Derivative=False, SwitchTimes=[], DF=None):
    if DF is None: DF = pd.read_csv(Path)
    Fig, ax = plt.subplots()
    if Derivative:
        #print(f'({DF.iloc[5,1]}-{DF.iloc[5-1,1]})/({DF.iloc[5,0]}-{DF.iloc[5-1,0]} = {(DF.iloc[5,1]-DF.iloc[5-1,1])/(DF.iloc[5,0]-DF.iloc[5-1,0])})')
        y = [(DF.iloc[i,1]-DF.iloc[i-1,1])/(DF.iloc[i,0]-DF.iloc[i-1,0]) for i in range(1,DF.shape[0])]
        #print(y)
        #ax.step(DF.iloc[1:,0], y, where="post")
        ax.scatter(DF.iloc[1:,0], y)
    else:
        #ax.step(DF.iloc[:,0], DF.iloc[:,1], where="post")
        ax.scatter(DF.iloc[:,0], DF.iloc[:,1])
    ax.set_title("Goodput")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Bytes/s")
    for i in SwitchTimes:
        ax.axvline(i, color="tab:red", alpha=0.5)
    #plt.show()
    #plt.figure()
    return Fig,ax

def PlotRTT(Path, yLim=None, SwitchTimes=[], DF=None):
    if DF is None: DF = pd.read_csv(Path)
    #print(DF)
    Fig, ax = plt.subplots()
    ax.step(DF.iloc[:,0], DF.iloc[:,1], where="post")
    if not yLim is None:
        ax.set_ylim(0,yLim)
    ax.set_title("Round Trip Time")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Time (s)")
    for i in SwitchTimes:
        ax.axvline(i, color="tab:red", alpha=0.5)
    #plt.show()
    #plt.figure()
    return Fig,ax

def PlotRTO(Path, yLim=None, SwitchTimes=[], DF=None):
    if DF is None: DF = pd.read_csv(Path)
    #print(DF)
    Fig, ax = plt.subplots()
    ax.step(DF.iloc[:,0], DF.iloc[:,1], where="post")
    if not yLim is None:
        ax.set_ylim(0,yLim)
    ax.set_title("Re-transmission Timeout")
    ax.set_xlabel("Time (s)")
    ax.set_ylabel("Time (s)")
    for i in SwitchTimes:
        ax.axvline(i, color="tab:red", alpha=0.5)
    #plt.show()
    #plt.figure()
    return Fig,ax
    
def PlotCwndAndInFlight(Path, SwitchTimes=[]):
    Path1 = Path + "cwnd.data"
    DF1 = pd.read_csv(Path1)
    Path2 = Path + "inflight.data"
    DF2 = pd.read_csv(Path2)
    Fig, ax = plt.subplots(1,2, sharey=True)
    ax[0].step(DF1["Time"], DF1["Window Size"], where='post', label="Cwnd", color="tab:blue")
    ax[1].step(DF2.iloc[:,0], DF2.iloc[:,1], where="post", label = "In Flight", color="tab:blue")
    #ax.scatter(DF1["Time"], DF1["Window Size"], label="Cwnd", color="tab:blue")
    #ax.scatter(DF2.iloc[:,0], DF2.iloc[:,1], label = "SSThresh", color="tab:red")
    ax[0].set_title("Congestion Window")
    ax[0].set_xlabel("Time (s)")
    ax[0].set_ylabel("Bytes")
    ax[1].set_title("Bytes in Flight")
    ax[1].set_xlabel("Time (s)")
    for i in SwitchTimes:
        ax[0].axvline(i, color="tab:red")
        ax[1].axvline(i, color="tab:red")
    #ax.legend()
    return Fig, ax
    

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

def RoutingTest2(Path):
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
    --dropPackets:      Drop packets in queue when route switching happens [true]
    --error_p:          Packet error rate [0]
    --sack:             Enable or disable SACK option [false]
    --switchTime:       Start time of route switching in seconds [1]
    --RSP:              Route Switch Period in seconds [1]
    --stopTime:         Time for the simulation to stop in seconds [10]
    """
    #SubReturn = subprocess.run(["ns3", "run", "scratch/RoutingTest2.cc --transport_prot=TcpLinuxReno --switchTime=5000 --RSP=50 --stopTime=600 --bottleneckRate=50Kbps --bottleneckDelay=10ms"])
    # No route switching. Base testing for behaviour of algorithms in normal network
    #SubReturn = subprocess.run(["ns3", "run", "scratch/RoutingTest2.cc --datarate=500Kbps --delay=10ms --transport_prot=TcpCubic --switchTime=500000 --stopTime=200"])
    # Setup similar to what the other group has. This seems better or something idk
    #SubReturn = subprocess.run(["ns3", "run", "scratch/RoutingTest2.cc --datarate=1Mbps --delay=10ms --transport_prot=TcpCubic --switchTime=500000 --stopTime=200"])
    #SubReturn = subprocess.run(["ns3", "run", "scratch/RoutingTest2.cc --datarate=50Kbps --delay=10ms --mtu=400 --transport_prot=TcpNewReno --switchTime=500000 --stopTime=2000"])
    
    #RSP=100
    #SubReturn = subprocess.run(["ns3", "run", f"scratch/RoutingTest2.cc --bottleneckRate=50Kbps --bottleneckDelay=10ms --transport_prot=TcpNewReno --switchTime={RSP} --RSP={RSP} --stopTime=2000"])
    #LinkBreaks = [i*RSP for i in range(1,2000//RSP)]
    LinkBreaks = []
    Fig1,ax1 = PlotCWND(Path + "cwnd.data", SwitchTimes=LinkBreaks)
    PlotSSThresh(Path + "ssth.data", 50000, SwitchTimes=LinkBreaks)
    PlotInFlight(Path + "inflight.data")
    PlotGoodput(Path + "goodput.data", SwitchTimes=LinkBreaks)
    PlotGoodput(Path + "goodput2.data", True, SwitchTimes=LinkBreaks)
    #PlotCwndAndInFlight("Statistics/RoutingTest2/")
    plt.show()

def DynamicLinks():
    """
    --numNodes:        Number of nodes in each orbit [3]
    --srcIndex:        Index of the sending node in the source orbit [0]
    --dstIndex:        Index of the recieving node in the destination orbit [0]
    --transport_prot:  Transport protocol to use: TcpNewReno, TcpLinuxReno, TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, TcpBic,
                                                  TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, TcpLp, TcpDctcp, TcpCubic, TcpBbr [TcpNewReno]
    --data:            Number of Megabytes of data to transmit (0 means unlimited) [0]
    --mtu:             Size of IP packets to send in bytes [400]
    --datarate:        Datarate of point to point links [50Kbps]
    --delay:           Delay of point to point links [10ms]
    --error_p:         Packet error rate [0]
    --speed:           Movement speed of the satellites in orbit (m/s) [1]
    --ISD:             Inter-satellite distance in meters. Used for satellites in same orbit. [5]
    --IOD:             Inter-Orbit distance in meters. Used for distance between orbits. [5]
    --COD:             Cutoff distance for RF links in meters [31.25]
    --sack:            Enable or disable SACK option [false]
    --DSP:             Distance Sampling Period [1]
    --stopTime:        Time for the simulation to stop [25]
    """
    #SubReturn = subprocess.run(["ns3", "run", "scratch/DynamicLinks.cc --numNodes=6"])
    SubReturn = subprocess.run(["ns3", "run", "scratch/DynamicLinks.cc --numNodes=6 --transport_prot=TcpCubic --speed=12350 --ISD=926537 --IOD=2736000 --COD=2888627 --stopTime=500"])
    LinkBreaks = [1,76,151,226,301]
    PlotCWND("Statistics/DynamicLinks/cwnd.data", SwitchTimes=LinkBreaks)
    PlotSSThresh("Statistics/DynamicLinks/ssth.data", yLim=50000, SwitchTimes=LinkBreaks)
    PlotInFlight("Statistics/DynamicLinks/inflight.data")
    PlotGoodput("Statistics/DynamicLinks/goodput.data", SwitchTimes=LinkBreaks)
    PlotGoodput("Statistics/DynamicLinks/goodput2.data", True, SwitchTimes=LinkBreaks)
    PlotRTT("Statistics/DynamicLinks/rtt.data", SwitchTimes=LinkBreaks)
    PlotRTO("Statistics/DynamicLinks/rto.data", SwitchTimes=LinkBreaks)
    #print(SubReturn)
    #DF = pd.read_csv("Statistics/DynamicLinks-cwnd.data")
    #print(DF)
    #Fig, ax = plt.subplots()
    #ax.scatter(DF["Time"], DF["Window Size"])
    #Fig.show()
    plt.show()

def VariableLinks():
    """
    --numNodes:          Number of nodes in each orbit [1]
    --srcIndex:          Index of the sending node in the source orbit [0]
    --dstIndex:          Index of the recieving node in the destination orbit [0]
    --transport_prot:    Transport protocol to use: TcpNewReno, TcpLinuxReno, TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, TcpBic, 
                                                    TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, TcpLp, TcpDctcp, TcpCubic, TcpBbr [TcpNewReno]
    --data:              Number of Megabytes of data to transmit [0]
    --mtu:               Size of IP packets to send in bytes [400]
    --error_p:           Packet error rate [0]
    --speed:             Movement speed of the satellites in orbit (m/s) [200000]
    --ISD:               Inter-satellite distance in meters. Used for satellites in same orbit. [1000]
    --IOD:               Inter-Orbit distance in meters. Used for distance between orbits. [3000]
    --sack:              Enable or disable SACK option [false]
    --initCwnd:          Initial size of the congestion window in segments [10]
    --DSP:               Distance Sampling Period [1]
    --switchTime:        Time for the route to swtich [10]
    --stopTime:          Time for the simulation to stop [25]
    --power:             Maximum transmission power in W [10]
    --frequency:         Frequency of RF links [2.6e+10]
    --bandwidth:         Maximum bandwidth of RF links [5e+08]
    --aDiameterTx:       Diameter of transmitting antenna [0.26]
    --aDiameterRx:       Diameter of receiving antenna [0.26]
    --noiseFigure:       Noise figure in dB [2]
    --noiseTemperature:  Noise temperature in kelvin [290]
    --pointingLoss:      Pointing loss in dB [0.3]
    --efficiency:        The efficiency of the RF links [0.55]
    """
    #SubReturn = subprocess.run(["ns3", "run", "scratch/VariableLinks.cc --bandwidth=10000 --speed=1000000 --noiseFigure=49 --numNodes=3"])
    #SubReturn = subprocess.run(["ns3", "run", "scratch/VariableLinks.cc --power=10 --speed=0 --stopTime=10 --bandwidth=50000"])
    #SubReturn = subprocess.run(["ns3", "run", "scratch/VariableLinks.cc --help"])
    #SubReturn = subprocess.run(["ns3", "run", "scratch/VariableLinks.cc --transport_prot=TcpCubic --power=0.001 --speed=12350 --ISD=926537 --IOD=2736000 --stopTime=500 --bandwidth=50000"])
    SubReturn = subprocess.run(["ns3", "run", "scratch/VariableLinks.cc --numNodes=6 --transport_prot=TcpCubic --power=0.001 --speed=12350 --ISD=926537 --IOD=2736000 --stopTime=500 --bandwidth=50000"])
    PlotCWND("Statistics/VariableLinks/cwnd.data")
    PlotSSThresh("Statistics/VariableLinks/ssth.data")
    PlotInFlight("Statistics/VariableLinks/inflight.data")
    PlotGoodput("Statistics/VariableLinks/goodput.data")
    PlotGoodput("Statistics/VariableLinks/goodput2.data", True)
    plt.show()

def TCPComparissonTest():
    SubReturn = subprocess.run(["ns3", "run", "scratch/tcp-variants-comparison.cc --transport_prot=TcpCubic --prefix_name=scratch/P5/TCPVariants/Test --bandwidth=50Kbps --delay=40ms --tracing=true --duration=2000 --pcap_tracing=true --sack=false"])
    PlotCWND("TCPVariants/Test-cwnd.data", 50000/8)
    PlotSSThresh("TCPVariants/Test-ssth.data", 50000)
    PlotInFlight("TCPVariants/Test-inflight.data")

if __name__=="__main__":
    #DynamicLinks()
    #PlotCWND("Statistics/RoutingTest2-cwnd.data")
    RoutingTest2("TestResults/RoutingTests/ControlCases/Cubic/")
    #VariableLinks()
    #TCPComparissonTest()
    
