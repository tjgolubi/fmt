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
file(COPY "${TJG_SOURCE_DIR}/tjg" "${TJG_SOURCE_DIR}/LICENSE"
     DESTINATION "${source}")
file(COPY "${TJG_SOURCE_DIR}/include/tjg" DESTINATION "${source}/include")

run("${CMAKE_COMMAND}"
    -S
    "${source}/tjg"
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
file(RENAME "${prefix}" "${relocated}")

# The consumer sets no language standard, or include path itself.
run("${CMAKE_COMMAND}"
    -S
    "${TJG_SOURCE_DIR}/tjg/test/package-test"
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
