* JSIM deck file generated with TimEx
* === DEVICE-UNDER-TEST ===
*=========================================
*  JTL2c  JeSEF Technology 
*  27.05.2004 Thomas Ortlepp
*  01.07.2004 resistor shift
*  25.05.2005 bias level 0.7
*  19.12.2011 InductEx extraction by Coenrad Fourie
* (c) RSFQ design group TU Ilmenau
*=========================================
*$Ports      in_in  out_out
.SUBCKT JTL  1      2
L1  1      4        2.080pH
L2  4      8        2.059pH
L3  8      5        2.059pH
L4  5      2        2.080pH
B1   4      6       jjtl1 area=1.0
RB1  4      9       1.0  
Lp1  9      6       1.0pH
L6   6      0       0.214pH
B2   5      7       jjtl2 area=1.0
RB2  5     10       1.0  
Lp2 10      7       1.0pH
L7   7      0       0.214pH
* Bias source 2.5mV and 7.41 Ohm
ib1  0     8        pwl(0 0 5p 350uA 100n 350uA)   
.MODEL jjtl1 JJ(RTYPE=0, ICRIT=250uA, CAP=1.262PF, RN=90)
.MODEL jjtl2 JJ(RTYPE=0, ICRIT=250uA, CAP=1.262PF, RN=90)
.ENDS
*========================================
* === SOURCE DEFINITION ===
.SUBCKT SOURCECELL  30 2
l10  30    3        0.1ph
l1   3     0        3.904ph
l2a  3     4        0.604ph
l2b  11    5        1.126ph
l3   5     7        4.484ph
l4   7     2        2.080ph
b1   11    4        jdcsfq1 area=1.0
rb1  4     51       1.111
lp1  51    11        1.037ph
b2   5     8        jdcsfq2 area=1.0
rb2  5     81       1.111
lp2  81    8        1.037ph
l6   8     0        0.198ph
b3   7      9       jdcsfq3 area=1.0
rb3  7     91       1.000
lp3  91     9       1.0ph
l7   9      0       0.110ph
ib1  0     11       pwl(0 0 5p 275.000ua 100n 275.000ua)
ib2  0     7       pwl(0 0 5p 175.000ua 100n 175.000ua)
.model jdcsfq1 jj(rtype=0, icrit=225.000ua, cap=1.132pf, rn=90)
.model jdcsfq2 jj(rtype=0, icrit=225.000ua, cap=1.132pf, rn=90)
.model jdcsfq3 jj(rtype=0, icrit=250.000ua, cap=1.262pf, rn=90)
.ENDS SOURCECELL
* === INPUT LOAD DEFINITION ===
.SUBCKT LOADINCELL  1 2
l1  1      4        2.080ph
l2  4      8        2.059ph
l3  8      5        2.059ph
l4  5      2        2.080ph
b1   4      6       jjtl1 area=1.0
rb1  4      9       1.0
lp1  9      6       1.0ph
l6   6      0       0.214ph
b2   5      7       jjtl2 area=1.0
rb2  5     10       1.0
lp2 10      7       1.0ph
l7   7      0       0.214ph
ib1  0     8        pwl(0 0 5p 350ua 100n 350ua)
.model jjtl1 jj(rtype=0, icrit=250ua, cap=1.262pf, rn=90)
.model jjtl2 jj(rtype=0, icrit=250ua, cap=1.262pf, rn=90)
.ENDS LOADINCELL
* === OUTPUT LOAD DEFINITION ===
.SUBCKT LOADOUTCELL  1 2
l1  1      4        2.080ph
l2  4      8        2.059ph
l3  8      5        2.059ph
l4  5      2        2.080ph
b1   4      6       jjtl1 area=1.0
rb1  4      9       1.0
lp1  9      6       1.0ph
l6   6      0       0.214ph
b2   5      7       jjtl2 area=1.0
rb2  5     10       1.0
lp2 10      7       1.0ph
l7   7      0       0.214ph
ib1  0     8        pwl(0 0 5p 350ua 100n 350ua)
.model jjtl1 jj(rtype=0, icrit=250ua, cap=1.262pf, rn=90)
.model jjtl2 jj(rtype=0, icrit=250ua, cap=1.262pf, rn=90)
.ENDS LOADOUTCELL
* === SINK DEFINITION ===
.SUBCKT SINKCELL  1
r1 1 0 2
.ENDS SINKCELL
* ===== MAIN =====
Iin 0 1 pwl(0 0 5p 0 2E-11 0 3E-11 0.0006 4E-11 0 8E-11 0 9E-11 0.0006 1E-10 0)
XSOURCEINin SOURCECELL 1 2
XLOADINin LOADINCELL 2 3
XLOADOUTout LOADOUTCELL 4 5
XSINKOUTout SINKCELL 5
XDUT jtl 3 4
.tran 1E-13 2E-10 0 1E-13
.PRINT NODEV 1 0
.PRINT NODEV 2 0
.PRINT NODEV 3 0
.PRINT NODEV 4 0
.PRINT DEVI XDUT_L6
.PRINT DEVI XDUT_B1
.PRINT DEVI XDUT_L2
.PRINT DEVI XDUT_L3
.PRINT DEVI XDUT_B2
.PRINT DEVI XDUT_L7
.PRINT DEVI Iin
.PRINT DEVI XLoadinin_L1
.PRINT PHASE XSourceinin_b3
.PRINT PHASE XLOADINin_b1
.PRINT PHASE XLOADINin_b2
.end
