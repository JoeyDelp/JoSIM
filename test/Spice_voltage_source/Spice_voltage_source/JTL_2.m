% JTL_2 test in matlab

hbar_2e = 3.038510188E15;
hn = 0.25E-12;
N = 4000;
% JJ Model
rN = 16;
r0 = 160;
vG = 2.5E-3;
C = 0.07E-12;
Icrit = 0.1E-3;
% Components
   IA_mat = zeros(18, 18);
   IB_mat = zeros(18, 18);
   L1_mat = zeros(18, 18);
   L2_mat = zeros(18, 18);
   L3_mat = zeros(18, 18);
   L4_mat = zeros(18, 18);
   L5_mat = zeros(18, 18);
   R1_mat = zeros(18, 18);
   B1_mat = zeros(18, 18);
   B2_mat = zeros(18, 18);
L1 = 2E-12;
L2 = 2.4E-12;
L3 = 2.4E-12;
L4 = 2E-12;
L5 = 0.278E-12;
R1 = 2;
IA_mat(1,9) = -1;
IA_mat(9,9) = 1;
L1_mat(1,10) = 1;
L1_mat(2,10) = -1;
L1_mat(10,1) = 1;
L1_mat(10,2) = -1;
L1_mat(10,10) = -2 * L1 / hn;
L2_mat(2,11) = 1;
L2_mat(3,11) = -1;
L2_mat(11,2) = 1;
L2_mat(11,3) = -1;
L2_mat(11,11) = -2 * L2 / hn;
L3_mat(3,12) = 1;
L3_mat(4,12) = -1;
L3_mat(12,3) = 1;
L3_mat(12,4) = -1;
L3_mat(12,12) = -2 * L3 / hn;
L4_mat(4,13) = 1;
L4_mat(5,13) = -1;
L4_mat(13,4) = 1;
L4_mat(13,5) = -1;
L4_mat(13,13) = -2 * L4 / hn;
L5_mat(6,14) = 1;
L5_mat(3,14) = -1;
L5_mat(14,6) = 1;
L5_mat(14,3) = -1;
L5_mat(14,14) = -2 * L5 / hn;
IB_mat(6,15) = -1;
IB_mat(15,15) = 1;
R1_mat(5,16) = 1;
R1_mat(16,5) = -1/R1;
R1_mat(16,16) = -1;
B1_mat(2,17) = 1;
B1_mat(7,2) = -hn/2 * hbar_2e;
B1_mat(7,7) = hn/2 * hbar_2e;
B1_mat(17,2) = 2*C / hn + 1/r0;
B1_mat(17,17) = -1;
B2_mat(4,18) = 1;
B2_mat(8,4) = -hn/2 * hbar_2e;
B2_mat(8,8) = hn/2 * hbar_2e;
B2_mat(18,4) = 2*C / hn + 1/r0;
B2_mat(18,18) = -1;
A_00 = IA_mat + IB_mat + L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + R1_mat + B1_mat + B2_mat;
B1_mat(17,2) = (2 * C / hn) + (1/rN);
A_n0 = IA_mat + IB_mat + L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + R1_mat + B1_mat + B2_mat;
B1_mat(17,2) = (2 * C / hn) + (1/r0);
B2_mat(18,4) = (2 * C / hn) + (1/rN);
A_0n = IA_mat + IB_mat + L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + R1_mat + B1_mat + B2_mat;
B1_mat(17,2) = (2 * C / hn) + (1/rN);
B2_mat(18,4) = (2 * C / hn) + (1/rN);
A_nn = IA_mat + IB_mat + L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + R1_mat + B1_mat + B2_mat;

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
 

VN2_1 = 0;
VN2_1d = 0;
VN2_2 = 0;
VN2_2d = 0;
VN4_1 = 0;
VN4_1d = 0;
VN4_2 = 0;
VN4_2d = 0;
phiN2_1 = 0;
phiN4_1 = 0;

T = zeros(1, N);
RHS = zeros(1, 18);
LHS = zeros(1, 18);
V_N1 = zeros(1, N);
V_N2 = zeros(1, N);
V_N3 = zeros(1, N);
V_N4 = zeros(1, N);
V_N5 = zeros(1, N);
V_N6 = zeros(1, N);
V_Phi1 = zeros(1, N);
V_Phi2 = zeros(1, N);
I_IA = zeros(1, N);
I_L1 = zeros(1, N);
I_L2 = zeros(1, N);
I_L3 = zeros(1, N);
I_L4 = zeros(1, N);
I_L5 = zeros(1, N);
I_IB = zeros(1, N);
I_R1 = zeros(1, N);
I_B1 = zeros(1, N);
I_B2 = zeros(1, N);

A = A_00;

for i = 1:N
    IsB1 = -Icrit * sin(phiN2_1 + hn/2 * hbar_2e * (VN2_1 + VN2_1 + hn*(VN2_1d))) + ((2*C/hn) * VN2_1) + (C * VN2_1d);
    IsB2 = -Icrit * sin(phiN4_1 + hn/2 * hbar_2e * (VN4_1 + VN4_1 + hn*(VN4_1d))) + ((2*C/hn) * VN4_1) + (C * VN4_1d);
    VN1_1 = LHS(1);
    VN2_1 = LHS(2);
    VN3_1 = LHS(3);
    VN4_1 = LHS(4);
    VN5_1 = LHS(5);
    VN6_1 = LHS(6);
    VPhi1_1 = LHS(7);
    VPhi2_1 = LHS(8);
    IIA_1 = LHS(9);
    IL1_1 = LHS(10);
    IL2_1 = LHS(11);
    IL3_1 = LHS(12);
    IL4_1 = LHS(13);
    IL5_1 = LHS(14);
    IIB_1 = LHS(15);
    IR1_1 = LHS(16);
    IB1_1 = LHS(17);
    IB2_1 = LHS(18);
    RHS_N1 = 0;  
    RHS_N2 = 0;
    RHS_N3 = 0;
    RHS_N4 = 0;
    RHS_N5 = 0;
    RHS_N6 = 0;
    RHS_phi1 = phiN2_1 + (hn/2 * hbar_2e)*(VN2_1);
    RHS_phi2 = phiN4_1 + (hn/2 * hbar_2e)*(VN4_1);
    RHS_IA = IA(i);
    RHS_IL1 = (-2 * L1 / hn)*(IL1_1) - (VN2_1 - VN1_1);
    RHS_IL2 = (-2 * L2 / hn)*(IL2_1) - (VN3_1 - VN2_1);
    RHS_IL3 = (-2 * L3 / hn)*(IL3_1) - (VN4_1 - VN3_1);
    RHS_IL4 = (-2 * L4 / hn)*(IL4_1) - (VN5_1 - VN4_1);
    RHS_IL5 = (-2 * L5 / hn)*(IL5_1) - (VN3_1 - VN6_1);
    RHS_IB = IB(i);
    RHS_IR1 = 0;
    RHS_IB1 = IsB1;
    RHS_IB2 = IsB2;
    RHS = [RHS_N1; RHS_N2; RHS_N3; RHS_N4; RHS_N5; RHS_N6; RHS_phi1; RHS_phi2; RHS_IA; RHS_IL1; RHS_IL2; RHS_IL3; RHS_IL4; RHS_IL5; RHS_IB; RHS_IR1; RHS_IB1; RHS_IB2];
    LHS = A\RHS;
    V_N1(i) = LHS(1);
    V_N2(i) = LHS(2);
    V_N3(i) = LHS(3);
    V_N4(i) = LHS(4);
    V_N5(i) = abs(LHS(5));
    V_N6(i) = LHS(6);
    V_Phi1(i) = LHS(7);
    V_Phi2(i) = LHS(8);
    I_IA = LHS(9);
    I_L1 = LHS(10);
    I_L2 = LHS(11);
    I_L3 = LHS(12);
    I_L4 = LHS(13);
    I_L5 = LHS(14);
    I_IB = LHS(15);
    I_R1 = LHS(16);
    I_B1 = LHS(17);
    I_B2 = LHS(18);
    T(i) = hn*i;
    phiN2_1 = RHS_phi1;
    phiN4_1 = RHS_phi2;
    VN2_2 = VN2_1;
    VN2_1 = LHS(2);
    VN2_2d = VN2_1d;
    VN2_1d = (2/hn)*(VN2_1 - VN2_2) - VN2_2d;
    VN4_2 = VN4_1;
    VN4_1 = LHS(4);
    VN4_2d = VN4_1d;
    VN4_1d = (2/hn)*(VN4_1 - VN4_2) - VN4_2d;
    if LHS(1) > vG
        if LHS(3) > vG
            A = A_nn;
        else
            A = A_n0;
        end
    else
        if LHS(3) > vG
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
plot(T, V_N5);
subplot(3, 1, 3);
plot(T, V_Phi2);