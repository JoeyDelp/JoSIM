// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_RNG_HPP
#define JOSIM_RNG_HPP

#include <cstdint>
#include <random>

namespace JoSIM {

class Rng {
  public:
    static void             start_run_auto();
    static void             start_run(uint64_t seed);
    static void             rewind();
    static uint64_t         base_seed();

    // Separate deterministic streams
    static std::mt19937_64& noise();
    static std::mt19937_64& spread();

    // Deterministic N(0,1) from the noise stream (Box–Muller).
    static double           normal01_noise();

    // Deterministic N(mean, sigma) from the spread stream.
    static double           normal_spread(double mean, double sigma);

  private:
    struct BMState {
        bool   hasSpare = false;
        double spare    = 0.0;
    };

    static double                 normal01_(std::mt19937_64& eng, BMState& st);

    static inline bool            seeded_ = false;
    static inline uint64_t        seed_   = 0;
    static inline std::mt19937_64 noise_;
    static inline std::mt19937_64 spread_;
};

} // namespace JoSIM

#endif // JOSIM_RNG_HPP
