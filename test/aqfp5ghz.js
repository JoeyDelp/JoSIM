**** **** **** **** **** **** **** **** **** **** **** 
*JSIM AQFP MITLL Timing Test Circuit
**** **** **** **** **** **** **** **** **** **** ****

*JSIM model for MITLL
.model jjmod jj(Rtype=1, Vg=2.6mV, Cap=0.07pF, R0=144ohm, Rn=16ohm, Icrit=0.1mA)


*** netlist file ***
**** **** **** **** **** **** **** ****+
*** Lib : mit_ayala_lib
*** Cell: test_bfr_timing_tb
*** View: schematic
**** **** **** **** **** **** **** ****

*** bfr
.subckt bfr          1          2          3          4          5          6
***      dcin     dcout       din      dout       xin      xout
Kxout              Lx       Lout 0.003
Kd1                Ld         L1 -0.137
Kxq                Lx         Lq -0.001
Kdout              Ld       Lout 0.001
Kxd                Lx         Ld 0.222
Kdq                Ld         Lq -0.001
Kd2                Ld         L2 -0.137
Kx1                Lx         L1 -0.215
Kx2                Lx         L2 -0.215
Kout               Lq       Lout -0.472
Ld                 1         2   5.059pH fcheck
Lx                 5         6   5.010pH fcheck
Lin                3         7   1.219pH fcheck
L1                 7         8   1.418pH fcheck
L2                 9         7   1.418pH fcheck
Lout              10         4  28.404pH fcheck
Lq                 7         0   8.226pH fcheck
R1                10         0   1.000pohm
B2                 9         0  jjmod area=0.50
B1                 8         0  jjmod area=0.50
.ends

*** top cell: test_bfr_timing_tb
XI11              bfr         11          0         13          0         14          0
*** ("12" "12" "12") mapped to 0
XI10              bfr         15         11         16         13          0         17
*** ("12") mapped to 0
XI9               bfr          7         15         18         16         14         19
XI8               bfr         20          7         21         18         22         17
XI7               bfr         23         20         24         21         25         19
XI6               bfr         26         23         27         24         22         28
XI5               bfr         29         26         30         27         25         31
XI4               bfr         32         29         33         30         34         28
XI3               bfr         35         32         36         37         38          0
*** ("39") mapped to 0
XI2               bfr         40         35         41         36          0         42
*** ("39") mapped to 0
XI1               bfr         43         40         44         41         38         45
XI0               bfr         46         43         47         44         48         42
Lx1delay           8        34   0.100pH fcheck
Lx2delay          49        31   0.100pH fcheck
Lx1               50        48   0.100pH fcheck
Lx2               51        45   0.100pH fcheck
Lxd1              52        46   0.100pH fcheck
LIC               37        33   5.700pH fcheck
Vx2delay 53 0 SIN(0 926.0mV 5.0GHz 200.0ps 0)
Vx1delay 54 0 SIN(0 926.0mV 5.0GHz 250.0ps 0)
Vin 55 0 PWL(0ps 0mV 1.0ps -5.0mV 200.0ps -5.0mV 201.0ps 5.0mV 400.0ps 5.0mV 401.0ps -5.0mV 600.0ps -5.0mV 601.0ps 5.0mV 800.0ps 5.0mV 801.0ps -5.0mV 1000.0ps -5.0mV 1001.0ps 5.0mV 1200.0ps 5.0mV 1201.0ps 5.0mV 1400.0ps 5.0mV 1401.0ps -5.0mV 1600.0ps -5.0mV 1601.0ps -5.0mV 1800.0ps -5.0mV )
Vxd 56 0 PWL(0ps 0mV 20.0ps 1238.0mV)
Vx2 57 0 SIN(0 926.0mV 5.0GHz 200.0ps 0)
Vx1 58 0 SIN(0 926.0mV 5.0GHz 250.0ps 0)
Rx2delay          53        49  1000.00ohm
Rx1delay          54         8  1000.00ohm
Rin               55        47  1000.00ohm
Rxd1              56        52  1000.00ohm
Rx2               57        51  1000.00ohm
Rx1               58        50  1000.00ohm

*** netlist file ***

*** jsim input file ***
.tran 0.2ps 2000.0ps 0ps 0.1ps
.print devi Lx1
.print devi Lx2
.print devi Lxd1

.print devi XI0_Lx
.print devv XI0_Lx
.print devi XI0_Lin
.print devi XI0_Lq
.print phase XI0_B1
.print phase XI0_B2

.print devi XI1_Lx
.print devv XI1_Lx
.print devi XI1_Lin
.print devi XI1_Lq
.print phase XI1_B1
.print phase XI1_B2

.print devi XI2_Lx
.print devv XI2_Lx
.print devi XI2_Lin
.print devi XI2_Lq
.print phase XI2_B1
.print phase XI2_B2

.print devi XI3_Lx
.print devv XI3_Lx
.print devi XI3_Lin
.print devi XI3_Lq
.print phase XI3_B1
.print phase XI3_B2

.print devi LIC
.print devv LIC

.print devi XI4_Lx
.print devv XI4_Lx
.print devi XI4_Lin
.print devi XI4_Lq
.print phase XI4_B1
.print phase XI4_B2

.print devi XI5_Lx
.print devv XI5_Lx
.print devi XI5_Lin
.print devi XI5_Lq
.print phase XI5_B1
.print phase XI5_B2

.print devi XI6_Lx
.print devv XI6_Lx
.print devi XI6_Lin
.print devi XI6_Lq
.print phase XI6_B1
.print phase XI6_B2

.print devi XI7_Lx
.print devv XI7_Lx
.print devi XI7_Lin
.print devi XI7_Lq
.print phase XI7_B1
.print phase XI7_B2

.print devi XI8_Lx
.print devv XI8_Lx
.print devi XI8_Lin
.print devi XI8_Lq
.print phase XI8_B1
.print phase XI8_B2

.print devi XI9_Lx
.print devv XI9_Lx
.print devi XI9_Lin
.print devi XI9_Lq
.print phase XI9_B1
.print phase XI9_B2

.print devi XI10_Lx
.print devv XI10_Lx
.print devi XI10_Lin
.print devi XI10_Lq
.print phase XI10_B1
.print phase XI10_B2

.print devi XI11_Lx
.print devv XI11_Lx
.print devi XI11_Lin
.print devi XI11_Lq
.print phase XI11_B1
.print phase XI11_B2
*** jsim input file ***
