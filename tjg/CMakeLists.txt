cmake_minimum_required(VERSION 3.20...3.28)
project(TJG LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(TJG_INSTALL "Generate TJG install rules." ON)
option(TJG_TEST "Build TJG tests." OFF)
option(TJG_WERROR "Treat TJG test warnings as errors." OFF)

add_library(tjg INTERFACE)
add_library(tjg::tjg ALIAS tjg)
target_compile_features(tjg INTERFACE cxx_std_23)
target_include_directories(
  tjg INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if (TJG_INSTALL)
  set(TJG_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/tjg")
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/tjg-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/tjg-config.cmake"
    INSTALL_DESTINATION "${TJG_CMAKE_DIR}")
  install(
    TARGETS tjg
    EXPORT tjg-targets
    COMPONENT tjg)
  install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../include/tjg"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT tjg
    FILES_MATCHING
    PATTERN "*.h")
  install(
    EXPORT tjg-targets
    NAMESPACE tjg::
    DESTINATION "${TJG_CMAKE_DIR}"
    COMPONENT tjg)
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/tjg-config.cmake"
    DESTINATION "${TJG_CMAKE_DIR}"
    COMPONENT tjg)
  install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE"
          "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE-Boost"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/tjg"
    COMPONENT tjg)
endif ()

if (TJG_TEST)
  if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
  elseif (CMAKE_CXX_STANDARD LESS 23)
    message(FATAL_ERROR "TJG tests require C++23 or later")
  endif ()
  enable_testing()
  add_subdirectory(test)
endif ()
