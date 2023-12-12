#!/usr/bin/env python
import sys, getopt, csv
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.cbook as cbook
from subprocess import call
import scipy.constants as cnst
import time

sim = sys.argv[1];
infile = sys.argv[2];
outfile = infile.replace(".cir",".csv");
Phizero = 2*cnst.e / cnst.hbar;
k = Phizero / (2 * cnst.pi);
avgV = list()
avgVolt = list()
current = list()
call([sim, "-o", outfile, infile])
filename = outfile
data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
t = data[:,0]
p = data[:,3]
v = data[:,2]
avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);
netlist = open(infile, "r");
netlistdata = netlist.readlines();
netlist.close();
tokens = netlistdata[1].split();
current.append(float(tokens[8]));
tokens[8] = repr(float(tokens[8]) + 25E-7)
current.append(float(tokens[8]));
netlistdata[1] = ' '.join(tokens) + "\n"
infile = infile.replace(".cir","_tmp.cir");
netlist = open(infile, "w");
netlist.writelines(netlistdata);
netlist.close();
#call(["jsim_n", "-o", outfile, infile])
call([sim, "-o", outfile, infile])
filename = outfile;
data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
t = data[:,0]
p = data[:,3]
v = data[:,2]
avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);
for i in range(1, 99):
    netlist = open(infile, "r");
    netlistdata = netlist.readlines();
    netlist.close();
    tokens = netlistdata[1].split();
    tokens[8] = repr(float(tokens[8]) + 25E-7);
    current.append(float(tokens[8]));
    netlistdata[1] = ' '.join(tokens) + "\n"
    netlist = open(infile, "w");
    netlist.writelines(netlistdata);
    netlist.close();
    call([sim, "-o", outfile, infile])
    filename = outfile;
    data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
    t = data[:,0]
    p = data[:,3]
    v = data[:,2]
    avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
    avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);
netlist = open(infile, "r");
netlistdata = netlist.readlines();
netlist.close();
tokens = netlistdata[1].split();
tokens[6] = tokens[8];
tokens[8] = repr(float(tokens[8]) - 25E-7)
current.append(float(tokens[8]));
netlistdata[1] = ' '.join(tokens) + "\n"
netlist = open(infile, "w");
netlist.writelines(netlistdata);
netlist.close();
call([sim, "-o", outfile, infile])
filename = outfile;
data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
t = data[:,0]
p = data[:,3]
v = data[:,2]
avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);
for i in range(1, 101):
    netlist = open(infile, "r");
    netlistdata = netlist.readlines();
    netlist.close();
    tokens = netlistdata[1].split();
    #tokens[6] = tokens[8];
    tokens[8] = repr(float(tokens[8]) - 25E-7)
    current.append(float(tokens[8]));
    netlistdata[1] = ' '.join(tokens) + "\n"
    netlist = open(infile, "w");
    netlist.writelines(netlistdata);
    netlist.close();
    call([sim, "-o", outfile, infile])
    filename = outfile;
    data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
    t = data[:,0]
    p = data[:,3]
    v = data[:,2]
    avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
    avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);
netlist = open(infile, "r");
netlistdata = netlist.readlines();
netlist.close();
tokens = netlistdata[1].split();
tokens[6] = repr(0);
tokens[8] = repr(float(tokens[8]) - 25E-7)
current.append(float(tokens[8]));
netlistdata[1] = ' '.join(tokens) + "\n"
netlist = open(infile, "w");
netlist.writelines(netlistdata);
netlist.close();
call([sim, "-o", outfile, infile])
filename = outfile;
data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
t = data[:,0]
p = data[:,3]
v = data[:,2]
avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);
for i in range(1, 99):
    netlist = open(infile, "r");
    netlistdata = netlist.readlines();
    netlist.close();
    tokens = netlistdata[1].split();
    #tokens[6] = tokens[8];
    tokens[8] = repr(float(tokens[8]) - 25E-7)
    current.append(float(tokens[8]));
    netlistdata[1] = ' '.join(tokens) + "\n"
    netlist = open(infile, "w");
    netlist.writelines(netlistdata);
    netlist.close();
    call([sim, "-o", outfile, infile])
    filename = outfile;
    data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
    t = data[:,0]
    p = data[:,3]
    v = data[:,2]
    avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
    avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);
netlist = open(infile, "r");
netlistdata = netlist.readlines();
netlist.close();
tokens = netlistdata[1].split();
tokens[6] = tokens[8];
tokens[8] = repr(float(tokens[8]) + 25E-7)
current.append(float(tokens[8]));
netlistdata[1] = ' '.join(tokens) + "\n"
netlist = open(infile, "w");
netlist.writelines(netlistdata);
netlist.close();
call([sim, "-o", outfile, infile])
filename = outfile;
data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
t = data[:,0]
p = data[:,3]
v = data[:,2]
avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);
for i in range(1, 100):
    netlist = open(infile, "r");
    netlistdata = netlist.readlines();
    netlist.close();
    tokens = netlistdata[1].split();
    #tokens[6] = tokens[8];
    tokens[8] = repr(float(tokens[8]) + 25E-7)
    current.append(float(tokens[8]));
    netlistdata[1] = ' '.join(tokens) + "\n"
    netlist = open(infile, "w");
    netlist.writelines(netlistdata);
    netlist.close();
    call([sim, "-o", outfile, infile])
    filename = outfile;
    data = np.genfromtxt(filename, delimiter=',', skip_header=1, dtype=None)
    t = data[:,0]
    p = data[:,3]
    v = data[:,2]
    avgVolt.append(np.mean(v[int(len(v)/2):len(v)]));
    avgV.append(((p[len(t) - 1] - p[int(len(t)/2)]) / t[len(t) - 1]/2) * k);

t = data[:,0]
p = data[:,1]
plt.figure()
plt.grid()
plt.plot(avgVolt, current)
plt.title("JoSIM IV Curve")
plt.xlabel("Voltage")
plt.ylabel("Current")
plt.show()
