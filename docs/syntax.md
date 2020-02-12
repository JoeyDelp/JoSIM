# Syntax

In this section we will attempt to provide the user with a comprehensive guide of the available syntax within JoSIM

JoSIM is ***CaSe InSeNsItIvE*** as each line is cast to uppercase upon read-in.

Each line follows similar syntax which uses the first non-blank space character as identifier. Each identifier tells JoSIM how to handle that specific line. 

Identifiers that start with a letter relate to physical components in the design, e.g. L, C, R. Lines of this kind almost always follows the same syntax in that it requires a label and two nodes. These nodes can be alphanumeric with the restriction of **0** and **GND** which indicate a grounded node. Additionally, the use of underscores (**\_**) or vertical bars (**\|**) in label or node names are prohibited as they are reserved characters within JoSIM.

Lines that start with a period (**.**) indicate that the line relates in some way to simulation control. In this case whatever follows the period identifies the control, e.g. .tran, .print, .end.

Comments are lines that start with an asterisk (**\***) or a pound sign (**\#**). Comments are meant to be in a line of their own and will not work if placed at the end of a line.

Lines that end with a plus sign (**+**) indicate that the line that follows is a continuation of this line. Internally the two lines will be combined.

In most cases the **VALUE** of a component can be replaced by a variable name or an expression. Variables can be defined using the **.PARAM** control and expressions can be evaluated by encapsulating the expression in braces (**{}**). These will be discussed in detail further.

Values in JoSIM can be modified with engineering notation or through suffixes. A list of the available suffixes is found below:

<table>
  <th>Suffix</th><th>Meaning</th><th>Engineering Notation Equivalent</th>
  <tr><td>F</td><td>Femto</td><td>1E-15</td></tr>
  <tr><td>P</td><td>Pico</td><td>1E-12</td></tr>
  <tr><td>N</td><td>Nano</td><td>1E-9</td></tr>
  <tr><td>U</td><td>Micro</td><td>1E-6</td></tr>
  <tr><td>M</td><td>Milli</td><td>1E-3</td></tr>
  <tr><td>K</td><td>Kilo</td><td>1E3</td></tr>
  <tr><td>MEG</td><td rowspan="2">Mega</td><td rowspan="2">1E6</td></tr>
  <tr><td>X</td></tr>
  <tr><td>G</td><td>Giga</td><td>1E9</td></tr>
  <tr><td>T</td><td>Terra</td><td>1E12</td></tr>
</table>



We will now run through all the available physical components and their limitations. Any parameter surrounded by square brackets are optional and nested square brackets mean that the encapsulated parameter relies on the existance of the previous.

## Basic Components

### Resistor

**R**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**VALUE**

The value of a resistor is in Ohms.

### Inductor

**L**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**VALUE**

The value of an inductor is in Henry.

### Capacitor

**C**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**VALUE**

The value of a capacitor is in Farad.

### Josephson Junction (JJ)

**B**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*<PhaseNode\>*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**MODEL**&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[<**AREA**\>]

A Josephson junction is a two terminal device but could also be defined with a third non-connected node to allow compatibility with WRspice. This node is not used in any way in JoSIM.

The Josephson junction requires specification of a model name which can be defined anyware in the program using the control **.MODEL**.

This model control has the following syntax

**.MODEL**&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*ModelName*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*ModelType*([**MODEL PARAMETERS**])

The only junction model currently supported by JoSIM is the RCSJ model and thus the only available ModelType is **jj** with the following tunable parameters:

<table>
  <th>Parameter</th><th>Range</th><th>Default</th><th>Description</th>
  <tr><td>RTYPE</td><td>0, 1</td><td>1</td><td>Linearisation model used </td></tr>
  <tr><td>VG or VGAP</td><td>-\(\infty\), \(\infty\)</td><td>2.8E-3</td><td>Junction gap voltage</td></tr>
  <tr><td>IC or ICRIT</td><td>-\(\infty\), \(\infty\)</td><td>1E-3</td><td>Junction critical current</td></tr>
  <tr><td>RN</td><td>0, \(\infty\)</td><td>5</td><td>Junction normal resistance</td></tr>
  <tr><td>R0</td><td>0, \(\infty\)</td><td>30</td><td>Junction subgap resistance</td></tr>
  <tr><td>C or CAP</td><td>0, \(\infty\)</td><td>2.5E-12</td><td>Junction capacitance</td></tr>
  <tr><td>T</td><td>0, \(\infty\)</td><td>4.2</td><td>Boiling point of liquid coolant</td></tr>
  <tr><td>TC</td><td>0, \(\infty\)</td><td>9.1</td><td>Critical temperature of superconducting material</td></tr>
  <tr><td>DELV</td><td>0, \(\infty\)</td><td>0.1E-3</td><td>Transitional voltage from subgap to normal</td></tr>
  <tr><td>D</td><td>0.0, 1</td><td>0.0</td><td>Point contact transparency affecting current phase relationship</td></tr>
  <tr><td>ICFCT or ICFACT</td><td>0, 1</td><td>\(\pi\)/4</td><td>Ratio of critical current to quasiparticle step height</td></tr>
  <tr><td>PHI</td><td>0, n\(\pi\)</td><td>0</td><td>Starting phase for junction</td></tr>
</table>

The *.model* line is unique to the subcircuit it falls under and can thus allow different models with the same name under seperate subcircuits. If the model is not found under the subcircuit it will be searched for globally and if not found default values (default model) will be used instead.

### Transmission Line

**T**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode1*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode1*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode2*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode2*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**TD=VALUE**&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**Z0=VALUE**

**TD** is the time delay in pico seconds.

**Z0** is the impedance in Ohms.

### Mutual Inductance

**K**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Inductor1&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Inductor2&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**VALUE**

The value is the coupling factor *k*.

## Sources

### Voltage Source

**V**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**SOURCETYPE**

### Current Source

**I**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**SOURCETYPE**

### Phase Source

**P**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PosNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*NegNode*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**SOURCETYPE**

### Source Types

#### Piece Wise Linear (PWL)

**pwl(0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;0**&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*Time1*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*Amplitude1*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*...*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*TimeN*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*AmplitudeN* **)**

This source linearly interpolates amplitude values for every time point in the simulation between the specified amplitudes.

The initial two values are required to be zero at the start of the simulation.

#### Pulse

**pulse(**\(A_1\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\(A_2\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[\(T_D\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[\(T_R\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[\(T_F\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[*PW*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[*PER*]]]]] **)**

This source generates a pulse between two amplitudes (\(A_1\) & \(A_2\)), starts after \(T_D\) and has a rise and fall time (\(T_R\) & \(T_F\)), which default to the transient simulation step size.

*PW* and *PER* refer to the pulse width and the period respectivrespectively. These values default to the transient simulation stop time when not specified.

This source allows the continuous generation of a pulse at a set frequency.

#### Sinusoidal

**sin(**\(A_O\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\(A\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[*FREQ*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[\(T_D\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[*THETA*]]] **)**

A source that generates a sinusoidal signal with \(A_O\) offset and *A* amplitude at a frequency of *FREQ* which defaults to 1/(Transient simulation stop time).

\(T_D\) sets the stop time and *THETA* modulates the signal amplitude.

The function generates a data point for each step in the transient simulation based on the following equation:

\(f(t) = A_O + A\sin (2\pi FREQ(t-T_D))e^{-THETA(t-T_D)}\)

#### Custom Waveform

**cus(** *wavefile*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\(T_S\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*SF*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*IM*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[\(T_D\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PER]* **)**

This source allows the generates a function based on the points inside the plain text wave file. This file should contain a single line of space seperated numbers. E.g 0 2 3 6 2 1 0

Each number in this line represents an amplitude seperated by time step \(T_S\) and scaled using scale factor *SF*. The values between the points are interpolated using either no interpolation (0), linear (1), cubic (2) or spline (3). The function can become periodic if PER is set to 1, whereby the pattern is repeated for the entire simulation.

## Control Commands

The simulation engine requires control commands to know what to do with the components it has just been provided with.

The most important of these control commands is the transient simulation command as no simulation can be performed without it.

### Simulation

**.tran**&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\(T_{step}\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\(T_{stop}\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[\(T_{start}\)]

This generates a simulation that runs from \(T_{start}\) (0 unless specified) until \(T_{stop}\) and generates a simulation point at every \(T_{step}\) between those two points. The difference between start and stop divided by the step size floored to the closest integer value indicates how many simulation steps there will be.

JoSIM does **not** at present support any form of variable timestep input. It will in addition complain if a timestep is chosen that results in a phase step larger than 20% of 2\(\pi\).

### Output

A simulation is meaningless unless the results are stored or viewed in some way. In order to know which of these results are relevant and need to be stored the simulator needs output control commands.

These output commands can be of either 

**.print**&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**.plot**&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;**.save**

Any of these commands can be followed by either of the following commands

*PrintType*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*Device* or *Node*

*PType(Device or Node)*\(_0\)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*...*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*PType(Device or Node)*\(_n\)

Where *PrintType* can be either device voltage (*DEVV*), device current (*DEVI*), device phase (*PHASE*), node voltage (*NODEV*) or node phase (*NODEP*).

When specifying a device type store only a single device can be specified, but when a node type store is specified 2 nodes can be specified to store the difference between them.

*PType* is shorthand for the above and can have multiple per line requests. *PType* can be either of *V*, *I* (or *C*) or *P* followed by the device or node in brackets. If more than one device or node is specified by comma seperation (maximum 2) the difference between the two devices or nodes is stored.

An additional save type exists that has the following syntax

@*Device*[*PType*]

This stores the *PType* of the device specified.

### Subcircuits

Subcircuits allow subdivision and reuse of smaller circuits within a larger design. When wrapped in a subcircuit control devices are allowed to have the same label names as specified elsewhere in the netlist as the subcircuit completely isolates them.

Subcircuits have the following wrapping control syntax

**.subckt**&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*SubcktName*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*IO Nodes*

...

**.ends**

The *SubcktName* specifies the name of the subcircuit and *IO Nodes* specify the which nodes within the subcircuit connects to outside.

A subcircuit can be used in the main netlist or another subcircuit (nesting) using the following syntax

**X**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*SubcktName*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*IO Nodes*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(JSIM mode)

**X**Label&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*IO Nodes*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*SubcktName*&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(WRspice (normal SPICE) mode)

Additional features of the subcircuit include isolation of parameters (discussed next) and the ability to plot/print the devices or nodes within a subcircuit by specifying the *XLabel* followed by either a *_* or a *|* and then the device or node.

Subcircuits as mentioned before can also be nested almost indefinitely as they are expanded upon simulation.

### Parameters

The final control command that is of importance in JoSIM is the parameters command with the following syntax

**.param** &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*VarName*=*Expression*

*VarName* is the variable name that can be used anywhere else in the circuit and *Expression* is a mathematical expression that is evaluated using an implementation of Dijkstra's shunting yard algorithm, whereby the expression is converted into reverse polish notation (RPN) and evaluated.

Additionally, expressions can also contain other variables and parameters will be continuosly evaluated untill all variables are reduced to values. If variables are not defined the program will halt and produce an error.

### Control Block

Any of the above controls can be wrapped inside a control block with the following syntax

**.control**

...

**.endc**

Wherein all controls can be specified by omitting the usual prepending period to the command.

### Include

JoSIM allows the use of a *.include* control card that uses the following syntax

**.include** *relative_path_to_file*

This command reads in the contents of the relevant file pointed to by the relevant path uppon parsing of the netlist essentially extending the netlist by the linked file. This is incredibly handy when large subcircuits are involved and reuse of subcircuits accross multiple files is required.

This can also be used to house all the models used in simulation allowing a central point of alteration if the model is changed.

### Standard Input

JoSIM now allows input from standard input allowing a line-by-line read in of a netlist until the *.end* card is found or alternatively the EOF character is returned.



