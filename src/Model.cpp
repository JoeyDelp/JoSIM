// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Model.hpp"

#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"
#include "JoSIM/TypeDefines.hpp"

#include <regex>

using namespace JoSIM;

static inline std::string trim_copy(std::string s) {
    auto is_ws = [](unsigned char c) { return std::isspace(c); };
    while (!s.empty() && is_ws((unsigned char) s.front())) { s.erase(s.begin()); }
    while (!s.empty() && is_ws((unsigned char) s.back())) { s.pop_back(); }
    return s;
}

static std::vector<std::string> split_top_level_kv_items(const std::string& s) {
    // Split by whitespace or commas, but ONLY at depth=0 (not inside (...) or {...})
    std::vector<std::string> out;
    std::string              cur;
    int                      paren = 0;
    int                      brace = 0;

    auto                     flush = [&]() {
        auto t = trim_copy(cur);
        if (!t.empty()) { out.push_back(t); }
        cur.clear();
    };

    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];

        if (c == '(') {
            paren++;
            cur.push_back(c);
            continue;
        }
        if (c == ')') {
            if (paren == 0) { Errors::model_errors(ModelErrors::PARAM_PARENTHESIS, s); }
            --paren;
            cur.push_back(c);
            continue;
        }
        if (c == '{') {
            brace++;
            cur.push_back(c);
            continue;
        }
        if (c == '}') {
            if (brace == 0) { Errors::model_errors(ModelErrors::PARAM_PARENTHESIS, s); }
            --brace;
            cur.push_back(c);
            continue;
        }

        const bool top = (paren == 0 && brace == 0);

        if (top && (std::isspace((unsigned char) c) || c == ',')) {
            flush();
            continue;
        }

        cur.push_back(c);
    }
    if (paren != 0 || brace != 0) { Errors::model_errors(ModelErrors::PARAM_PARENTHESIS, s); }
    flush();
    return out;
}

static bool starts_with_ci(const std::string& s, const std::string& pfx) {
    if (s.size() < pfx.size()) { return false; }
    for (size_t i = 0; i < pfx.size(); ++i) {
        if (std::toupper((unsigned char) s[i]) != std::toupper((unsigned char) pfx[i])) { return false; }
    }
    return true;
}

void JoSIM::Model::parse_model(const std::pair<tokens_t, string_o>& s,
                               vector_pair_t<Model, string_o>&      models,
                               const param_map&                     p) {
    if (s.first.size() < 3) {
        Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, Misc::vector_to_string(s.first));
    }

    Model temp;
    temp.modelName(s.first.at(1));

    // Join everything from token[2:] back into one string (preserves any spaces inside lists/functions)
    std::string spec;
    {
        tokens_t tail(s.first.begin() + 2, s.first.end());
        spec = Misc::vector_to_string(tail);
        spec = trim_copy(spec);
    }

    // Must be JJ...
    if (!starts_with_ci(spec, "JJ")) {
        Errors::model_errors(ModelErrors::UNKNOWN_MODEL_TYPE, Misc::vector_to_string(s.first));
    }

    const auto  lp = spec.find('(');
    const auto  rp = spec.rfind(')');
    std::string inside;
    if (lp != std::string::npos && rp != std::string::npos && rp > lp) {
        inside = spec.substr(lp + 1, rp - lp - 1);
        if (!trim_copy(spec.substr(rp + 1)).empty()) { Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, spec); }
    } else if (lp == std::string::npos && rp == std::string::npos) {
        inside = spec.substr(2);
    } else {
        Errors::model_errors(ModelErrors::PARAM_PARENTHESIS, spec);
    }
    inside           = std::regex_replace(inside, std::regex("\\s*=\\s*"), "=");

    // Split into KEY=VALUE items at top-level only
    const auto items = split_top_level_kv_items(inside);

    for (const auto& kv : items) {
        const auto eq = kv.find('=');
        if (eq == std::string::npos) { Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, kv); }

        std::string key = trim_copy(kv.substr(0, eq));
        std::string val = trim_copy(kv.substr(eq + 1));
        if (key.empty() || val.empty()) { Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, kv); }

        // Normalize key to uppercase (consistent with your style)
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return (char) std::toupper(c); });

        if (key == "CPR") {
            // Expect {a,b,c} or a,b,c
            if (!val.empty() && val.front() == '{' && val.back() == '}') { val = val.substr(1, val.size() - 2); }
            std::vector<double> v;
            tokens_t            tvals = Misc::tokenize(val, ",");
            for (auto& tok : tvals) {
                const double x = parse_param(trim_copy(tok), p, s.second);
                if (std::isnan(x)) { Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, "CPR=" + val); }
                v.emplace_back(x);
            }
            temp.cpr(v);
            continue;
        }

        // Numeric params
        const double value = parse_param(val, p, s.second);
        if (std::isnan(value)) { Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, key + "=" + val); }

        if (key == "VG" || key == "VGAP") {
            temp.vg(value);
        } else if (key == "IC" || key == "ICRIT") {
            temp.ic(value);
        } else if (key == "RTYPE") {
            temp.rtype((int64_t) value);
        } else if (key == "RN") {
            temp.rn(value);
        } else if (key == "R0") {
            temp.r0(value);
        } else if (key == "CAP" || key == "C") {
            temp.c(value);
        } else if (key == "T") {
            temp.t(value);
            temp.tDep(true);
        } else if (key == "TC") {
            temp.tc(value);
            temp.tDep(true);
        } else if (key == "DELV") {
            temp.deltaV(value);
        } else if (key == "GWIDTH") {
            if (!std::isfinite(value) || value <= 0.0) {
                Errors::model_errors(ModelErrors::BAD_MODEL_DEFINITION, "GWIDTH must be finite and positive");
            }
            temp.gwidth(value);
        } else if (key == "D") {
            temp.d(value);
            temp.tDep(true);
        } else if (key == "ICFACT" || key == "ICFCT") {
            temp.icFct(value);
        } else if (key == "PHI") {
            temp.phiOff(value);
        } else {
            Errors::model_errors(
                    ModelErrors::PARAM_TYPE_ERROR,
                    Misc::vector_to_string(tokens_t{Misc::vector_to_string(s.first), "\nThe parameter: ", key}));
        }
    }

    models.emplace_back(std::make_pair(temp, s.second));
}

const Model* Model::find_model(const std::string& name, const vector_pair_t<Model, string_o>& models, string_o scope) {
    for (;;) {
        for (const auto& model : models) {
            if (model.first.modelName() == name && model.second == scope) { return &model.first; }
        }
        if (!scope) { return nullptr; }
        scope = parent_parameter_scope(scope);
    }
}
