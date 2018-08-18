* jj_characteristics.js
IS         0          1          pwl(0      0 10p 0 50p 250u )
B1         1          0          jj1        area=1
R1         1          0          1k        
.model jj1 jj(rtype=0, vg=2.8mV, cap=0.07pF, r0=160, rn=16, icrit=0.1mA)
.tran 0.05p 1n 0 0.25p
.print PHASE B1
.print DEVI IS
.print NODEV 1 0
.end
