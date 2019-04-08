#include <pybind11/pybind11.h>

namespace py = pybind11;

// Define submodules
namespace pyjosim {

void input(py::module &m);
void matrix(py::module &m);
void output(py::module &m);
void simulation(py::module &m);

}; // namespace pyjosim

// Define python module
PYBIND11_MODULE(pyjosim, m) {
  m.doc() = "JoSIM python interface";

  auto input = m.def_submodule("input", "Input related");
  auto matrix = m.def_submodule("matrix", "Matrix related");
  auto simulation = m.def_submodule("simulation", "Simulation related");
  auto output = m.def_submodule("output", "Output related");

  pyjosim::input(input);
  pyjosim::matrix(matrix);
  pyjosim::simulation(simulation);
  pyjosim::output(output);
}
