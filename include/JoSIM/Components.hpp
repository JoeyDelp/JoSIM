// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_J_COMPONENTS_H
#define JOSIM_J_COMPONENTS_H

#include "./Misc.hpp"
#include "./Parameters.hpp"
#include "./Constants.hpp"
#include "./Input.hpp"
#include "./Errors.hpp"
#include "./Resistor.hpp"
#include "./Inductor.hpp"
#include "./Capacitor.hpp"
#include "./JJ.hpp"
#include "./VoltageSource.hpp"
#include "./PhaseSource.hpp"
#include "./CurrentSource.hpp"

#include <unordered_map>

class device {
public:
  std::string label;
  std::string posNodeR, posNodeC;
  std::string negNodeR, negNodeC;
  int posNRow, posNCol;
  int negNRow, negNCol;
  int ppPtr, nnPtr, pnPtr, npPtr;
  double value;
  device() : 
    label("NOTHING"),
    posNodeR("NONE"),
    posNodeC("NONE"),
    negNodeR("NONE"),
    negNodeC("NONE"),
    posNRow(-1),
    posNCol(-1),
    negNRow(-1),
    negNCol(-1),
    ppPtr(-1),
    nnPtr(-1),
    pnPtr(-1),
    npPtr(-1),
    value(0)
  {

  }
};

class res_volt : public device {
public:
  res_volt(){};
};

class ind_volt : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol;
  int indPtr;
  std::unordered_map<int, double> mut;
  std::unordered_map<std::string, int> mutPtr;
  ind_volt() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
    indPtr = -1;
  }
};

class cap_volt : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol;
  int capPtr;
  cap_volt() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
    capPtr = -1;
  }
};

class jj_volt : public device {
public:
  std::string voltNodeR, voltNodeC, phaseNodeR, phaseNodeC;
  int voltNRow, voltNCol, phaseNRow, phaseNCol,
      pPtr, nPtr,
      rType;
  double phi0, Phi0n1, pn1, pn2, dPn1, dPn2, v0, vn1, vn2, dVn1, dVn2,
        r0, rN, C, iC, iCFact, delV, vG, iS, iT, gLarge, lowerB, upperB,
        subCond, transCond, normalCond, T, tC, Del0, Del, D, rNCalc;
  std::vector<double> jjCur;
  bool storeCurrent;
  jj_volt() {
    voltNodeR = voltNodeC = "NONE";
    voltNRow = voltNCol = -1;
    pPtr = nPtr = -1;
    phi0 = Phi0n1 = pn1 = pn2 = dPn1 = dPn2 = 0.0;
    v0 = vn1 = vn2 = dVn1 = dVn2 = 0.0;
    rType = 1;
    vG = 2.8E-3;
    delV = 0.1E-3;
    r0 = 30;
    rN = 5;
    C = 2.5E-12;
    iC = 1E-3;
    iCFact = JoSIM::Constants::PI / 4;
    iT = iS = gLarge = 0.0;
    T = 4.2;
    tC = 9.1;
    D = 0.0;
    storeCurrent = false;
  }
};

class tx_line : public device {
public:
  std::string posNode2R, posNode2C;
  std::string negNode2R, negNode2C;
  std::string curNode1R, curNode1C;
  std::string curNode2R, curNode2C;
  int posN2Row, posN2Col;
  int negN2Row, negN2Col;
  int curN1Row, curN1Col;
  int curN2Row, curN2Col;
  int k;
  double tD;
  double v1n1, v1n2, v1nk, v1nk1, dV1n1, dV1n2, dV1nk, dV1nk1, v2n1, v2n2, v2nk,
      v2nk1, dV2n1, dV2n2, dV2nk, dV2nk1;
  tx_line() {
    posNode2R = posNode2C = "NONE";
    negNode2R = negNode2C = "NONE";
    curNode1R = curNode1C = "NONE";
    curNode2R = curNode2C = "NONE";
    posN2Row = posN2Col = -1;
    negN2Row = negN2Col = -1;
    curN1Row = curN1Col = -1;
    curN2Row = curN2Col = -1;
    k = 0;
    tD = 0;
  }
};

class vs_volt : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol;
  vs_volt() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
  }
};

class ps_volt : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol;
  ps_volt() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
  }
};

class res_phase : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol;
  int resPtr;
  double IRn1, pn1;
  res_phase() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
    resPtr = -1;
    IRn1 = pn1 = 0;
  }
};

class ind_phase : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol;
  int indPtr, mutPtr;
  ind_phase() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
    indPtr = mutPtr = -1;
  }
};

class cap_phase : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol;
  int capPtr;
  double ICn1, pn1, pn2, dPn1, dPn2;
  cap_phase() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
    capPtr = -1;
    ICn1 = pn1 = pn2 = dPn1 = dPn2 = 0.0;
  }
};

class jj_phase : public device {
public:
  std::string voltNodeR, voltNodeC, phaseNodeR, phaseNodeC;
  int voltNRow, voltNCol, phaseNRow, phaseNCol,
      pPtr, nPtr,
      rType;
  double phi0, Phi0n1, pn1, pn2, dPn1, dPn2, v0, vn1, vn2, dVn1, dVn2,
        r0, rN, C, iC, iCFact, delV, vG, iS, iT, gLarge, lowerB, upperB,
        subCond, transCond, normalCond, T, tC, Del0, Del, D, rNCalc;
  std::vector<double> jjCur;
  bool storeCurrent;
  jj_phase() {
    voltNodeR = voltNodeC = "NONE";
    voltNRow = voltNCol = -1;
    pPtr = nPtr = -1;
    phi0 = Phi0n1 = pn1 = pn2 = dPn1 = dPn2 = 0.0;
    v0 = vn1 = vn2 = dVn1 = dVn2 = 0.0;
    rType = 1;
    vG = 2.8E-3;
    delV = 0.1E-3;
    r0 = 30;
    rN = 5;
    C = 2.5E-12;
    iC = 1E-3;
    iCFact = JoSIM::Constants::PI / 4;
    iT = iS = gLarge = 0.0;
    T = 4.2;
    tC = 9.1;
    D = 0.0;
    storeCurrent = false;
  }
};

class vs_phase : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol, sourceDex;
  double pn1;
  vs_phase() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
    pn1 = 0.0;
    sourceDex = -1;
  }
};

class ps_phase : public device {
public:
  std::string curNodeR, curNodeC;
  int curNRow, curNCol;
  ps_phase() {
    curNodeR = curNodeC = "NONE";
    curNRow = curNCol = -1;
  }
};

class tx_phase : public tx_line {
public:
  double p1n1, p1n2, p1nk, p1nk1, dP1n1, dP1n2, dP1nk, dP1nk1, p2n1, p2n2, p2nk,
      p2nk1, dP2n1, dP2n2, dP2nk, dP2nk1;
  tx_phase() {
    p1n1 = p1n2 = p1nk = p1nk1 = dP1n1 = dP1n2 = dP1nk = dP1nk1 = 0.0;
    p2n1 = p2n2 = p2nk = p2nk1 = dP2n1 = dP2n2 = dP2nk = dP2nk1 = 0.0;
  }
};

struct ComponentConnections {
  enum class Type {
    ResistorP,
    ResistorN,
    CapacitorP,
    CapacitorN,
    InductorP,
    InductorN,
    JJP,
    JJN,
    VSP,
    VSN,
    CSP,
    CSN,
    PSP,
    PSN,
    TXP1,
    TXP2,
    TXN1,
    TXN2
  } type;
  int index;
};

struct NodeConnections {
  std::string name;
  std::vector<ComponentConnections> connections;
};

class Components {
public:
  std::vector<res_phase> phaseRes;
  std::vector<ind_phase> phaseInd;
  std::vector<cap_phase> phaseCap;
  std::vector<jj_phase> phaseJJ;
  std::vector<vs_phase> phaseVs;
  std::vector<ps_phase> phasePs;

  std::vector<res_volt> voltRes;
  std::vector<ind_volt> voltInd;
  std::vector<cap_volt> voltCap;
  std::vector<jj_volt> voltJJ;
  std::vector<vs_volt> voltVs;
  std::vector<ps_volt> voltPs;

  std::vector<tx_line> txLine;
  std::vector<tx_phase> txPhase;

  std::vector<std::pair<std::string, std::string>> mutualInductanceLines;

  Components(){};

  void jj_model(std::string &modelstring, std::string &area, const int &jjIndex,
                Input &iObj, const std::string &subckt = "");

  void jj_model_phase(std::string &modelstring, std::string &area,
                      const int &jjIndex, Input &iObj,
                      const std::string &subckt = "");
};

class Components_new {
  public:
    std::vector<Resistor> resistors;
    std::vector<Inductor> inductors;
    std::vector<Capacitor> capacitors;
    std::vector<JJ> jjs;
    std::vector<VoltageSource> voltagesources;
    std::vector<PhaseSource> phasesources;
    std::vector<CurrentSource> currentsources;
};
#endif
