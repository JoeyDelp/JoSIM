# JoSIM - Superconducting Circuit Simulator

Developers Manual v2.3

## Project Status

### Testing: v2.3 - Status: [![Build Status](https://joeydelp.visualstudio.com/JoSIM/_apis/build/status/JoSIM-CI-Devel?branchName=testing)](https://joeydelp.visualstudio.com/JoSIM/_build/latest?definitionId=1&branchName=testing)

### Stable: v2.3 - Status: [![Build Status](https://joeydelp.visualstudio.com/JoSIM/_apis/build/status/JoeyDelp.JoSIM?branchName=master)](https://joeydelp.visualstudio.com/JoSIM/_build/latest?definitionId=3&branchName=master)

## Introduction

JoSIM was developed under IARPA contract SuperTools(via the U.S. Army Research Office grant W911NF-17-1-0120). JoSIM is a analogue circuit simulator with SPICE syntax input that has inherent support for the superconducting Josephson junction element.

JoSIM is meant to function as a replacement to the aging simulators such as JSIM[@jsim] and WRspice[@wrspice]. JoSIM is written in modern C++ and is fully customizable and extendable to offer support for improved superconducting elements as well better approximations to the Josephson effect in superconducting materials.

A *.cir* file containing a SPICE syntax circuit netlist is provided as input. The circuit netlist, given appropriate input excitations can then be simulated through transient analysis. Results of this simulation can be dumped to standard output or saved in various formats such as a comma separated value (*.csv*) file.

Fig. 1 shows an overview of how JoSIM operates internally. This is much like any other SPICE deck simulator with the exception that it incorporates native handling of the Josephson junction. We will discuss each of these blocks in further detail in the [Technical Discussion](tech_disc.md) section.

<figure>
	<img src="img/josim_macro.pdf" alt="JoSIM macro overview" class="center">
	<figcaption align="center"> Fig. 1 - JoSIM Macro Overview.</figcaption>
</figure>

## Project layout

The JoSIM repository has the following layout. A quick description shows the purpose of the various files and folders.

    cmake   			# CMake scripts.
    docs/
        index.md		# The documentation homepage.
        ...      		# Other markdown pages, images and other files.
    include/
        JoSIM			# JoSIM header files.
        suitesparse		# SuiteSparse header files.
    lib					# Libraries for different platforms.
    site				# Where this documentation spawns from.
    src/
        josim_vs		# Visual Studio solution for JoSIM.
        ...				# JoSIM source files.
    test				# Folder containing various examples.
    README.md			# Basic readme to get the user going.
    LICENSE				# License that governs use of JoSIM.
    CMakeLists.txt		# Configuration to compile code.
    ...					# Other configuration files and scripts.

## Initial setup
Release versions of JoSIM can be found on the [release page](https://www.github.com/JoeyDelp/JoSIM/releases) of the open source github repository repository. At the time of writing this version is 2.3.

The source can also be cloned and compiled for either macOS, Linux or Windows. Within this repository there will be a **CMakeLists.txt** which is a recipe used to compile JoSIM using CMake.

To compile the source a working C++ compiler with support for C++14 is required. Additionally SuiteSparse linear algebra libraries are required but are provided in the repository. Git version control software is recommended, but is not required to compile JoSIM. A single executable binary is generated using the CMake recipe and can be placed anywhere on the system as well as freely redistributed.

### License
JoSIM is governed by the MIT license, which is a very permissive license that allows anyone to redistribute the source as well as commercialize it without repercussions. The MIT license allows use of this software within proprietary software as long as all copies of the licensed software includes a copy of the MIT license as well as the copyright notice.

### Building from source
#### Linux

These instructions were executed on a vanilla install of CentOS 7 to reduce oversight in the compilation instructions created by previous package installs. For other distributions please use the package manager relevant to the distribution of choice.

A working internet connection is required, as well as the ability to install packages.

JoSIM source can be directly downloaded from the repository as a compressed *.tar.gz* file or by cloning the repository. In either case, navigate to a directory where compilation will take place and extract the tarball or execute:

	$ sudo yum install git
	$ git clone https://github.com/JoeyDelp/JoSIM.git

CMake 3 will be required to compile the source and would require the activation of the *epel-release* repository.
	
	$ sudo yum install epel-release
	$ sudo yum install cmake3

Finally, JoSIM requires only a single external library called SuiteSparse [@suitesparse]. To install it, execute the following command:

	$ sudo yum install suitesparse suitesparse-devel

Navigate to the newly cloned/extracted JoSIM directory then run the following commands:

	$ mkdir build
	$ cd build
	$ cmake3 ..
	$ make

This will generate a JoSIM executable in the **build** directory.

#### Apple macOS

Apple macOS is very similar to most Unix systems and therefore follows mostly the same procedure. The user would clone the repository and install CMake as well as the necessary library as indicated in previous sections. These requirements can be installed using either Homebrew, Macports or compiled from source using the standard macOS compilers (installed through Xcode).

#### Windows

A Microsoft Visual Studio solution is provided and can be found in the **src** folder. This is by far the easiest way to compile the software under a Microsoft Windows environment. Simply open the solution and click build (F6) to build the Release target for the software.

It is also posisble to compile JoSIM on Windows using CMake. To do this, [CMake for Windows](https://cmake.org/download/) would need to be installed along with the MSVC compiler, which is included in the [Build Tools for Visual Studio 2019](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=BuildTools&rel=16).

Once installed, the graphical interface for CMake can be used to choose the source directory, build directory and compiler to use (MSVC in this case). This will build the Windows version of JoSIM using the CMakeLists.txt configuration script.

<style>
.center {
    display: block;
    margin: 0 auto;
}
</style>