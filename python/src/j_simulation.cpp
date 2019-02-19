#include <pybind11/pybind11.h>

#include "j_simulation.h"

namespace py = pybind11;

namespace pyjosim {

// using namespace JoSim
void simulation(py::module &m) {
  py::class_<Simulation>(m, "Simulation")
      .def(py::init<>())
      .def("identify_simulation",
           [](Simulation &simulation, Input &input) {
             simulation.identify_simulation(
                 input.controls, input.transSim.prstep, input.transSim.tstop,
                 input.transSim.tstart, input.transSim.maxtstep);
           })
      .def("transient_voltage_simulation",
           [](Simulation &simulation, Input &input, Matrix &matrix) {
             simulation.transient_voltage_simulation(input, matrix);
           })
      .def("transient_phase_simulation",
           [](Simulation &simulation, Input &input, Matrix &matrix) {
             simulation.transient_phase_simulation(input, matrix);
           });
}

} // namespace pyjosim
