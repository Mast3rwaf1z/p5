import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import subprocess
import pickle
from PlotStuff import PlotCWND

def PickleWrite(Obj, Path):
    with open(Path, "wb") as f:
        pickle.dump(Obj, f)

def PickleRead(Path):
    with open(Path, "rb") as f:
        Obj = pickle.load(f)
    return Obj

def RoutingTests(Path, TCP="TcpNewReno"):
    Cwnd = []
    SSThresh = []
    InFlight = []
    GoodPut = []
    GoodPut2 = []
    GoodPutDetailed = []
    RTT = []
    RTO = []
    FlowStats = []
    BytesDropped = []
    PacketsDropped = []
    CongState = []
    for i in range(10):
        print(i+1)
        RSP = (i+1)*10
        #SubReturn = subprocess.run(["ns3", "run", f"scratch/RoutingTest2.cc --datarate=50Kbps --delay=10ms --bottleneckRate=50Kbps --bottleneckDelay=10ms --transport_prot={TCP} --switchTime={RSP} --RSP={RSP} --stopTime=2000"])
        SubReturn = subprocess.run(["ns3", "run", f"scratch/RoutingTest2.cc --datarate=1Mbps --delay=10ms --bottleneckRate=1Mbps --bottleneckDelay=10ms --transport_prot={TCP} --switchTime={RSP} --RSP={RSP} --stopTime=200"])
        Cwnd.append(pd.read_csv("Statistics/RoutingTest2/cwnd.data"))
        SSThresh.append(pd.read_csv("Statistics/RoutingTest2/ssth.data"))
        InFlight.append(pd.read_csv("Statistics/RoutingTest2/inflight.data"))
        GoodPut.append(pd.read_csv("Statistics/RoutingTest2/goodput.data"))
        GoodPut2.append(pd.read_csv("Statistics/RoutingTest2/goodput2.data"))
        GoodPutDetailed.append(pd.read_csv("Statistics/RoutingTest2/goodput-Detailed.data"))
        RTT.append(pd.read_csv("Statistics/RoutingTest2/rtt.data"))
        RTO.append(pd.read_csv("Statistics/RoutingTest2/rto.data"))
        FlowStats.append(pd.read_csv("Statistics/RoutingTest2/flowStats.data"))
        BytesDropped.append(pd.read_csv("Statistics/RoutingTest2/bytesDropped.data"))
        PacketsDropped.append(pd.read_csv("Statistics/RoutingTest2/packetsDropped.data"))
        CongState.append(pd.read_csv("Statistics/RoutingTest2/congState.data"))
    PickleWrite(Cwnd, Path + "cwnd.obj")
    PickleWrite(SSThresh, Path + "ssth.obj")
    PickleWrite(InFlight, Path + "inflight.obj")
    PickleWrite(GoodPut, Path + "goodput.obj")
    PickleWrite(GoodPut2, Path + "goodput2.obj")
    PickleWrite(GoodPutDetailed, Path + "goodput-Detailed.obj")
    PickleWrite(RTT, Path + "rtt.obj")
    PickleWrite(RTO, Path + "rto.obj")
    PickleWrite(FlowStats, Path + "flowStats.obj")
    PickleWrite(BytesDropped, Path + "bytesDropped.obj")
    PickleWrite(PacketsDropped, Path + "packetsDropped.obj")
    PickleWrite(CongState, Path + "congState.obj")

def RoutingTests2(Path, TCP="TcpNewReno"):
    Cwnd = []
    SSThresh = []
    InFlight = []
    GoodPut = []
    GoodPut2 = []
    GoodPutDetailed = []
    RTT = []
    RTO = []
    FlowStats = []
    BytesDropped = []
    PacketsDropped = []
    CongState = []
    for i in range(5):
        print(i)
        Rate = 5*10**i
        SubReturn = subprocess.run(["ns3", "run", f"scratch/RoutingTest2.cc --datarate={Rate}Kbps --delay=10ms --bottleneckRate={Rate}Kbps --bottleneckDelay=10ms --transport_prot={TCP} --switchTime=10 --RSP=10 --stopTime=200"])
        Cwnd.append(pd.read_csv("Statistics/RoutingTest2/cwnd.data"))
        SSThresh.append(pd.read_csv("Statistics/RoutingTest2/ssth.data"))
        InFlight.append(pd.read_csv("Statistics/RoutingTest2/inflight.data"))
        GoodPut.append(pd.read_csv("Statistics/RoutingTest2/goodput.data"))
        GoodPut2.append(pd.read_csv("Statistics/RoutingTest2/goodput2.data"))
        GoodPutDetailed.append(pd.read_csv("Statistics/RoutingTest2/goodput-Detailed.data"))
        RTT.append(pd.read_csv("Statistics/RoutingTest2/rtt.data"))
        RTO.append(pd.read_csv("Statistics/RoutingTest2/rto.data"))
        FlowStats.append(pd.read_csv("Statistics/RoutingTest2/flowStats.data"))
        BytesDropped.append(pd.read_csv("Statistics/RoutingTest2/bytesDropped.data"))
        PacketsDropped.append(pd.read_csv("Statistics/RoutingTest2/packetsDropped.data"))
        CongState.append(pd.read_csv("Statistics/RoutingTest2/congState.data"))
    PickleWrite(Cwnd, Path + "cwnd.obj")
    PickleWrite(SSThresh, Path + "ssth.obj")
    PickleWrite(InFlight, Path + "inflight.obj")
    PickleWrite(GoodPut, Path + "goodput.obj")
    PickleWrite(GoodPut2, Path + "goodput2.obj")
    PickleWrite(GoodPutDetailed, Path + "goodput-Detailed.obj")
    PickleWrite(RTT, Path + "rtt.obj")
    PickleWrite(RTO, Path + "rto.obj")
    PickleWrite(FlowStats, Path + "flowStats.obj")
    PickleWrite(BytesDropped, Path + "bytesDropped.obj")
    PickleWrite(PacketsDropped, Path + "packetsDropped.obj")
    PickleWrite(CongState, Path + "congState.obj")

def AnalyzeCwnd(Cwnd, SimTime=2000):
    Area = []
    Mean = []
    for i in Cwnd:
        Time = i.iloc[:,0].to_numpy()
        Size = i.iloc[:,1].to_numpy()
        #print(Time)
        #print(Size)
        Sum = 0
        for j in range(0,len(Time)-1):
            Sum += (Time[j+1]-Time[j])*Size[j]
        Sum += (SimTime-Time[-1])*Size[-1]
        Area.append(Sum)
        Mean.append(np.mean(Size))
    return Area,Mean
        
def AnalyzeGoodput(Goodput, SimTime=2000):
    Mean = []
    for i in Goodput:
        Mean.append(np.mean(i.iloc[:,1].to_numpy()))
    return Mean


if __name__=="__main__":
    RoutingTests("TestResults/RoutingTests1Mbps/RSPTests/NewReno/", TCP="TcpNewReno")
    RoutingTests("TestResults/RoutingTests1Mbps/RSPTests/WestwoodPlus/", TCP="TcpWestwoodPlus")
    RoutingTests("TestResults/RoutingTests1Mbps/RSPTests/Cubic/", TCP="TcpCubic")
    #RoutingTests2("TestResults/RoutingTests/DataRateTests/NewReno/", TCP="TcpNewReno")
    #RoutingTests2("TestResults/RoutingTests/DataRateTests/WestwoodPlus/", TCP="TcpWestwoodPlus")
    #RoutingTests2("TestResults/RoutingTests/DataRateTests/Cubic/", TCP="TcpCubic")
    """
    Cwnd = PickleRead("TestResults/RoutingTests/DataRateTests/NewReno/cwnd.obj")
    #print(Cwnd)
    Res = AnalyzeCwnd(Cwnd)
    #print(Res)
    Goodput = PickleRead("TestResults/RoutingTests/DataRateTests/NewReno/goodput.obj")
    Mean = AnalyzeGoodput(Goodput)
    Fig, ax = plt.subplots(3,1, sharex=True)
    x = [i*10 for i in range(1,len(Cwnd)+1)]
    ax[0].scatter(x, Res[0])
    ax[1].scatter(x, Res[1])
    ax[2].scatter(x, Mean)
    ax[0].set_title("Integrated Congestion Window")
    ax[1].set_title("Average Congestion Window Size")
    ax[2].set_title("Average Goodput")
    ax[0].set_ylabel("Area (Bytes*s)")
    ax[1].set_ylabel("Size (Bytes)")
    ax[2].set_ylabel("Goodput (Bytes)")
    ax[2].set_xlabel("Route Switch Period (s)")
    plt.show()
    """
    #print(Mean)
    #for i in range(10):
    #    RSP = (i+1)*10
    #    PlotCWND("", DF=Cwnd[i], SwitchTimes=[i*RSP for i in range(1,2000//RSP)])
    #plt.show()
    