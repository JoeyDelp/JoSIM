# Component Stamps

In this section we will explore each of the available component stamps found in JoSIM. This will serve as insight into how trapezoidal integration is performed to produce a time dependent voltage or phase equation for each of these components.

This will serve as a continuation of what was shown in the [Technical Discussion](tech_disc.md) section for the Inductor.

### Resistor

<figure>
  <img src="../img/resistor.pdf" alt="Resistor Symbol" class="center" width="25%">
	<figcaption align="center">Fig. 1 - A basic resistor with current flowing through it</figcaption>
</figure>

A resistor is a passive circuit element for which the voltage dependent equation is defined through Ohm's law:

$$v_{12}(t) = i_{R1}(t) R1$$

Written as:

$$\frac{v_{12}(t)}{R1} = i_{R1}(t)$$

$$ V_{12}-RI_{R} = 0 $$

This can then be written as a matrix stamp in the form

\begin{equation}
	\begin{bmatrix}
	0 & 0 & 1 \\
	0 & 0 & -1 \\
	1 & -1 & -R
	\end{bmatrix}
	\begin{bmatrix}
		V_{1} \\
		V_{2}\\
		I_{R}
\end{bmatrix}
	=
	\begin{bmatrix}
		0\\
		0\\
		0 
	\end{bmatrix}
\end{equation}

If we expand this using the voltage-phase relation shown in [Technical Discussion](tech_disc.md), then we can rewrite the equation as:

$$\frac{\Phi_{0}}{2\pi}\frac{d\phi_{12}}{dt}\frac{1}{R1} = I_{R1}$$

This equation will now require the application of the trapezoidal method:

$$\frac{\Phi_{0}}{2\pi R1}\frac{2}{h_n}(\phi_n - \phi_{n-1})-{I_{R1}}_{n-1} = {I_{R1}}_{n}$$

This reduces to:

$$\frac{\Phi_{0}}{\pi R1 h_n}\phi_n - {I_{R1}}_{n} = \frac{\Phi_{0}}{\pi R1h_n}\phi_{n-1}+{I_{R1}}_{n-1}$$

$$\phi_{n}-\frac{\pi R1h_n}{\Phi_0}{I_{R1}}_{n}=\phi_{n-1}+\frac{\pi R1h_n}{\Phi_0}{I_{R1}}_{n-1}$$

This allows us to create a phase resistor matrix stamp as:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 1 \\
		0 & 0 & -1 \\ 1 & -1 & \frac{\pi R1h_n}{\Phi_0}	\end{bmatrix}
	\begin{bmatrix}
		\phi_{1} \\
		\phi_{2} \\ I_{R1}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0\\
		0 \\  \phi_{n-1}+\frac{\pi R1h_n}{\Phi_0}{I_{R1}}_{n-1}
	\end{bmatrix}
\end{equation}

### Capacitor

<figure>
  <img src="../img/capacitor.pdf" alt="Capacitor Symbol" class="center" width="25%">
	<figcaption align="center">Fig. 2 - A basic capacitor with current flowing through it</figcaption>
</figure>

A capacitor as shown in Figure 2 has a voltage dependent equation defined by:

$$i_{C1}(t) =C1\frac{v_{12}(t)}{dt}$$

This is a first order differential and needs to be expanded using the trapezoidal rule:

$$\frac{2C1}{h_n}(V_n - V_{n-1})-{I_{C1}}_{n-1} = {I_{C1}}_{n}$$

$$V_{n} - \frac{h_n}{2C1}{I_{C1}}_{n} = V_{n-1} +  \frac{h_n}{2C1}{I_{C1}}_{n-1}$$

This allows us to create the capacitor stamp as:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 1 \\
		0 & 0 & -1 \\
		1 & -1 &  -\frac{h_n}{2C1}
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
		V_{n-1} +  \frac{h_n}{2C1}{I_{C1}}_{n-1}
	\end{bmatrix}
\end{equation}



If we wish to create a phase mode analysis stamp for the capacitor we need to replace the voltage with the voltage-phase relation.

$$i_{C1} = C1\frac{\Phi_0}{2\pi}\frac{d^2\phi}{dt}$$

This second order differential needs to be expanded using the trapezoidal rule:

$${I_{C1}}_{n} = \frac{C1\Phi_0}{2\pi}\left[ \frac{2}{h_n} \left({\frac{d\phi}{dt}}_{n} - {\frac{d\phi}{dt}}_{n-1}\right) - \left(\frac{d^2\phi}{dt}\right)_{n-1} \right]$$

$${I_{C1}}_{n} = \frac{C1\Phi_0}{\pi h_n} \left({\frac{d\phi}{dt}}_{n} - {\frac{d\phi}{dt}}_{n-1}\right) -{I_{C1}}_{n-1}$$

$${I_{C1}}_{n} = \frac{2C1\Phi_0}{\pi h_n^2} {\phi}_{n} - \frac{2C1\Phi_0}{\pi h_n^2}  {\phi}_{n-1} - \frac{2C1\Phi_0}{\pi h_n}{\frac{d\phi}{dt}}_{n-1} -{I_{C1}}_{n-1}$$

$$\frac{\pi h_n^2}{2C1\Phi_0}{I_{C1}}_{n} - {\phi}_{n} = -{\phi}_{n-1} - h_n{\frac{d\phi}{dt}}_{n-1} - \frac{\pi h_n^2}{2C1\Phi_0}{I_{C1}}_{n-1}$$

$${\phi}_{n} - \frac{\pi h_n^2}{2C1\Phi_0}{I_{C1}}_{n} = {\phi}_{n-1} + h_n{\frac{d\phi}{dt}}_{n-1} + \frac{\pi h_n^2}{2C1\Phi_0}{I_{C1}}_{n-1}$$

Which leads to the phase component stamp matrix:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 1 \\
		0 & 0 & -1 \\ 1 & -1 & - \frac{\pi h_n^2}{2C1\Phi_0}	\end{bmatrix}
	\begin{bmatrix}
		\phi_{1} \\
		\phi_{2} \\ I_{C1}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0\\
		0 \\ {\phi}_{n-1} + h_n{\frac{d\phi}{dt}}_{n-1} + \frac{\pi h_n^2}{2C1\Phi_0}{I_{C1}}_{n-1}
	\end{bmatrix}
\end{equation}

### Josephson Junction

<figure>
  <img src="../img/jj.pdf" alt="Josephson Junction Symbol" class="center" width="25%">
	<figcaption align="center">Fig. 3 - A basic Josephson junction with current flowing through it</figcaption>
</figure>

The Josephson junction shown in Figure 3 is based on the resistively and capacitivrely shunted junction (RCSJ) model. To determine the junction current, the summation of the currents through all three branches of the RCSJ is required.

If we use the basic sinusoidal of the phase to represenent the junction branch, we formulate the total current as:

$$I_{B1} = I_c \sin{\phi} + \frac{V_{12}}{R} + C\frac{dV_{12}}{dt}$$

Which when expanded with the trapezoidal rule becomes:

$${I_{B1}}_{n} = I_c\sin{\phi_n} + \frac{V_{n}}{R} + \frac{2C}{h_n}(V_{n} - V_{n-1}) - C{\frac{dV}{dt}}_{n-1} $$
$$-\frac{V_{n}}{R} - \frac{2C}{h_{n}}V_{n} + I_{B1_{n}} = I_{c}\sin{\phi_{n}} - \frac{2C}{h_n}V_{n-1}-C\frac{dV}{dt}_{n-1}$$

$$(\frac{1}{R} + \frac{2C}{h_n})V_n - {I_{B1}}_{n} = -I_c\sin{\phi_n} + \frac{2C}{h_n}V_{n-1} + C{\frac{dV}{dt}}_{n-1}$$
$$V_{n} - \frac{h_{n}R}{h_{n} + 2RC}I_{B1_{n}} = \left(\frac{h_{n}R}{h_{n} + 2RC}\right)\left(-I_{c}\sin{\phi_{n}} + \frac{2C}{h_{n}}V_{n-1} + C\frac{dV}{dt}_{n-1}\right) $$

This equation depends on the phase at the present time step, which needs to be solved. Phase is not, however, solved and we therefore need to use the voltage-phase relationship to find a voltage dependent solution for the phase:

$$V_n = \frac{\hbar}{2e}{\frac{d\phi}{dt}}_{n}$$

$$V_{n} = \frac{2}{h_n}\frac{\hbar}{2e}(\phi_n - \phi_{n-1}) - V_{n-1}$$

$$V_n - \frac{\hbar}{h_n e}\phi_{n} = - \frac{\hbar}{h_n e}\phi_{n-1} - V_{n-1}$$

We can now combine these two equations to form the component stamp matrix:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 0 & 1 \\
		0 & 0 & 0 & -1 \\
		1 & -1 & - \frac{\hbar}{h_n e} & 0 \\ 
		1 & -1 & 0 & - \frac{h_{n}R}{h_{n} + 2RC}	
	\end{bmatrix}
	\begin{bmatrix}
		V_1 \\
		V_2 \\ 
		\phi_{n}\\
		I_{B1_{n}}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0 \\
		0 \\
		-\frac{\hbar}{h_n e}\phi_{n-1} - V_{n-1} \\
		I_{s} 
	\end{bmatrix}
\end{equation}

Where $$I_{s} = \left(\frac{h_{n}R}{h_{n} + 2RC}\right)\left(-I_{c}\sin{\phi_{n}} + \frac{2C}{h_{n}}V_{n-1} + C\frac{dV}{dt}_{n-1}\right)$$

It is, however, not possible to use the phase value for the current time step in the calculation of the current time step, we therefore have to rely on an estimated phase value based on previous values.

We define this estimation as:

$$\phi_n^0=\phi_{n-1}+\frac{h_n e}{\hbar}(V_{n-1} + v_n^0)$$

$$v_n^0=V_{n-1}+h_n{\frac{dV}{dt}}_{n-1}$$

The current phase case is rather basic and we therefore replace this term with the more general term defined by Haberkorn[@haberkorn]:

$$I_s = \frac{\pi\Delta}{2eR_N}\frac{\sin{\phi}}{\sqrt{1 - \overline{D}\sin^2\left(\frac{\phi}{2}\right)}}\tanh\left[\frac{\Delta}{2k_BT}\sqrt{1-\overline{D}\sin^2\left(\frac{\phi}{2}\right)}\right]$$

This equation introduces temperature dependence within the junction model through \(\Delta\):

$$\Delta_0 = 1.76k_BT_c$$

$$\Delta = \Delta_0\sqrt{\cos\left[\frac{\pi}{2}\left(\frac{T}{T_c}\right)^2\right]}$$

with \(T\), the boiling point of liquid Helium (4.2K), \(T_c\) the critical temperature of Niobium (9.1K) and \(k_B\) is Boltzmann's constant for average kinetic energy of particles.

The resistance value \(R_N\) is defined as:

$$R_N = \frac{\pi\Delta}{2eI_c}\tanh\left(\frac{\Delta}{2k_{B}T}\right)$$

This allows us to change the characteristics of the tunnel current by simply altering the transparency value \(\overline{D}\). For values of \(\overline{D} \ll 1\) the equation becomes the normal sinusoidal equation whereas for large values of \(\overline{D}\) it becomes the non-sinusoidal ballistic tunneling equation.

To define the Josephson junction in phase we simply swap the voltage and phase of the component previously identified. The equations remain the same since the Josephson junction is already a phase element.

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 0 & 1 \\
		0 & 0 & 0 & -1 \\
		1 & -1 & - \frac{h_n e}{\hbar} & 0 \\ 
		0 & 0 & 1 & - \frac{h_{n}R}{h_{n} + 2RC}	
	\end{bmatrix}
	\begin{bmatrix}
		\phi_1 \\
		\phi_2 \\ 
		V_{n}\\
		I_{B1_{n}}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0 \\
		0 \\
		\phi_{n-1} + \frac{h_n e}{\hbar}V_{n-1} \\
		I_{s} 
	\end{bmatrix}
\end{equation}

### Voltage Source

<figure>
  <img src="../img/vs.pdf" alt="Voltage Source Symbol" class="center" width="25%">
	<figcaption align="center">Fig. 4 - A basic voltage source</figcaption>
</figure>

A voltage source is nothing more than is implied. It is a source of voltage, this indicates that the voltage at any time step is known. We can therefore easily create a component stamp matrix in the form:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 1 \\
		0 & 0 & -1 \\ 1 & -1 & 0	\end{bmatrix}
	\begin{bmatrix}
		V_{1} \\
		V_{2} \\ I_{V1}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0\\
		0 \\ V1
	\end{bmatrix}
\end{equation}

The phase version of this element simply sees the voltage replaced with the voltage-phase relation:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 1 \\
		0 & 0 & -1 \\ 1 & -1 & 0	\end{bmatrix}
	\begin{bmatrix}
		\phi_{1} \\
		\phi_{2} \\ I_{V1}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0\\
		0 \\ \frac{\Phi_0}{\pi h_n}(V_n - V_{n-1}) + \phi_{n-1}
	\end{bmatrix}
\end{equation}

### Current Source

<figure>
  <img src="../img/cs.pdf" alt="Current Source Symbol" class="center" width="25%">
	<figcaption align="center">Fig. 5 - A basic current source</figcaption>
</figure>

A current source is, as implied, a current that is known at every time step in the simulation. It can therefore simply be applied where needed on the RHS.

\begin{equation}
	\begin{bmatrix}
		0 & 0  \\
		0 & 0 	\end{bmatrix}
	\begin{bmatrix}
		V_{1} \\
		V_{2}
	\end{bmatrix}
	=
	\begin{bmatrix}
		-I1\\
		I1
	\end{bmatrix}
\end{equation}

### Phase Source

Like a voltage source simply applies a phase where needed within the circuit.

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 1 \\
		0 & 0 & -1 \\ 1 & -1 & 0	\end{bmatrix}
	\begin{bmatrix}
		\phi_{1} \\
		\phi_{2} \\ I_{\phi}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0\\
		0 \\ \phi_n
	\end{bmatrix}
\end{equation}

### Transmission Line

<figure>
  <img src="../img/xline.pdf" alt="TX Symbol" class="center" width="45%">
	<figcaption align="center">Fig. 6 - A basic transmission line model</figcaption>
</figure>

A transmission line is, at present, simply a delay element where the delay \(T_D\) and impedence \(Z_0\) define the length of the transmission line.

At present we model the transmission line as an ideal element without any losses. We will in future implement a more accurate model of the transmission line.

The equations that govern this lossless transmission line are:

$$v_{12}(t)-Z_0i_1(t) = v_{34}(t-T_D)+Z_0\centerdot i_2(t-T_D)$$

$$v_{34}(t)-Z_0i_2(t) = v_{12}(t-T_D)+Z_0\centerdot i_1(t-T_D)$$

which leads to

$${V_{12}}_{n}-Z_0{I_1}_{n}={V_{34}}_{n-k}+Z_0{I_2}_{n-k}$$

$${V_{34}}_{n}-Z_0{I_2}_{n}={V_{12}}_{n-k}+Z_0{I_1}_{n-k}$$

where

$$k=\frac{T_D}{h_n}$$

Which allows us to create a component stamp matrix

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 0 & 0 & 1 & 0 \\
		0 & 0 & 0 & 0 & -1 & 0 \\ 0 & 0 & 0 & 0 & 0 & 1 \\ 0 & 0 & 0 & 0 & 0 & -1 \\ 1 & -1 & 0 & 0 & -Z_0 & 0 \\ 0 & 0 & 1 & -1 & 0 & -Z_0 	\end{bmatrix}
	\begin{bmatrix}
		V_{12} \\
		V_{21} \\ V_{34} \\ V_{43} \\ I_{1} \\ I_{2}	\end{bmatrix}
	=
	\begin{bmatrix}
		0\\
		0 \\ 0 \\ 0 \\ {V_{34}}_{n-k}+Z_0{I_2}_{n-k} \\ {V_{12}}_{n-k}+Z_0{I_1}_{n-k}
	\end{bmatrix}
\end{equation}

The phase equations for the transmission line are:

$$\frac{\Phi_0}{2\pi}{\frac{d\phi_{12}}{dt}}_{n} - Z_0 {I_{1}}_{n} = \frac{\Phi_0}{2\pi}{\frac{d\phi_{34}}{dt}}_{n-k} + Z_0 {I_{2}}_{n-k}$$

$$\frac{\Phi_0}{2\pi}{\frac{d\phi_{34}}{dt}}_{n} - Z_0 {I_{2}}_{n} = \frac{\Phi_0}{2\pi}{\frac{d\phi_{12}}{dt}}_{n-k} + Z_0 {I_{1}}_{n-k}$$

Since the equations are reciprocal, we only work with one side to simplify the rest of the equations. We rewrite this equation in terms of the differential as follows:

$$\frac{\Phi_0}{2\pi}{\frac{d\phi_{12}}{dt}}_{n} = Z_0 {I_{1}}_{n} + \frac{\Phi_0}{2\pi}{\frac{d\phi_{34}}{dt}}_{n-k} + Z_0 {I_{2}}_{n-k}$$

We now expand this using the trapezoidal method as:

$$\frac{\Phi_0}{2\pi}\frac{2}{h_{n}}\left(\phi_{12_{n}} - \phi_{12_{n-1}}\right) - \frac{\Phi_0}{2\pi}\frac{d\phi_{12}}{dt}_{n-1} = Z_{0}I_{1_{n}}\\ + \frac{\Phi_0}{2\pi}\left(\phi_{34_{n-k}} - \phi_{34_{n-k-1}}\right) - \frac{\Phi_0}{2\pi}\frac{d\phi_{34}}{dt}_{n-k-1} + Z_{0}I_{2_{n-k}} $$

and substitute the unexpanded equation into the derivative:

$$\frac{\Phi_0}{2\pi}\frac{2}{h_{n}}\left(\phi_{12_{n}} - \phi_{12_{n-1}}\right) - Z_0 {I_{1}}_{n-1} + \frac{\Phi_0}{2\pi}{\frac{d\phi_{34}}{dt}}_{n-k-1} + Z_0 {I_{2}}_{n-k-1} = Z_{0}I_{1_{n}}\\ + \frac{\Phi_0}{2\pi}\left(\phi_{34_{n-k}} - \phi_{34_{n-k-1}}\right) - \frac{\Phi_0}{2\pi}\frac{d\phi_{34}}{dt}_{n-k-1} + Z_{0}I_{2_{n-k}} $$

Simplifying this equation results in:

$$\phi_{12_{n}} - \frac{2\pi h_{n}Z_{0}}{2\Phi_{0}}I_{1_{n}} = \frac{2\pi h_{n}Z_{0}}{2\Phi_{0}}\left(I_{1_{n-1}} + I_{2_{n-k}} - I_{2_{n-k-1}}\right) + \phi_{12_{n-1}} +\phi_{34_{n-k}} - \phi_{34_{n-k-1}} $$

With the reciprocal:

$${\phi_{34}}_{n} - \frac{2\pi h_n Z_0}{\Phi_0}{2I_{2}}_{n} = \frac{2\pi h_n Z_0}{2\Phi_0}\left(I_{2_{n-1}} + I_{1_{n-k}} - I_{1_{n-k-1}}\right) + {\phi_{34}}_{n-1} +\phi_{12_{n-k}} - \phi_{12_{n-k-1}} $$

This leads to the component stamp matrix:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 0 & 0 & 1 & 0 \\
		0 & 0 & 0 & 0 & -1 & 0 \\ 0 & 0 & 0 & 0 & 0 & 1 \\ 0 & 0 & 0 & 0 & 0 & -1 \\ 1 & -1 & 0 & 0 & - \frac{2\pi h_n Z_0}{2 \Phi_0} & 0 \\ 0 & 0 & 1 & -1 & 0 & - \frac{2\pi h_n Z_0}{2\Phi_0} 	\end{bmatrix}
	\begin{bmatrix}
		\phi_{12} \\
		\phi_{21} \\ \phi_{34} \\ \phi_{43} \\ I_{1} \\ I_{2}	\end{bmatrix}
	=
	\begin{bmatrix}
		0\\
		0 \\ 0 \\ 0 \\ V_{T1} \\ V_{T2}
	\end{bmatrix}
\end{equation}

With \(V_{T1}\) and \(V_{T2}\) the RHS of the expanded equations above.

### Mutual inductance

<figure>
  <img src="../img/mutual.pdf" alt="Mutual Symbol" class="center" width="45%">
	<figcaption align="center">Fig. 7 - Mutual inductance model</figcaption>
</figure>

Mutual inductance allows simulation of coupling between inductors. This coupling adds an additional term to the inductor equation:

$$v_{L1}(t) = L1 \frac{di_{1}(t)}{dt} + M\frac{di_2(t)}{dt}$$

$$v_{L2}(t) = L2 \frac{di_{2}(t)}{dt} + M\frac{di_1(t)}{dt}$$

with 

$$M = k\sqrt{L_1 L_2}$$

We expand these equations to:

$${V_{L1}}_{n} - \frac{2L_1}{h_n}{I_{1}}_{n} - \frac{2M}{h_n}{I_2}_{n} = - \frac{2L_1}{h_n}{I_{1}}_{n-1} - \frac{2M}{h_n}{I_2}_{n-1} - {V_{L1}}_{n-1}$$

$${V_{L2}}_{n} - \frac{2L_2}{h_n}{I_{2}}_{n} - \frac{2M}{h_n}{I_1}_{n} = - \frac{2L_2}{h_n}{I_{2}}_{n-1} - \frac{2M}{h_n}{I_1}_{n-1} - {V_{L2}}_{n-1}$$

This leads to the component stamp matrix:

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 0 & 0 & 1 & 0 \\
		0 & 0 & 0 & 0 & -1 & 0 \\		0 & 0 & 0 & 0 & 0 & 1 \\
		0 & 0 & 0 & 0 & 0 & -1 \\
		1 & -1 & 0 & 0 & -\frac{2L_1}{h_n} & -\frac{2M}{h_n} \\		0 & 0 & 1 & -1 &  -\frac{2M}{h_n} & -\frac{2L_2}{h_n} \\
	\end{bmatrix}
	\begin{bmatrix}
		V_{L1} \\
		-V_{L1} \\		V_{L2} \\		-V_{L2} \\
		I_{L1} \\ I_{L2}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0 \\
		0 \\0 \\
		0 \\ \frac{2L_1}{h_n}{I_{1}}_{n-1} - \frac{2M}{h_n}{I_2}_{n-1} - {V_{L1}}_{n-1} \\ - \frac{2L_2}{h_n}{I_{2}}_{n-1} - \frac{2M}{h_n}{I_1}_{n-1} - {V_{L2}}_{n-1}
\end{bmatrix}
\end{equation}

The phase variant of this

$$\frac{\Phi_0}{2\pi}\frac{d\phi_1}{dt} = L_1\frac{di_1}{dt} + M\frac{di_2}{dt}$$

$$\frac{\Phi_0}{2\pi}\frac{d\phi_2}{dt} =M\frac{di_1}{dt} + L_2\frac{di_2}{dt}$$

With integration on both sides

$$\frac{\Phi_0}{2\pi}\phi_1 = L_1I_1 + MI_2$$

$$\frac{\Phi_0}{2\pi}\phi_2 = MI_1 + L_2I_2$$

This leads to a quite simplified component stamp matrix

\begin{equation}
	\begin{bmatrix}
		0 & 0 & 0 & 0 & 1 & 0 \\
		0 & 0 & 0 & 0 & -1 & 0 \\		0 & 0 & 0 & 0 & 0 & 1 \\
		0 & 0 & 0 & 0 & 0 & -1 \\
		1 & -1 & 0 & 0 & -\frac{2\pi}{\Phi_0}L_1 & -\frac{2\pi}{\Phi_0}M \\		0 & 0 & 1 & -1 & -\frac{2\pi}{\Phi_0}M & -\frac{2\pi}{\Phi_0}L_2 \\
	\end{bmatrix}
	\begin{bmatrix}
		\phi_{L1} \\
		-\phi_{L1} \\		\phi_{L2} \\		-\phi_{L2} \\
		I_{L1} \\ I_{L2}
	\end{bmatrix}
	=
	\begin{bmatrix}
		0 \\
		0 \\0 \\
		0 \\0 \\ 0
\end{bmatrix}
\end{equation}

### Current Controlled Current Source

<figure>
  <img src="../img/cccs.pdf" alt="Current Controlled Current Source Symbol" class="center" width="35%">
	<figcaption align="center">Fig. 8 - Current controlled current source model</figcaption>
</figure>

Current controlled current source allows modulation of current in a particular branch through the current in a remote branch.

$$ I_{out} = \beta I_{in} $$
$$ V_{1} - V_{2} = 0 $$
$$ \beta = value $$

\begin{equation}
	\begin{bmatrix}
	0 & 0 & 0 & 0 & \frac{1}{\beta} \\
	0 & 0 & 0 & 0 & -\frac{1}{\beta} \\	
	0 & 0 & 0 & 0 & 1 \\
	0 & 0 & 0 & 0 & -1 \\
	1 & -1 & 0 & 0 & 0
	\end{bmatrix}
	\begin{bmatrix}
	V_{1}\\
	V_{2}\\
	V_{3}\\
	V_{4}\\
	I_{out}
	\end{bmatrix}
	=
	\begin{bmatrix}
	0\\
	0\\
	0\\
	0\\
	0
	\end{bmatrix}
\end{equation}

### Current Controlled Voltage Source

<figure>
  <img src="../img/ccvs.pdf" alt="Current Controlled Voltage Source Symbol" class="center" width="35%">
	<figcaption align="center">Fig. 9 - Current controlled voltage source model</figcaption>
</figure>

Current controlled voltage source allows the modulation of a voltage node through a remote current source.

$$V_{1} - V_{2} = 0$$
$$V_{3} - V_{4} = \mu I_{in}$$

\begin{equation}
\begin{bmatrix}
0 & 0 & 0 & 0 & 1 & 0\\
0 & 0 & 0 & 0 & -1 & 0\\
0 & 0 & 0 & 0 & 0 & 1\\
0 & 0 & 0 & 0 & 0 & -1\\
0 & 0 & 1 & -1 & -\mu & 0\\
1 & -1 & 0 & 0 & 0 & -\mu
\end{bmatrix}
\begin{bmatrix}
V_{1}\\
V_{2}\\
V{3}\\
V{4}\\
I_{in}\\
I_{out}\end{bmatrix}
=
\begin{bmatrix}
0\\
0\\
0\\
0\\
0\\
0
\end{bmatrix}
\end{equation}

### Voltage Controlled Current Source

<figure>
  <img src="../img/vccs.pdf" alt="Voltage Controlled Current Source Symbol" class="center" width="35%">
	<figcaption align="center">Fig. 10 - Voltage controlled current source model</figcaption>
</figure>

Voltage controlled current source allows the modulation of a current in a branch through a remote voltage source.

$$\alpha(V_{1} - V_{2}) = I_{out}$$

\begin{equation}
\begin{bmatrix}
0 & 0 & 0 & 0 & 0\\
0 & 0 & 0 & 0 & 0\\
0 & 0 & 0 & 0 & 1\\
0 & 0 & 0 & 0 & -1\\
1 & -1 & 0 & 0 & -\frac{1}{\alpha}
\end{bmatrix}
\begin{bmatrix}
V_{1}\\
V_{2}\\
V{3}\\
V{4}\\
I_{in}
\end{bmatrix}
=
\begin{bmatrix}
0\\
0\\
0\\
0\\
0
\end{bmatrix}
\end{equation}

### Voltage Controlled Voltage Source

<figure>
  <img src="../img/vcvs.pdf" alt="Voltage Controlled Voltage Source Symbol" class="center" width="35%">
	<figcaption align="center">Fig. 11 - Voltage controlled voltage source model</figcaption>
</figure>

Voltage controlled current source allows the modulation of a current in a branch through a remote voltage source.

$$A(V_{1} - V_{2}) = V_{3}-V_{4}$$

\begin{equation}
\begin{bmatrix}
0 & 0 & 0 & 0 & 0\\
0 & 0 & 0 & 0 & 0\\
0 & 0 & 0 & 0 & 1\\
0 & 0 & 0 & 0 & -1\\
G & -G & 1 & -1 & 0
\end{bmatrix}
\begin{bmatrix}
V_{1}\\
V_{2}\\
V{3}\\
V{4}\\
I_{out}
\end{bmatrix}
=
\begin{bmatrix}
0\\
0\\
0\\
0\\
0
\end{bmatrix}
\end{equation}

This concludes the section on component stamps and we move on to the [Syntax Guide](syntax.md)

<style>
.center {
    display: block;
    margin: 0 auto;
}
</style>