find_package(pybind11 REQUIRED)

pybind11_add_module(pyjosim SHARED
  src/j_input.cpp
  src/pyjosim.cpp
)

target_link_libraries(pyjosim PRIVATE
  josim
)
