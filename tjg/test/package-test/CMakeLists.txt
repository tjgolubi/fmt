cmake_minimum_required(VERSION 3.20)
project(tjg_consumer LANGUAGES CXX)

find_package(tjg CONFIG REQUIRED)
if (TARGET fmt OR TARGET fmt::fmt)
  message(FATAL_ERROR "The TJG package must not require fmt")
endif ()
add_executable(tjg-consumer main.cc)
target_link_libraries(tjg-consumer PRIVATE tjg::tjg)
set_target_properties(tjg-consumer PROPERTIES
  CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
  RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_BINARY_DIR}/bin>")
