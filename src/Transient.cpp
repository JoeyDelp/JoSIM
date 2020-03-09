// Copyright (c) 2019 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Transient.hpp"
#include "JoSIM/Misc.hpp"

#include <string>
#include <vector>

using namespace JoSIM;

void Transient::identify_simulation(const std::vector<std::string> &controls, Transient &tObj) {
  std::vector<std::string> simtokens;
  bool transFound = false;
  for (const auto &i : controls) {
    if (i.find("TRAN") != std::string::npos) {
      transFound = true;
      simtokens = Misc::tokenize_delimiter(i, " ,");
      if (simtokens.at(0).find("TRAN") != std::string::npos) {
        if (simtokens.size() < 2) {
          Errors::control_errors(ControlErrors::TRANS_ERROR, "Too few parameters: " + i);
          tObj.set_prstep(1E-12);
          tObj.set_maxtstep(1E-12);
          tObj.set_tstop(1E-9);
          tObj.set_tstart(0);
        } else {
          tObj.set_prstep(Misc::modifier(simtokens.at(1)));
          if (simtokens.size() > 2) {
            tObj.set_tstop(Misc::modifier(simtokens.at(2)));
            if (simtokens.size() > 3) {
              tObj.set_tstart(Misc::modifier(simtokens.at(3)));
              if (simtokens.size() > 4) {
                tObj.set_maxtstep(Misc::modifier(simtokens.at(4)));
              } else
                tObj.set_maxtstep(1E-12);
            } else {
              tObj.set_tstart(0);
              tObj.set_maxtstep(1E-12);
            }
          } else {
            tObj.set_tstop(1E-9);
            tObj.set_tstart(0);
            tObj.set_maxtstep(1E-12);
          }
        }
      }
      if(tObj.get_prstep() == 0 || tObj.get_tstop() == 0 ) {
        Errors::control_errors(ControlErrors::TRANS_ERROR, i);
        tObj.set_prstep(1E-12);
        tObj.set_maxtstep(1E-12);
        tObj.set_tstop(1E-9);
        tObj.set_tstart(0);
      }
    }
  }
  tObj.set_simsize();
  if (!transFound) {
    Errors::control_errors(ControlErrors::NO_SIM, "");
  }
}