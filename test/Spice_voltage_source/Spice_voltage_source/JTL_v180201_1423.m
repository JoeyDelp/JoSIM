clear;
% JTL test in matlab (voltage input)
phi0 = 2.06783383113E-15;
e2_over_h = 2*pi/phi0;
hn = 0.25E-12;
runtime = 1000E-12;
N = round(runtime/hn);
% JJ Model
jj_area = 2.16;
jj_c = 0.07E-12;
jj_rn = 16;
jj_r0 = 160;
jj_icrit = 0.1E-3;
C = jj_c * jj_area;
rN = jj_rn / jj_area;
r0 = jj_r0 / jj_area;
vG = 2.8E-3;
Icrit = jj_icrit * jj_area;
% Components
L1 = 2E-12;
L2 = 2.425E-12;
L3 = 2.425E-12;
L4 = 2.031E-12;
L5 = 0.278E-12;
R1 = 2;
RB1 = 5.23;
RB2 = 5.23;

L1_mat = zeros(14, 14);
L2_mat = zeros(14, 14);
L3_mat = zeros(14, 14);
L4_mat = zeros(14, 14);
L5_mat = zeros(14, 14);
B1_mat = zeros(14, 14);
B2_mat = zeros(14, 14);
R1_mat = zeros(14, 14);
VA_mat = zeros(14, 14);

% Column indexes
C_NV1 = 1;
C_NV2 = 2;
C_NV3 = 3;
C_NV4 = 4;
C_NV5 = 5;
C_NV6 = 6;
C_IL1 = 7;
C_IL2 = 8;
C_IL3 = 9;
C_IL4 = 10;
C_IL5 = 11;
C_PB1 = 12;
C_PB2 = 13;
C_VA  = 14;

% Row indexes
R_N1 = 1;
R_N2 = 2;
R_N3 = 3;
R_N4 = 4;
R_N5 = 5;
R_N6 = 6;
R_L1 = 7;
R_L2 = 8;
R_L3 = 9;
R_L4 = 10;
R_L5 = 11;
R_B1 = 12;
R_B2 = 13;
R_VA = 14;

L1_mat(R_N1,C_IL1) = 1;
L1_mat(R_N2,C_IL1) = -1;
L1_mat(R_L1,C_NV1) =  1;
L1_mat(R_L1,C_NV2) = -1;
L1_mat(R_L1,C_IL1) = -2 * L1 / hn;

L2_mat(R_N2,C_IL2) = 1;
L2_mat(R_N3,C_IL2) = -1;
L2_mat(R_L2,C_NV2) =  1;
L2_mat(R_L2,C_NV3) = -1;
L2_mat(R_L2,C_IL2) = -2 * L2 / hn;

L3_mat(R_N3,C_IL3) = 1;
L3_mat(R_N4,C_IL3) = -1;
L3_mat(R_L3,C_NV3) =  1;
L3_mat(R_L3,C_NV4) = -1;
L3_mat(R_L3,C_IL3) = -2 * L3 / hn;

L4_mat(R_N4,C_IL4) = 1;
L4_mat(R_N5,C_IL4) = -1;
L4_mat(R_L4,C_NV4) =  1;
L4_mat(R_L4,C_NV5) = -1;
L4_mat(R_L4,C_IL4) = -2 * L4 / hn;

L5_mat(R_N6,C_IL5) = 1;
L5_mat(R_N3,C_IL5) = -1;
L5_mat(R_L5,C_NV6) =  1;
L5_mat(R_L5,C_NV3) = -1;
L5_mat(R_L5,C_IL5) = -2 * L5 / hn;

R1_mat(R_N5,C_NV5) = 1/R1;

B1_mat(R_N2,C_NV2) = (2 * C / hn) + (1/r0) + (1/RB1);
B1_mat(R_B1,C_NV2) = (-hn / 2) * e2_over_h;
B1_mat(R_B1,C_PB1) = 1;

B2_mat(R_N4,C_NV4) = (2 * C / hn) + (1/r0) + (1/RB2);
B2_mat(R_B2,C_NV4) = (-hn / 2) * e2_over_h;
B2_mat(R_B2,C_PB2) = 1;

VA_mat(R_N1,C_VA) = 1;
VA_mat(R_VA,C_NV1) = 1;

A_00 = L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + R1_mat + B1_mat + B2_mat + VA_mat;
B1_mat(R_N2,C_NV2) = (2 * C / hn) + (1/rN) + (1/RB1);
A_n0 = L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + R1_mat + B1_mat + B2_mat + VA_mat;
B1_mat(R_N2,C_NV2) = (2 * C / hn) + (1/r0) + (1/RB1);
B2_mat(R_N4,C_NV4) = (2 * C / hn) + (1/rN) + (1/RB2);
A_0n = L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + R1_mat + B1_mat + B2_mat + VA_mat;
B1_mat(R_N2,C_NV2) = (2 * C / hn) + (1/rN) + (1/RB1);
B2_mat(R_N4,C_NV4) = (2 * C / hn) + (1/rN) + (1/RB2);
A_nn = L1_mat + L2_mat + L3_mat + L4_mat + L5_mat + R1_mat + B1_mat + B2_mat + VA_mat;

% Sources
sfq_start = 200E-12;
sfq_up = 2.5E-12;
sfq_down = 2.5E-12;
sfq_ampl = 827.13E-6;
VA = zeros(1, sfq_start/hn-1);
for i = (sfq_start/hn):((sfq_start + sfq_up)/hn-1)
    VA = [VA, sfq_ampl/sfq_up * (i*hn - sfq_start)];
end
for i = ((sfq_start + sfq_up)/hn):((sfq_start + sfq_up + sfq_down)/hn - 1)
    VA = [VA, sfq_ampl - sfq_ampl/sfq_down * (i*hn - (sfq_start + sfq_up))];
end
[VA_rows,VA_cols] = size(VA);
VA = [VA, zeros(1, N-VA_cols)];

IB = 0;
for i = 1:(5E-12/hn - 1)
    IB = [IB, (280E-6 - 0)/(5E-12 - 0) * (i*hn)];
end
[IB_rows,IB_cols] = size(IB);
IB = [IB, 280E-6*ones(1,N-IB_cols)];

T = zeros(1, N);
RHS = zeros(14,1);
LHS = zeros(14,1);
V_N1 = zeros(1, N);
V_N2 = zeros(1, N);
V_N3 = zeros(1, N);
V_N4 = zeros(1, N);
V_N5 = zeros(1, N);
V_N6 = zeros(1, N);
V_Phi1 = zeros(1, N);
V_Phi2 = zeros(1, N);
I_L1 = zeros(1, N);
I_L2 = zeros(1, N);
I_L3 = zeros(1, N);
I_L4 = zeros(1, N);
I_L5 = zeros(1, N);

A = A_00;

% Intialize variables
VB1_prev = 0;
VB2_prev = 0;
VB1_dt_prev = 0;
VB2_dt_prev = 0;
PB1_prev = 0;
PB2_prev = 0;
VN1_prev = 0;
VN2_prev = 0;
VN3_prev = 0;
IL1_prev = 0;
IL2_prev = 0;
IL3_prev = 0;
IL4_prev = 0;
IL5_prev = 0;
IsB1 = 0;
IsB2 = 0;

for i = 1:N    
    % Construct RHS
    RHS(R_N6) = IB(i);
    RHS(R_L1) = -(2*L1/hn)*LHS(C_IL1) - (LHS(C_NV1) - LHS(C_NV2));
    RHS(R_L2) = -(2*L2/hn)*LHS(C_IL2) - (LHS(C_NV2) - LHS(C_NV3));
    RHS(R_L3) = -(2*L3/hn)*LHS(C_IL3) - (LHS(C_NV3) - LHS(C_NV4));
    RHS(R_L4) = -(2*L4/hn)*LHS(C_IL4) - (LHS(C_NV4) - LHS(C_NV5));
    RHS(R_L5) = -(2*L5/hn)*LHS(C_IL5) - (LHS(C_NV6) - LHS(C_NV3));
    RHS(R_N2) = IsB1;
    RHS(R_N4) = IsB2;
    RHS(R_B1) = LHS(C_PB1) + (hn/2 * e2_over_h)*LHS(C_NV2);
    RHS(R_B2) = LHS(C_PB2) + (hn/2 * e2_over_h)*LHS(C_NV4);
    RHS(R_VA) = VA(i);
    
    % Solve linear system
    LHS = A\RHS;
    
    % Guess junction voltage and phase for next time step
    VB1 = LHS(C_NV2);
    VB2 = LHS(C_NV4);
    PB1 = LHS(C_PB1);
    PB2 = LHS(C_PB2);    
    VB1_dt = (2/hn)*(VB1 - VB1_prev) - VB1_dt_prev;
    VB2_dt = (2/hn)*(VB2 - VB2_prev) - VB2_dt_prev;
    VB1_guess = VB1 + VB1_dt*hn;
    VB2_guess = VB2 + VB2_dt*hn;
    PB1_guess = PB1 + (hn/2 * e2_over_h)*(VB1 + VB1_guess);
    PB2_guess = PB2 + (hn/2 * e2_over_h)*(VB2 + VB2_guess);
    
    % Calculate junction currents for next time step
    IsB1 = -Icrit * sin(PB1_guess) +  ((2*C/hn) * VB1) + (C * VB1_dt);
    IsB2 = -Icrit * sin(PB2_guess) + ((2*C/hn) * VB2) + (C * VB2_dt);
    
    % Store junction voltage and phase for next time step
    VB1_prev = VB1;
    VB2_prev = VB2;
    VB1_dt_prev = VB1_dt;
    VB2_dt_prev = VB2_dt;

    % Store current time step for output
    T(i) = hn*i;
    V_N1(i) = LHS(C_NV1);
    V_N2(i) = LHS(C_NV2);
    V_N3(i) = LHS(C_NV3);
    V_N4(i) = LHS(C_NV4);
    V_N5(i) = LHS(C_NV5);
    V_N6(i) = LHS(C_NV6);
    I_L1(i) = LHS(C_IL1);
    I_L2(i) = LHS(C_IL2);
    I_L3(i) = LHS(C_IL3);
    I_L4(i) = LHS(C_IL4);
    I_L5(i) = LHS(C_IL5);
    V_Phi1(i) = LHS(C_PB1);
    V_Phi2(i) = LHS(C_PB2);
    
%     if LHS(C_NV2) > vG
%         if LHS(C_NV4) > vG
%             A = A_nn;
%         else
%             A = A_n0;
%         end
%     else
%         if LHS(C_NV4) > vG
%             A = A_0n;
%         else
%             A = A_00;
%         end
%     end
end

figure(2);
subplot(4, 1, 1);
plot(T, V_N2);
subplot(4, 1, 2);
plot(T, V_Phi1);
subplot(4, 1, 3);
plot(T, V_N4);
subplot(4, 1, 4);
plot(T, V_Phi2);