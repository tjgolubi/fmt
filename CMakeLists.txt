cmake_minimum_required(VERSION 3.24)
project(FMT VERSION 12.2.1 LANGUAGES CXX)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
  message(FATAL_ERROR "This C++23 fmt fork supports GCC and Clang only")
endif()
if(DEFINED CMAKE_CXX_STANDARD AND CMAKE_CXX_STANDARD LESS 23)
  message(FATAL_ERROR "This fmt fork requires C++23")
endif()
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
  #include <format>
  #include <print>
  #include <version>
  #if !defined(__cpp_lib_format) || __cpp_lib_format < 202110L
  #error C++23 formatting required
  #endif
  #if !defined(__cpp_lib_print) || __cpp_lib_print < 202207L
  #error C++23 printing required
  #endif
  int main() { std::print(\"{}\", std::format(\"{}\", 42)); }
" FMT_HAS_CXX23_STANDARD_LIBRARY)
if(NOT FMT_HAS_CXX23_STANDARD_LIBRARY)
  message(FATAL_ERROR "The selected standard library must supply C++23 std::format and std::print; no fallback is provided")
endif()

option(FMT_TEST "Build color/style regression tests" ${PROJECT_IS_TOP_LEVEL})
option(FMT_INSTALL "Install fmt" ${PROJECT_IS_TOP_LEVEL})
# Both public targets describe the same header-only implementation.
add_library(fmt INTERFACE)
add_library(fmt::fmt ALIAS fmt)
add_library(fmt-header-only INTERFACE)
add_library(fmt::fmt-header-only ALIAS fmt-header-only)
target_compile_features(fmt INTERFACE cxx_std_23)
target_include_directories(fmt INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(fmt-header-only INTERFACE fmt)
if(FMT_TEST)
  enable_testing()
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    separate_arguments(FMT_TEST_CXX_FLAGS NATIVE_COMMAND "${CMAKE_CXX_FLAGS}")
    add_test(NAME compile-checks COMMAND ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/test/compile-checks.py
      ${CMAKE_CXX_COMPILER} ${CMAKE_CURRENT_SOURCE_DIR}/include ${FMT_TEST_CXX_FLAGS})
  endif()
  add_executable(color-test test/color-test.cc)
  target_link_libraries(color-test PRIVATE fmt::fmt)
  add_test(NAME color-test COMMAND color-test)
  add_executable(cxx23-test test/cxx23-test.cc)
  target_link_libraries(cxx23-test PRIVATE fmt::fmt)
  add_test(NAME cxx23-test COMMAND cxx23-test)
  add_executable(cxx23-header-only-test test/cxx23-test.cc)
  target_link_libraries(cxx23-header-only-test PRIVATE fmt::fmt-header-only)
  add_test(NAME cxx23-header-only-test COMMAND cxx23-header-only-test)
endif()
if(FMT_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)
  install(TARGETS fmt fmt-header-only EXPORT fmt-targets)
  install(DIRECTORY include/fmt DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  set(FMT_VERSION ${PROJECT_VERSION})
  set(includedir_for_pc_file "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
  configure_file(support/cmake/fmt.pc.in ${CMAKE_CURRENT_BINARY_DIR}/fmt.pc @ONLY)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fmt.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  install(EXPORT fmt-targets NAMESPACE fmt:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fmt)
  set(targets_export_name fmt-targets)
  configure_package_config_file(support/cmake/fmt-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/fmt-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fmt)
  write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/fmt-config-version.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fmt-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/fmt-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fmt)
endif()
