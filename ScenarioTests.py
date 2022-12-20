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
        SubReturn = subprocess.run(["ns3", "run", f"scratch/RoutingTest2.cc --datarate=50Kbps --delay=10ms --bottleneckRate=50Kbps --bottleneckDelay=10ms --transport_prot={TCP} --switchTime={RSP} --RSP={RSP} --stopTime=2000"])
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

def PlotInDepth(Path, Index=9, Datarate=50000):
    Cwnd = PickleRead(Path+"cwnd.obj")
    Goodput = PickleRead(Path+"goodput.obj")
    RTT = PickleRead(Path+"rtt.obj")
    RSP = (Index+1)*10
    SwitchTimes = [i*RSP for i in range(1,2000//RSP)]
    Fig, ax = plt.subplots(3, 1, sharex="all")
    ax[0].step(Cwnd[Index].iloc[:,0], Cwnd[Index].iloc[:,1], where="post")
    ax[1].scatter(Goodput[Index].iloc[:, 0], Goodput[Index].iloc[:, 1])
    ax[2].step(RTT[Index].iloc[:, 0], RTT[Index].iloc[:, 1], where="post")
    ax[0].set_title("Congestion Window")
    ax[1].set_title("Goodput")
    ax[2].set_title("Round Trip Time")
    ax[0].set_ylabel("Window Size (Bytes)")
    ax[1].set_ylabel("Goodput (Bytes/s)")
    ax[2].set_ylabel("RTT (s)")
    for i in SwitchTimes:
        ax[0].axvline(i, color="tab:red", alpha=0.5)
        ax[1].axvline(i, color="tab:red", alpha=0.5)
        ax[2].axvline(i, color="tab:red", alpha=0.5)
    GPMean = np.mean(Goodput[Index].iloc[:,1])
    #ax[1].axhline(GPMean, color="tab:green", alpha=0.5)
    #ax[1].axhline(Datarate/8, color="k")
    #ax[1].set_xticks(ax[1].get_xticks().append(GPMean))
    ax[2].set_xlabel("Time (s)")
    #ax[2].set_xticks([i*100 for i in range(0,21)])
    return Fig, ax

def PlotFullResults(Path, MaxGoodput):
    Cwnd = PickleRead(Path + "cwnd.obj")
    Goodput = PickleRead(Path + "goodput.obj")
    RTT = PickleRead(Path + "rtt.obj")
    FlowStats = PickleRead(Path + "flowStats.obj")
    Area,CwndMean = AnalyzeCwnd(Cwnd)
    Mean = AnalyzeGoodput(Goodput)
    MeanRTT = AnalyzeGoodput(RTT)
    PacketLoss = []
    for i in FlowStats:
        PacketLoss.append(i.iloc[-1,1])
    Fig, ax = plt.subplots(4, 1, sharex="all")
    x = [i * 10 for i in range(1, len(Cwnd) + 1)]
    ax[0].scatter(x, CwndMean)
    ax[1].scatter(x, Mean)
    ax[2].scatter(x, MeanRTT)
    ax[3].scatter(x, PacketLoss)
    ax[1].axhline(MaxGoodput, color="tab:green")
    ax[0].set_title(""
                    "Average Congestion Window")
    ax[1].set_title("Average Goodput")
    ax[2].set_title("Average Round Trip Time")
    ax[3].set_title("Number of Lost Packets")
    ax[0].set_ylabel("Size (Bytes)")
    ax[1].set_ylabel("Goodput (Bytes/s)")
    ax[2].set_ylabel("RTT (s)")
    ax[3].set_ylabel("# Packets Lost")
    ax[3].set_xlabel("Route Switch Period (s)")
    ax[3].set_xticks(x)
    return Fig, ax

def PlotResults(FileName, ylabel, Index = 9, Scatter=False, Path="TestResults/RoutingTests/RSPTests/", StopTime=2000):
    Cwnd1 = PickleRead(Path + "NewReno/" + FileName)
    Cwnd2 = PickleRead(Path + "WestwoodPlus/" + FileName)
    Cwnd3 = PickleRead(Path + "Cubic/" + FileName)
    RSP = (Index + 1) * 10
    SwitchTimes = [i * RSP for i in range(1, StopTime // RSP)]
    Fig, ax = plt.subplots(3, 1, sharex="all", sharey="all")
    if Scatter:
        ax[0].scatter(Cwnd1[Index].iloc[:, 0], Cwnd1[Index].iloc[:, 1])
        ax[1].scatter(Cwnd2[Index].iloc[:, 0], Cwnd2[Index].iloc[:, 1])
        ax[2].scatter(Cwnd3[Index].iloc[:, 0], Cwnd3[Index].iloc[:, 1])
    else:
        ax[0].step(Cwnd1[Index].iloc[:, 0], Cwnd1[Index].iloc[:, 1], where="post")
        ax[1].step(Cwnd2[Index].iloc[:, 0], Cwnd2[Index].iloc[:, 1], where="post")
        ax[2].step(Cwnd3[Index].iloc[:, 0], Cwnd3[Index].iloc[:, 1], where="post")
    ax[0].set_title("New Reno")
    ax[1].set_title("Westwood Plus")
    ax[2].set_title("Cubic")
    for i in ax:
        i.set_ylabel(ylabel)
    for i in SwitchTimes:
        for j in ax:
            j.axvline(i, color="tab:red", alpha=0.5)
    #Fig.suptitle("Congestion Window Size")
    return Fig, ax

def PlotResults2(FileName, ylabel, Scatter=False, Path = "TestResults/RoutingTests/ControlCases/"):
    Cwnd1 = pd.read_csv(Path + "NewReno/" + FileName)
    Cwnd2 = pd.read_csv(Path + "WestwoodPlus/" + FileName)
    Cwnd3 = pd.read_csv(Path + "Cubic/" + FileName)
    Fig, ax = plt.subplots(3, 1, sharex="all", sharey="all")
    if Scatter:
        ax[0].scatter(Cwnd1.iloc[:, 0], Cwnd1.iloc[:, 1])
        ax[1].scatter(Cwnd2.iloc[:, 0], Cwnd2.iloc[:, 1])
        ax[2].scatter(Cwnd3.iloc[:, 0], Cwnd3.iloc[:, 1])
    else:
        ax[0].step(Cwnd1.iloc[:, 0], Cwnd1.iloc[:, 1], where="post")
        ax[1].step(Cwnd2.iloc[:, 0], Cwnd2.iloc[:, 1], where="post")
        ax[2].step(Cwnd3.iloc[:, 0], Cwnd3.iloc[:, 1], where="post")
    ax[0].set_title("New Reno")
    ax[1].set_title("Westwood Plus")
    ax[2].set_title("Cubic")
    for i in ax:
        i.set_ylabel(ylabel)
    return Fig, ax

def CrazyBigFigure(Index = 9):
    Path = ["NewReno", "WestwoodPlus", "Cubic"]
    Cwnd = []
    Goodput = []
    RTT = []
    for i in range(3):
        Cwnd.append(PickleRead("TestResults/RoutingTests/RSPTests/" + Path[i] + "/cwnd.obj"))
        Goodput.append(PickleRead("TestResults/RoutingTests/RSPTests/" + Path[i] + "/goodput.obj"))
        RTT.append(PickleRead("TestResults/RoutingTests/RSPTests/" + Path[i] + "/rtt.obj"))
    Data = [Cwnd, Goodput, RTT]
    RSP = (Index + 1) * 10
    SwitchTimes = [i * RSP for i in range(1, 2000 // RSP)]
    Fig, ax = plt.subplots(3,3, sharex='all', sharey='row')
    for i in range(len(ax)):
        for j in range(len(ax[i])):
            if i==1:
                ax[i][j].scatter(Data[i][j][Index].iloc[:, 0], Data[i][j][Index].iloc[:, 1])
            else:
                ax[i][j].step(Data[i][j][Index].iloc[:,0], Data[i][j][Index].iloc[:,1], where="post")
    ax[0][1].set_title("Congestion Window")
    ax[1][1].set_title("Goodput")
    ax[2][1].set_title("Round Trip Time")
    ax[0][0].set_ylabel("Size (Bytes)")
    ax[1][0].set_ylabel("Goodput (Bytes/s)")
    ax[2][0].set_ylabel("RTT (s)")
    ax[2][1].set_xlabel("Time (s)")
    for i in ax:
        for j in i:
            for k in SwitchTimes:
                j.axvline(k, color="tab:red", alpha=0.5)

    return Fig, ax

def CongStateVSCwnd(Path, Index=9):
    Cwnd = PickleRead(Path + "cwnd.obj")
    CongState = PickleRead(Path + "congState.obj")
    RSP = (Index + 1) * 10
    SwitchTimes = [i * RSP for i in range(1, 2000 // RSP)]
    Fig, ax = plt.subplots()
    ax.scatter(CongState[Index].iloc[:,0],CongState[Index].iloc[:,1])
    y = Cwnd[Index].iloc[:,1]
    ax.step(Cwnd[Index].iloc[:,0],4*y/np.max(y), where="post")
    for i in SwitchTimes:
        ax.axvline(i, color="tab:red", alpha=0.5)
    return Fig, ax

if __name__=="__main__":
    #RoutingTests("TestResults/RoutingTests/RSPTests/NewReno/", TCP="TcpNewReno")
    #RoutingTests("TestResults/RoutingTests/RSPTests/WestwoodPlus/", TCP="TcpWestwoodPlus")
    #RoutingTests("TestResults/RoutingTests/RSPTests/Cubic/", TCP="TcpCubic")
    #RoutingTests2("TestResults/RoutingTests/DataRateTests/NewReno/", TCP="TcpNewReno")
    #RoutingTests2("TestResults/RoutingTests/DataRateTests/WestwoodPlus/", TCP="TcpWestwoodPlus")
    #RoutingTests2("TestResults/RoutingTests/DataRateTests/Cubic/", TCP="TcpCubic")
    """
    Cwnd = PickleRead("TestResults/RoutingTests/RSPTests/NewReno/cwnd.obj")
    #print(Cwnd)
    Res = AnalyzeCwnd(Cwnd)
    #print(Res)
    Goodput = PickleRead("TestResults/RoutingTests/RSPTests/NewReno/goodput.obj")
    Mean = AnalyzeGoodput(Goodput)
    Fig, ax = plt.subplots(3, 1, sharex="all")
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
    ax[2].set_xticks(x)
    plt.show()
    """
    #Fig, ax = PlotInDepth("TestResults/RoutingTests/RSPTests/NewReno/", Index=0)
    #Fig.show()
    #Fig1, ax1 = PlotResults("cwnd.obj", "Size (Bytes)")
    #Fig1, ax1 = PlotResults("goodput.obj", "Goodput (Bytes/s)", Scatter=True)
    #Fig1, ax1 = PlotResults("rtt.obj", "RTT (s)")
    #Fig1, ax1 = PlotResults("cwnd.obj", "Size (Bytes)", Index=9, Path="TestResults/RoutingTests1Mbps/RSPTests/", StopTime=200)
    #Fig1, ax1 = PlotResults("rtt.obj", "RTT (s)", Index=1, Path="TestResults/RoutingTests1Mbps/RSPTests/", StopTime=200)
    #Fig1, ax1 = CrazyBigFigure()
    #Fig1, ax1 = CongStateVSCwnd("TestResults/RoutingTests/RSPTests/Cubic/", Index=2)
    #Fig1.show()
    Fig2, ax2 = PlotFullResults("TestResults/RoutingTests/RSPTests/NewReno/", 6250)
    #Fig2, ax2 = PlotFullResults("TestResults/RoutingTests1Mbps/RSPTests/NewReno/", 125000)
    Fig2.show()
    #Fig3, ax3 = PlotResults2("cwnd.data", "Size (Bytes)")
    #Fig3, ax3 = PlotResults2("goodput.data", "Goodput (Bytes/s)", Scatter=True)
    #Fig3, ax3 = PlotResults2("rtt.data", "RTT (s)")
    #Fig3, ax3 = PlotResults2("cwnd.data", "Size (Bytes)", Path="TestResults/RoutingTests1Mbps/ControlTests/")
    #Fig3, ax3 = PlotResults2("rtt.data", "RTT (s)", Path="TestResults/RoutingTests1Mbps/ControlTests/")
    #Fig3.show()
    #print(Mean)
    #for i in range(10):
    #    RSP = (i+1)*10
    #    PlotCWND("", DF=Cwnd[i], SwitchTimes=[i*RSP for i in range(1,2000//RSP)])
    #plt.show()

