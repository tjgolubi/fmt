cmake_minimum_required(VERSION 3.20)

foreach (required TJG_SOURCE_DIR TJG_TEST_BINARY_DIR TJG_CXX_COMPILER
                  TJG_GENERATOR)
  if (NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing package-test setting: ${required}")
  endif ()
endforeach ()

set(generator_options -G "${TJG_GENERATOR}")
if (TJG_GENERATOR_PLATFORM)
  list(APPEND generator_options -A "${TJG_GENERATOR_PLATFORM}")
endif ()
if (TJG_GENERATOR_TOOLSET)
  list(APPEND generator_options -T "${TJG_GENERATOR_TOOLSET}")
endif ()

function (run)
  execute_process(
    COMMAND ${ARGV}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
  if (NOT result EQUAL 0)
    message(FATAL_ERROR "Command failed: ${ARGV}\n${output}\n${error}")
  endif ()
endfunction ()

# Copy only TJG and its license: neither build nor install can use fmt sources.
set(source "${TJG_TEST_BINARY_DIR}/source")
set(package_build "${TJG_TEST_BINARY_DIR}/package-build")
set(prefix "${TJG_TEST_BINARY_DIR}/prefix")
set(relocated "${TJG_TEST_BINARY_DIR}/relocated")
set(consumer_build "${TJG_TEST_BINARY_DIR}/consumer-build")
file(REMOVE_RECURSE "${source}" "${package_build}" "${prefix}" "${relocated}"
     "${consumer_build}")
file(MAKE_DIRECTORY "${source}/include")
file(COPY "${TJG_SOURCE_DIR}/CMakeLists.txt"
     "${TJG_SOURCE_DIR}/color-config.cmake.in" "${TJG_SOURCE_DIR}/LICENSE"
     "${TJG_SOURCE_DIR}/THIRD-PARTY-NOTICES" DESTINATION "${source}")
file(COPY "${TJG_SOURCE_DIR}/include/tjg" DESTINATION "${source}/include")

run("${CMAKE_COMMAND}"
    -S
    "${source}"
    -B
    "${package_build}"
    ${generator_options}
    "-DCMAKE_MAKE_PROGRAM=${TJG_MAKE_PROGRAM}"
    "-DCMAKE_CXX_COMPILER=${TJG_CXX_COMPILER}"
    -DCMAKE_BUILD_TYPE=Debug
    "-DCMAKE_INSTALL_PREFIX=${prefix}"
    -DTJG_INSTALL=ON
    -DTJG_TEST=OFF)
run("${CMAKE_COMMAND}" --build "${package_build}" --config Debug)
run("${CMAKE_COMMAND}" --install "${package_build}" --config Debug)
if (EXISTS "${prefix}/include/fmt")
  message(FATAL_ERROR "The standalone install must contain only TJG headers")
endif ()
# A freshly installed directory can be briefly locked on Windows.
foreach (attempt RANGE 1 3)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E rename "${prefix}" "${relocated}"
    RESULT_VARIABLE result
    ERROR_VARIABLE error)
  if (result EQUAL 0)
    break()
  endif ()
  run("${CMAKE_COMMAND}" -E sleep 1)
endforeach ()
if (NOT result EQUAL 0)
  message(FATAL_ERROR "Could not relocate the installed TJG package:\n${error}")
endif ()

# The consumer sets no language standard, or include path itself.
run("${CMAKE_COMMAND}"
    -S
    "${CMAKE_CURRENT_LIST_DIR}"
    -B
    "${consumer_build}"
    ${generator_options}
    "-DCMAKE_MAKE_PROGRAM=${TJG_MAKE_PROGRAM}"
    "-DCMAKE_CXX_COMPILER=${TJG_CXX_COMPILER}"
    -DCMAKE_BUILD_TYPE=Debug
    "-DCMAKE_PREFIX_PATH=${relocated}")
run("${CMAKE_COMMAND}"
    --build
    "${consumer_build}"
    --config
    Debug
    --parallel
    2)
run("${consumer_build}/bin/tjg-consumer${TJG_EXE_SUFFIX}")
message(STATUS "TJG-only install, relocation, and consumer checks passed")
