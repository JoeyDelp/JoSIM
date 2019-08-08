# Technical Discussion

## Modified nodal analysis

There are many ways to setup a set of linear equations to solve the voltage or currents in a circuit. One of the more well known ways is to use nodal analysis which creates an equation for each node defined in the circuit netlist. This method is the basis on which the original Berkeley SPICE[@spice] was built. This method, however, only calculates the voltages of every node which makes it difficult to handle components that are voltage dependent such as inductors and junctions.

This drawback lead to the creation of the modified nodal analysis which is an extension to the prior with the ability to calculating branch currents in addition to nodal voltages[@mna]. We therefore make use of the MNA method to build the set of linear equations within JoSIM due to the large use of inductors as well as Josephson junctions in superconductivity.

A useful feature of the MNA method allows every component to be represented as a sub-matrix called a stamp. The summation of all the stamps provide us with the **A**, **x** as well as **b** matrices that are required to solve the linear equations. These stamps will be discussed further in the following subsection.

## Trapezoidal integration

<figure>
  <fig>
  </fig>
  <figcaption 
</figure>

We therefore rather focus on higher order methods such as the trapezoidal rule for solving the linear equations as the accuracy of this method increases by reducing the time step size. 

We can express the trapezoidal method as:

$$\left( \frac{dx}{dt}\right) _{n}=\frac{2}{h_{n}}\left( x_{n} - x_{n-1} \right) - \left(\frac{dx}{dt} \right) _{n-1} $$

In this case \(n\) is the current time step and \(n-1​\) refers to the previous time step. By using this method to solve differential equations we are able to create generic stamps for each component that JoSIM can handle.

To demonstrate this method and how a stamp is formed we will show an example of an inductor.
<figure>
	<img src="../img/inductor.pdf" alt="Inductor Symbol" class="center" width="25%">
	<figcaption align="center">Fig. 1 - A basic inductor with current flowing through it</figcaption>
</figure>

The inductor in Fig. 1 has a general equation to determine the voltage across it as:

$$ v_{L1}(t) = L1\frac{di}{dt} $$

When we apply the trapezoidal rule we find that this equation can be rewritten as:

\begin{align}
	v_{n} &= L1 \left[ \frac{2}{h_n} \left( I_{n} - I_{n-1} \right) - \left( \frac{di}{dt}\right)_{n-1} \right] \\
	& = \frac{2L1}{h_n} \left( I_{n} - I_{n-1} \right) - L1 \left( \frac{di}{dt}\right)_{n-1} \\
	& = \frac{2L1}{h_n} \left( I_{n} - I_{n-1} \right) - v_{n-1} \\
	\therefore I_{n} & = \frac{h_{n}}{2L1} \left( V_{n} + V_{n-1}\right) + I_{n-1} \\
	\therefore \frac{h_{n}}{2L1}V_{n} - I_{n} & = -\frac{h_{n}}{2L1}V_{n-1} + I_{n-1}
\end{align}

Where the above is the current step voltage and current as a function of the previous step values. We further expand this equation by stating that the voltage is the potential across the two nodes: \(v = v_{1} - v_{2} \).

\begin{equation}
	\frac{h_{n}}{2L1}(V_{1})_{n} - \frac{h_{n}}{2L1}(V_{2})_{n} - I_{n} = -\left( \frac{h_{n}}{2L1}V(V_{1})_{n-1} - \frac{h_{n}}{2L1}(V_{2})_{n-1}\right)  - I_{n-1}
\end{equation}

Which we can then write in matrix form as:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 1 \\
		0 & 0 & -1 \\
		1 & -1 & \frac{-2L1}{h_n}
	\end{bmatrix}
	\begin{bmatrix}
		V_{1} \\
		V_{2} \\
		I_{L1}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0 \\
		0 \\
		-\frac{2L1}{h_{n}}(I_{L1})_{n-1} - \left( (V_{1})_{n-1} - (V_{2})_{n-1}\right)
	\end{bmatrix}
\end{equation}

The matrix above is a generic stamp that we can place into the A matrix which describes the inductor L1.

We can do the same for a resistor, capacitor, current source, voltage source, Josephson junction and a transmission line. The stamps for each of these components can be found in the [Component Stamps](comp_stamps.md) section.

## Modified nodal phase analysis

First introduced in version 2.0 of JoSIM, the ability to perform a simulation that calculates the nodal phase instead of voltage is presented. This new analysis method is named the modified nodal phase analysis (MNPA) and utilizes the voltage-phase [@orlando] relationship seen below.

\begin{equation}
	v = \frac{\Phi_{0}}{2\pi}\frac{d\phi}{dt}
\end{equation}

If this relationship is applied to all the component models found in JoSIM we obtain the MNPA stamps, which allow us to solve the phase directly. As example we demonstrate this on the inductor equation shown in the previous section.

\begin{align}
	\frac{\Phi_{0}}{2\pi}\frac{d\phi}{dt} &= L1\frac{di}{dt} \\ 
	\frac{\Phi_{0}}{2\pi}\phi_{n} &= L1 I_{n} \\
	\frac{\Phi_{0}}{2\pi L1}\phi_{n}^{+} -  \frac{\Phi_{0}}{2\pi L1}\phi_{n}^{-}&= I_{n}
\end{align}

The above is functionally equivalent to the resistor in voltage analysis. The computation required to solve the phase of an inductor is therefore far less complex than that of solving the voltage. With superconducting circuits being largely inductive, the use of phase reduces the overall complexity of the solution which in turn provides faster simulations and reduced memory usage.

JoSIM has been adapted to allow phase analysis on any design that works with voltage analysis without requiring alterations to the netlist file. Since the voltage is simply the scaled time derivative of the phase the voltage can be calculated as a post process if the user requests it.

Phase mode simulation can be enabled using the command line switch **-a** followed by a **1**. If not provided the default for this command is **0**, which indicates a voltage mode simulation will be performed.

All the MNPA stamps are included in the [Component Stamps](comp_stamps.md) section.

## LU decomposition

When the A matrix has been set up as detailed in the previous section all that is left to do is to solve the \(Ax=b​\) problem using some form of iterative method. We choose KLU from the SuiteSparse[@suitesparse] library to accomplish this task.

This requires the A matrix to be in compressed row storage (CRS)[@crs] format which is a data structure of 3 vectors. The first of these vectors contains all the non-zero elements in the A matrix. The second contains an initial zero followed by the total number of non-zero elements at the end of each row such that the final entry in the vector is the total number of non-zero elements. The third vector contains the column index of each non-zero element. As an example, the following sparse \(5x5\) matrix:

\begin{equation}
	\begin{bmatrix}
		1 & 0 & 0 & 4 & 0\\
		0 & 3 & 2 & 0 & 0 \\
		0 & 0 & 1 & 0  & 0\\
		0& 4 & 0 & 5 & 0 \\
		0 & 0 & 0 & 0 & 1
	\end{bmatrix}
\end{equation}

would yield a CSR format data structure of 

\begin{align}
	nnz & = \left[ 1, 4, 3, 2, 1, 4, 5, 1 \right]  \\
	rowptr & = \left[ 0, 2, 4, 5, 7, 8 \right] \\
	colind & = \left[ 0, 3, 1, 2, 2, 1, 3, 4 \right]
\end{align}

which has a total of 21 elements compared to the 25 required for the original A matrix. This difference, ofcourse, becomes much greater as the A matrix grows in size as well as the sparser the matrix becomes, which is almost always the case for electrical type simulations.

Once in this format we can proceed with the KLU factorization. Due to the MNA forcing only the RHS to change upon every time step, we only have to compute the LU decomposition for the A matrix once at the start of the time loop. With this single calculation of the LU decomposition done, the only calculation required is the solving of \(x\) (LHS) upon each iteration using the ever changing \(b\) (RHS).

## Data structures & speed considerations

JoSIM relies heavily on the use of the C++ map data structure which creates a hash table for quick lookup. This immensely simplifies code legibility as well as component identification within later stages of the process. The drawback of this, however, is the hashing and comparing becomes quite slow when using string types. 

Initially a standard map was used, however, this very negatively impacted the execution speed of the program and thus alternatives were sought. These ordered map (standard maps) were replaced by the much faster unordered map since the map no longer needed to be sorted by keys upon each new entry. Ideally the use of unordered maps are not perfect if speed was the only consideration. They do, however, provide a good balance between implementation, speed and debugging.

To avoid some of the speed problems, a string map is only used initially to create components, mapping component labels to integer indices. During the simulation step only the indices are used. The results are then mapped back to the string equivalents to find the relevant output vectors requested by the user.

Additional steps have been implemented to reduce the required memory for the result vectors. This is achieved by idenitifying which vectors are required to produce the relevant output prior to simulation. During simulation only these required vectors are then populated with values as opposed to storing all calculated values. This drastically reduces the required memory.

Through the implementation of the MNPA method in JoSIM, additional improvements were introduced such as the objectification of every component at the matrix creation level that increases efficiency and debugging. This change allows for simplification of output functions since a direct link to the rows of the result matrix for each component reduces the indexing time required to identify the correct row. 

## Input Files

Input files for JoSIM are called netlists and follow mostly a standard SPICE syntax as described in [@spice]. There are, however, many functions and components of this syntax that have not been implemented since they are only of relevance to semiconductor circuits. A full syntax guideline is avalable in the [Syntax](syntax.md) section.

There are a few caviates to making JoSIM compatible with other netlist generating systems. Most of these can be ironed out through routines within the code itself to make the simulator as accepting as possible. One of these caviates, however, requires user input and this is to identify the convention used when simulating using subcircuits.

When a netlist with subcircuits is generated to be JSIM[@jsim] compatible, a subcircuit line would for example be:

X01&nbsp;&nbsp;&nbsp;&nbsp;SUBNAME&nbsp;&nbsp;&nbsp;&nbsp;    1&nbsp;&nbsp;&nbsp;&nbsp;   2

This indicates that a subcircuit of name SUBNAME is to be used with I/O nodes 1 and 2. This we call convention **0** and is the default for JoSIM.

If a circuit is generated for compatibility with WRspice[@wrspice] then this convention would be **1** and an example of a subcircuit line would be:

X01&nbsp;&nbsp;&nbsp;&nbsp;1&nbsp;&nbsp;&nbsp;&nbsp;2&nbsp;&nbsp;&nbsp;&nbsp;SUBNAME

This is the general SPICE syntax convention and in all honesty makes more sense since a component definition line should always carry the notation:

LABEL&nbsp;&nbsp;&nbsp;&nbsp;POSNODE&nbsp;&nbsp;&nbsp;&nbsp;NEGNODE

It is required that the user specify the convention when working with a non JSIM netlist. The convention can be set using the **-c** command line switch followed by the convention required.

## Output Files

As stated before, JoSIM is a simulator that solves \(Ax=b\) problems. As a result the output that JoSIM provides to the user is always that of the \(x\) vector. At present JoSIM only does a transient analysis, which means that each entry of the \(x\) vector contains another vector of result values for each time step in the transient analysis.

When the user does not request any output, and does not specify a file within which to save the results, all of the results for the entire \(x\) vector are dumped to the screen in a space seperated way. This can be resource intensive and does not mean much to the user. It is therefore advisable to always specify the vectors of interest using the output commands specfied in [Syntax](syntax.md) as well as a resulting file to store the output in. 

The output file can be either a *comma seperated variable* (*.csv*) file or a *space seperated* (*.dat*) file. The the type and name of the output file are specified using the **-o** command switch followed by the file name. The file extension determines the type. 

There is a third option of output format which follows the standard SPICE RAW data syntax. This format is compatible with various output trace viewers and can be specified by simply ommiting the file extension of in the output name.

### Plotting interfaces

In previous versions of JoSIM there existed 2 plotting windows, namely FLTK and Matplotlib. These interfaces were, however, ultimately scrapped due to maintainability issues as well as cross-platform compatibility. The user is requested to use the plotting system they are most comfortable with. 

Below we provide a simple Python 3 script that plots all the results in a *.csv* file. It is very basic and the user should modify it as required. This script requires installation of the **Numpy** as well as **Matplotlib** packages for Python 3. Optionally, we comment the lines for use of the **Qt5** backend for plotting. The user can enable this if the **PyQt5** package is installed.

```python
#!/usr/bin/env python
import math
# import matplotlib
# matplotlib.use('Qt5Agg')
import matplotlib.pyplot as pl
from matplotlib import gridspec
import numpy as np
import csv
import sys

time = []
data = []
labels = set()
with open(sys.argv[1], 'r') as csvFile:
    reader = csv.DictReader(csvFile)
    labels = reader.fieldnames
    data.append([])
    for row in reader:
        time.append(float(row[labels[0]]))
        for var in range(1,len(labels)):
            data.append([])
            data[var].append(float(row[labels[var]]))
csvFile.close()

N = len(labels) - 1
cols = int(math.ceil(N / 4))
rows = int(math.ceil(N / cols))

gs = gridspec.GridSpec(rows, cols)
fig = pl.figure()
for var in range(1,len(labels)):
    ax = fig.add_subplot(gs[var - 1])
    ax.plot(time, data[var])
    ax.set_xlabel(labels[0])
    ax.set_ylabel(labels[var])

fig.set_tight_layout(True)
fig.show()
input()
```



<style>
.center {
    display: block;
    margin: 0 auto;
}
</style>