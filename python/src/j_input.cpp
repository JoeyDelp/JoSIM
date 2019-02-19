#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <vector>

#include "j_input.h"
#include "j_parser.h"

namespace py = pybind11;

namespace pyjosim {
// using namespace JoSim
void input(py::module &m) {
  py::class_<Transient>(m, "Transient")
      .def(py::init<>())
      .def_readwrite("prstep", &Transient::prstep)
      .def_readwrite("tstop", &Transient::tstop)
      .def_readwrite("tstart", &Transient::tstart)
      .def_readwrite("maxtstep", &Transient::maxtstep)
      .def("simize", &Transient::simsize);

  py::class_<Parameter>(m, "Parameter")
      .def("has_unused_parameters",
           [](Parameter &param) { return !param.unparsedParams.empty(); });

  py::class_<Subcircuit>(m, "Subcircuit");

  py::class_<Netlist>(m, "Netlist");

  py::class_<Input>(m, "Input")
      .def(py::init<>())
      .def("display_help",
           [](Input &input) {
             py::scoped_ostream_redirect cout;
             py::scoped_estream_redirect cerr;
             input.display_help();
           })
      .def("version_info",
           [](Input &input) {
             py::scoped_ostream_redirect cout;
             py::scoped_estream_redirect cerr;
             input.version_info();
           })
      .def("read_input_file",
           [](Input &input) {
             input.read_input_file(input.fileName, input.fileLines);
           })
      .def("split_netlist",
           [](Input &input) {
             input.split_netlist(input.fileLines, input.controls,
                                 input.parameters, input.netlist);
           })
      .def("parse_parameters",
           [](Input &input) {
             if (input.parameters.unparsedParams.size() > 0)
               Parser::parse_parameters(input.parameters.unparsedParams,
                                        input.parameters.parsedParams);
           })
      .def("expand_subcircuits", &Input::expand_subcircuits)
      .def("expand_maindesign", &Input::expand_maindesign)
      .def("parse_arguments",
           [](Input &input, std::vector<std::string> args) {
             // Convert vector of strings to argc, argv
             int argc = args.size() + 1;
             const char *josim_cstr = "JoSIM";
             auto argv = std::make_unique<const char *[]>(argc);

             argv[0] = josim_cstr;
             for (std::size_t i = 0; i < args.size(); i++)
               argv[i + 1] = args[i].c_str();

             input.parse_arguments(argc, argv.get());
           })
      .def_readonly("parameters", &Input::parameters)
      .def_readonly("transient_simulation", &Input::transSim)
      .def_readonly("netlist", &Input::netlist)
      .def_readonly("cli_verbose", &Input::argVerb)
      .def_readonly("cli_analysis", &Input::argAnal)
      .def_readonly("cli_save", &Input::argSave)
      .def_readonly("cli_extention", &Input::argExt);
}

} // namespace pyjosim
