// Copyright (c) 2026 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
// This is a set of self-test functions meant to only run in debug mode
#include <cstdlib>
#include <cstdint>
#include <cmath>
#ifndef NDEBUG
namespace JoSIM {
    struct PNoiseState;
//    class Input;
}

struct Welford {
    long long n = 0;
    double mean = 0.0;
    double M2 = 0.0;
    void add(double x) {
        ++n;
        double d = x - mean;
        mean += d / n;
        double d2 = x - mean;
        M2 += d * d2;
    }
    double var() const { return (n > 1) ? (M2 / (n - 1)) : 0.0; }
};

double analytic_Sx(const JoSIM::PNoiseState& st, double f_hz);

double target_variance(const JoSIM::PNoiseState& st);

bool rel_close(double a, double b, double rel);

bool close_abs_rel(double a, double b, double rel, double abs);

// Welford (stable stddev)
void welford_update(double x, uint64_t n, double& mean, double& m2);

int josim_debug_selftest_noise_rms();


//bool josim_debug_selftest_pnoise(const JoSIM::Input& iObj);
bool josim_debug_selftest_pnoise();

bool josim_debug_selftest_fnoise();

bool josim_debug_selftest_icnoise();
#endif