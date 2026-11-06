// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_FUNCTION_HPP
#define JOSIM_FUNCTION_HPP

#include "Input.hpp"

#include <optional>
#include <vector>

namespace JoSIM {

enum class FunctionType {
    PWL      = 0,
    PULSE    = 1,
    SINUSOID = 2,
    CUS      = 3,
    NOISE    = 4,
    PWS      = 5,
    DC       = 6,
    EXP      = 7,
    PNOISE   = 8,
    FNOISE   = 9,
    ICNOISE  = 10
};

struct PNoiseState {
    // User parameters
    double              A1    = 0.0; // ASD at 1 Hz [units/sqrt(Hz)]
    double              alpha = 1.0; // exponent
    double              td    = 0.0; // start delay [s]
    double              dt    = 0.0; // noise update interval [s]
    double              tcorr = 0.0; // correlation time [s]
    int                 N     = 48;  // number of OU processes

    // Derived band limits
    double              fL    = 0.0; // ~ 1/tcorr
    double              fH    = 0.0; // ~ 1/(2*dt)

    // OU bank (all length N)
    std::vector<double> tau; // time constants [s]
    std::vector<double> a;   // exp(-dt/tau)
    std::vector<double> b;   // sqrt(var*(1-a^2))
    std::vector<double> var; // stationary variances
    std::vector<double> y;   // current states

    // Helpful for debug/verification
    double              scale = 1.0; // variance scaling applied to base weights
    double              S1Hz  = 0.0; // continuous-OU diagnostic PSD at 1 Hz, before sampling/interpolation
};

class Function {
  private:
    FunctionType               fType_ = FunctionType::PWL;
    std::vector<double>        timeValues_;
    std::vector<double>        ampValues_;
    std::vector<double>        miscValues_;
    std::optional<PNoiseState> pnoise_;

    void                       parse_pwl(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       parse_pulse(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       parse_sin(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       parse_cus(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       parse_noise(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       initialize_pnoise(PNoiseState& state);
    void                       parse_pnoise(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       parse_fnoise(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       parse_icnoise(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       parse_dc(const tokens_t& t, const Input& iObj, const string_o& s);
    void                       parse_exp(const tokens_t& t, const Input& iObj, const string_o& s);
    double                     return_pwl(double& x);
    double                     return_pulse(double& x);
    double                     return_sin(double& x);
    double                     return_cus(double& x);
    double                     return_noise(double& x);
    double                     return_pnoise(double& x);
    double                     return_fnoise(double& x);
    double                     return_icnoise(double& x);
    double                     return_pws(double& x);
    double                     return_dc();
    double                     return_exp(double& x);

  public:
    Function() {};
    void                       parse_function(const std::string& str, const Input& iObj, const string_o& subckt);
    double                     value(double x);
    void                       ampValues(std::vector<double> values);

    std::vector<double>&       ampValuesRef() { return ampValues_; }

    const std::vector<double>& ampValues() const { return ampValues_; }

    void                       clearMisc() { miscValues_.clear(); }



}; // class Function

} // namespace JoSIM

#endif // JOSIM_FUNCTION_HPP
