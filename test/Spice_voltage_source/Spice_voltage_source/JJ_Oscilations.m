% JJ Oscilation example test in matlab
PHI0 = 2.06783383113E-15;
hbar_2e = 2*pi/PHI0;
hn = 0.0025E-12;
N = 4000;
% JJ Model
jj_area = 1;
jj_c = 0.07E-12;
jj_rn = 16;
jj_r0 = 16;
jj_icrit = 0.1E-3;
C = jj_c * jj_area;
Rn = jj_rn / jj_area;
R0 = jj_r0 / jj_area;
Vg = 2.8E-3;
IA = 0;
for i = 1:(30E-15/hn - 1)
    IA = [IA, (200E-6 - 0)/(30E-15 - 0) * (i*hn)];
end
IA = [IA, 200E-6*ones(1, N-12)];

Vpn_1 = 0;
Vnn_1 = 0;
Vpn_1d = 0;
Vpn_2 = 0;
Vnn_2 = 0;
Vpn_2d = 0;

phin_1 = 0;
phin_2 = 0;

T = zeros(1, N);

%           V+                  V-              Phi
rBp =   [  2*C/hn + 1/Rn,    -2*C/hn - 1/Rn,     0];
rBn =   [ -2*C/hn - 1/Rn,     2*C/hn + 1/Rn,     0];
rPhi=   [-hn/2 * hbar_2e,    hn/2 * hbar_2e,     1];

A = [rBp; rBn; rPhi];
A = A(2:3, 2:3);
RHS = zeros(1, 2);
LHS = zeros(1, 2);
Vp = zeros(1, N);
Vn = zeros(1, N);
Phi = zeros(1, N);

for i = 1:N
    V_P = LHS(1);
    %V_N = LHS(2);
    Phi_n = LHS(2);
    Is = -Icrit * sin(phin_1 + hn/2 * hbar_2e * (Vpn_1 + Vpn_1 + hn*(Vpn_1d))) + ((2*C/hn) * Vpn_1) + (C * Vpn_1d);
    RHS_IAp = IA(i) + Is;
    %RHS_IAn = -IA(i);
    RHS_Phin_1 = phin_1 + (hn/2 * hbar_2e)*(Vpn_1);
    RHS = [RHS_IAp; RHS_Phin_1];
    LHS = A\RHS;
    Vp(i) = LHS(1);
    %Vn(i) = LHS(2);
    Phi(i) = LHS(2);
    T(i) = hn*i;
    phin_1 = RHS_Phin_1;
    Vpn_2 = Vpn_1;
    Vpn_1 = LHS(1);
    Vpn_2d = Vpn_1d;
    Vpn_1d = (2/hn)*(Vpn_1 - Vpn_2) - Vpn_2d;
    %Vnn_1 = LHS(2);
end

figure;
plot(T, Vp);
ylabel("Voltage(mV)");
xlabel("Time(ps)");