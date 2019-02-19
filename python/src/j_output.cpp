#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>

#include "j_output.h"

namespace py = pybind11;

namespace pyjosim {

// using namespace JoSim
void output(py::module &m) {
  py::class_<Output>(m, "Output")
      .def(py::init<>())
      .def("relevant_traces", &Output::relevant_traces)
      .def("write_csv", &Output::write_data)
      .def("write_dat", &Output::write_legacy_data)
      .def("write_wr", &Output::write_wr_data)
      .def("write_stdout",
           [](Output &output, Matrix &matrix, Simulation &simulation) {
             py::scoped_ostream_redirect cout;
             py::scoped_estream_redirect cerr;
             output.write_cout(matrix, simulation);
           });
}

} // namespace pyjosim
