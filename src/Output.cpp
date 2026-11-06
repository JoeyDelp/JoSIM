// Copyright (c) 2025 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)

#include "JoSIM/Output.hpp"

#include "JoSIM/AnalysisType.hpp"
#include "JoSIM/Constants.hpp"
#include "JoSIM/Errors.hpp"
#include "JoSIM/FileOutputType.hpp"
#include "JoSIM/Input.hpp"
#include "JoSIM/ProgressBar.hpp"
#include "JoSIM/Simulation.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>

using namespace JoSIM;

Output::Output(Input& iObj, Matrix& mObj, Simulation& sObj) {
    if (iObj.cli_output_file) { iObj.ensure_safe_output(iObj.cli_output_file.value().name()); }
    for (auto& output : iObj.output_files) { iObj.ensure_safe_output(output.name()); }
    // Write the output in type agnostic format
    write_output(iObj, mObj, sObj);
    // Format the output into the relevant type
    if (iObj.cli_output_file) {
        if (iObj.cli_output_file.value().type() == FileOutputType::Csv) {
            format_csv_or_dat(iObj.cli_output_file.value().name(), ',', iObj.argMin);
        } else if (iObj.cli_output_file.value().type() == FileOutputType::Dat) {
            format_csv_or_dat(iObj.cli_output_file.value().name(), ' ', iObj.argMin);
        } else if (iObj.cli_output_file.value().type() == FileOutputType::Raw) {
            format_raw(iObj.cli_output_file.value().name(), iObj.argMin);
        }
    }
    if (!iObj.output_files.empty()) {
        for (auto i = 0; i < iObj.output_files.size(); ++i) {
            if (iObj.output_files.at(i).type() == FileOutputType::Csv) {
                format_csv_or_dat(iObj.output_files.at(i).name(), ',', iObj.argMin, i);
            } else if (iObj.output_files.at(i).type() == FileOutputType::Dat) {
                format_csv_or_dat(iObj.output_files.at(i).name(), ' ', iObj.argMin, i);
            } else if (iObj.output_files.at(i).type() == FileOutputType::Raw) {
                format_raw(iObj.output_files.at(i).name(), iObj.argMin, i);
            }
        }
    }
    if (!iObj.cli_output_file && iObj.output_files.empty()) { format_cout(iObj.argMin); }
}

void Output::write_output(const Input& iObj, Matrix& mObj, Simulation& sObj) {
    // Shorthand
    auto&   x           = sObj.results.xVector;
    auto&   t           = sObj.results.timeAxis;
    auto&   tran        = iObj.transSim;
    tran.validate_output();
    if (t.empty()) { Errors::control_errors(ControlErrors::TRANS_ERROR, "Output contains no simulation samples"); }
    const auto phase_at = [&](int64_t index, int64_t sample) {
        if (index < 0) { return 0.0; }
        if (sample >= 0) { return x.at(index).value().at(sample); }
        return sObj.results.phaseBefore.at(index).at(static_cast<size_t>(-sample - 1));
    };
    // Create downsampling FIR window, use Hanning function with odd number of taps.
    // The FIR filter is centered around the requested point in time, i. e. looks into the past and future.
    const double window = std::min(tran.prfirwindow(), tran.tstop());
    int64_t halffirsize = static_cast<int64_t>(window <= 0. ? 0 : .5 * window / tran.tstep());
    int64_t firsize     = halffirsize * 2 + 1; // Guarantees odd number of taps or 1 in case of disabled filter.
    std::vector<double> firwindow;
    firwindow.reserve(firsize);
    // Calculate Hanning function 1 - cos(x) with x in ]0; 2*pi[.
    // For firsize == 1 (filter disabled), this results in a firwindow = {1.0},
    //   i. e. a convolution with delta(x), which has no effect.
    double firsum = 0.0;
    for (int64_t k = 0; k < firsize; k++) {
        double hanning  = 1. - cos(((double) k + 0.5) * (2. * Constants::PI / (double) firsize));
        firsum         += hanning;
        firwindow.emplace_back(hanning);
    }
    double firscale = 1. / firsum; // Normalize Hanning function to sum(firwindow) = 1
    for (auto& hanning : firwindow) { hanning = firscale * hanning; }
    // Indices to print
    std::vector<int64_t> result_indices;
    const auto reaches_print_point = [&](double sample, double requested) {
        // Treat roundoff at an exact grid tie consistently. The tolerance is
        // a few ulps of time, not a fraction of the physical timestep.
        const double scale = std::max({std::abs(sample), std::abs(requested), tran.tstep()});
        return sample >= requested - 8 * std::numeric_limits<double>::epsilon() * scale;
    };
    for (auto i = 0; i < t.size(); ++i) {
        if (reaches_print_point(t.at(i), tran.prstart())) {
            result_indices.emplace_back(i);
            break;
        }
    }
    if (result_indices.empty()) {
        Errors::control_errors(ControlErrors::TRANS_ERROR, "Output window contains no simulation samples");
    }
    int64_t print_point = 1;
    double next_print_point = std::fma(static_cast<double>(print_point), tran.prstep(), tran.prstart());
    for (auto i = static_cast<size_t>(result_indices.back()) + 1; i < t.size(); ++i) {
        if (reaches_print_point(t.at(i), next_print_point)) {
            result_indices.emplace_back(i);
            ++print_point;
            next_print_point = std::fma(static_cast<double>(print_point), tran.prstep(), tran.prstart());
        }
    }
    // Create the time trace
    traces.emplace_back("time");
    traces.back().type_ = 'T';
    traces.back().data_.reserve(result_indices.size());
    for (auto i : result_indices) { traces.back().data_.emplace_back(t.at(i)); }
    // Print only the indices of the relevant traces
    int64_t cc = 0;
    if (mObj.relevantTraces.size() != 0) {
        std::vector<std::pair<RelevantTrace, bool>> requests;
        for (const auto& trace : mObj.relevantTraces) {
            requests.emplace_back(trace, false);
            if (trace.subtract) { requests.emplace_back(*trace.subtract, true); }
        }
        // Create progress bar
        ProgressBar bar;
        if (!iObj.argMin) {
            bar.set_bar_width(30);
            bar.fill_bar_progress_with("O");
            bar.fill_bar_remainder_with(" ");
            bar.set_status_text("Formatting Output");
            bar.set_total((float) requests.size());
            bar.create_thread();
        }
        // Reset progress counter
        cc = 0;
        for (const auto& request : requests) {
            const auto& i = request.first;
            auto& st = i.storageType;
            if (!iObj.argMin) { bar.update(static_cast<float>(cc)); }
            // Shorthand for the optional value
            auto i1    = i.index1.has_value() ? i.index1.value() : -1;
            auto i2    = i.index2.has_value() ? i.index2.value() : -1;
            auto vi    = i.variableIndex.has_value() ? i.variableIndex.value() : -1;
            // Size of original data vectors available to FIR filter
            auto xsize = t.size();
            if (i1 != -1 && x.at(i1).value().size() < xsize) { xsize = x.at(i1).value().size(); }
            if (i2 != -1 && x.at(i2).value().size() < xsize) { xsize = x.at(i2).value().size(); }
            if (vi != -1 && x.at(vi).value().size() < xsize) { xsize = x.at(vi).value().size(); }
            // If this is a voltage we are storing
            if (st == StorageType::Voltage) {
                // Set the label for the plot
                traces.emplace_back(i.deviceLabel.value());
                traces.back().fileIndex = i.fIndex;
                traces.back().type_     = 'V';
                traces.back().data_.reserve(result_indices.size());
                // Push the FIR filtered values for each value on the time axis
                for (auto j : result_indices) { // j: Index of window center
                    // FIR filter voltage
                    double accumulator = 0.;
                    for (int64_t k = 0; k < firsize; k++) { // k: Index used for the window
                        int64_t jk = j + halffirsize - k;   // jk: Index used for the data
                        // Sample only valid indices
                        if (jk < 0) {
                            jk = 0;
                        } else if (static_cast<size_t>(jk) >= xsize) {
                            jk = xsize - 1;
                        }
                        // Calculate individual voltage sample
                        double value;
                        // Shorthand values
                        auto   valin1 = i1 != -1 ? x.at(i1).value().at(jk) : 0;
                        auto   valin2 = i2 != -1 ? x.at(i2).value().at(jk) : 0;
                        // If the analysis method was voltage
                        if (iObj.argAnal == AnalysisType::Voltage) {
                            value = valin1 - valin2;
                            // Else calculate the voltage from the phase value
                        } else {
                            if (i.deviceLabel.value().at(3) == 'B' && vi != -1) {
                                value = x.at(vi).value().at(jk);
                            } else {
                                const double valin1n1 = phase_at(i1, jk - 1), valin1n2 = phase_at(i1, jk - 2);
                                const double valin2n1 = phase_at(i2, jk - 1), valin2n2 = phase_at(i2, jk - 2);
                                value = ((3.0 * Constants::SIGMA) / (2.0 * iObj.transSim.tstep()))
                                        * ((valin1 - valin2) - (4.0 / 3.0) * (valin1n1 - valin2n1)
                                           + (1.0 / 3.0) * (valin1n2 - valin2n2));
                            }
                        }
                        accumulator += firwindow.at(k) * value;
                    }
                    traces.back().data_.emplace_back(accumulator);
                }
            } else if (st == StorageType::Phase) {
                // Set the label for the plot
                traces.emplace_back(i.deviceLabel.value());
                traces.back().fileIndex = i.fIndex;
                traces.back().type_     = 'P';
                traces.back().data_.reserve(result_indices.size());
                // If the analysis type is phase
                if (iObj.argAnal == AnalysisType::Phase) {
                    // Push the FIR filtered values for each value on the time axis
                    for (auto j : result_indices) { // j: Index of window center
                        // FIR filter phase
                        double accumulator = 0.;
                        for (int64_t k = 0; k < firsize; k++) { // k: Index used for the window
                            int64_t jk = j + halffirsize - k;   // jk: Index used for the data
                            // Sample only valid indices
                            if (jk < 0) {
                                jk = 0;
                            } else if (static_cast<size_t>(jk) >= xsize) {
                                jk = xsize - 1;
                            }
                            // Calculate individual phase sample
                            auto valin1  = i1 != -1 ? x.at(i1).value().at(jk) : 0;
                            auto valin2  = i2 != -1 ? x.at(i2).value().at(jk) : 0;
                            auto value   = valin1 - valin2;
                            accumulator += firwindow.at(k) * value;
                        }
                        traces.back().data_.emplace_back(accumulator);
                    }
                    // Else if the analysis type was voltage
                } else {
                    if (i.deviceLabel.value().at(3) == 'B' && vi != -1) {
                        // Push the FIR filtered values for each value on the time axis
                        for (auto j : result_indices) { // j: Index of window center
                            // FIR filter phase
                            double accumulator = 0.;
                            for (int64_t k = 0; k < firsize; k++) { // k: Index used for the window
                                int64_t jk = j + halffirsize - k;   // jk: Index used for the data
                                // Sample only valid indices
                                if (jk < 0) {
                                    jk = 0;
                                } else if (static_cast<size_t>(jk) >= xsize) {
                                    jk = xsize - 1;
                                }
                                // Get individual phase sample
                                auto value   = x.at(vi).value().at(jk);
                                accumulator += firwindow.at(k) * value;
                            }
                            traces.back().data_.emplace_back(accumulator);
                        }
                    } else {
                        // Calculate the phase for all points in time.
                        // This is required for proper integration in case of voltage analysis.
                        // It takes extra memory and time, so do it only if necessary.
                        std::vector<double> vectphase;
                        vectphase.reserve(xsize);
                        double phaseN1 = phase_at(i1, -1) - phase_at(i2, -1);
                        double phaseN2 = phase_at(i1, -2) - phase_at(i2, -2);
                        for (size_t j = 0; j < xsize; j++) {
                            auto valin1 = i1 != -1 ? x.at(i1).value().at(j) : 0;
                            auto valin2 = i2 != -1 ? x.at(i2).value().at(j) : 0;
                            auto value  = ((2.0 * iObj.transSim.tstep()) / (3.0 * Constants::SIGMA)) * (valin1 - valin2)
                                         + (4.0 / 3.0) * (phaseN1) - (1.0 / 3.0) * (phaseN2);
                            phaseN2 = phaseN1;
                            phaseN1 = value;
                            vectphase.emplace_back(value);
                        }
                        // Push the FIR filtered values for each value on the time axis
                        for (auto j : result_indices) { // j: Index of window center
                            // FIR filter phase
                            double accumulator = 0.;
                            for (int64_t k = 0; k < firsize; k++) { // k: Index used for the window
                                int64_t jk = j + halffirsize - k;   // jk: Index used for the data
                                // Sample only valid indices
                                if (jk < 0) {
                                    jk = 0;
                                } else if (static_cast<size_t>(jk) >= xsize) {
                                    jk = xsize - 1;
                                }
                                // Get individual phase sample
                                auto value   = vectphase.at(jk);
                                accumulator += firwindow.at(k) * value;
                            }
                            traces.back().data_.emplace_back(accumulator);
                        }
                    }
                }
            } else if (st == StorageType::Current) {
                traces.emplace_back(i.deviceLabel.value());
                traces.back().fileIndex = i.fIndex;
                traces.back().type_     = 'I';
                traces.back().data_.reserve(result_indices.size());
                // Push the FIR filtered values for each value on the time axis
                for (auto j : result_indices) { // j: Index of window center
                    // FIR filter current
                    double accumulator = 0.;
                    for (int64_t k = 0; k < firsize; k++) { // k: Index used for the window
                        int64_t jk = j + halffirsize - k;   // jk: Index used for the data
                        // Sample only valid indices
                        // Also apply index clipping to sourcegen in order to filter everything the same.
                        if (jk < 0) {
                            jk = 0;
                        } else if (static_cast<size_t>(jk) >= xsize) {
                            jk = xsize - 1;
                        }
                        // Get individual current sample
                        double value;
                        if (!i.sourceIndex) {
                            value = x.at(i1).value().at(jk);
                        } else {
                            value = sObj.results.sourceCurrents.at(i.sourceIndex.value()).value().at(jk);
                        }
                        accumulator += firwindow.at(k) * value;
                    }
                    traces.back().data_.emplace_back(accumulator);
                }
            }
            if (request.second) {
                auto& first = traces.at(traces.size() - 2).data_;
                const auto& second = traces.back().data_;
                for (size_t sample = 0; sample < first.size(); ++sample) { first.at(sample) -= second.at(sample); }
                traces.pop_back();
            }
            ++cc;
        }
        if (!iObj.argMin) {
            bar.complete();
            std::cout << "\n";
        }
    } else {
        ProgressBar bar;
        if (!iObj.argMin) {
            bar.set_bar_width(30);
            bar.fill_bar_progress_with("O");
            bar.fill_bar_remainder_with(" ");
            bar.set_status_text("Formatting Output");
            bar.set_total((float) mObj.nm.size());
            bar.create_thread();
        }
        int64_t cc = 0;
        for (const auto& i : mObj.nm) {
            if (!iObj.argMin) { bar.update(static_cast<float>(cc)); }
            if (iObj.argAnal == AnalysisType::Voltage) {
                traces.emplace_back("V(" + i.first + ")");
                traces.back().type_ = 'V';
            } else {
                traces.emplace_back("P(" + i.first + ")");
                traces.back().type_ = 'P';
            }
            // FIR filter, then push into trace
            auto xsize = x.at(i.second).value().size();
            for (auto j : result_indices) {
                double accumulator = 0.;
                for (int64_t k = 0; k < firsize; k++) {
                    int64_t jk = j + halffirsize - k;
                    // Sample only valid indices
                    if (jk < 0) {
                        jk = 0;
                    } else if (static_cast<size_t>(jk) >= xsize) {
                        jk = xsize - 1;
                    }
                    accumulator += firwindow.at(k) * x.at(i.second).value().at(jk);
                }
                traces.back().data_.emplace_back(accumulator);
            }
            ++cc;
        }
        if (!iObj.argMin) {
            bar.update(100);
            bar.complete();
            std::cout << "\n";
        }
    }
    // Finite solved unknowns can overflow during conversion, filtering or subtraction.
    // Reject the complete result before any destination is opened or truncated.
    for (const auto& trace : traces) {
        for (size_t sample = 0; sample < trace.data_.size(); ++sample) {
            if (!std::isfinite(trace.data_.at(sample))) {
                Errors::output_errors(OutputErrors::NONFINITE_TRACE,
                    trace.name_ + " at output sample " + std::to_string(sample));
            }
        }
    }
}

void Output::format_csv_or_dat(const std::string& filename, const char& delimiter, bool argmin, int64_t fIndex) {
    std::vector<int64_t> tIndices = {0};
    for (auto i = 1; i < traces.size(); ++i) {
        if (traces.at(i).fileIndex == fIndex || fIndex == -1) { tIndices.emplace_back(i); }
    }
    std::ofstream outfile(filename);
    outfile << std::setprecision(15);
    if (outfile.is_open()) {
        for (auto i = 0; i < tIndices.size() - 1; ++i) { outfile << traces.at(tIndices.at(i)).name_ << delimiter; }
        outfile << traces.at(tIndices.at(tIndices.size() - 1)).name_ << "\n";
        ProgressBar bar;
        if (!argmin) {
            bar.set_bar_width(30);
            bar.fill_bar_progress_with("O");
            bar.fill_bar_remainder_with(" ");
            bar.set_status_text("Writing Output");
            bar.set_total((float) traces.at(0).data_.size());
            bar.create_thread();
        }
        for (auto j = 0; j < traces.at(0).data_.size(); ++j) {
            if (!argmin) { bar.update(static_cast<float>(j)); }
            for (auto i = 0; i < tIndices.size() - 1; ++i) {
                outfile << std::scientific << std::setprecision(6) << traces.at(tIndices.at(i)).data_.at(j)
                        << delimiter;
            }
            outfile << std::scientific << std::setprecision(6)
                    << traces.at(tIndices.at(tIndices.size() - 1)).data_.at(j) << "\n";
        }
        outfile.close();
        if (!outfile) { Errors::output_errors(OutputErrors::WRITE_FAILED, filename); }
        if (!argmin) {
            bar.complete();
            std::cout << "\n\n";
        }
    } else {
        Errors::output_errors(OutputErrors::CANNOT_OPEN_FILE, filename);
    }
}

// Writes the output to a standard spice raw file
void Output::format_raw(const std::string& filename, bool argmin, int64_t fIndex) {
    std::vector<int64_t> tIndices = {0};
    for (auto i = 1; i < traces.size(); ++i) {
        if (traces.at(i).fileIndex == fIndex || fIndex == -1) { tIndices.emplace_back(i); }
    }
    // Variable to store the total number of points to be saved
    int64_t       loopsize = 0;
    // Opens an output stream with provided file name
    std::ofstream outfile(filename);
    // Set the output presicion
    outfile << std::setprecision(6);
    // Ensure that the file could be opened
    if (outfile.is_open()) {
        // If the traces are not empty
        if (!traces.empty()) {
            // Write the rawfile preamble
            outfile << "Title: " << std::filesystem::path(filename).stem() << "\n";
            std::time_t result = std::time(nullptr);
            outfile << "Date: " << std::asctime(std::localtime(&result));
            outfile << "Plotname: Transient Analysis\n";
            outfile << "Flags: real\n";
            outfile << "No. Variables: " << tIndices.size() << "\n";
            loopsize = traces.at(0).data_.size();
            outfile << "No. Points: " << loopsize << "\n";
            outfile << "Command: JoSIM v" << VERSION << "\n";
            outfile << "Variables:\n";
            outfile << " 0 time Seconds\n";
            // Determine the variable name and type in a format acceptable for raw
            for (auto i = 1; i < tIndices.size(); ++i) {
                // Make a copy of the name so we can alter it
                std::string name = traces.at(tIndices.at(i)).name_;
                // Erase all the double quote characters
                name.erase(std::remove(name.begin(), name.end(), '\"'), name.end());
                // If this is a voltage variable
                if (traces.at(tIndices.at(i)).type_ == 'V') {
                    outfile << " " << i << " " << name << " Voltage\n";
                    // If this is a phase variable
                } else if (traces.at(tIndices.at(i)).type_ == 'P') {
                    outfile << " " << i << " " << name << " Phase\n";
                    // If this is a current variable
                } else if (traces.at(tIndices.at(i)).type_ == 'I') {
                    std::replace(name.begin(), name.end(), '|', '.');
                    name = name.substr(2);
                    name = name.substr(0, name.size() - 1);
                    // Append '#branch' since currents always flow in branches
                    outfile << " " << i << " " << name << "#branch Current\n";
                }
            }
            // Start filling the values
            outfile << "Values:\n";
            int64_t     pointSizeSpacing = std::to_string(loopsize).length() + 1;
            ProgressBar bar;
            if (!argmin) {
                bar.set_bar_width(30);
                bar.fill_bar_progress_with("O");
                bar.fill_bar_remainder_with(" ");
                bar.set_status_text("Writing Output");
                bar.set_total((float) loopsize);
                bar.create_thread();
            }
            for (int64_t i = 0; i < loopsize; ++i) {
                if (!argmin) { bar.update(static_cast<float>(i)); }
                // Point and time value
                outfile << std::left << std::setw(pointSizeSpacing) << i << traces.at(0).data_.at(i) << "\n";
                // Fill in rest of variable values
                for (auto j = 1; j < tIndices.size(); ++j) {
                    outfile << std::left << std::setw(pointSizeSpacing) << "" << traces.at(tIndices.at(j)).data_.at(i)
                            << "\n";
                }
            }
            if (!argmin) {
                bar.complete();
                std::cout << "\n\n";
            }
            // If the traces were empty (aka nothing to plot)
        } else if (traces.empty()) {
            // Complain about it
            Errors::output_errors(OutputErrors::NOTHING_SPECIFIED);
        }
        outfile.close();
        if (!outfile) { Errors::output_errors(OutputErrors::WRITE_FAILED, filename); }
        // If the file could not be opened for writing
    } else {
        // Complain about it
        Errors::output_errors(OutputErrors::CANNOT_OPEN_FILE, filename);
    }
}

void Output::format_cout(const bool& argMin) {
    if (!argMin) {
        for (auto i = 0; i < traces.size() - 1; ++i) { std::cout << traces.at(i).name_ << " "; }
        std::cout << traces.at(traces.size() - 1).name_ << "\n";
        for (auto j = 0; j < traces.at(0).data_.size(); ++j) {
            for (auto i = 0; i < traces.size() - 1; ++i) {
                std::cout << std::setw(15) << std::scientific << std::setprecision(6) << traces.at(i).data_.at(j)
                          << " ";
            }
            std::cout << std::setw(15) << std::scientific << std::setprecision(6)
                      << traces.at(traces.size() - 1).data_.at(j) << "\n";
        }
        std::cout.flush();
        if (!std::cout) { Errors::output_errors(OutputErrors::WRITE_FAILED, "standard output"); }
    }
}
