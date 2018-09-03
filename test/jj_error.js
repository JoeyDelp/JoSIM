* test
IS 0 1 pwl(0 0 10p 600u)
B1 1 0 jj1 area=1
.model jj1 jj(rtype=1, vg=2.8mV, cap=0.07pF, rn=16, r0=160, icrit=0.1mA)
.tran 0.001p 10p 0 0.001p
.print DEVV B1
.end