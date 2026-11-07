# Development-only instrumentation. Release builds leave both options disabled.
option(JOSIM_COVERAGE "Instrument first-party C++ targets for LLVM coverage" OFF)
option(JOSIM_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(JOSIM_COVERAGE AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR "JOSIM_COVERAGE requires Clang and matching LLVM coverage tools")
endif()
if(JOSIM_SANITIZERS)
  if(MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "JOSIM_SANITIZERS requires a GCC/Clang sanitizer toolchain")
  endif()
  # Include the C solver dependencies in memory/undefined-behaviour checks.
  add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all
                      -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined -fno-sanitize-recover=all)
endif()

function(josim_instrument_target target)
  if(JOSIM_COVERAGE)
    target_compile_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    # Static-library consumers, including the fresh installed consumer, need
    # the matching instrumentation runtime at their final link step.
    target_link_options(${target} PUBLIC -fprofile-instr-generate)
  endif()
  if(JOSIM_SANITIZERS)
    target_compile_options(${target} PUBLIC -fsanitize=address,undefined
                           -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(${target} PUBLIC -fsanitize=address,undefined
                        -fno-sanitize-recover=all)
  endif()
endfunction()
