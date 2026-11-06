// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Transient.hpp"

#include "JoSIM/Misc.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace JoSIM;

void Transient::identify_simulation(std::vector<tokens_t>& controls, Transient& tObj, param_map& params) {
    // Flag to store that a transient simulation was found
    bool transFound = false;
    // Loop through all the controls
    for (auto& i : controls) {
        // If the first token of the line contains "TRAN"
        if (i.front().find("TRAN") != std::string::npos) {
            // Set the flag as true
            transFound = true;
            // UIC is retained as a legacy SPICE token; DST controls JoSIM startup.
            while (i.size() > 1 && (i.back() == "DST" || i.back() == "UIC")) {
                if (i.back() == "DST") { tObj.startup(false); }
                i.pop_back();
            }
            // Initialize the numerical parameters to default values
            tObj.tstep(0.25E-12);
            tObj.tstop(1E-9);
            tObj.prstep(tObj.tstep());
            tObj.prstart(0);
            tObj.prfirwindow(0);
            // If there are less than 3 tokens (documentation requires both Tstart and Tstop parameters)
            if (i.size() < 3 || i.size() > 6) {
                // Complain of invalid transient analysis specification
                Errors::control_errors(ControlErrors::TRANS_ERROR, "Expected TSTEP TSTOP [PRSTART [PRSTEP [FIRWINDOW]]]: " + Misc::vector_to_string(i));
            }
            // Interpret all given parameters
            if (i.size() >= 2) {
                // Set the step size
                tObj.tstep(parse_param(i.at(1), params));
                tObj.prstep(tObj.tstep()); // tstep doubles as prstep default, might be changed later
            }
            if (i.size() >= 3) {
                // Set the simulation stop time
                tObj.tstop(parse_param(i.at(2), params));
            }
            if (i.size() >= 4) {
                // Set the print start time
                tObj.prstart(parse_param(i.at(3), params));
            }
            if (i.size() >= 5) {
                // Set the print step size
                tObj.prstep(parse_param(i.at(4), params));
            }
            if (i.size() >= 6) {
                // Set the print FIR window size in seconds (default 0 means no FIR filtering)
                tObj.prfirwindow(parse_param(i.at(5), params));
            }
            if (!std::isfinite(tObj.tstep()) || tObj.tstep() <= 0.0
                || !std::isfinite(tObj.tstop()) || tObj.tstop() <= 0.0
                || !std::isfinite(tObj.prstep()) || tObj.prstep() <= 0.0
                || !std::isfinite(tObj.prstart()) || tObj.prstart() < 0.0 || tObj.prstart() >= tObj.tstop()
                || !std::isfinite(tObj.prfirwindow()) || tObj.prfirwindow() < 0.0) {
                Errors::control_errors(ControlErrors::TRANS_ERROR,
                                       "Steps and stop time must be finite and positive; the print window must lie within the run");
            }
            if (tObj.prfirwindow() > tObj.tstop()) { tObj.prfirwindow(tObj.tstop()); }
            //  Also if PSTEP is smaller than TSTEP
            if (tObj.tstep() > tObj.prstep()) {
                // Reduce TSTEP to match PRSTEP
                tObj.tstep(tObj.prstep());
            }
            const double points = tObj.tstop() / tObj.tstep();
            if (!std::isfinite(points) || points < 1.0
                || points >= static_cast<double>(std::numeric_limits<int64_t>::max())) {
                Errors::control_errors(ControlErrors::TRANS_ERROR, "Simulation point count is outside the supported range");
            }
        }
    }
    // If no transient was found in the controls
    if (!transFound) {
        // Complain and exit as no simulation will take place
        Errors::control_errors(ControlErrors::NO_SIM);
    }
}
