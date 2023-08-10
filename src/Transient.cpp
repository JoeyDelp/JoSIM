// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Transient.hpp"

#include <string>
#include <vector>

#include "JoSIM/Misc.hpp"

using namespace JoSIM;

void Transient::identify_simulation(std::vector<tokens_t>& controls,
                                    Transient& tObj, param_map& params) {
  // Flag to store that a transient simulation was found
  bool transFound = false;
  // Loop through all the controls
  for (auto& i : controls) {
    // If the first token of the line contains "TRAN"
    if (i.front().find("TRAN") != std::string::npos) {
      // Set the flag as true
      transFound = true;
      // Check for disable startup flag
      if (i.back() == "DST") {
        tObj.startup(false);
        i.pop_back();
      }
      // If there are less than 2 tokens
      if (i.size() < 2) {
        // Complain of invalid transient analysis specification
        Errors::control_errors(
            ControlErrors::TRANS_ERROR,
            "Too few parameters: " + Misc::vector_to_string(i));
        // Set the parameters to default values and continue
        tObj.tstep(0.25E-12);
        tObj.prstep(1E-12);
        tObj.tstop(1E-9);
        tObj.prstart(0);
        // If there are more than 2 tokens
      } else {
        // Set the step size
        tObj.tstep(parse_param(i.at(1), params));
        if (i.size() > 2) {
          // Set the simulation stop time
          tObj.tstop(parse_param(i.at(2), params));
          if (i.size() > 3) {
            // Set the print start time
            tObj.prstart(parse_param(i.at(3), params));
            if (i.size() > 4) {
              // Set the print step size
              tObj.prstep(parse_param(i.at(4), params));
            } else
              // Set default
              tObj.prstep(tObj.tstep());
          } else {
            // Set default
            tObj.prstart(0);
            tObj.prstep(tObj.tstep());
          }
        } else {
          // Set default
          tObj.tstop(1E-9);
          tObj.prstart(0);
          tObj.prstep(tObj.tstep());
        }
      }
      // If either the step size or stop time is 0
      if (tObj.tstep() == 0 || tObj.tstop() == 0) {
        // Complain
        Errors::control_errors(ControlErrors::TRANS_ERROR,
                               Misc::vector_to_string(i));
        // Set defaults and continue
        tObj.tstep(0.25E-12);
        tObj.prstep(tObj.tstep());
        tObj.tstop(1E-9);
        tObj.prstart(0);
      }
      //// If user provided time step is larger than 0.25ps junction will fail
      // if (tObj.tstep() > 0.25E-12) {
      //   tObj.tstep(0.25E-12);
      // }
      //  Also if PSTEP is smaller than TSTEP
      if (tObj.tstep() > tObj.prstep()) {
        // Reduce TSTEP to match PRSTEP
        tObj.tstep(tObj.prstep());
      }
    }
  }
  // If no transient was found in the controls
  if (!transFound) {
    // Complain and exit as no simulation will take place
    Errors::control_errors(ControlErrors::NO_SIM);
  }
}