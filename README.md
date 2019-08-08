# JoSIM
### Superconductor Circuit Simulator

##### Testing: v2.3 - Status: [![Build Status](https://joeydelp.visualstudio.com/JoSIM/_apis/build/status/JoSIM-CI-Release?branchName=testing)](https://joeydelp.visualstudio.com/JoSIM/_build/latest?definitionId=2&branchName=testing)

##### Stable: v2.3 - Status: [![Build Status](https://joeydelp.visualstudio.com/JoSIM/_apis/build/status/JoeyDelp.JoSIM?branchName=master)](https://joeydelp.visualstudio.com/JoSIM/_build/latest?definitionId=3&branchName=master)

---

## Overview

JoSIM was developed under IARPA contract SuperTools(via the U.S. Army Research Office grant W911NF-17-1-0120).

JoSIM is a SPICE syntax circuit simulator specifically created to handle superconducting elements such as the Josephson junction. It reads in a standard SPICE deck, creates an A matrix and solves the linear algebra problem *Ax=b*. The linear algebra package KLU by [Tim Davis](http://faculty.cse.tamu.edu/davis/suitesparse.html) is used to solve the system of equations.

A two stage distribution platform is implemented in JoSIM with the first being a command line interface (CLI) binary that is always built as standard during compilation. This is accompanied by a C++ library (*libjosim*).

JoSIM allows output of results in various formats such as comma seperated value (CSV) or raw SPICE output. JoSIM has inherent support for .PARAM commands that allow components to have variable values as well as expression parsing. JoSIM implements the RCSJ model of the Josephson junction and only supports transient analysis at present.

JoSIM takes a *.cir* file as input and produces a *.dat/.csv* file as output.

Prebuilt binaries can be found here:

https://github.com/JoeyDelp/JoSIM/releases

Referencing:

[JoSIM — Superconductor SPICE Simulator](https://ieeexplore.ieee.org/document/8633946/)

---

## Getting Started

### Binary Distributions
Each release of JoSIM CLI is distributed through pre-built binaries for Windows, Mac OS and Linux (Centos 7). These binaries are stand alone and should work as is from any directory on the host system.

These releases can be found at the [release](https://github.com/JoeyDelp/JoSIM/releases) tab of this repository.

### Source Compilation
JoSIM requires the following:
- A working C++ development environment
- CMake
- Conan
- [SuiteSparse](http://faculty.cse.tamu.edu/davis/suitesparse.html)
  
Since the instructions on how to install these for the intended host system vary quite significantly they will not be covered here.

Download or clone the repository to a local directory:

`git clone https://github.com/JoeyDelp/JoSIM.git && cd JoSIM`

Create a directory for the build:

`mkdir build && cd build`

Install requirements:

`conan install ..`

Run Cmake and make:

`cmake ..`

`make`

This will generate the josim-cli binary along with the library within the current build directory.

## Usage

### Binary

JoSIM CLI can be executed from any cli interface using the following command:

`josim-cli <input file>`

Though this will simply print the results of the simulation to standard output. To save the results of the simulation to a file append the output switch:

`josim-cli <input file> -o <output file>`

Command line switches can be appended in any order as long as an input file is provided. All the available switches can be displayed using the `-h` help switch.

|Switch|Description|
|-------|----------|
|-a           |  Specifies the analysis type.
|--analysis=  |  0 for Voltage analysis (Default).
|             |  1 for Phase analysis. 
|-c           |  Sets the subcircuit convention to left(0) or right(1).
|--convention=|  Default is left. WRSpice (normal SPICE) use right.
|             |  Eg. X01 SUBCKT 1 2 3     vs.     X01 1 2 3 SUBCKT 
|-h           |  Displays this help menu
|--help       |    
|-o           |  Specify output file for simulation results (.csv).
|--output=    |  Default will be output.csv if no file is specified. 
|-p           |  (EXPERIMENTAL) Enables parallelization of certain functions.
|--parallel   |  Requires compilation with OPENMP switch enabled.
|             |  Threshold applies, overhead on small circuits negates performance. 
|-V           |  Runs JoSIM in verbose mode.
|--verbose    |   
|-v           |  Displays the JoSIM version info only.
|--version    |   

Multiple examples are included in the *test* folder of this repository

### libjosim

The JoSIM C++ library can be used like any other library in the sense that as long as the header files and library are pointed to upon compilation the classes of JoSIM can be used in any other C++ program.

To ensure that this works the path to the library needs to be specified using the `-L` command and the library using the `-ljosim` command. The path to the header files needs to be specified using `-I` command. This allows the compiler to find the classes declared in JoSIM.

At present, the library is not very object orientated and rather coupled in the sense that some functions need to be called ahead of others. If the correct procedure is not maintained segmentation faults arise with no meaningful error message. This will change in the future as the code becomes more objectified.

The current function call order is roughly:
```
# Create the 4 objects
Input iObj;
Matrix mObj;
Simulation sObj;
Output oObj;

# Split netlist lines into sections
iObj.split_netlist(iObj.fileLines, iObj.controls, iObj.parameters, iObj.netlist);
# Parse parameters, if any
if (iObj.parameters.unparsedParams.size() > 0)
    Parser::parse_parameters(iObj.parameters);
# Expand subcircuits
iObj.expand_subcircuits();
# Expand main design using expanded subcircuits
iObj.expand_maindesign();

# Identify the simulation type
sObj.identify_simulation(iObj.controls, iObj.transSim.prstep, iObj.transSim.tstop, iObj.transSim.tstart, iObj.transSim.maxtstep);
# Create the matrix
mObj.create_matrix(iObj);
# Identify only relevant traces
mObj.find_relevant_x(iObj);
# Perform simulation
sObj.transient_voltage_simulation(iObj, mObj);
# Store results
oObj.relevant_traces(iObj, mObj, sObj);
```
---

## Documentation

For a comprehensive guide (development guideline) please refer to the [ReadMe.pdf](https://github.com/JoeyDelp/JoSIM//blob/master/doc/JoSIM%20-%20ReadMe.pdf) in the *doc* folder 

---

## Changelog
### v2.3 - 10/05/2019
- Major changes to JoSIM internal engine
- Removal of plotting engines in favor of external tools such as Python
- Introduction of pyJoSIM and libjosim packages
- Immense speed-up of internals
- Various bug fixes

### v2.2 - 12/02/2019
- Major rewrite of most of JoSIM code to accommodate library creation for use in C++ and Python
- Implementation of a phase source for use in phase-mode simulation

### v2.1 - 16/01/2019
- Inclusion of the Haberkorn junction current which allows for user switching between ballistic and normal electron tunneling
- Various bug fixes

### v2.0 - 10/10/2018
- New analysis method that allows Modified Nodal Phase Analysis (see ReadMe)
- Major memory improvement by not creating intermediate A matrix
- Various bug fixes

### v1.3 - 28/07/2018
- Addition of mutual inductance with the netlist label **K**
- Addition of the sinusoidal source (Voltage and Current)
- Various bug fixes
- AQFP now works

### v1.2 - 17/07/2018
- New Release brings RType=1 to the RCSJ model of the Josephson Junction.
- Various bugs have been fixed. 
- Data structures have been majorly altered.
- Other changes and fixes

### v1.1 - 04/06/2018
- Ability to change between plotting libraries (NONE, FLTK or Matplotlib [Default])
- Major bug fixes
- JoSIM now supports WRSpice syntax (mostly)
- Parameterization of component values with expression parsing
- Code cleanup and commenting

### v1.0 - 03/05/2018
- Initial release of JoSIM
- Default FLTK plotting engine
- Replaces JSIM as default simulator
