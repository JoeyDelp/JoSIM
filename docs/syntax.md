# Syntax Guide

In this section we will attempt to provide the user with a comprehensive guide of the available syntax within JoSIM

JoSIM is ***CaSe InSeNsItIvE*** as each line is cast to uppercase upon read-in.

Additionally, it is **discouraged** to add units to values since in some specific cases this creates confusion in interpretation of the values. An example of this is specifying Farad when assigning a capacitor its value since a value followed by F would be interpreted as Femto. This will lead to wanting a 5 Farad capacitor but receiving a 5 *Femto* Farad capacitor. Units are not interpreted by the simulator in any way since the component type implies the unit. 

## Basic Syntax

Each line follows similar syntax which uses the first non-blank space character as identifier. Each identifier tells JoSIM how to handle that specific line. 

Identifiers that start with a letter relate to physical components in the design, e.g. L, C, R. Lines of this kind almost always follows the same syntax in that it requires a label and two nodes. These nodes can be alphanumeric with the restriction of **0** and **GND** which indicate a grounded node. Additionally, the use of period (**.**) or vertical bars (**|**) in label or node names are prohibited as these are reserved characters within JoSIM.

Lines that start with a period (**.**) indicate that the line relates in some way to simulation control. In this case the command that follows the period identifies the control, e.g. .tran, .print, .end.

Comments are lines that start with an asterisk (**\***) or a hash (**\#**). Comments are meant to be in a line of their own and will not work if placed at the end of a line.

Lines that start with a plus sign (**+**) indicate that the line is a continuation of the previous line. Internally the two lines will be combined.

In most cases the **VALUE** of a component can be replaced by a variable name or an expression. Variables can be defined using the **.PARAM** control. These will be discussed in detail further.

Values in JoSIM can be modified with engineering notation or through suffixes. A list of the available suffixes is found below:

<center>

| Suffix | Meaning | Engineering Notation Equivalent |
| ------ | ------- | ------------------------------- |
| F      | Femto   | 1E-15                           |
| P      | Pico    | 1E-12                           |
| N      | Nano    | 1E-9                            |
| U      | Micro   | 1E-6                            |
| M      | Milli   | 1E-3                            |
| K      | Kilo    | 1E3                             |
| MEG    | Mega    | 1E6                             |
| X      | Mega    | 1E6                             |
| G      | Giga    | 1E9                             |
| T      | Tera    | 1E12                            |

</center>

## Basic Components

We will now run through all the available physical components and their limitations. Any parameter surrounded by square brackets are optional and nested square brackets mean that the encapsulated parameter relies on the existence of the previous.

### Resistor

**R**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;**VALUE**&emsp;[temp=<**TEMP**>]&emsp;[neb=<**FREQ**>]

The value of a resistor is in Ohms.

Temperature used for noise analysis in  Kelvin. Units need to be excluded since **K** for Kelvin will be interpreted as Kilo by JoSIM, e.g. 4K will become 4000.

Optional frequency parameter sets the noise effective bandwidth during noise analysis. Default is 1GHz.

### Inductor

**L**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;**VALUE**

The value of an inductor is in Henry.

### Capacitor

**C**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;**VALUE**

The value of a capacitor is in Farad.

### Josephson Junction (JJ)

**B**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;*<PhaseNode\>*&emsp;**MODEL**&emsp;[area=<**AREA**\>]&emsp;[ic=<**IC**>]&emsp;[temp=<**TEMP**>]&emsp;[neb=<**FREQ**>]

A Josephson junction is a two terminal device but could also be defined with a third non-connected node to allow compatibility with WRspice. This node is not used in any way in JoSIM.

The Josephson junction requires specification of a model name which can be defined anywhere in the program using the control **.MODEL**.

When **AREA** or **IC** is not specified then an area=1 is used as default.

The temp and neb commands have the same descriptions as for the resistor.

#### Model

This model control has the following syntax

**.MODEL**&emsp;*ModelName*&emsp;*ModelType*([**MODEL PARAMETERS**])

The only junction model currently supported by JoSIM is the RCSJ model and thus the only available ModelType is **jj** with the following tunable parameters:

| Parameter       | Range                 | Default         | Description                                                  |
| --------------- | --------------------- | --------------- | ------------------------------------------------------------ |
| RTYPE           | 0, 1                  | 1               | Linearization model used                                     |
| VG or VGAP      | -$\infty$, $\infty$   | 2.8E-3          | Junction gap voltage                                         |
| IC or ICRIT     | -$\infty$, $\infty$   | 1E-3            | Junction critical current                                    |
| RN              | 0, $\infty$           | 5               | Junction normal resistance                                   |
| R0              | 0, $\infty$           | 30              | Junction subgap resistance                                   |
| C or CAP        | 0, $\infty$           | 2.5E-12         | Junction capacitance                                         |
| T               | 0, $\infty$           | 4.2             | Junction temperature in K                                    |
| TC              | 0, $\infty$           | 9.1             | Critical temperature of superconducting material             |
| DELV            | 0, $\infty$           | 0.1E-3          | Transitional voltage from subgap to normal                   |
| D               | 0.0, 1                | 0.0             | Point of contact transparency affecting current phase relationship |
| ICFCT or ICFACT | 0, 1                  | $\frac{\pi}{4}$ | Ratio of critical current to quasiparticle step height       |
| PHI             | 0, $2\pi$             | 0               | Allows phi-junction capability such as the $\pi$-junction.   |
| CPR             | {$-\infty$, $\infty$} | {1}             | Sets the Current Phase Relationship harmonic amplitudes.     |

The *.model* line is unique to the subcircuit it falls under and can thus allow different models with the same name under separate subcircuits. If the model is not found under the subcircuit it will be searched for globally and if not found default values (default model) will be used instead.

The **AREA** and **IC** parameters act as modifiers to the model parameters. **AREA** is a critical current multiplier, where if **IC** is specified it replaces the **AREA** value by $AREA=\frac{IC_{jj}}{IC_{model}}$. 

By setting the **PHI** parameter of the model, the phase value is persistantly subtracted from the phase ($\phi$) in the $\sin(\phi)$ part of the JJ current. This allows elements such as the $\pi$-junction to be modeled. 

The **CPR** parameter alters each individual harmonic of the junction supercurrent. It can either be a singular value or an array of *n* values encapsulated with curly braces. i.e *CPR={0.9, 0.1}* to set the current phase relationship to the following $I = I_C(0.9\sin(\phi) + 0.1\sin(2\phi))$. 

### Transmission Line

**T**Label&emsp;$N^{+}_{1}$&emsp;$N^{-}_{1}$&emsp;$N^{+}_{2}$&emsp;$N^{-}_{2}$&emsp;**TD=VALUE**&emsp;**Z0=VALUE**

**TD** is the time delay in seconds.

**Z0** is the impedance in Ohms.

### Mutual Inductance

**K**Label&emsp;$L_{1}$&emsp;$L_{2}$&emsp;**VALUE**

The value is the coupling factor *k*.

## Sources

### Independent Sources

#### Voltage Source

**V**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;**SOURCETYPE**

#### Current Source

**I**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;**SOURCETYPE**

#### Phase Source

**P**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;**SOURCETYPE**

### Dependent Sources

#### Current controlled current source

**F**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;$N^{+}$Control&emsp;$N^{-}$Control&emsp;**CURRENT GAIN**

#### Current controlled voltage source

**H**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;$N^{+}$Control&emsp;$N^{-}$Control&emsp;**TRANSRESISTANCE IN OHMS**

#### Voltage controlled current source

**G**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;$N^{+}$Control&emsp;$N^{-}$Control&emsp;**TRANSCONDUCTANCE IN MHOS**

#### Voltage controlled voltage source

**E**Label&emsp;$N^{+}$&emsp;$N^{-}$&emsp;$N^{+}$Control&emsp;$N^{-}$Control&emsp;**VOLTAGE GAIN**

### Source Types

#### Piece Wise Linear (PWL)

**pwl(0**&emsp;**0**&emsp;$T_{1}$&emsp;$A_{1}$&emsp;...&emsp;$T_{n}$&emsp;$A_{n}$**)**

This source linearly interpolates amplitude values for every time point in the simulation between the specified amplitudes.

The initial two values are required to be zero at the start of the simulation.

#### Pulse

**pulse(**$A_{1}$&emsp;$A_{2}$&emsp;[$T_{D}$&emsp;[$T_{R}$&emsp;[$T_{F}$&emsp;[*PW*&emsp;[*PER*]]]]] **)**

This source generates a pulse between two amplitudes ($A_{1}$ and $A_{2}$), starts after $T_{D}$ and has a rise and fall time ($T_{R}$ & $T_{F}$), which default to the transient simulation step size.

*PW* and *PER* refer to the pulse width and the period respectively. These values default to the transient simulation stop time when not specified.

This source allows the continuous generation of a pulse at a set frequency.

#### Sinusoidal

**sin(**$A_{O}$&emsp;$A$&emsp;[$f$&emsp;[$T_{D}$&emsp;[$\theta$]]] **)**

A source that generates a sinusoidal signal with $A_{O}$ offset and $A$ amplitude at a frequency of $f$ which defaults to $\frac{1}{T_{STOP}}$.

$T_{D}$ sets the stop time and $\theta$ modulates the signal amplitude.

The function generates a data point for each step in the transient simulation based on the following equation:

$f(t)=A_{O}+A\sin(2\pi f(t-T_{D}))e^{-\theta(t-T_{D})}$

#### Custom Waveform

**cus(** *wavefile*&emsp;$T_{S}$&emsp;*SF*&emsp;*IM*&emsp;[$T_{D}$&emsp;*PER]* **)**

This source allows the generates a function based on the points inside the plain text wave file. This file should contain a single line of space separated numbers. E.g 0 2 3 6 2 1 0

Each number in this line represents an amplitude separated by time step $T_{S}$ and scaled using scale factor *SF*. The values between the points are interpolated using either no interpolation (0), linear (1) or cubic (2). The function can become periodic if PER is set to 1, whereby the pattern is repeated for the entire simulation.

The waveform only starts of $T_{D}$.

#### DC

*dc*&emsp;$A$

A DC source that is always at $A$ at any given time during the simulation.

#### Noise

**noise(**$A$&emsp;$T_{D}$&emsp;$T_{STEP}$**)**

This source produces a noise value for the time step provided that it is after $T_{D}$. The $T_{STEP}$ is the inverse of the noise effective bandwidth set globally using `.neb` or locally for a resistor using `neb=`.

The noise value returned is calculated using the following equation:

$f(t) = A\frac{GRAND()}{\sqrt{2T_{STEP}}}$

$GRAND()$ is a Gaussian random number generating function.

#### Exponential

**exp(** $A_{1}$&emsp;$A_{2}$&emsp;$T_{D1}$&emsp;$\tau_{1}$&emsp;$T_{D2}$&emsp;$\tau_{2}$**)**

<center>

| Parameter  | Default           |
| ---------- | ----------------- |
| $T_{D1}$   | 0.0               |
| $T_{D2}$   | $T_{STEP}$        |
| $\tau_{1}$ | $T_{D1}+T_{STEP}$ |
| $\tau_{2}$ | $T_{STEP}$        |

</center>

Returns different values for the 3 different time segments.

For $t < T_{D1}$:

$f(t)=A_{1}$ 

For $T_{D1}\le t < T_{D2}$:

$f(t) = A_{1}+(A_{2}-A_{1})(1-e^{\frac{t - T_{D1}}{\tau_{1}}})$

For $T_{D2} \le t$:

$f(t) = A_{1}+(A_{2}-A_{1})(1-e^{\frac{t - T_{D1}}{\tau_{1}}})+(A_{1}-A_{2})(1-e^{\frac{t - T_{D2}}{\tau_{2}}})$

## Control Commands

The simulation engine requires control commands to know what to do with the components it has just been provided with.

The most important of these control commands is the transient simulation command as no simulation can be performed without it.

### Transient Analysis

**.tran**&emsp;$T_{STEP}$&emsp;$T_{STOP}$&emsp;[$P_{START}$&emsp;[$P_{STEP}$]]&emsp;DST

This generates a simulation that runs from 0 until $T_{STOP}$. The amount simulation steps that will be performed is $n=\frac{T_{STOP}}{T_{STEP}}$.

$P_{START}$ indicates at what point output will start printing. $P_{STEP}$ sets the size of the print steps. This has to be larger or equal to $T_{STEP}$.

DST disables the start-up time. The start-up time is a period calculated internally by the simulator in which components settle. This is equivalent to the few picoseconds from when a circuit initially receives power (power switch flipped).

### Subcircuits

Subcircuits allow subdivision and reuse of smaller circuits within a larger design. When wrapped in a subcircuit control devices are allowed to have the same label names as specified elsewhere in the netlist as the subcircuit completely isolates them.

Subcircuits have the following wrapping control syntax

**.subckt**&emsp;*SubcktName*&emsp;*IO Nodes*

...

**.ends**

The *SubcktName* specifies the name of the subcircuit and *IO Nodes* specify the which nodes within the subcircuit connects to outside.

A subcircuit can be used in the main netlist or another subcircuit (nesting) using the following syntax

**X**Label&emsp;*SubcktName*&emsp;*IO Nodes*&emsp;(JSIM mode)

**X**Label&emsp;*IO Nodes*&emsp;*SubcktName*&emsp;(WRspice (normal SPICE) mode)

Additionally, keywords in the form of **LABEL=VALUE** can be appended to the end of the subcircuit declaration line which when instantiated will replace the value of the **LABEL** component within the subcircuit with the associated **VALUE**. This allows for unique subcircuit instantiations which would prove useful in testing various parameters without altering the original subcircuit or having multiple instances of the same subcircuit definition. This could open the door for potential future margin and optimization software.

### Noise

As mentioned in the technical discussion, noise can be automatically inserted as current sources in parallel to each resistor. This thermal noise temperature and bandwidth can be specified globally using the following commands:

**.temp**&emsp;*Temperature in Kelvin*

**.neb**&emsp;*Bandwidth in Hertz*

### Spread

JoSIM allows each value of inductors, resistors, capacitors and JJ (area/Ic) to be spread uniformly within a specified percentage range from the nominal value. Each time the value is used (in matrix creation), a new random value from the uniform distribution is chosen.

This allows for process variation to be simulated.

To set the spread globally, the following control needs to be set:

**.spread**&emsp;*Normalized percentage*&emsp;*[L=Inductor spread]*&emsp;*[B=JJ spread]*&emsp;*[C=Capacitor spread]*&emsp;*[R=Resistor spread]*

Each individual component can also be spread by adding the **spread=** named parameter to the component declaration line.

The order of precedence is taken as local, specific global and then global. This means that if a global spread exists but  a global inductor specific spread also exists and the inductor being added has a local spread then the local spread will take precedence.

An example:

If an inductor has a value of *2pH*, this is its nominal value. If a local spread of *0.2* is specified this means a random value can be chosen anywhere between *1.6pH* and *2.4pH*. If global inductor specific spread is specified as *0.3* and the local spread is not defined, the value can be anything within the range *1.4pH -  2.6pH*. Similarly if the global spread is defined as *0.5* then the value each time the simulation is run can be anywhere between *1pH* and *3pH*.

### IV Curve

JoSIM allows the user to output an IV curve for a specified JJ model within the netlist using the following command:

**.iv**&emsp;*modelname*&emsp;*max_current*&emsp;*filepath*

This command outputs a comma seperated value (CSV) at the *filepath* specified which contains the IV curve data for the *modelname* from negative *max_current* to positive *max_current*.

Subcircuit models can be output using the `.`(period) or `|`(vertical bar) as separator between the *modelname* and the subcircuit NAME.

### Output

A simulation is meaningless unless the results are post processed. In order to know which of these results are relevant for storage the simulator needs output control commands.

These output commands can be of either 

**.print**&emsp;**.plot**&emsp;**.save**

Any of these commands can be followed by either of the following commands

*PrintType*&emsp;&nbsp;*Device* or *Node*

*PType(Device or Node)*\(_0\)&emsp;&nbsp;*...*&emsp;*PType(Device or Node)*\(_n\)

Where *PrintType* can be either device voltage (*DEVV*), device current (*DEVI*), device phase (*PHASE*), node voltage (*NODEV*) or node phase (*NODEP*).

When specifying a device type store only a single device can be specified, but when a node type store is specified 2 nodes can be specified to store the difference between them.

*PType* is shorthand for the above and can have multiple per line requests. *PType* can be either of *V*, *I* (or *C*) or *P* followed by the device or node in brackets. If more than one device or node is specified by comma seperation (maximum 2) the difference between the two devices or nodes is stored.

An additional save type exists that has the following syntax

@*Device*[*PType*]

This stores the *PType* of the device specified.

Subcircuit parameters can be output using the `.`(period) or `|`(vertical bar) as separator between the device label and the subcircuit label name. 

i.e. **.print** v(14.X01) p(B01.X02)

This method follows the path from inside to out when nested subcircuits are used. 

Subcircuits as mentioned before can also be nested almost indefinitely as they are expanded upon simulation.

If the label is not immediately apparent and required for output, the simulation can be run using the `-V 3` cli option to show the expanded main design, allowing the exact label name to be identified. 

### File

Multi-file output can be achieved by separating output commands with the following command:

**.file**&emsp;*filepath*

This indicates that every output request that follows this command needs to be stored in the file specified by *filepath*. 

I.e. 

```
.file output1.csv
.print p(B1) p(B2) p(B3)
.file output2.dat
.plot i(L01) i(L02) i(L03)
.file output3
.save v(1) v(2) v(3)
```

This will create 3 output files. The first will be a CSV file containing only the phase outputs. The second file will be a space separated file containing only the currents. Lastly, the third file will be in a SPICE RAW format and contain only the voltages.

The use of this command does not affect the command line option request to output a file. The command line output option, if requested, will output an additional file which contains all of the output requests (phase, current and voltage).

### Parameters

The final control command that is of importance in JoSIM is the parameters command with the following syntax

**.param** &emsp;*VarName*=*Expression*

*VarName* is the variable name that can be used anywhere else in the circuit and *Expression* is a mathematical expression that is evaluated using an implementation of Dijkstra's shunting yard algorithm, whereby the expression is converted into reverse polish notation (RPN) and evaluated.

Additionally, expressions can also contain other variables and parameters will be continuously evaluated until all variables are reduced to values. If variables are not defined the program will halt and produce an error.

Expression parsing is exclusive to the *.param* control. This means that if expressions are loosely provided as values to components or as parameters to plot or model controls, JoSIM will error in *std::invalid_argument: stod: no conversion* as it tries to convert a string into a double. Please be mindful when using expressions and restrict them to *.param* controls.

### Control Block

Any of the above controls can be wrapped inside a control block with the following syntax

**.control**

...

**.endc**

Wherein all controls can be specified by omitting the usual prepending `.`(period) to the command.

This block, though seemingly useless at present, will be used for more advanced functionality in the future.

### Include

JoSIM allows the use of a *.include* control card that uses the following syntax

**.include** *relative_path_to_file*

This command reads in the contents of the relevant file pointed to by the relevant path upon parsing of the netlist essentially extending the netlist by the linked file. This is incredibly handy when large subcircuits are involved and reuse of subcircuits across multiple files is required.

This can also be used to house all the models used in simulation allowing a central point of alteration if the model is changed.

### Standard Input

JoSIM now allows input from standard input allowing a line-by-line read in of a netlist until the *.end* card is found or alternatively the EOF character is returned.



## Constants

JoSIM has a set of built in constants that when used expand to the corresponding values. Below is a list of these constants and their values. The constants names are case insensitive:

| Constant  | Symbol                                       | Value                   |
| --------- | -------------------------------------------- | ----------------------- |
| PI        | $\pi$                                        | 3.141592653589793238463 |
| PHI_ZERO  | $\Phi_{0}$                                   | 2.067833831170082E-15   |
| BOLTZMANN | $k_{B}$                                      | 1.38064852E-23          |
| EV        | $e$                                          | 1.6021766208E-19        |
| HBAR      | $\bar{h}$                                    | 1.0545718001391127E-34  |
| C         | $c$                                          | 299792458               |
| MU0       | $\mu_{0}$                                    | 12.566370614E-7         |
| EPS0      | $\epsilon_{0}$                               | 8.854187817E-12         |
| SIGMA     | $\sigma$ (short for $\frac{\Phi_{0}}{2\pi}$) | 3.291059757E-16         |

[^1]: I. Salameh, E. G. Friedman and S. Kvatinsky, "Superconductive Logic Using 2ϕ—Josephson Junctions With Half Flux Quantum Pulses," in *IEEE Transactions on Circuits and Systems II: Express Briefs*, vol. 69, no. 5, pp. 2533-2537, May 2022, doi: 10.1109/TCSII.2022.3162723.
[^2]:S. V. Bakurskiy et al., "Current-phase relations in SIsFS junctions in the vicinity of 0-$\pi$ Transition", *Phys. Rev. B Condens. Matter*, vol. 95, pp. 94522-94528, Mar. 2017.
