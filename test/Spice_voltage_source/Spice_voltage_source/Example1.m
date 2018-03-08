R1 = 5;
R2 = 10;
R3 = 15;
R4 = 20;
L1 = 10E-12;
L2 = 20E-12;
L3 = 10E-12;
h = 0.001e-12;
N = 5000;
IA = [zeros(1,5), ones(1,(N-5))];
VB = [zeros(1,5), 3*ones(1,(N-5))];

%        VN1       VN2       VN3   VN4        VN5  IR1 IL1 IR2 IL2 IR3 IL3 IR4
rR1 = [  1/R1,    -1/R1,        0,    0,         0, -1,  0,  0,  0,  0,  0,  0];
rL1 = [     0, h/(2*L1),        0,    0,         0,  0, -1,  0,  0,  0,  0,  0];
rR2 = [     0,     1/R2,    -1/R2,    0,         0,  0,  0, -1,  0,  0,  0,  0];
rL2 = [     0,        0, h/(2*L2),    0,         0,  0,  0,  0, -1,  0,  0,  0];
rR3 = [     0,        0,    -1/R3, 1/R3,         0,  0,  0,  0,  0, -1,  0,  0];
rL3 = [     0,        0, h/(2*L3),    0, -h/(2*L3),  0,  0,  0,  0,  0, -1,  0];
rR4 = [     0,        0,        0,    0,      1/R4,  0,  0,  0,  0,  0,  0, -1];
rN1 = [     0,        0,        0,    0,         0, -1,  0,  0,  0,  0,  0,  0];
rN2 = [     0,        0,        0,    0,         0,  1, -1, -1,  0,  0,  0,  0];
rN3 = [     0,        0,        0,    0,         0,  0,  0,  1, -1,  1, -1,  0];
rN5 = [     0,        0,        0,    0,         0,  0,  0,  0,  0,  0,  1, -1];
rV4 = [     0,        0,        0,    1,         0,  0,  0,  0,  0,  0,  0,  0]; % <-- voltage source at node 5

A = [rR1; rL1; rR2; rL2; rR3; rL3; rR4; rN1; rN2; rN3; rN5; rV4];
RHS = zeros(12,1);
LHS = zeros(12,1);
V1 = zeros(1, N);
V2 = zeros(1, N);
V3 = zeros(1, N);
V4 = zeros(1, N);
V5 = zeros(1, N);
T = zeros(1, N);

for i = 2:N
    V_N1 = LHS(1);
    V_N2 = LHS(2);
    V_N3 = LHS(3);
    V_N4 = LHS(4);
    V_N5 = LHS(5);
    I_R1 = LHS(6);
    I_L1 = LHS(7);
    I_R2 = LHS(8);
    I_L2 = LHS(9);
    I_R3 = LHS(10);
    I_L3 = LHS(11);
    I_R4 = LHS(12);
    RHS_R1 = 0;
    RHS_L1 = -h/(2*L1)*(V_N2) - I_L1;
    RHS_R2 = 0;
    RHS_L2 = -h/(2*L2)*(V_N3) - I_L2;
    RHS_R3 = 0;
    RHS_L3 = -h/(2*L3)*(V_N3 - V_N5) - I_L3;
    RHS_R4 = 0;
    RHS_N1 = IA(i);
    RHS_N2 = 0;
    RHS_N3 = 0;
    RHS_N5 = 0;
    RHS_V4 = VB(i);
    RHS = [RHS_R1; RHS_L1; RHS_R2; RHS_L2; RHS_R3; RHS_L3; RHS_R4; RHS_N1; RHS_N2; RHS_N3; RHS_N5; RHS_V4];
    LHS = A\RHS;
    V1(i) = LHS(1);
    V2(i) = LHS(2);
    V3(i) = LHS(3);
    V4(i) = LHS(4);
    V5(i) = LHS(5);
    T(i) = h*i;
end

figure(1);
plot(T, V5);