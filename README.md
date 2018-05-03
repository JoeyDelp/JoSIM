# JoSIM
Superconductor Circuit Simulator

JoSIM was developed under IARPA contract SuperTools(via the U.S. Army Research Office grant W911NF-17-1-0120).

JoSIM is a SPICE syntax circuit simulator specifically created to handle superconducting elements such as the Josephson junction. It reads in a standard SPICE deck, creates an A matrix and solves the linear algebra problem Ax=b. The linear algebra package KLU is used to solve the system of equations.

JoSIM can display output using built-in FLTK GUI library or store it as space/comma delimeted files. JoSIM has inherent support for .PARAM commands that allow components to have variable values as well as expression parsing. JoSIM implements the RCSJ model of the Josephson junction and only supports transient analysis at present.

JoSIM takes a .cir/.js file as input and produces a .dat/.csv file as output.

Prebuilt binaries can be found here:
https://github.com/JoeyDelp/JoSIM/releases
