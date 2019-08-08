function(make_target_static target)
  if (NOT MAKING_STATIC_BUILD)
    return()
  endif()

  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # TODO OSX is special handle later
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # using Clang
    # TODO clang
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # using GCC
    target_compile_options(${target} PRIVATE -static-libgcc -static-libstdc++)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # using Intel C++
    # TODO test if this actually works
    target_compile_options(${target} PRIVATE -static-libgcc -static-libstdc++)
  elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # using Visual Studio C++
    # TODO figure out what one should do
  endif()

endfunction()
