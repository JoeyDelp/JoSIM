// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Transient.hpp"
#include "JoSIM/Misc.hpp"

#include <string>
#include <vector>

using namespace JoSIM;

void Transient::identify_simulation(
  const std::vector<tokens_t> &controls, Transient &tObj) {
  // Flag to store that a transient simulation was found
  bool transFound = false;
  // Loop through all the controls
  for (const auto &i : controls) {
    // If the first token of the line contains "TRAN"
    if (i.front().find("TRAN") != std::string::npos) {
      // Set the flag as true
      transFound = true;
      // If there are less than 2 tokens
      if (i.size() < 2) {
        // Complain of invalid transient analysis specification
        Errors::control_errors(
          ControlErrors::TRANS_ERROR, 
          "Too few parameters: " + Misc::vector_to_string(i));
        // Set the parameters to default values and continue
        tObj.set_prstep(1E-12);
        tObj.set_maxtstep(1E-12);
        tObj.set_tstop(1E-9);
        tObj.set_tstart(0);
      // If there are more than 2 tokens
      } else {
        // Set the step size
        tObj.set_prstep(Misc::modifier(i.at(1)));
        if (i.size() > 2) {
          // Set the simulation stop time
          tObj.set_tstop(Misc::modifier(i.at(2)));
          if (i.size() > 3) {
            // Set the simulation start time
            tObj.set_tstart(Misc::modifier(i.at(3)));
            if (i.size() > 4) {
              // Set the maximum step size (not yet used)
              tObj.set_maxtstep(Misc::modifier(i.at(4)));
            } else
              // Set default
              tObj.set_maxtstep(1E-12);
          } else {
            // Set default
            tObj.set_tstart(0);
            tObj.set_maxtstep(1E-12);
          }
        } else {
          // Set default
          tObj.set_tstop(1E-9);
          tObj.set_tstart(0);
          tObj.set_maxtstep(1E-12);
        }
      }
      // If either the step size or stop time is 0
      if(tObj.get_prstep() == 0 || tObj.get_tstop() == 0 ) {
        // Complain
        Errors::control_errors(
          ControlErrors::TRANS_ERROR, Misc::vector_to_string(i));
        // Set defaults and continue
        tObj.set_prstep(1E-12);
        tObj.set_maxtstep(1E-12);
        tObj.set_tstop(1E-9);
        tObj.set_tstart(0);
      }
    }
  }
  // Calculate the simulation size (Stop - Start / Step)
  tObj.set_simsize();
  // If no transient was found in the controls
  if (!transFound) {
    // Complain and exit as no simulation will take place
    Errors::control_errors(ControlErrors::NO_SIM);
  }
}