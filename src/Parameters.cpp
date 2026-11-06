// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Parameters.hpp"

#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/Misc.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <limits>

using namespace JoSIM;

size_t JoSIM::parameter_start(const tokens_t& tokens, size_t first) {
    for (size_t i = first; i < tokens.size(); ++i) {
        if (tokens[i] == "PARAMS:" || tokens[i].find('=') != std::string::npos
            || (i + 1 < tokens.size() && tokens[i + 1].rfind("=", 0) == 0)) { return i; }
    }
    return tokens.size();
}

std::vector<std::pair<std::string, std::string>> JoSIM::parameter_assignments(const tokens_t& tokens, size_t first) {
    std::vector<std::pair<std::string, std::string>> result;
    if (first >= tokens.size()) { return result; }
    const auto text = Misc::vector_to_string(tokens_t(tokens.begin() + first, tokens.end()));
    auto fail = [&] { Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION, text); };
    auto space = [](unsigned char c) { return std::isspace(c) || c == ','; };
    auto name_char = [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '|' || c == '.'; };
    size_t pos = 0;
    if (text.rfind("PARAMS:", 0) == 0) { pos = 7; }
    while (pos < text.size()) {
        while (pos < text.size() && space(text[pos])) { ++pos; }
        if (pos == text.size()) { break; }
        const auto begin = pos;
        while (pos < text.size() && name_char(text[pos])) { ++pos; }
        const auto name = text.substr(begin, pos - begin);
        while (pos < text.size() && space(text[pos])) { ++pos; }
        if (name.empty() || pos == text.size() || text[pos++] != '=') { fail(); }
        const auto expression = pos;
        std::string brackets;
        while (pos < text.size()) {
            const char c = text[pos];
            if (std::string("({[").find(c) != std::string::npos) { brackets += c; }
            else if (std::string(")}]").find(c) != std::string::npos) {
                if (brackets.empty() || std::string("({[").find(brackets.back()) != std::string(")}]").find(c)) { fail(); }
                brackets.pop_back();
            } else if (brackets.empty() && space(c)) {
                size_t next = pos;
                while (next < text.size() && space(text[next])) { ++next; }
                const auto nextName = next;
                while (next < text.size() && name_char(text[next])) { ++next; }
                const auto nextEnd = next;
                while (next < text.size() && space(text[next])) { ++next; }
                if (nextEnd > nextName && next < text.size() && text[next] == '=') { break; }
            }
            ++pos;
        }
        auto value = text.substr(expression, pos - expression);
        while (!value.empty() && space(value.back())) { value.pop_back(); }
        if (!brackets.empty() || value.find_first_not_of(" \t,") == std::string::npos
            || std::any_of(result.begin(), result.end(), [&](const auto& kv) { return kv.first == name; })) { fail(); }
        result.emplace_back(name, value);
    }
    if (result.empty()) { fail(); }
    return result;
}

string_o JoSIM::parent_parameter_scope(const string_o& scope) {
    if (scope && !scope->empty() && scope->front() == '|') {
        const auto separator = scope->find('|', 1);
        if (separator != std::string::npos) { return scope->substr(separator); }
    }
    return std::nullopt;
}

void JoSIM::create_parameter(const tokens_t& s, param_map& parameters, string_o subc) {
    if (s.size() < 2) { Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION, Misc::vector_to_string(s)); }
    for (const auto& assignment : parameter_assignments(s, 1)) {
        Parameter temp;
        temp.set_expression(assignment.second);
        const ParameterName name(assignment.first, subc);
        if (parameters.count(name)) {
            Errors::parsing_errors(ParsingErrors::EXPRESSION_ARLEADY_DEFINED, Misc::vector_to_string(s));
        }
        parameters[name] = temp;
    }
}

// Possible functions that can be called
const std::vector<std::string> funcs = {"COS",
                                        "SIN",
                                        "TAN",
                                        "ACOS",
                                        "ASIN",
                                        "ATAN",
                                        "COSH",
                                        "SINH",
                                        "TANH",
                                        "ACOSH",
                                        "ASINH",
                                        "ATANH",
                                        "EXP",
                                        "LOG",
                                        "LOG10",
                                        "SQRT",
                                        "CBRT"};

int64_t                        JoSIM::precedence_lvl(const std::string& op) {
    switch (op.at(0)) {
            // + and - are lowest level
        case '+':
        case '-':
            return 1;
            // * and / are higher level
        case '*':
        case '/':
            return 2;
            // ^ (pow) is highest level
        case '^':
            return 3;
            // default (functions) are max level
        default: return 4;
    }
    return 4;
}

double JoSIM::parse_param(const std::string& expr, const param_map& params, string_o subc, bool single) {
    std::string text = expr;
    text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c); }), text.end());
    if (text.empty()) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
    auto uppercase = [](std::string name) {
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
        return name;
    };
    auto parameter = [&](const std::string& name) {
        auto scope = subc;
        const auto upper = uppercase(name);
        for (;;) {
            auto found = params.find(ParameterName(name, scope));
            if (found == params.end() && upper != name) { found = params.find(ParameterName(upper, scope)); }
            if (found != params.end() || !scope) { return found; }
            scope = parent_parameter_scope(scope);
        }
    };
    auto resolved = [&](const Parameter& value) {
        if (!value.get_value()) {
            if (!single) { return std::numeric_limits<double>::quiet_NaN(); }
            Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, expr);
        }
        if (!std::isfinite(value.get_value().value())) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
        return value.get_value().value();
    };
    // Preserve exact whole-parameter lookup, including names supplied directly
    // by embedding callers, before interpreting an expression's operators.
    auto whole = parameter(text);
    if (whole != params.end()) { return resolved(whole->second); }

    struct Token {
        std::string op;
        double value;
    };
    std::vector<Token> output;
    std::vector<std::string> operators;
    bool operand = true;
    auto function = [](const std::string& name) { return std::find(funcs.begin(), funcs.end(), name) != funcs.end(); };
    auto opening = [](const std::string& token) { return token.size() == 1 && std::string("([{").find(token[0]) != std::string::npos; };
    auto priority = [](const std::string& op) { return op == "U-" || op == "U+" ? int64_t(3) : precedence_lvl(op); };
    auto emit = [&] { output.push_back({operators.back(), 0.0}); operators.pop_back(); };
    const std::string delimiters = "+-*/^()[]{}";
    size_t position = 0;
    while (position < text.size()) {
        const char c = text[position];
        if (std::isdigit(static_cast<unsigned char>(c))
            || (c == '.' && position + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[position + 1])))) {
            if (!operand) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
            size_t numeric = 0;
            try { std::stod(text.substr(position), &numeric); }
            catch (const std::exception&) { Errors::misc_errors(MiscErrors::STOD_ERROR, text.substr(position)); }
            // std::stod identifies exponent signs as part of the number. Only
            // subsequent operators delimit its optional engineering/unit suffix.
            size_t end = text.find_first_of(delimiters, position + numeric);
            if (end == std::string::npos) { end = text.size(); }
            const auto literal = text.substr(position, end - position);
            const auto suffix = literal.substr(numeric);
            if ((!suffix.empty() && (suffix.front() == 'E' || suffix.front() == 'e'))
                || std::any_of(suffix.begin(), suffix.end(), [](unsigned char ch) { return !std::isalpha(ch); })) {
                Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr);
            }
            const double value = Misc::modifier(literal);
            if (!std::isfinite(value)) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
            output.push_back({"", value});
            position = end;
            operand = false;
            continue;
        }
        if (c == '(' || c == '[' || c == '{') {
            if (!operand) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
            operators.emplace_back(1, c);
            ++position;
            continue;
        }
        if (c == ')' || c == ']' || c == '}') {
            if (operand) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
            while (!operators.empty() && !opening(operators.back())) { emit(); }
            if (operators.empty()
                || std::string("([{").find(operators.back()[0]) != std::string(")]}").find(c)) {
                Errors::parsing_errors(ParsingErrors::MISMATCHED_PARENTHESIS, expr);
            }
            operators.pop_back();
            if (!operators.empty() && function(operators.back())) { emit(); }
            ++position;
            operand = false;
            continue;
        }
        if (std::string("+-*/^").find(c) != std::string::npos) {
            if (operand) {
                if (c != '+' && c != '-') { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
                // Unary signs bind after exponentiation but before products;
                // pushing them directly also permits a signed exponent.
                operators.push_back(c == '-' ? "U-" : "U+");
            } else {
                const std::string op(1, c);
                while (!operators.empty() && !opening(operators.back())
                       && (priority(operators.back()) > priority(op)
                           || (priority(operators.back()) == priority(op) && c != '^'))) { emit(); }
                operators.push_back(op);
                operand = true;
            }
            ++position;
            continue;
        }
        if (!operand) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
        size_t end = text.find_first_of(delimiters, position);
        if (end == std::string::npos) { end = text.size(); }
        const auto name = text.substr(position, end - position);
        auto found = parameter(name);
        if (found != params.end()) {
            const double value = resolved(found->second);
            if (std::isnan(value)) { return value; }
            output.push_back({"", value});
            operand = false;
        } else {
            const auto canonical = uppercase(name);
            if (function(canonical)) {
                if (end == text.size() || std::string("([{").find(text[end]) == std::string::npos) {
                    Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr);
                }
                operators.push_back(canonical);
            } else if (Misc::string_constant(canonical) != 0.0) {
                output.push_back({"", Misc::string_constant(canonical)});
                operand = false;
            } else {
                if (!single) { return std::numeric_limits<double>::quiet_NaN(); }
                Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, expr);
            }
        }
        position = end;
    }
    if (operand) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
    while (!operators.empty()) {
        if (opening(operators.back())) { Errors::parsing_errors(ParsingErrors::MISMATCHED_PARENTHESIS, expr); }
        emit();
    }
    std::vector<double> values;
    for (const auto& token : output) {
        if (token.op.empty()) { values.push_back(token.value); continue; }
        const bool sign = token.op == "U-" || token.op == "U+";
        const int64_t arity = sign || function(token.op) ? 1 : 2;
        if (values.size() < static_cast<size_t>(arity)) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
        const double right = values.back();
        values.pop_back();
        double result;
        if (sign) { result = token.op == "U-" ? -right : right; }
        else {
            double left = 0.0;
            if (arity == 2) { left = values.back(); values.pop_back(); }
            int64_t consumed = 0;
            result = parse_operator(token.op, left, right, consumed);
            if (consumed != arity) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
        }
        if (!std::isfinite(result)) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
        values.push_back(result);
    }
    if (values.size() != 1) { Errors::parsing_errors(ParsingErrors::INVALID_RPN, expr); }
    return values.back();
}


double JoSIM::parse_operator(const std::string& op, double val1, double val2, int64_t& popCount) {
    if (std::find(funcs.begin(), funcs.end(), op) != funcs.end()) {
        popCount = 1;
        if (op == "SIN") {
            return sin(val2);
        } else if (op == "COS") {
            return cos(val2);
        } else if (op == "TAN") {
            return tan(val2);
        } else if (op == "ASIN") {
            return asin(val2);
        } else if (op == "ACOS") {
            return acos(val2);
        } else if (op == "ATAN") {
            return atan(val2);
        } else if (op == "SINH") {
            return sinh(val2);
        } else if (op == "COSH") {
            return cosh(val2);
        } else if (op == "TANH") {
            return tanh(val2);
        } else if (op == "ASINH") {
            return asinh(val2);
        } else if (op == "ACOSH") {
            return acosh(val2);
        } else if (op == "ATANH") {
            return atanh(val2);
        } else if (op == "EXP") {
            return exp(val2);
        } else if (op == "LOG") {
            return log(val2);
        } else if (op == "LOG10") {
            return log10(val2);
        } else if (op == "SQRT") {
            return sqrt(val2);
        } else if (op == "CBRT") {
            return cbrt(val2);
        }
    } else {
        popCount = 2;
        switch (op.at(0)) {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/': return val1 / val2;
            case '^': return pow(val1, val2);
        }
    }
    return 0.0;
}

void JoSIM::parse_parameters(param_map& parameters) {
    std::vector<ParameterName> names;
    for (const auto& parameter : parameters) { names.push_back(parameter.first); }
    parse_parameters(parameters, names);
}

void JoSIM::parse_parameters(param_map& parameters, const std::vector<ParameterName>& names) {
    std::vector<ParameterName> pending;
    for (const auto& name : names) {
        if (!parameters.at(name).get_value()) { pending.push_back(name); }
    }
    while (!pending.empty()) {
        const auto previous = pending.size();
        pending.erase(std::remove_if(pending.begin(), pending.end(), [&](const ParameterName& name) {
            auto& parameter = parameters.at(name);
            const double value = parse_param(parameter.get_expression(), parameters, name.subcircuit(), false);
            if (std::isnan(value)) { return false; }
            parameter.set_value(value);
            return true;
        }), pending.end());
        if (pending.size() == previous) {
            std::string unknown;
            for (const auto& name : pending) { unknown += name.name() + " " + name.subcircuit().value_or("") + "\n"; }
            Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, unknown);
        }
    }
}

void JoSIM::update_parameters(param_map& parameters) {
    for (auto& parameter : parameters) { parameter.second.reset_value(); }
    parse_parameters(parameters);
}

void JoSIM::expand_inline_parameters(std::pair<tokens_t, string_o>& s, param_map& parameters) {
    int_o oPos, cPos;
    // Loop through all the provided tokens, expanding any parameters
    for (auto i = 0; i < s.first.size(); ++i) {
        // If there exists a opening curly parenthesis, an expression exists
        if (s.first.at(i).find("{") != std::string::npos) { oPos = i; }
        // If there exists a closing curly parenthesis, an expression exists
        if (s.first.at(i).find("}") != std::string::npos) { cPos = i; }
    }
    // If an expression opening was found
    if (oPos) {
        //  Then a closing brace must exist
        if (!cPos) {
            // Complain if it doesn't
            Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION, Misc::vector_to_string(s.first));
        }
        if (cPos.value() != oPos.value()) {
            s.first.at(oPos.value()) = Misc::vector_to_string(
                    tokens_t{s.first.begin() + oPos.value(), s.first.begin() + cPos.value() + 1}, "");
            s.first.erase(s.first.begin() + oPos.value() + 1, s.first.begin() + cPos.value() + 1);
        }
        tokens_t temp  = Misc::tokenize(s.first.at(oPos.value()), "{}", true, true, 2);
        double   value = parse_param(temp.back(), parameters, s.second, false);
        // If the returned value is not NaN
        if (std::isnan(value)) {
            // Complain of invalid parameter expression
            Errors::parsing_errors(ParsingErrors::UNIDENTIFIED_PART, Misc::vector_to_string(s.first));
        }
        // Erase the expression part of the tokens
        s.first.erase(s.first.begin() + oPos.value(), s.first.begin() + oPos.value() + 1);
        if (temp.size() > 1) {
            // Insert the double value in the place of the expression
            s.first.insert(s.first.begin() + oPos.value(),
                           Misc::vector_to_string(tokens_t{temp.front(), Misc::precise_to_string(value)}, ""));
        } else {
            s.first.insert(s.first.begin() + oPos.value(), Misc::precise_to_string(value));
        }
    }
    // If an expression closing was found
    if (cPos) {
        //  Then a opening brace must exist
        if (!oPos) {
            // Complain if it doesn't
            Errors::parsing_errors(ParsingErrors::INVALID_DECLARATION, Misc::vector_to_string(s.first));
        }
    }
}
