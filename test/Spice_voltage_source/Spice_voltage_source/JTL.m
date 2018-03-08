% JTL test in matlab

hbar_2e = 3.038510188E15;
hn = 0.25E-12;
N = 4000;
% JJ Model
Rn = 16;
R0 = 160;
Vg = 2.5E-3;
C = 0.07E-12;
Icrit = 0.1E-3;
% Components
   L1_mat = zeros(10, 10);
   L2_mat = zeros(10, 10);
   L3_mat = zeros(10, 10);
   L4_mat = zeros(10, 10);
   L5_mat = zeros(10, 10);
   B1_mat = zeros(10, 10);
   B2_mat = zeros(10, 10);
L1 = 2E-12;
L2 = 2.4E-12;
L3 = 2.4E-12;
L4 = 2E-12;
L5 = 0.278E-12;
R1 = 2;
L1_mat(1,6) = -1;
L1_mat(6,1) = -1;
L1_mat(6,6) = -2 * L1 / hn;
L2_mat(1,7) = 1;
L2_mat(2,7) = -1;
L2_mat(7,7) =  -2 * L2 / hn;
L2_mat(7,2) = -1;
L2_mat(7,1) = 1;
L3_mat(2,8) = 1;
L3_mat(3,8) = -1;
L3_mat(8,8) =  -2 * L3 / hn;
L3_mat(8,3) = -1;
L3_mat(8,2) = 1;
L4_mat(3,3) = 1/R1;
L4_mat(3,9) = -1;
L4_mat(9,3) = -1;
L4_mat(9,9) = -2 * L4 / hn;
L5_mat(2,10) = -1;
L5_mat(10,2) = -1;
L5_mat(10,10) = -2 * L5 / hn;
B1_mat(1,1) = (2 * C / hn) + (1/R0);
B1_mat(4,1) = (-hn / 2) * hbar_2e;
B1_mat(4,4) = 1;
B2_mat(3,3) = (2 * C / hn) + (1/R0);
B2_mat(5,3) = (-hn / 2) * hbar_2e;
B2_mat(5,5) = 1;
A_00 = L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + B1_mat + B2_mat;
B1_mat(1,1) = (2 * C / hn) + (1/Rn);
A_n0 = L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + B1_mat + B2_mat;
B1_mat(1,1) = (2 * C / hn) + (1/R0);
B2_mat(3,3) = (2 * C / hn) + (1/Rn);
A_0n = L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + B1_mat + B2_mat;
B1_mat(1,1) = (2 * C / hn) + (1/Rn);
B2_mat(3,3) = (2 * C / hn) + (1/Rn);
A_nn = L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + B1_mat + B2_mat;

% Sources
IA = 0;
IB = 0;
IA = zeros(1, 800-1);
for i = (200E-12/hn - 1):(205E-12/hn - 1)
    IA = [IA, (600E-6 - 0)/(205E-12 - 200E-12) * (i*hn - 200E-12)];
end
for i = (205E-12/hn):(210E-12/hn - 1)
    IA = [IA, 600E-6 + (0 - 600E-6)/(210E-12 - 205E-12) * (i*hn - 205E-12)];
end
IA = [IA, zeros*ones(1, N-840)];
for i = 1:(5E-12/hn - 1)
    IB = [IB, (250E-6 - 0)/(5E-12 - 0) * (i*hn)];
end
IB = [IB, 250E-6*ones(1,N-20)];
 

VN1_1 = 0;
VN1_1d = 0;
VN1_2 = 0;
VN1_2d = 0;
VN3_1 = 0;
VN3_1d = 0;
VN3_2 = 0;
VN3_2d = 0;


phiN1_1 = 0;
phiN1_2 = 0;
phiN3_1 = 0;
phiN3_2 = 0;



T = zeros(1, N);
RHS = zeros(1, 10);
LHS = zeros(1, 10);
V_N1 = zeros(1, N);
V_N2 = zeros(1, N);
V_N3 = zeros(1, N);
V_Phi1 = zeros(1, N);
V_Phi2 = zeros(1, N);
I_L1 = zeros(1, N);
I_L2 = zeros(1, N);
I_L3 = zeros(1, N);
I_L4 = zeros(1, N);
I_L5 = zeros(1, N);

A = A_00;

for i = 1:N
    IsB1 = -Icrit * sin(phiN1_1 + hn/2 * hbar_2e * (VN1_1 + VN1_1 + hn*(VN1_1d))) + ((2*C/hn) * VN1_1) + (C * VN1_1d);
    IsB2 = -Icrit * sin(phiN3_1 + hn/2 * hbar_2e * (VN3_1 + VN3_1 + hn*(VN3_1d))) + ((2*C/hn) * VN3_1) + (C * VN3_1d);
    VN1 = LHS(1);
    VN2 = LHS(2);
    VN3 = LHS(3);
    VPhi1 = LHS(4);
    VPhi2 = LHS(5);
    IL1_1 = LHS(6);
    IL2_1 = LHS(7);
    IL3_1 = LHS(8);
    IL4_1 = LHS(9);
    IL5_1 = LHS(10);
    RHS_1 = IA(i) + IsB1;  
    RHS_2 = IB(i);
    RHS_3 = IsB2;
    RHS_phi1 = phiN1_1 + (hn/2 * hbar_2e)*(VN1_1);
    RHS_phi2 = phiN3_1 + (hn/2 * hbar_2e)*(VN3_1);
    RHS_IL1 = -(2*L1)/hn*(IL1_1) - (VN1);
    RHS_IL2 = -(2*L2)/hn*(IL2_1) - (VN2 - VN1);
    RHS_IL3 = -(2*L3)/hn*(IL3_1) - (VN3 - VN2);
    RHS_IL4 = -(2*L4)/hn*(IL4_1) - (VN3);
    RHS_IL5 = -(2*L5)/hn*(IL5_1) - (VN2);
    RHS = [RHS_1; RHS_2; RHS_3; RHS_phi1; RHS_phi2; RHS_IL1; RHS_IL2; RHS_IL3; RHS_IL4; RHS_IL5];
    LHS = A\RHS;
    V_N1(i) = LHS(1);
    V_N2(i) = LHS(2);
    V_N3(i) = LHS(3);
    V_Phi1(i) = LHS(4);
    V_Phi2(i) = LHS(5);
    I_L1 = LHS(6);
    I_L2 = LHS(7);
    I_L3 = LHS(8);
    I_L4 = LHS(9);
    I_L5 = LHS(10);
    T(i) = hn*i;
    phiN1_1 = RHS_phi1;
    phiN3_1 = RHS_phi2;
    VN1_2 = VN1_1;
    VN1_1 = LHS(2);
    VN1_2d = VN1_1d;
    VN1_1d = (2/hn)*(VN1_1 - VN1_2) - VN1_2d;
    VN3_2 = VN3_1;
    VN3_1 = LHS(4);
    VN3_2d = VN3_1d;
    VN3_1d = (2/hn)*(VN3_1 - VN3_2) - VN3_2d;
    if LHS(1) > Vg
        if LHS(3) > Vg
            A = A_nn;
        else
            A = A_n0;
        end
    else
        if LHS(3) > Vg
            A = A_0n;
        else
            A = A_00;
        end
    end
end

figure;
subplot(3, 1, 1);
plot(T, V_N1);
subplot(3, 1, 2);
plot(T, V_N2);
subplot(3, 1, 3);
plot(T, V_N3);