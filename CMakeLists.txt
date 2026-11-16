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
  tjg INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if (TJG_INSTALL)
  set(TJG_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/tjg")
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/color-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/tjg-config.cmake"
    INSTALL_DESTINATION "${TJG_CMAKE_DIR}")
  install(
    TARGETS tjg
    EXPORT tjg-targets
    COMPONENT tjg)
  install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/tjg"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT tjg
    FILES_MATCHING
    PATTERN "*.hpp")
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
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
          "${CMAKE_CURRENT_SOURCE_DIR}/THIRD-PARTY-NOTICES"
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

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/support/mkdocs-pages.yml")
  find_package(Python3 QUIET COMPONENTS Interpreter)
  find_program(TJG_DOXYGEN NAMES doxygen)
  if (Python3_Interpreter_FOUND AND TJG_DOXYGEN)
    set(doc_venv "${CMAKE_CURRENT_BINARY_DIR}/doc-venv")
    set(doc_requirements "${CMAKE_CURRENT_SOURCE_DIR}/support/doc-requirements.txt")
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" -c
              "import os; print('Scripts/python.exe' if os.name == 'nt' else 'bin/python')"
      OUTPUT_VARIABLE doc_python_relative
      OUTPUT_STRIP_TRAILING_WHITESPACE
      COMMAND_ERROR_IS_FATAL ANY)
    set(doc_python "${doc_venv}/${doc_python_relative}")
    add_custom_command(
      OUTPUT "${doc_venv}/requirements-installed"
      COMMAND "${Python3_EXECUTABLE}" -m venv "${doc_venv}"
      COMMAND "${doc_python}" -m pip install --require-hashes -r "${doc_requirements}"
      COMMAND "${CMAKE_COMMAND}" -E touch "${doc_venv}/requirements-installed"
      DEPENDS "${doc_requirements}" "${Python3_EXECUTABLE}"
      COMMENT "Installing the pinned guide dependencies"
      VERBATIM)
    add_custom_target(
      doc
      COMMAND
        "${CMAKE_COMMAND}" -E env "PYTHONPATH=support/python"
        "TJG_DOXYGEN=${TJG_DOXYGEN}"
        "TJG_DOXYXML=${CMAKE_CURRENT_BINARY_DIR}/doxyxml"
        "${doc_python}" -m mkdocs build --config-file support/mkdocs-pages.yml
        --site-dir "${CMAKE_CURRENT_BINARY_DIR}/doc-html" --no-directory-urls
      DEPENDS "${doc_venv}/requirements-installed"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Building the color guide"
      VERBATIM)
  else ()
    message(STATUS "Target 'doc' requires Python 3 and Doxygen")
  endif ()
endif ()
