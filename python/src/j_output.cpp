#include <pybind11/iostream.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include "JoSIM/j_output.h"

namespace py = pybind11;

PYBIND11_MAKE_OPAQUE(std::vector<Trace>);

namespace pyjosim {

// using namespace JoSim
void output(py::module &m) {
  py::class_<Trace>(m, "Trace")
      .def_readonly("name", &Trace::name)
      .def("get_data", [](Trace &trace) {
        std::vector<double> &vec = [&]() -> std::vector<double> & {
          if (trace.pointer)
            return *trace.traceData;
          else
            return trace.calcData;
        }();

        double *data = vec.data();
        size_t size = vec.size();
        return py::array_t<double>(size, data);
      });

  py::bind_vector<std::vector<Trace>>(m, "VectorTrace");

  py::class_<Output>(m, "Output")
      .def(py::init<>())
      .def("relevant_traces", &Output::relevant_traces, py::keep_alive<1, 2>(),
           py::keep_alive<1, 3>(), py::keep_alive<1, 4>())
      .def("write_csv", &Output::write_data)
      .def("write_dat", &Output::write_legacy_data)
      .def("write_wr", &Output::write_wr_data)
      .def("write_stdout",
           [](Output &output, Matrix &matrix, Simulation &simulation) {
             py::scoped_ostream_redirect cout;
             py::scoped_estream_redirect cerr;
             output.write_cout(matrix, simulation);
           })
      .def("get_timesteps",
           [](Output &output) {
             double *data = output.timesteps->data();
             size_t size = output.timesteps->size();
             return py::array_t<double>(size, data);
           },
           py::keep_alive<0, 1>())
      .def("get_traces",
           [](Output &output) {
             return output.traces;
           },
           py::return_value_policy::reference_internal);
}

} // namespace pyjosim
