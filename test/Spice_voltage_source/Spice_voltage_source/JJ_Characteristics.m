% JJ Oscilation example test in matlab

hn = 0.25E-12;
N = 4000;
rN = 16;
r0 = 16;
vG = 2.5E-3;
C = 0.07E-12;
Icrit = 0.1E-3;
IA = 0;
hbar_2e = 3.038510188E15;
for i = 1:(200E-12/hn - 1)
    IA = [IA, (600E-6 - 0)/(200E-12 - 0) * (i*hn)];
end
IA = [IA, 600E-6*ones(1, N-1600)];
for i = (400E-12/hn):(600E-12/hn - 1)
    IA = [IA, 600E-6 + (0 - 600E-6)/(600E-12 - 400E-12 - 0) * (i*hn - 400E-12)];
end

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
rBp =   [  2*C/hn + 1/rN,    -2*C/hn - 1/rN,     0];
rBn =   [ -2*C/hn - 1/rN,     2*C/hn + 1/rN,     0];
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
subplot(2, 1, 1);
plot(T, Vp);
subplot(2, 1, 2);
plot(T, IA);