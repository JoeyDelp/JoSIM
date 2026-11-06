include(${CMAKE_CURRENT_LIST_DIR}/CPM.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dependency_sources.cmake)

foreach(dependency IN LISTS JOSIM_DEPENDENCIES)
  CPMAddPackage(
    NAME ${dependency}
    VERSION 0.1.0
    URL ${JOSIM_${dependency}_URL}
    URL_HASH SHA256=${JOSIM_${dependency}_SHA256}
  )
endforeach()

# clock_gettime is supplied by librt on glibc versions before 2.17.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  get_target_property(josim_config_target suitesparse::config ALIASED_TARGET)
  target_link_libraries(${josim_config_target} INTERFACE rt)
endif()
