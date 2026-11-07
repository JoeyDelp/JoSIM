// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Function.hpp"

#include "JoSIM/Misc.hpp"
#include "JoSIM/Noise.hpp"
#include "JoSIM/Parameters.hpp"
#include "spline.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>

using namespace JoSIM;

static int noise_process_count(double value) {
    if (!std::isfinite(value) || std::floor(value) != value || value < 4.0 || value > 512.0) {
        Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "N must be an integer from 4 to 512");
    }
    return static_cast<int>(value);
}

static void validate_colored_noise(const PNoiseState& state) {
    if (!std::isfinite(state.A1) || state.A1 < 0.0 || !std::isfinite(state.alpha)
        || state.alpha < 0.0 || state.alpha > 2.0 || !std::isfinite(state.td) || state.td < 0.0
        || !std::isfinite(state.dt) || state.dt <= 0.0 || !std::isfinite(state.tcorr) || state.tcorr <= 0.0
        || !(state.td + state.dt > state.td)) {
        Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS,
                                "ASD and delay must be non-negative; alpha must be in [0,2]; timing must be finite and positive");
    }
}

static inline std::string trim_copy(std::string s) {
    auto is_ws = [](unsigned char c) { return std::isspace(c); };
    while (!s.empty() && is_ws((unsigned char) s.front())) { s.erase(s.begin()); }
    while (!s.empty() && is_ws((unsigned char) s.back())) { s.pop_back(); }
    return s;
}

static inline std::string upper_copy(std::string s) {
    for (auto& c : s) { c = (char) std::toupper((unsigned char) c); }
    return s;
}

void Function::parse_function(const std::string& str_in, const Input& iObj, const string_o& subckt) {
    const std::string str = trim_copy(str_in);
    const std::string u   = upper_copy(str);

    const size_t      lp  = u.find('(');

    // ---- Only attempt DC / bare-number parsing when there is NO '(' ----
    if (lp == std::string::npos) {
        // "DC <value>" or bare numeric
        tokens_t w = Misc::tokenize(str, " \t,");
        if (!w.empty() && u.rfind("DC", 0) == 0) {
            if (!std::isnan(parse_param(w.back(), iObj.parameters, subckt))) {
                fType_ = FunctionType::DC;
                parse_dc(w, iObj, subckt);
                return;
            }
        }

        if (!w.empty() && !std::isnan(parse_param(w.back(), iObj.parameters, subckt))) {
            fType_ = FunctionType::DC;
            parse_dc(w, iObj, subckt);
            return;
        }

        Errors::function_errors(FunctionErrors::UNKNOWN_FUNCTION, str);
        return;
    }

    // ---- Parenthesized function path ----
    const size_t rp = u.rfind(')');
    if (rp == std::string::npos || rp <= lp + 1) {
        Errors::function_errors(FunctionErrors::PARENTHESIS, str);
        return;
    }

    const std::string name   = Misc::tokenize(trim_copy(u.substr(0, lp))).back();
    const std::string inside = str.substr(lp + 1, rp - lp - 1);
    tokens_t          t      = Misc::tokenize(inside, " ,");

    if (name == "PNOISE") {
        fType_ = FunctionType::PNOISE;
        parse_pnoise(t, iObj, subckt);
    } else if (name == "FNOISE") {
        fType_ = FunctionType::FNOISE;
        parse_fnoise(t, iObj, subckt);
    } else if (name == "ICNOISE") {
        fType_ = FunctionType::ICNOISE;
        parse_icnoise(t, iObj, subckt);
    } else if (name == "NOISE") {
        fType_ = FunctionType::NOISE;
        parse_noise(t, iObj, subckt);
    } else if (name == "PWL") {
        fType_ = FunctionType::PWL;
        parse_pwl(t, iObj, subckt);
    } else if (name == "PULSE") {
        fType_ = FunctionType::PULSE;
        parse_pulse(t, iObj, subckt);
    } else if (name == "SIN" || name == "SINE") {
        fType_ = FunctionType::SINUSOID;
        parse_sin(t, iObj, subckt);
    } else if (name == "CUS") {
        fType_ = FunctionType::CUS;
        parse_cus(t, iObj, subckt);
    } else if (name == "PWS") {
        fType_ = FunctionType::PWS;
        parse_pwl(t, iObj, subckt);
    } else if (name == "EXP") {
        fType_ = FunctionType::EXP;
        parse_exp(t, iObj, subckt);
    } else {
        Errors::function_errors(FunctionErrors::UNKNOWN_FUNCTION, str);
    }
}

void Function::parse_pwl(const tokens_t& t, const Input& iObj, const string_o& s) {
    /* PWL(0 V0 T1 V1 T2 V2 ... TN VN) */

    std::vector<double> timesteps;
    std::vector<double> values;

    // Must have pairs
    if (t.size() < 2 || (t.size() % 2) != 0) {
        Errors::function_errors(FunctionErrors::TOO_FEW_VALUES,
                                "Expected time/value pairs, got " + std::to_string(t.size()) + " tokens");
        timeValues_.clear();
        ampValues_.clear();
        return;
    }

    // Parse first pair using parse_param (NOT stod)
    double t0 = parse_param(t.at(0), iObj.parameters, s);
    double v0 = parse_param(t.at(1), iObj.parameters, s);
    if (!std::isfinite(t0) || t0 < 0.0 || !std::isfinite(v0)) {
        Errors::function_errors(FunctionErrors::INVALID_PWL_PARAMETERS,
                                "First time must be finite and non-negative; amplitudes must be finite");
    }

    // If first time isn't 0, prepend (0, v0) instead of (0,0)
    if (t0 != 0.0) {
        Errors::function_errors(FunctionErrors::INITIAL_VALUES, t.at(0) + " & " + t.at(1));
        timesteps.push_back(0.0);
        values.push_back(v0);
    }

    // Add the actual first point
    timesteps.push_back(t0);
    values.push_back(v0);

    // Parse remaining pairs
    for (size_t i = 2; i < t.size(); i += 2) {
        double ti = parse_param(t.at(i), iObj.parameters, s);
        double vi = parse_param(t.at(i + 1), iObj.parameters, s);
        if (!std::isfinite(ti) || !std::isfinite(vi)) {
            Errors::function_errors(FunctionErrors::INVALID_PWL_PARAMETERS, "Time/value pairs must be finite");
        }

        // Enforce strictly increasing time
        if (ti <= timesteps.back()) {
            Errors::function_errors(FunctionErrors::TOO_FEW_TIMESTEPS,
                                    "Time values must be strictly increasing. Got " + std::to_string(ti) + " after "
                                            + std::to_string(timesteps.back()));
            // You can choose to return/throw; returning prevents undefined behavior later
            timeValues_.clear();
            ampValues_.clear();
            return;
        }

        timesteps.push_back(ti);
        values.push_back(vi);
    }

    // Final sanity (should always match now)
    if (timesteps.size() != values.size()) {
        Errors::function_errors(FunctionErrors::TOO_FEW_VALUES,
                                std::to_string(timesteps.size()) + " timesteps & " + std::to_string(values.size())
                                        + " values");
    }

    timeValues_ = std::move(timesteps);
    ampValues_  = std::move(values);
}

void Function::parse_pulse(const tokens_t& t, const Input& iObj, const string_o& s) {
    /* PULSE(A1 A2 TD[0.0] TR[TSTEP] TF[TSTEP] PW[TSTOP] PER[TSTOP])*/
    if (t.size() < 2) { Errors::function_errors(FunctionErrors::PULSE_TOO_FEW_ARGUMENTS, std::to_string(t.size())); }
    if (t.size() > 7) { Errors::function_errors(FunctionErrors::INVALID_PULSE_PARAMETERS, "Expected at most 7 arguments"); }
    // Step and stop time shorthand
    const double& tstop = iObj.transSim.tstop();
    const double& tstep = iObj.transSim.tstep();
    // Set the 2 amplitudes
    ampValues_.clear();
    timeValues_.clear();
    ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
    ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
    // Set default values for optional arguments
    timeValues_.emplace_back(0.0);
    timeValues_.emplace_back(tstep);
    timeValues_.emplace_back(tstep);
    timeValues_.emplace_back(tstop);
    timeValues_.emplace_back(tstop);
    // Test if values exist
    if (t.size() >= 3) { timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s); }
    if (t.size() >= 4) { timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s); }
    if (t.size() >= 5) { timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s); }
    if (t.size() >= 6) { timeValues_.at(3) = parse_param(t.at(5), iObj.parameters, s); }
    if (t.size() >= 7) { timeValues_.at(4) = parse_param(t.at(6), iObj.parameters, s); }
    if (!std::isfinite(ampValues_.at(0)) || !std::isfinite(ampValues_.at(1))
        || std::any_of(timeValues_.begin(), timeValues_.end(), [](double time) { return !std::isfinite(time) || time < 0; })
        || timeValues_.at(4) == 0) {
        Errors::function_errors(FunctionErrors::INVALID_PULSE_PARAMETERS,
                                "Amplitudes must be finite; TD/TR/TF/PW must be finite and nonnegative; PER must be finite and positive");
    }
    // Store the stop time
    timeValues_.emplace_back(tstop);
}

void Function::parse_sin(const tokens_t& t, const Input& iObj, const string_o& s) {
    /* SIN(VO VA FREQ[1/TSTOP] TD[0.0] THETA[0.0] PHASE[0.0deg] NCYCLES[0]) */

    if (t.size() < 2) {
        Errors::function_errors(FunctionErrors::SIN_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
        return;
    }
    if (t.size() > 7) {
        Errors::function_errors(FunctionErrors::SIN_TOO_MANY_ARGUMENTS, std::to_string(t.size()));
    }

    const double tstop = iObj.transSim.tstop();

    ampValues_.clear();
    timeValues_.clear();

    // VO, VA
    ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s)); // VO
    ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s)); // VA

    if (iObj.argVerb && ampValues_.at(1) == 0.0) { Errors::function_errors(FunctionErrors::SIN_VA_ZERO, t.at(1)); }

    // timeValues_ layout:
    // [0]=FREQ (Hz), [1]=TD (s), [2]=THETA (1/s), [3]=PHASE (rad), [4]=NCYCLES
    timeValues_.assign(5, 0.0);

    timeValues_.at(0) = (tstop > 0.0) ? (1.0 / tstop) : 0.0; // default freq
    timeValues_.at(1) = 0.0;                                 // TD
    timeValues_.at(2) = 0.0;                                 // THETA
    timeValues_.at(3) = 0.0;                                 // PHASE (rad)
    timeValues_.at(4) = 0.0;                                 // NCYCLES

    if (t.size() >= 3) { timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s); }
    if (t.size() >= 4) { timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s); }
    if (t.size() >= 5) { timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s); }

    // PHASE in degrees -> radians (SPICE convention)
    if (t.size() >= 6) {
        const double phase_deg = parse_param(t.at(5), iObj.parameters, s);
        timeValues_.at(3)      = phase_deg * (Constants::PI / 180.0);
    }

    // NCYCLES (0 or negative => infinite)
    if (t.size() >= 7) { timeValues_.at(4) = parse_param(t.at(6), iObj.parameters, s); }

    // Basic sanity (avoid div-by-zero when using NCYCLES)
    if (timeValues_.at(4) > 0.0 && timeValues_.at(0) <= 0.0) {
        Errors::function_errors(FunctionErrors::SIN_TOO_FEW_ARGUMENTS, "NCYCLES requires positive FREQ");
    }
}

void Function::parse_cus(const tokens_t& t, const Input& iObj, const string_o& s) {
    /* CUS(WaveFile.dat TS SF[1.0] IM[1] TD[0.0] PER[0]) */
    if (t.size() < 2) { Errors::function_errors(FunctionErrors::CUS_TOO_FEW_ARGUMENTS, std::to_string(t.size())); }
    if (t.size() > 6) { Errors::function_errors(FunctionErrors::CUS_TOO_MANY_ARGUMENTS, std::to_string(t.size())); }
    timeValues_.clear();
    ampValues_.clear();
    miscValues_.clear();
    // First argument is required and is the wave file name
    std::filesystem::path WFline(t.at(0));
    if (WFline.is_relative()) {
        WFline = std::filesystem::path(iObj.fileParentPath.value_or(".")) / WFline;
    }
    if (!std::filesystem::exists(WFline)) {
        auto pp   = WFline.parent_path();
        auto stem = WFline.filename().string();
        std::transform(stem.begin(), stem.end(), stem.begin(), ::tolower);
        WFline = std::filesystem::path(pp.append(stem));
        if (!std::filesystem::exists(WFline)) {
            Errors::function_errors(FunctionErrors::CUS_WF_NOT_FOUND,
                                    WFline.string() + " (requested " + t.at(0) + ")");
        }
    }
    // Default values for the arguments
    miscValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
    miscValues_.emplace_back(1.0);
    miscValues_.emplace_back(1);
    miscValues_.emplace_back(0.0);
    miscValues_.emplace_back(0);
    // Test if optional values exist
    if (t.size() >= 3) { miscValues_.at(1) = parse_param(t.at(2), iObj.parameters, s); }
    if (t.size() >= 4) { miscValues_.at(2) = parse_param(t.at(3), iObj.parameters, s); }
    if (t.size() >= 5) { miscValues_.at(3) = parse_param(t.at(4), iObj.parameters, s); }
    if (t.size() >= 6) { miscValues_.at(4) = parse_param(t.at(5), iObj.parameters, s); }

    const double ts = miscValues_.at(0), scale = miscValues_.at(1);
    const double method = miscValues_.at(2), delay = miscValues_.at(3), periodic = miscValues_.at(4);
    if (!std::isfinite(ts) || ts <= 0.0 || !std::isfinite(scale) || !std::isfinite(delay) || delay < 0.0
        || !(delay + ts > delay) || !std::isfinite(periodic) || (periodic != 0.0 && periodic != 1.0)) {
        Errors::function_errors(FunctionErrors::INVALID_CUS_PARAMETERS,
                                "TS must be finite and positive, SF finite, TD finite and non-negative, and PER either 0 or 1");
    }
    if (!std::isfinite(method) || std::floor(method) != method || method > 2.0 || method < 0.0) {
        Errors::function_errors(FunctionErrors::CUS_UNKNOWN_IM, Misc::vector_to_string(t));
    }
    if (iObj.argVerb && scale == 0.0) { Errors::function_errors(FunctionErrors::CUS_SF_ZERO, "0"); }
    std::ifstream wffile(std::filesystem::absolute(WFline));
    std::string   line;
    if (wffile.good()) {
        getline(wffile, line);
    } else {
        Errors::function_errors(FunctionErrors::CUS_WF_NOT_FOUND, line);
    }
    const auto values = Misc::tokenize(line, " ,;\t\r\n");
    if (values.empty() || (method == 2.0 && values.size() < 3)) {
        Errors::function_errors(FunctionErrors::INVALID_CUS_PARAMETERS,
                                "Waveform " + t.at(0) + " needs at least one sample, or three samples for cubic interpolation");
    }
    const double duration = values.size() * ts;
    if (!std::isfinite(duration) || !std::isfinite(delay + duration)) {
        Errors::function_errors(FunctionErrors::INVALID_CUS_PARAMETERS, "Waveform duration is not finite: " + t.at(0));
    }
    for (size_t i = 0; i < values.size(); ++i) {
        const double value = Misc::modifier(values.at(i)) * scale;
        if (!std::isfinite(value)) {
            Errors::function_errors(FunctionErrors::INVALID_CUS_PARAMETERS, "Waveform samples must be finite: " + t.at(0));
        }
        ampValues_.emplace_back(value);
        timeValues_.emplace_back(i * ts);
    }
}

void Function::parse_noise(const tokens_t& t, const Input& iObj, const string_o& s) {
    /* NOISE(VA TD[0.0] TSTEP[1/(3*NEB) or sim tstep]) */

    if (t.size() > 3) { Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "NOISE accepts at most 3 arguments"); }
    if (t.size() < 1) { Errors::function_errors(FunctionErrors::NOISE_TOO_FEW_ARGUMENTS, std::to_string(t.size())); }

    const double tstep_sim = iObj.transSim.tstep();

    const double VA        = parse_param(t.at(0), iObj.parameters, s);
    double       td        = 0.0;
    if (t.size() >= 2) { td = parse_param(t.at(1), iObj.parameters, s); }

    // Default TSTEP: prefer NEB if available, otherwise fall back to sim timestep
    double tstep_noise = tstep_sim;
    if (t.size() >= 3) {
        tstep_noise = parse_param(t.at(2), iObj.parameters, s);
    } else if (iObj.neB != 0.0) {
        tstep_noise = Noise::determine_sampling_interval(iObj.neB, tstep_sim);
    }

    if (!std::isfinite(VA) || VA < 0.0 || !std::isfinite(td) || td < 0.0
        || !std::isfinite(tstep_noise) || tstep_noise <= 0.0 || !(td + tstep_noise > td)) {
        Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS,
                                "ASD and delay must be non-negative; TSTEP must be finite and positive");
    }

    // You cannot realize bandwidth above Nyquist of the simulation step; clamp.
    if (tstep_noise < tstep_sim) { tstep_noise = tstep_sim; }

    // Store
    ampValues_.clear();
    timeValues_.clear();
    miscValues_.clear();

    ampValues_.emplace_back(VA);  // VA (one-sided ASD)
    ampValues_.emplace_back(0.0); // pre-TD output

    timeValues_.emplace_back(td);
    timeValues_.emplace_back(tstep_noise);

    // misc: y1, y2, x1, x2
    miscValues_.emplace_back(0.0);
    miscValues_.emplace_back(Misc::grand() / (std::sqrt(2.0) * std::sqrt(tstep_noise)));
    miscValues_.emplace_back(td);
    miscValues_.emplace_back(td + tstep_noise);

    if (iObj.argVerb) {
        if (VA == 0.0) { Errors::function_errors(FunctionErrors::NOISE_VA_ZERO, t.at(0)); }
    }
}

void Function::initialize_pnoise(PNoiseState& st) {
    st.fL = 1.0 / st.tcorr;
    st.fH = 0.5 / st.dt;
    if (!std::isfinite(st.fL) || !std::isfinite(st.fH) || !(st.fL > 0) || !(st.fH > st.fL)
        || !std::isfinite(st.td + st.dt) || !(st.td + st.dt > st.td)) {
        Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS,
                                "Colored noise requires finite positive band edges with TCORR>2*TSTEP");
    }
    st.tau.resize(st.N);
    st.a.resize(st.N);
    st.b.resize(st.N);
    st.var.resize(st.N);
    st.y.resize(st.N);

    const double logTwoPi = std::log(2 * Constants::PI);
    const double logFast = -logTwoPi - std::log(st.fH);
    const double logSlow = -logTwoPi - std::log(st.fL);
    const double logStep = (logSlow - logFast) / st.N;
    const double logAmplitude = st.A1 > 0 ? 2 * std::log(st.A1) : 0;
    // Integral identity: 4*C*integral(tau^alpha/(1+(2*pi*f*tau)^2), dlog(tau))
    // equals A1^2/f^alpha, C=A1^2*(2*pi)^alpha*sin(pi*alpha/2)/(2*pi).
    // Fold the missing fast/slow tails into the corresponding endpoint pole.
    const double logC = st.alpha > 0 && st.alpha < 2
        ? logAmplitude + (st.alpha - 1) * logTwoPi
          + std::log(std::sin(Constants::PI * std::min(st.alpha, 2 - st.alpha) / 2)) : 0;
    st.scale = 1; // Amplitude is already included in the stationary variances.
    st.S1Hz = 0;
    for (int k = 0; k < st.N; ++k) {
        const double logTau = logFast + (k + .5) * logStep;
        const double tau = std::exp(logTau);
        if (!std::isfinite(tau) || tau <= 0) {
            Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "Colored relaxation time is not representable");
        }
        st.tau[k] = tau;
        const double ratio = st.dt / tau;
        const double q = ratio / 2;
        // Exact AR(1) sampling raises a continuous OU pole's DC PSD by q*coth(q).
        // Compensate that gain before interpolation, rather than renormalizing
        // an arbitrary reference frequency at a rolling band edge.
        const double logCorrection = q == 0 ? 0 : std::log(std::tanh(q) / q);
        double variance = 0;
        if (st.A1 > 0) {
            if (st.alpha == 0) {
                if (k == 0) { variance = std::exp(logAmplitude - std::log(4.0) - logTau + logCorrection); }
            } else if (st.alpha == 2) {
                if (k == st.N - 1) { variance = std::exp(logAmplitude + 2 * std::log(Constants::PI) + logTau + logCorrection); }
            } else {
                variance = std::exp(logC + (st.alpha - 1) * logTau + std::log(logStep) + logCorrection);
                if (k == 0) {
                    variance += std::exp(logC + st.alpha * logFast - std::log(st.alpha) - logTau + logCorrection);
                }
                if (k == st.N - 1) {
                    variance += std::exp(logC + (st.alpha - 2) * logSlow - std::log(2 - st.alpha) + logTau + logCorrection);
                }
            }
        }
        if (!std::isfinite(variance)) {
            Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "Colored variance is not representable");
        }
        st.var[k] = variance;
        st.a[k] = std::exp(-ratio);
        st.b[k] = std::sqrt(variance) * std::sqrt(-std::expm1(-2 * ratio));
        st.y[k] = std::sqrt(variance) * Misc::grand();
        const double wTau = 2 * Constants::PI * tau;
        const double shape = wTau <= 1 ? tau / (1 + wTau * wTau)
            : ((1 / (2 * Constants::PI)) / wTau) / (1 + (1 / wTau) * (1 / wTau));
        st.S1Hz += variance * (4 * shape);
    }

    double y1 = 0, y2 = 0;
    for (int k = 0; k < st.N; ++k) {
        y1 += st.y[k];
        st.y[k] = st.a[k] * st.y[k] + st.b[k] * Misc::grand();
        y2 += st.y[k];
    }
    miscValues_ = {y1, y2, st.td, st.td + st.dt};
    pnoise_ = std::move(st);
}

void Function::parse_pnoise(const tokens_t& t, const Input& iObj, const string_o& s) {
    // PNOISE(A1, ALPHA, TD[0], TSTEP[iObj.transSim.tstep()], TCORR[iObj.transSim.tstop()], N[48])
    // A1: ASD at 1 Hz, one-sided, units/sqrt(Hz)
    // ALPHA: exponent in 1/f^alpha
    // TD: start delay (before TD -> 0)
    // TSTEP: noise update interval dt (fH ~= 1/(2*dt))
    // TCORR: correlation time (fL ~= 1/TCORR)
    // N: number of OU processes (log-spaced)

    if (t.size() > 6) { Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "PNOISE/ICNOISE accepts at most 6 arguments"); }
    ampValues_.clear();
    timeValues_.clear();
    miscValues_.clear();
    pnoise_.reset();

    if (t.size() < 2) {
        Errors::function_errors(FunctionErrors::NOISE_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
        return;
    }

    PNoiseState st;

    st.A1    = parse_param(t.at(0), iObj.parameters, s);
    st.alpha = parse_param(t.at(1), iObj.parameters, s);

    // Defaults
    st.td    = 0.0;
    st.dt    = iObj.transSim.tstep();
    st.tcorr = iObj.transSim.tstop();
    st.N     = 48;

    if (t.size() >= 3) { st.td = parse_param(t.at(2), iObj.parameters, s); }
    if (t.size() >= 4) { st.dt = parse_param(t.at(3), iObj.parameters, s); }
    if (t.size() >= 5) { st.tcorr = parse_param(t.at(4), iObj.parameters, s); }
    if (t.size() >= 6) {
        const double Nd = parse_param(t.at(5), iObj.parameters, s);
        st.N            = noise_process_count(Nd);
    }

    if (st.tcorr == 0.0) { st.tcorr = iObj.transSim.tstop(); }
    validate_colored_noise(st);

    // The simulation sampling interval bounds the generated bandwidth.
    const double dt_sim = iObj.transSim.tstep();
    if (st.dt < dt_sim) { st.dt = dt_sim; }
    if (st.tcorr <= 0.0) { st.tcorr = iObj.transSim.tstop(); }
    if (st.td < 0.0) { st.td = 0.0; }

    if (st.N < 4) { st.N = 4; }
    if (st.N > 512) { st.N = 512; }

    ampValues_ = {st.A1, st.alpha};
    timeValues_ = {st.td, st.dt, st.tcorr};
    initialize_pnoise(st);
}

void Function::parse_fnoise(const tokens_t& t, const Input& iObj, const string_o& s) {
    // FNOISE(Aphi0, ALPHA, PHIDC[0], TD[0], TSTEP[iObj.transSim.tstep()], TCORR[iObj.transSim.tstop()], N[48])
    //
    // Aphi0 : ASD at 1 Hz in Phi0/sqrt(Hz)
    // ALPHA : exponent
    // PHIDC : DC flux bias in Phi0
    // returns phase in radians: phi(t) = 2*pi*(PHIDC + dPhi(t))  with dPhi shaped as 1/f^alpha in-band

    if (t.size() > 7) { Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "FNOISE accepts at most 7 arguments"); }
    ampValues_.clear();
    timeValues_.clear();
    miscValues_.clear();
    pnoise_.reset();

    if (t.size() < 2) {
        Errors::function_errors(FunctionErrors::NOISE_TOO_FEW_ARGUMENTS, std::to_string(t.size()));
        return;
    }

    const double Aphi0 = parse_param(t.at(0), iObj.parameters, s);
    const double alpha = parse_param(t.at(1), iObj.parameters, s);

    double       PHIDC = 0.0;
    double       td    = 0.0;
    double       dt    = iObj.transSim.tstep();
    double       tcorr = iObj.transSim.tstop();
    int          N     = 48;

    if (t.size() >= 3) { PHIDC = parse_param(t.at(2), iObj.parameters, s); }
    if (t.size() >= 4) { td = parse_param(t.at(3), iObj.parameters, s); }
    if (t.size() >= 5) { dt = parse_param(t.at(4), iObj.parameters, s); }
    if (t.size() >= 6) { tcorr = parse_param(t.at(5), iObj.parameters, s); }
    if (t.size() >= 7) {
        const double Nd = parse_param(t.at(6), iObj.parameters, s);
        N               = noise_process_count(Nd);
    }

    // Convert to phase units (radians)
    const double Aphi  = 2.0 * Constants::PI * Aphi0; // [rad/sqrt(Hz)] at 1 Hz
    const double phiDC = 2.0 * Constants::PI * PHIDC; // [rad]

    // Build state exactly like PNOISE, but in phase units
    PNoiseState  st;
    st.A1               = Aphi;
    st.alpha            = alpha;
    st.td               = td;
    st.dt               = dt;
    st.tcorr            = tcorr;
    st.N                = N;

    if (st.tcorr == 0.0) { st.tcorr = iObj.transSim.tstop(); }
    validate_colored_noise(st);
    if (!std::isfinite(PHIDC)) {
        Errors::function_errors(FunctionErrors::INVALID_NOISE_PARAMETERS, "PHIDC must be finite");
    }

    // The simulation sampling interval bounds the generated bandwidth.
    const double dt_sim = iObj.transSim.tstep();
    if (st.dt < dt_sim) { st.dt = dt_sim; }
    if (st.tcorr <= 0.0) { st.tcorr = iObj.transSim.tstop(); }
    if (st.td < 0.0) { st.td = 0.0; }

    if (st.N < 4) { st.N = 4; }
    if (st.N > 512) { st.N = 512; }

    // Public amplitudes retain flux units; the process state uses radians.
    ampValues_ = {Aphi0, alpha, phiDC};
    timeValues_ = {st.td, st.dt, st.tcorr};
    initialize_pnoise(st);
}

void Function::parse_icnoise(const tokens_t& t, const Input& iObj, const string_o& s) {
    // Same engine as PNOISE; ICNOISE is just a semantic wrapper for δ(t) (dimensionless).
    parse_pnoise(t, iObj, s);
    // (parse_pnoise() doesn't set fType_, but keep this explicit)
    fType_ = FunctionType::ICNOISE;
}

void Function::parse_dc(const tokens_t& t, const Input& iObj, const string_o& s) {
    /* DC VALUE */
    // Set amplitudes
    ampValues_.emplace_back(parse_param(t.back(), iObj.parameters, s));
}

void Function::parse_exp(const tokens_t& t, const Input& iObj, const string_o& s) {
    /* EXP(V1 V2 TD1[0.0] TAU1[TSTEP] TD2[TSTEP] TAU2[TSTEP]) */
    if (t.size() < 2) { Errors::function_errors(FunctionErrors::EXP_TOO_FEW_ARGUMENTS, std::to_string(t.size())); }
    const double& tstep = iObj.transSim.tstep();
    // Set amplitudes
    ampValues_.emplace_back(parse_param(t.at(0), iObj.parameters, s));
    ampValues_.emplace_back(parse_param(t.at(1), iObj.parameters, s));
    // Default values for the arguments
    timeValues_.emplace_back(0.0);
    timeValues_.emplace_back(tstep);
    timeValues_.emplace_back(tstep);
    timeValues_.emplace_back(tstep);
    // Test if optional values exist
    if (t.size() >= 3) { timeValues_.at(0) = parse_param(t.at(2), iObj.parameters, s); }
    if (t.size() >= 4) { timeValues_.at(1) = parse_param(t.at(3), iObj.parameters, s); }
    if (t.size() >= 5) { timeValues_.at(2) = parse_param(t.at(4), iObj.parameters, s); }
    if (t.size() >= 6) { timeValues_.at(3) = parse_param(t.at(5), iObj.parameters, s); }
}

double Function::return_pwl(double& x) {
    // If x larger than the last timestep then assume last amplitude value
    if (x >= timeValues_.back()) {
        return ampValues_.back();
        // Else check within which range x falls
    } else {
        for (auto i = 0; i < timeValues_.size() - 1; ++i) {
            if (x >= timeValues_.at(i) && x < timeValues_.at(i + 1)) {
                double& y2 = ampValues_.at(i + 1);
                double& y1 = ampValues_.at(i);
                double& x2 = timeValues_.at(i + 1);
                double& x1 = timeValues_.at(i);
                // Calculate function value and return it
                return y1 + ((y2 - y1) / (x2 - x1)) * (x - x1);
            }
        }
    }
    return 0.0;
}

double Function::return_pulse(double& x) {
    // Shorthand for the relevant time values
    double& td    = timeValues_.at(0);
    double& tr    = timeValues_.at(1);
    double& tf    = timeValues_.at(2);
    double& pw    = timeValues_.at(3);
    double& per   = timeValues_.at(4);
    double& alow  = ampValues_.at(0);
    double& ahigh = ampValues_.at(1);
    auto    val   = alow;
    if (x < td) { return (val); }
    const double elapsed = x - td;
    const double cycles = elapsed / per;
    // Preserve ordinary pulse rounding; avoid an undefined cast for large cycle counts.
    const double time = cycles < std::ldexp(1.0, 63)
        ? elapsed - static_cast<int64_t>(cycles) * per : std::fmod(elapsed, per);
    if (time < tr) {
        val = alow + (ahigh - alow) * time / tr;
    } else if (time < tr + pw) {
        val = ahigh;
    } else if (time < tr + pw + tf) {
        val = ahigh + (alow - ahigh) * (time - tr - pw) / tf;
    } else {
        val = alow;
    }

    return (val);
}

double Function::return_sin(double& x) {
    const double vo      = ampValues_.at(0);
    const double va      = ampValues_.at(1);

    const double freq    = timeValues_.at(0);
    const double td      = timeValues_.at(1);
    const double theta   = timeValues_.at(2);
    const double phase   = timeValues_.at(3); // radians
    const double ncycles = timeValues_.at(4);

    // Before delay: DC offset only
    if (x < td) { return vo; }

    const double t = x - td;

    // Stop after N cycles (if requested)
    if (ncycles > 0.0 && freq > 0.0) {
        const double t_end = ncycles / freq;
        if (t >= t_end) {
            return vo; // "stops" at offset
        }
    }

    // Damped sine: VO + VA * exp(-theta*t) * sin(2*pi*freq*t + phase)
    return vo + va * std::sin(2.0 * Constants::PI * freq * t + phase) * std::exp(-theta * t);
}

double Function::return_cus(double& x) {
    const double ts = miscValues_.at(0), delay = miscValues_.at(3);
    const int method = static_cast<int>(miscValues_.at(2));
    if (x < delay) { return 0.0; }
    const double duration = ampValues_.size() * ts;
    double local = x - delay;
    if (miscValues_.at(4) != 0.0) { local = std::fmod(local, duration); }
    else if (local >= duration) { return 0.0; }
    const size_t index = std::min(static_cast<size_t>(local / ts), ampValues_.size() - 1);
    if (method == 0 || index == ampValues_.size() - 1) { return ampValues_.at(index); }
    if (method == 1) {
        const double fraction = (local - timeValues_.at(index)) / ts;
        return ampValues_.at(index) + fraction * (ampValues_.at(index + 1) - ampValues_.at(index));
    }
    if (method == 2) {
        tk::spline spline(timeValues_, ampValues_);
        return spline(local);
    }
    return 0.0;
}

double Function::return_noise(double& x) {
    const double td = timeValues_.front();
    const double T  = timeValues_.back();

    if (x < td) { return ampValues_.at(1); }

    double& y1 = miscValues_.at(0);
    double& y2 = miscValues_.at(1);
    double& x1 = miscValues_.at(2);
    double& x2 = miscValues_.at(3);

    // Ensure sane interval
    if (x2 <= x1) { x2 = x1 + T; }

    // Advance as many noise segments as needed (handles dt > T too)
    while (x >= x2) {
        y1 = y2;
        y2 = Misc::grand() / (std::sqrt(2.0) * std::sqrt(T));
        x1 = x2;
        x2 = x1 + T;
    }

    // Linear interpolation within current segment
    const double u = (x - x1) / (x2 - x1); // 0..1
    return ampValues_.at(0) * (y1 + (y2 - y1) * u);
}

double Function::return_pnoise(double& x) {
    // Before delay: return 0
    if (!pnoise_) { return 0.0; }

    const double td = pnoise_->td;
    if (x < td) { return 0.0; }

    // miscValues_: [0]=y1, [1]=y2, [2]=x1, [3]=x2
    if (miscValues_.size() < 4) { return 0.0; }

    auto step_once = [&]() -> double {
        double sum = 0.0;
        for (int k = 0; k < pnoise_->N; ++k) {
            pnoise_->y.at(k)  = pnoise_->a.at(k) * pnoise_->y.at(k) + pnoise_->b.at(k) * Misc::grand();
            sum              += pnoise_->y.at(k);
        }
        return sum;
    };

    double& y1 = miscValues_.at(0);
    double& y2 = miscValues_.at(1);
    double& x1 = miscValues_.at(2);
    double& x2 = miscValues_.at(3);

    // Advance noise ticks robustly (same fix as for NOISE)
    while (x >= x2) {
        y1 = y2;
        y2 = step_once();
        x1 = x2;
        x2 = x1 + pnoise_->dt;
    }

    const double dx = x2 - x1;
    if (dx <= 0.0) { return y2; }

    const double u = (x - x1) / dx; // 0..1 in normal operation
    return y1 + (y2 - y1) * u;
}

double Function::return_fnoise(double& x) {
    // ampValues_: [0]=Aphi0 (Phi0/sqrtHz), [1]=alpha, [2]=phiDC (rad)
    const double phiDC = (ampValues_.size() >= 3) ? ampValues_.at(2) : 0.0;

    // Before TD, return DC only (keeps bias present from t=0)
    if (!pnoise_) { return phiDC; }

    // return_pnoise already returns 0 before td
    return phiDC + return_pnoise(x);
}

double Function::return_icnoise(double& x) {
    // Identical time-domain waveform to PNOISE
    return return_pnoise(x);
}

double Function::return_pws(double& x) {
    if (timeValues_.empty() || ampValues_.empty()) { return 0.0; }

    // Clamp before first point (optional but usually correct)
    if (x <= timeValues_.front()) { return ampValues_.front(); }

    // Hold last value after last time (your original behavior)
    if (x >= timeValues_.back()) { return ampValues_.back(); }

    // Find the segment [x1, x2)
    for (size_t i = 0; i + 1 < timeValues_.size(); ++i) {
        double x1 = timeValues_[i];
        double x2 = timeValues_[i + 1];

        if (x >= x1 && x < x2) {
            double y1 = ampValues_[i];
            double y2 = ampValues_[i + 1];

            double dx = x2 - x1;
            if (dx <= 0.0) {
                return y2; // should never happen if parse validated
            }

            double u = (x - x1) / dx; // 0..1

            // sinusoidal spline (half-cosine)
            return y1 + (y2 - y1) * 0.5 * (1.0 - std::cos(Constants::PI * u));
        }
    }

    // Should be unreachable if x is within range
    return ampValues_.back();
}

double Function::return_dc() { return ampValues_.back(); }

double Function::return_exp(double& x) {
    // Shorthand for time values
    double& a1   = ampValues_.at(0);
    double& a2   = ampValues_.at(1);
    double& td1  = timeValues_.at(0);
    double& tau1 = timeValues_.at(1);
    double& td2  = timeValues_.at(2);
    double& tau2 = timeValues_.at(3);
    if (x < td1) {
        return ampValues_.at(0);
    } else if (x >= td1 && x < td2) {
        return a1 + (a2 - a1) * (1 - exp(-(x - td1) / tau1));
    } else if (x >= td2) {
        return a1 + (a2 - a1) * (1 - exp(-(x - td1) / tau1)) + (a1 - a2) * (1 - exp(-(x - td2) / tau2));
    }
    return 0.0;
}

double Function::value(double x) {
    switch (fType_) {
        case FunctionType::PWL: return return_pwl(x);
        case FunctionType::PULSE: return return_pulse(x);
        case FunctionType::SINUSOID: return return_sin(x);
        case FunctionType::CUS: return return_cus(x);
        case FunctionType::NOISE: return return_noise(x);
        case FunctionType::PNOISE: return return_pnoise(x);
        case FunctionType::FNOISE: return return_fnoise(x);
        case FunctionType::ICNOISE: return return_icnoise(x);
        case FunctionType::PWS: return return_pws(x);
        case FunctionType::DC: return return_dc();
        case FunctionType::EXP: return return_exp(x);
        default: return 0.0;
    }
}

void Function::ampValues(std::vector<double> values) { ampValues_ = values; }
