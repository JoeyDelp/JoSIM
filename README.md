# JoSIM
### Superconductor Circuit Simulator

---

## Full Documentation

Please see full documentation here:
[JoSIM Documentation](https://joeydelp.github.io/JoSIM)

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

## Changelog
### v2.6.9
- Fixed a bug resulting in bad optional access when shorting transmission line components.

### v2.6.8
- Add a fix to warn of subcircuit redefenitions. Redefintions now overwrite the previous definition.
- Add a fix when parameters are used in subcircuits but defined within the global scope.

### v2.6.7
- Added ability to parameterize time points specified for `.tran` and `pwl` commands.

### v2.6.6
- Fixed a bug in noise source generation when noise step is less than or equal to the simulation step.
- Reduced aggressiveness of timestep scale down when close to transmission line step delay size. Now only scales down if TX delay is less than 1 step (previously 4)
- Added output dimensions option to josim-plot script when writing directly to PDF, SVG, EPS, etc.

### v2.6.5
- Updated the CPR specification in the JJ model to allow multiple simulataneous harmonics.
- Added a Notepad++ UDL syntax highlighting script to the scripts folder.

### v2.6.4
- Added the ability to specify CPR in the JJ model to enable non-sinusoidal CPRs needed for pi-junctions.

### v2.6.3
- Fixed a bug where Windows and CentOS parameters do not coincide. Issue was due to expression substitution.

### v2.6.2
- Fixed sp_generator.py having invalid tab spacing
- Fixed Area= and Ic= JJ parameters not producing the same result for the same critical current

### v2.6.1
- Fixed an issue with custom waveform where it wasn't reading from the file properly
- Added spline method for cubic interpolation of custom waveform
- Moved entire codebase to 64bit integers to allow larger/longer simulations
- Fixed parameters
- Fixed transmission line integer step delay rounding issue that caused an error build up

### v2.6
- Full Johnson-Nyquist temperature noise for Resistors and Resistive branch of RCSJ model
- Phi-junction model with verified Pi junction results
- Various bug fixes and code clean up

### v2.5.8
- Updated KLU to a 64-bit version capable of handling non-zero count larger than the maximum value of a 32-bit integer.
- Included SuperLU 64-bit that can be activated using the *-x 1* switch (default 0 for KLU).
- Bug fixes to pulse command.

### v2.5.7
- Fixed an issue with comparison of double values for small numbers between various compilers.

### v2.5.6
- Fixed a initial values when converting between phase and voltage when simulation output does not start at 0.

### v2.5.5
- Fixed a bug in output conversion when plotting voltage in a phase simulation and vice versa.

### v2.5.4
- Fixed comparison function for GCC interpretation of double values

### v2.5.3
- Fixed an issue with the pulse command producing incorrect pulses
- Fixed an issue where timesteps smaller than 10fs were causing out-of-range issues
- Various other bug fixes

### v2.5.2
- Fixed a major issue with relative paths not being correctly evaluated causing files to not be found even though the correct path was specified.

### v2.5.1 - 18/05/2021
- Added .spread command. See documentation for more details.
- Added .file command. See documentation for more detauls.
- Added .iv command. See documentation for more details.
- Fixed a bug where .neb value was not being read properly
- Fixed a bug where pulse source type was outputting incorrect number of pulses.
- Fixed a bug where custom waveform was not allowing cubic interpolation method.
- Code cleanup.

### v2.5 - 05/03/2021
- Phase mode is now the standard analysis mode. Produces the exact same results as voltage.
- Trapezoidal method has been replaced by superior 2nd order Gear method.
- JoSIM now internally determines the convention. (-c option removed)
- New progress indicators added. Spawns a seperate non blocking thread.
- New verbose modes. 1: Circuit statistics. 2: 1 + Parameters and evaluated values. 3: 2 + Complete expanded netlist.
- New DC source type. I.e. V01 1 0 DC 5V
- JJ has new IC= parameters that supersedes AREA= but ultimately performs similar model parameter scaling.
- Fixed NOISE source type to have syntax NOISE(AMP TD TSTEP)
- New '.temp' and '.neb' commands for noise thermal noise analysis
- Transient simulation command now allows PRSTART and PRSTEP commands. .tran TSTEP TSTOP PRSTART PSTEP
- Various memory improvements and overall speed-up
- Reduced output file size.
- Rewrote documentation: [JoSIM Documentation](https://joeydelp.github.io/JoSIM)
- Various bug fixes

### v2.4.1 - 09/03/2020
- Fixed an issue where "GND" was not recognized as ground, creating a new node causing matrix singularity.
- Fixed an issue with multiport devices (TX, CCCS, CCVS, VCCS, VCVS) not adding the 3rd and 4th node to the node map.
- Fixed an issue with multiport devices not expanding properly if part of subcircuits.
- Various other bug fixes

### v2.4 - 07/02/2020
- Complete rewrite of core internals
- Modularization of each component allowing easier implementation of integration methods
- Error reporting major improvment. Less random exits with no warnings. Better exits.
- Focus on object orientation for improved library access
- Speed up of matrix creation algorithm
- Improvements to output format
- Inclusion of '.include' control card
- Capability to read from standard in using '-i' command
- Various bug fixes

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
