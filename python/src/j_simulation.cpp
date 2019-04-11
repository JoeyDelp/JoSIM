#include <pybind11/pybind11.h>

#include "JoSIM/j_simulation.h"

namespace py = pybind11;

namespace pyjosim {

class null_buffer : public std::streambuf
{
  public:
  int overflow(int c) { return c; }
};

class scoped_cout_null {
  null_buffer null_buffer_;
  std::streambuf* cache_;

public:
  scoped_cout_null() {
    cache_ = std::cout.rdbuf();
    std::cout.rdbuf(std::addressof(null_buffer_));
  }

  ~scoped_cout_null() {
    std::cout.rdbuf(cache_);
  }
};

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
             scoped_cout_null cout;
             simulation.transient_voltage_simulation(input, matrix);
           })
      .def("transient_phase_simulation",
           [](Simulation &simulation, Input &input, Matrix &matrix) {
             scoped_cout_null cout;
             simulation.transient_phase_simulation(input, matrix);
           });
}

} // namespace pyjosim
