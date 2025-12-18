// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Rng.hpp"

#include "JoSIM/Constants.hpp"

#include <chrono>

namespace JoSIM {

static uint64_t mix_seed_(uint64_t x) {
    // Simple splitmix64-style mixing
    x += 0x9e3779b97f4a7c15ULL;
    x  = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x  = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

void Rng::start_run(uint64_t seed) {
    seed_                  = seed;

    // Derive independent stream seeds from base seed.
    const uint64_t sNoise  = mix_seed_(seed ^ 0x4E4F495345ULL);   // "NOISE"
    const uint64_t sSpread = mix_seed_(seed ^ 0x535052454144ULL); // "SPREAD"

    std::seed_seq  seqN{uint32_t(sNoise), uint32_t(sNoise >> 32), 0x4E4F4953u};
    std::seed_seq  seqS{uint32_t(sSpread), uint32_t(sSpread >> 32), 0x53505244u};

    noise_.seed(seqN);
    spread_.seed(seqS);

    seeded_ = true;
}

void Rng::start_run_auto() {
    std::random_device rd;
    uint64_t           s  = (uint64_t(rd()) << 32) ^ uint64_t(rd());
    s                    ^= uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    start_run(s);
}

void Rng::rewind() {
    if (!seeded_) { start_run_auto(); }
    start_run(seed_); // re-seed both engines from the same base seed
}

uint64_t Rng::base_seed() {
    if (!seeded_) { start_run_auto(); }
    return seed_;
}

std::mt19937_64& Rng::noise() {
    if (!seeded_) { start_run_auto(); }
    return noise_;
}

std::mt19937_64& Rng::spread() {
    if (!seeded_) { start_run_auto(); }
    return spread_;
}

double Rng::normal01_(std::mt19937_64& eng, BMState& st) {
    if (st.hasSpare) {
        st.hasSpare = false;
        return st.spare;
    }

    // Use generate_canonical for deterministic [0,1)
    double u1 = std::generate_canonical<double, 64>(eng);
    double u2 = std::generate_canonical<double, 64>(eng);

    // avoid log(0)
    if (u1 <= 0.0) { u1 = std::numeric_limits<double>::min(); }

    const double r  = std::sqrt(-2.0 * std::log(u1));
    const double t  = 2.0 * Constants::PI * u2;

    const double z0 = r * std::cos(t);
    const double z1 = r * std::sin(t);

    st.spare        = z1;
    st.hasSpare     = true;
    return z0;
}

double Rng::normal01_noise() {
    static thread_local BMState st;
    return normal01_(noise(), st);
}

double Rng::normal_spread(double mean, double sigma) {
    if (sigma == 0.0) { return mean; }

    BMState st; // local is fine; spreads are drawn rarely
    return mean + sigma * normal01_(spread(), st);
}

} // namespace JoSIM
