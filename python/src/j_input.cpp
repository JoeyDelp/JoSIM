#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <vector>

#include "JoSIM/j_input.h"
#include "JoSIM/j_parser.h"

#include "JoSIM/CliOptions.hpp"

namespace py = pybind11;

namespace pyjosim {
// using namespace JoSim
void input(py::module &m) {
  using namespace JoSIM;

  py::class_<JoSIM::CliOptions>(m, "CliOptions")
      .def(py::init([](std::vector<std::string> args) {
        // In case of help
        py::scoped_ostream_redirect cout;
        py::scoped_estream_redirect cerr;

        // Convert vector of strings to argc, argv
        int argc = args.size() + 1;
        const char *josim_cstr = "JoSIM";
        auto argv = std::make_unique<const char *[]>(argc);

        argv[0] = josim_cstr;
        for (std::size_t i = 0; i < args.size(); i++)
          argv[i + 1] = args[i].c_str();

        return JoSIM::CliOptions::parse(argc, argv.get());
      }))
      .def_static("display_help",
                  []() {
                    py::scoped_ostream_redirect cout;
                    py::scoped_estream_redirect cerr;
                    JoSIM::CliOptions::display_help();
                  })
      .def_static("version_info",
                  []() {
                    py::scoped_ostream_redirect cout;
                    py::scoped_estream_redirect cerr;
                    JoSIM::CliOptions::version_info();
                  })
      .def_readonly("cir_file_name", &JoSIM::CliOptions::cir_file_name)
      .def_readonly("output_to_file", &JoSIM::CliOptions::output_to_file)
      .def_readonly("output_file_name", &JoSIM::CliOptions::output_file_name)
      .def_readonly("output_file_type", &JoSIM::CliOptions::output_file_type)
      .def_readonly("input_type", &JoSIM::CliOptions::input_type)
      .def_readonly("analysis_type", &JoSIM::CliOptions::analysis_type)
      .def_readonly("analysis_plot", &JoSIM::CliOptions::parallel)
      .def_readonly("verbose", &JoSIM::CliOptions::verbose)
      .def_readonly("plot", &JoSIM::CliOptions::plot);

  py::class_<Transient>(m, "Transient")
      .def(py::init<>())
      .def_readwrite("prstep", &Transient::prstep)
      .def_readwrite("tstop", &Transient::tstop)
      .def_readwrite("tstart", &Transient::tstart)
      .def_readwrite("maxtstep", &Transient::maxtstep)
      .def("simize", &Transient::simsize);

  py::class_<Parameters>(m, "Parameter")
      .def("has_unused_parameters",
           [](Parameters &param) { return !param.unparsedParams.empty(); });

  py::class_<Subcircuit>(m, "Subcircuit");

  py::class_<Netlist>(m, "Netlist");

  py::enum_<AnalysisType>(m, "AnalysisType")
      .value("Voltage", AnalysisType::Voltage)
      .value("Phase", AnalysisType::Phase);

  py::enum_<FileOutputType>(m, "FileOutputType")
      .value("Csv", FileOutputType::Csv)
      .value("WrSpice", FileOutputType::WrSpice)
      .value("Dat", FileOutputType::Dat);

  py::enum_<InputType>(m, "InputType")
      .value("Jsim", InputType::Jsim)
      .value("WrSpice", InputType::WrSpice);

  py::class_<Input>(m, "Input")
      .def(py::init<JoSIM::AnalysisType, JoSIM::InputType, bool>())
      .def("read_input_file",
           [](Input &input, std::string cir_file_name) {
             input.read_input_file(cir_file_name, input.fileLines);
           })
      .def("split_netlist",
           [](Input &input) {
             input.split_netlist(input.fileLines, input.controls,
                                 input.parameters, input.netlist);
           })
      .def("parse_parameters",
           [](Input &input) {
             if (input.parameters.unparsedParams.size() > 0)
               Parser::parse_parameters(input.parameters);
           })
      .def("expand_subcircuits", &Input::expand_subcircuits)
      .def("expand_maindesign", &Input::expand_maindesign)
      .def_readonly("parameters", &Input::parameters)
      .def_readonly("transient_simulation", &Input::transSim)
      .def_readonly("netlist", &Input::netlist)
      .def_readonly("cli_verbose", &Input::argVerb)
      .def_readonly("cli_analysis", &Input::argAnal);
}

} // namespace pyjosim
