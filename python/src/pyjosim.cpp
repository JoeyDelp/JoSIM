#include <pybind11/pybind11.h>

namespace py = pybind11;

// Define submodules
namespace pyjosim {

void input(py::module& m);

};

// Define python module
PYBIND11_MODULE(pyjosim, m)
{
  m.doc() = "JoSIM python interface";

  auto input = m.def_submodule("input", "Input related");
  pyjosim::input(input);
}
