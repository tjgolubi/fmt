include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
  [=[
  #include <format>
  #include <print>
  int main() {
    static_assert(std::formattable<int, char>);
    std::print("{}", std::format("{}", 42));
  }
]=]
  TJG_HAS_STD_FORMAT_AND_PRINT)
if (NOT TJG_HAS_STD_FORMAT_AND_PRINT)
  message(
    FATAL_ERROR
      "TJG tests need C++23 with working <format>, std::formattable, and <print>"
  )
endif ()

get_filename_component(TJG_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.."
                       ABSOLUTE)

# Reuse vendored Google Test without configuring or linking the fmt project.
if (TARGET gtest)
  set(TJG_GTEST_TARGET gtest)
elseif (EXISTS "${TJG_SOURCE_ROOT}/test/gtest/CMakeLists.txt")
  add_subdirectory("${TJG_SOURCE_ROOT}/test/gtest"
                   "${CMAKE_CURRENT_BINARY_DIR}/gtest")
  set(TJG_GTEST_TARGET gtest)
else ()
  find_package(GTest CONFIG REQUIRED)
  set(TJG_GTEST_TARGET GTest::gtest)
endif ()

add_library(tjg-test-main STATIC test-main.cc test-support.cc)
target_link_libraries(tjg-test-main PUBLIC ${TJG_GTEST_TARGET} tjg::tjg)

function (tjg_test_options name)
  if (MSVC)
    target_compile_definitions(${name} PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(${name} PRIVATE /W4)
    if (TJG_WERROR)
      target_compile_options(${name} PRIVATE /WX)
    endif ()
  else ()
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    if (TJG_WERROR)
      target_compile_options(${name} PRIVATE -Werror)
    endif ()
  endif ()
endfunction ()
tjg_test_options(tjg-test-main)

foreach (
  name
  color
  color-debug
  color-std-format
  color-ostream
  color-assert
  color-wide
  styled-text
  styled-text-format
  parse-styled-text
  strip-ansi
  backslash
  memory-buffer)
  add_executable(${name}-test ${name}-test.cc)
  target_link_libraries(${name}-test PRIVATE tjg-test-main)
  tjg_test_options(${name}-test)
  add_test(NAME ${name}-test COMMAND ${name}-test)
endforeach ()

add_executable(extension-standalone-test extension-standalone-test.cc
                                         extension-standalone-state.cc)
add_executable(styled-text-sharing-test styled-text-sharing-test.cc)
add_executable(color-api-test color-api-test.cc)
foreach (name extension-standalone styled-text-sharing color-api)
  target_link_libraries(${name}-test PRIVATE tjg::tjg)
  tjg_test_options(${name}-test)
  add_test(NAME ${name}-test COMMAND ${name}-test)
endforeach ()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-fno-exceptions TJG_HAVE_FNO_EXCEPTIONS)
if (TJG_HAVE_FNO_EXCEPTIONS)
  add_executable(tjg-noexception-test noexception-test.cc)
  target_link_libraries(tjg-noexception-test PRIVATE tjg::tjg)
  target_compile_options(tjg-noexception-test PRIVATE -fno-exceptions)
  tjg_test_options(tjg-noexception-test)
  add_test(NAME tjg-noexception-test COMMAND tjg-noexception-test)
endif ()

add_test(
  NAME tjg-package-test
  COMMAND
    ${CMAKE_COMMAND} "-DTJG_SOURCE_DIR=${TJG_SOURCE_ROOT}"
    "-DTJG_TEST_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/package-test"
    "-DTJG_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DTJG_GENERATOR=${CMAKE_GENERATOR}"
    "-DTJG_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
    "-DTJG_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
    "-DTJG_COMPACT_SGR=${TJG_COLOR_COMPACT_SGR}"
    "-DTJG_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
    "-DTJG_EXE_SUFFIX=${CMAKE_EXECUTABLE_SUFFIX}" -P
    "${CMAKE_CURRENT_SOURCE_DIR}/package-test/run.cmake")

get_property(
  tjg_tests
  DIRECTORY
  PROPERTY TESTS)
set_tests_properties(${tjg_tests} PROPERTIES LABELS tjg)
