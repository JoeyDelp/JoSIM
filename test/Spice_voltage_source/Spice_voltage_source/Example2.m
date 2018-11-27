R1 = 2;
vg = 2.8E-3;
cap = 0.07E-12;
r0 = 160;
rN = 16;
icrit = 0.1E-3;
hn = 2.5E-13;
two_c_hn = (cap + cap) / hn;
area = 1;
gn = area/r0;
xn1 = 0;
yn1 = 0;
xn1d = 0;
yn1d = 0;
phi_guess = 0;
source_val = 0;
source_val = icrit*sin(phi_guess)+source_val;
Is = two_c_hn*(xn1-yn1)+cap*(xn1d-yn1d)-source_val;
IA = [zeros(1, 200E-12/hn - 1)];
N = 4000;
for i = 1:N-1
   t(i) = hn * i; 
end
for i = (200E-12/hn):(400E-12/hn - 1)
    IA = [IA, (600E-6 - 0)/((400E-12)-(200E-12))*(i*hn - 200E-12)];
end
for i = (400E-12/hn):(600E-12/hn - 1)
    IA = [IA, 600E-6];
end
for i = (600E-12/hn):(800E-12/hn - 1)
    IA = [IA, 600E-6 + ((-600E-6 / 200E-12)*(i*hn - 600E-12))];
end
for i = (800E-12/hn):(4000-1)
   IA = [IA, i*0]; 
end

