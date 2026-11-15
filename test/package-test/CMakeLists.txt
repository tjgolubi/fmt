cmake_minimum_required(VERSION 3.20)
project(tjg_consumer LANGUAGES CXX)

find_package(tjg CONFIG REQUIRED)
if (TARGET fmt OR TARGET fmt::fmt)
  message(FATAL_ERROR "The TJG package must not require fmt")
endif ()
get_target_property(tjg_features tjg::tjg INTERFACE_COMPILE_FEATURES)
if (NOT "cxx_std_23" IN_LIST tjg_features)
  message(FATAL_ERROR "The TJG package must propagate C++23")
endif ()
add_executable(tjg-consumer main.cc)
target_link_libraries(tjg-consumer PRIVATE tjg::tjg)
if (CMAKE_SYSTEM_NAME STREQUAL "MSYS" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # libstdc++ <print> needs stdio declarations hidden by strict C++ mode on MSYS.
  target_compile_definitions(tjg-consumer PRIVATE _DEFAULT_SOURCE)
endif ()
set_target_properties(
  tjg-consumer
  PROPERTIES CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF
             RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_BINARY_DIR}/bin>")
