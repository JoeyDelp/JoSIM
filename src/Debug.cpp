// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#include "JoSIM/Debug.hpp"
#ifndef NDEBUG
#include "JoSIM/Function.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/Rng.hpp"

#include <algorithm>
#include <limits>
#include <numeric>

double analytic_Sx(const JoSIM::PNoiseState& st, double f_hz) {
    const double w = 2.0 * JoSIM::Constants::PI * f_hz;
    double       S = 0.0;
    for (int k = 0; k < st.N; ++k) {
        const double tau    = st.tau.at(k);
        const double denom  = 1.0 + (w * tau) * (w * tau);
        S                  += (4.0 * st.var.at(k) * tau) / denom; // one-sided
    }
    return S;
}

double target_variance(const JoSIM::PNoiseState& st) {
    // One-sided integral of A1^2 / f^alpha from fL..fH
    const double A2 = st.A1 * st.A1;
    const double a  = st.alpha;
    const double fL = st.fL;
    const double fH = st.fH;

    if (std::abs(a - 1.0) < 1e-12) {
        return A2 * std::log(fH / fL);
    } else {
        return (A2 / (1.0 - a)) * (std::pow(fH, 1.0 - a) - std::pow(fL, 1.0 - a));
    }
}

bool rel_close(double a, double b, double rel) {
    const double denom = std::max({std::abs(a), std::abs(b), 1e-300});
    return std::abs(a - b) / denom <= rel;
}

bool close_abs_rel(double a, double b, double rel, double abs) {
    const double diff = std::abs(a - b);
    if (diff <= abs) { return true; }
    const double denom = std::max({std::abs(a), std::abs(b), 1e-300});
    return diff / denom <= rel;
}

void welford_update(double x, uint64_t n, double& mean, double& m2) {
    const double delta   = x - mean;
    mean                += delta / double(n);
    const double delta2  = x - mean;
    m2                  += delta * delta2;
}

int josim_debug_selftest_noise_rms() {
    // Deterministic
    JoSIM::Rng::start_run(123456789ULL);

    // Pick values
    const double   VA             = 1e-6; // A / sqrt(Hz)
    const double   B              = 1e6;  // Hz (one-sided NEB)
    const double   TSTEP          = 1.0 / (2.0 * B);

    // Expected stddev at sample instants
    const double   sigma_expected = VA * std::sqrt(B);

    // Monte Carlo
    const uint64_t N              = 250000; // ~0.3% stddev error; fast enough
    double         mean           = 0.0;
    double         m2             = 0.0;

    for (uint64_t i = 1; i <= N; ++i) {
        const double x = VA * JoSIM::Misc::grand() / std::sqrt(2.0 * TSTEP);
        welford_update(x, i, mean, m2);
    }

    const double var        = m2 / double(N - 1);
    const double sigma_meas = std::sqrt(var);
    const double rel_err    = (sigma_meas - sigma_expected) / sigma_expected;

    std::cerr << "[SELFTEST] NOISE RMS check\n";
    std::cerr << "  VA             = " << VA << " A/sqrt(Hz)\n";
    std::cerr << "  B (NEB)         = " << B << " Hz (one-sided)\n";
    std::cerr << "  TSTEP           = " << TSTEP << " s\n";
    std::cerr << "  sigma_expected  = " << sigma_expected << " A\n";
    std::cerr << "  sigma_measured  = " << sigma_meas << " A\n";
    std::cerr << "  rel_error       = " << rel_err * 100.0 << " %\n";

    // Pass criteria (tight, but realistic)
    const double tol = 0.02; // 2%
    if (std::fabs(rel_err) > tol) {
        std::cerr << "[SELFTEST] FAIL (|rel_error| > " << tol * 100.0 << "%)\n";
        return 2;
    }

    std::cerr << "[SELFTEST] PASS\n";
    return 0;
}

// bool josim_debug_selftest_pnoise(const JoSIM::Input& iObj) {
bool josim_debug_selftest_pnoise() {
    // Choose parameters that keep 1 Hz inside band (fL < 1 Hz), otherwise A1@1Hz is ill-posed.
    // dt = 1e-4 => fH = 5 kHz; tcorr = 2 s => fL = 0.5 Hz
    const std::string expr = "PNOISE(1.0, 1.0, 0.0, 1E-4, 2.0, 64)";

    JoSIM::Function   f;
    JoSIM::Input      iObj;
    f.parse_function(expr, iObj, std::nullopt);

    const auto* st = f.dbg_pnoise_state();
    if (!st) {
        std::cerr << "[PNOISE SELFTEST] FAIL: no pnoise state (parse/order/value switch bug?)\n";
        return false;
    }

    // ---- P1: analytic scaling at 1 Hz ----
    const double S1       = analytic_Sx(*st, 1.0);
    const double targetS1 = st->A1 * st->A1;

    bool         ok       = true;

    if (!rel_close(S1, targetS1, 1e-12)) {
        std::cerr << "[PNOISE SELFTEST] FAIL P1: S(1Hz)=" << S1 << " target=" << targetS1 << "\n";
        ok = false;
    } else {
        std::cout << "[PNOISE SELFTEST] PASS P1: analytic S(1Hz) matches A1^2\n";
    }

    // ---- P3: cadence / segment width ----
    {
        double x1 = 0, x2 = 0;
        if (!f.dbg_segment(x1, x2)) {
            std::cerr << "[PNOISE SELFTEST] FAIL P3: cannot read segment\n";
            ok = false;
        } else {
            const double dt = st->dt;
            if (!rel_close((x2 - x1), dt, 1e-15)) {
                std::cerr << "[PNOISE SELFTEST] FAIL P3: segment width=" << (x2 - x1) << " dt=" << dt << "\n";
                ok = false;
            } else {
                // Force multiple tick advances by jumping time
                for (int k = 1; k <= 10; ++k) {
                    double t = st->td + k * dt + 0.123 * dt;
                    (void) f.value(t);
                    double nx1 = 0, nx2 = 0;
                    f.dbg_segment(nx1, nx2);
                    if (!rel_close((nx2 - nx1), dt, 1e-15)) {
                        std::cerr << "[PNOISE SELFTEST] FAIL P3: width drift at k=" << k << "\n";
                        ok = false;
                        break;
                    }
                }
                if (ok) { std::cout << "[PNOISE SELFTEST] PASS P3: tick cadence stable\n"; }
            }
        }
    }

    // ---- P2a: sample variance matches sum(var_k) (tight) ----
    const double    var_exact = std::accumulate(st->var.begin(), st->var.end(), 0.0);

    // Sample at tick times only to avoid FOH interpolation shaping.
    const double    dt        = st->dt;
    const double    T         = 20.0; // seconds (10 cycles at 0.5 Hz corner)
    const long long N         = static_cast<long long>(T / dt);

    Welford         wf;
    double          t = st->td;
    for (long long i = 0; i < N; ++i) {
        wf.add(f.value(t));
        t += dt;
    }
    const double var_meas = wf.var();

    if (!rel_close(var_meas, var_exact, 0.05)) {
        std::cerr << "[PNOISE SELFTEST] FAIL P2a: var_meas=" << var_meas << " var_exact(sum var_k)=" << var_exact
                  << "\n";
        ok = false;
    } else {
        std::cout << "[PNOISE SELFTEST] PASS P2a: sample variance matches sum(var_k)\n";
    }

    // ---- P2b: compare OU variance to target integral (looser: approximation quality) ----
    const double var_target = target_variance(*st);

    if (!rel_close(var_exact, var_target, 0.20)) {
        std::cerr << "[PNOISE SELFTEST] WARN P2b: sum(var_k)=" << var_exact << " target integral=" << var_target
                  << " (adjust N/log spacing if you want tighter)\n";
        // Not a hard fail unless you want it to be.
    } else {
        std::cout << "[PNOISE SELFTEST] PASS P2b: OU variance close to target band integral\n";
    }

    return ok;
}

bool josim_debug_selftest_fnoise() {
    // FNOISE(Aphi0, alpha, PHIDC, TD, dt, tcorr, N)
    const double      Aphi0 = 1.0e-6; // Phi0/sqrt(Hz) at 1 Hz
    const double      alpha = 1.0;
    const double      PHIDC = 0.25; // Phi0
    const double      TD    = 1.0;  // seconds (so we can test pre-TD DC)
    const double      dt    = 1.0e-4;
    const double      tcorr = 2.0;
    const int         Nbank = 64;

    const std::string expr  = "FNOISE(1.0E-6, 1.0, 0.25, 1.0, 1.0E-4, 2.0, 64)";

    JoSIM::Function   f;
    JoSIM::Input      iObj;
    f.parse_function(expr, iObj, std::nullopt);

    const auto* st = f.dbg_pnoise_state();
    if (!st) {
        std::cerr << "[FNOISE SELFTEST] FAIL: no pnoise state (parse/order/value switch bug?)\n";
        return false;
    }

    bool ok = true;

    // ---- FDC: before TD, must be constant DC phase = 2*pi*PHIDC ----
    {
        const double phiDC = 2.0 * JoSIM::Constants::PI * PHIDC;
        double       t     = 0.5; // < TD
        double       v     = f.value(t);
        if (!rel_close(v, phiDC, 1e-15)) {
            std::cerr << "[FNOISE SELFTEST] FAIL FDC: pre-TD value=" << v << " expected=" << phiDC << "\n";
            ok = false;
        } else {
            std::cout << "[FNOISE SELFTEST] PASS FDC: DC mapping pre-TD\n";
        }
    }

    // ---- F1: analytic scaling at 1 Hz (phase PSD) ----
    {
        const double S1       = analytic_Sx(*st, 1.0);
        const double targetS1 = std::pow(2.0 * JoSIM::Constants::PI * Aphi0, 2.0);

        if (!rel_close(S1, targetS1, 1e-12)) {
            std::cerr << "[FNOISE SELFTEST] FAIL F1: Sphi(1Hz)=" << S1 << " target=" << targetS1 << "\n";
            ok = false;
        } else {
            std::cout << "[FNOISE SELFTEST] PASS F1: analytic Sphi(1Hz) matches (2*pi*Aphi0)^2\n";
        }
    }

    // ---- F3: cadence / segment width ----
    {
        double x1 = 0, x2 = 0;
        if (!f.dbg_segment(x1, x2)) {
            std::cerr << "[FNOISE SELFTEST] FAIL F3: cannot read segment\n";
            ok = false;
        } else {
            const double width = x2 - x1;
            if (!close_abs_rel(width, st->dt, 1e-12, 1e-18)) {
                std::cerr << std::setprecision(17);
                std::cerr << "[FNOISE SELFTEST] FAIL F3: width=" << width << " dt=" << st->dt
                          << " diff=" << (width - st->dt) << "\n";
                ok = false;
            } else {
                std::cout << "[FNOISE SELFTEST] PASS F3: tick cadence stable\n";
            }
        }
    }

    // ---- F2a: sample variance matches sum(var_k) (DC doesn't matter) ----
    {
        const double    var_exact = std::accumulate(st->var.begin(), st->var.end(), 0.0);

        // Sample at tick times only, AFTER TD, to avoid FOH interpolation shaping and pre-TD region.
        const double    T         = 20.0;
        const long long Nsamp     = static_cast<long long>(T / st->dt);

        Welford         wf;
        double          t = st->td;
        for (long long i = 0; i < Nsamp; ++i) {
            wf.add(f.value(t));
            t += st->dt;
        }
        const double var_meas = wf.var();

        if (!rel_close(var_meas, var_exact, 0.05)) {
            std::cerr << "[FNOISE SELFTEST] FAIL F2a: var_meas=" << var_meas << " var_exact(sum var_k)=" << var_exact
                      << "\n";
            ok = false;
        } else {
            std::cout << "[FNOISE SELFTEST] PASS F2a: sample variance matches sum(var_k)\n";
        }
    }

    // ---- F2b: OU variance approximates ideal band integral (informational) ----
    {
        // Note: target_variance uses st.A1 in phase units already.
        const double var_exact  = std::accumulate(st->var.begin(), st->var.end(), 0.0);
        const double var_target = target_variance(*st);

        if (!rel_close(var_exact, var_target, 0.20)) {
            std::cerr << "[FNOISE SELFTEST] WARN F2b: sum(var_k)=" << var_exact << " target integral=" << var_target
                      << " (adjust N/log spacing if you want tighter)\n";
        } else {
            std::cout << "[FNOISE SELFTEST] PASS F2b: OU variance close to target band integral\n";
        }
    }

    return ok;
}

bool josim_debug_selftest_icnoise() {
    // Use values where 1 Hz is inside band
    // dt=1E-4 => fH=5 kHz ; tcorr=2 => fL=0.5 Hz
    const std::string expr = "PNOISE(1E-12, 1.0, 1.0, 1E-4, 2.0, 64)"; // td=1.0s

    JoSIM::Function f;
    JoSIM::Input iObj;
    f.parse_function(expr, iObj, std::nullopt);

    const auto* st = f.dbg_pnoise_state();
    if (!st) {
        std::cerr << "[ICNOISE SELFTEST] FAIL: no pnoise state (parse/order/value switch bug?)\n";
        return false;
    }

    bool ok = true;

    // ---- IC0: pre-TD returns 0 ----
    {
        const double t_pre = st->td - 0.25;
        const double y_pre = f.value(t_pre);
        if (!rel_close(y_pre, 0.0, 0.0) && std::abs(y_pre) > 0.0) {
            std::cerr << "[ICNOISE SELFTEST] FAIL IC0: pre-TD not zero: " << y_pre << "\n";
            ok = false;
        } else {
            std::cout << "[ICNOISE SELFTEST] PASS IC0: pre-TD returns 0\n";
        }
    }

    // ---- IC1: analytic scaling at 1 Hz ----
    {
        const double S1 = analytic_Sx(*st, 1.0);
        const double targetS1 = st->A1 * st->A1;
        if (!rel_close(S1, targetS1, 1e-12)) {
            std::cerr << "[ICNOISE SELFTEST] FAIL IC1: S(1Hz)=" << S1 << " target=" << targetS1 << "\n";
            ok = false;
        } else {
            std::cout << "[ICNOISE SELFTEST] PASS IC1: analytic S(1Hz) matches A1^2\n";
        }
    }

    // ---- IC3: cadence / segment width ----
    {
        double x1=0, x2=0;
        if (!f.dbg_segment(x1, x2)) {
            std::cerr << "[ICNOISE SELFTEST] FAIL IC3: cannot read segment\n";
            ok = false;
        } else {
            const double dt = st->dt;
            const double seg = (x2 - x1);
            if (!rel_close(seg, dt, 1e-12)) {
                std::cerr << "[ICNOISE SELFTEST] FAIL IC3: segment width=" << seg << " dt=" << dt << "\n";
                ok = false;
            } else {
                // Force multiple tick advances by jumping time
                for (int k = 1; k <= 10; ++k) {
                    const double t = st->td + k * dt + 0.123 * dt;
                    (void)f.value(t);
                    double nx1=0, nx2=0;
                    f.dbg_segment(nx1, nx2);
                    if (!rel_close((nx2 - nx1), dt, 1e-12)) {
                        std::cerr << "[ICNOISE SELFTEST] FAIL IC3: width drift at k=" << k << "\n";
                        ok = false;
                        break;
                    }
                }
                if (ok) std::cout << "[ICNOISE SELFTEST] PASS IC3: tick cadence stable\n";
            }
        }
    }

    // ---- IC2a: sample variance matches sum(var_k) ----
    {
        const double var_exact = std::accumulate(st->var.begin(), st->var.end(), 0.0);

        // Sample at tick times only to avoid interpolation shaping.
        const double dt = st->dt;
        const double T  = 20.0;
        const long long N = static_cast<long long>(T / dt);

        Welford wf;
        double t = st->td;
        for (long long i = 0; i < N; ++i) {
            wf.add(f.value(t));
            t += dt;
        }
        const double var_meas = wf.var();

        if (!rel_close(var_meas, var_exact, 0.05)) {
            std::cerr << "[ICNOISE SELFTEST] FAIL IC2a: var_meas=" << var_meas
                      << " var_exact(sum var_k)=" << var_exact << "\n";
            ok = false;
        } else {
            std::cout << "[ICNOISE SELFTEST] PASS IC2a: sample variance matches sum(var_k)\n";
        }

        // ---- IC2b: compare OU variance to target integral (looser) ----
        const double var_target = target_variance(*st);

        if (!rel_close(var_exact, var_target, 0.20)) {
            std::cerr << "[ICNOISE SELFTEST] WARN IC2b: sum(var_k)=" << var_exact
                      << " target integral=" << var_target
                      << " (adjust N/log spacing if you want tighter)\n";
        } else {
            std::cout << "[ICNOISE SELFTEST] PASS IC2b: OU variance close to target band integral\n";
        }
    }

    return ok;
}
#endif