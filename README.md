# JoSIM
### Superconductor Circuit Simulator

##### Testing: v2.3 - Status: [![Build Status](https://joeydelp.visualstudio.com/JoSIM/_apis/build/status/JoSIM-CI-Release?branchName=testing)](https://joeydelp.visualstudio.com/JoSIM/_build/latest?definitionId=2&branchName=testing)

##### Stable: v2.2 - Status: 

JoSIM was developed under IARPA contract SuperTools(via the U.S. Army Research Office grant W911NF-17-1-0120).

JoSIM is a SPICE syntax circuit simulator specifically created to handle superconducting elements such as the Josephson junction. It reads in a standard SPICE deck, creates an A matrix and solves the linear algebra problem Ax=b. The linear algebra package KLU is used to solve the system of equations.

JoSIM can display output using built-in FLTK GUI library or store it as space/comma delimeted files. JoSIM has inherent support for .PARAM commands that allow components to have variable values as well as expression parsing. JoSIM implements the RCSJ model of the Josephson junction and only supports transient analysis at present.

JoSIM takes a .cir/.js file as input and produces a .dat/.csv file as output.

Prebuilt binaries can be found here:
https://github.com/JoeyDelp/JoSIM/releases

## Important notes
### General
- Matplotlib requires the following packages: Numpy, Matplotlib and TKinter (Linux). These can be installed using pip install
- FLTK requires FLTK packages to be installed for the relevant system. Windows libraries are included. Mac and Linux require a package install or manual build.
- All versions require SuiteSparse to be installed. Windows libraries are provided. Mac and Linux require package installs or manual builds.
### Windows (Windows 10)
- Matplotlib versions compiled using Python 3.6 and require Anaconda3 to be installed and the following environment variables to be set.
- PYTHONHOME = Anaconda3 directory
- QT_QPA_PLATFORM_PLUGIN_PATH = %PYTHONHOME%/Library/plugins/platforms

- NOTE: In some cases it might be necessary to add the %PYTHONHOME% to the system PATH variable as well.
### Mac (macOS High Sierra 10.13.5)
- Matplotlib versions compiled using Python 3.6 and require Homebrew Python 3.6 to be installed.
- System python can also be used and should also require the next step.
- The following file needs to be created in the user home directory: *~/.matplotlib/matplotlibrc* with the following content *backend: TkAgg*
### Linux (CentOS 7)
- GCC 7 and CMake 3.10 is required to compile. GCC 7 can be obtained using SCL and CMAKE can be compiled from source once SCL 7 is enabled.
- Matplotlib version compiled using the system python 2.7, though additional packages will be required.
- TKinter: sudo yum install tkinter
- Pip & Wheel: sudo yum install python-pip python-wheel
- Python headers: sudo yum install python-devel
- Libraries: sudo pip install numpy matplotlib

## CHANGELOG
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
