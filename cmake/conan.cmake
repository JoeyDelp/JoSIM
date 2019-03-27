option(USE_CONAN_DEPENDENCIES "Use Conan to manage dependencies" ON)

if(USE_CONAN_DEPENDENCIES)
  message("Searching in ${PROJECT_BINARY_DIR}")
  if(EXISTS "${PROJECT_BINARY_DIR}/conanbuildinfo.cmake")
    include(${PROJECT_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
  else()
    message(FATAL_ERROR "conanbuildinfo.cmake is missing!\n"
                        "You must run conan install first")
  endif()
elseif(EXISTS "${PROJECT_BINARY_DIR}/conanbuildinfo.cmake")
  message(WARNING "Ignoring conanbuildinfo.cmake since"
                  "IRON_IO_CONAN_DEPENDENCIES == FALSE")
endif()
