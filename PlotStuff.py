import pandas as pd
import matplotlib.pyplot as plt
#import numpy as np
import subprocess

if __name__=="__main__":
    SubReturn = subprocess.run(["ns3", "run", "scratch/RoutingTest1.cc --datarate=1000Kbps --bottleneckRate=100Kbps --stopTime=2, switchTime=1"])
    #print(SubReturn)
    DF = pd.read_csv("RoutingTest1-cwnd.data")
    print(DF)
    Fig, ax = plt.subplots()
    ax.scatter(DF["Time"], DF["Window Size"])
    #Fig.show()
    plt.show()
    