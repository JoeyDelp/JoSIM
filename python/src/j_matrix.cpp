#include <pybind11/pybind11.h>

#include "JoSIM/j_matrix.h"

namespace py = pybind11;

namespace pyjosim {

// using namespace JoSim
void matrix(py::module &m) {
  py::class_<Matrix>(m, "Matrix")
      .def(py::init<>())
      .def("create_matrix", &Matrix::create_matrix);
}

} // namespace pyjosim
