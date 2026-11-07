// Test helpers for TJG; independent of the fmt engine and its test harness.
#ifndef TJG_TEST_SUPPORT_H_
#define TJG_TEST_SUPPORT_H_

#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

#include "gtest/gtest.h"

namespace tjg::test {
class output_redirect {
  std::FILE* _file;
  std::FILE* _capture = nullptr;
  int _saved = -1;

 public:
  explicit output_redirect(std::FILE* file);
  ~output_redirect();
  output_redirect(const output_redirect&) = delete;
  void operator=(const output_redirect&) = delete;
  auto restore_and_read() -> std::string;
};

template <typename Exception, typename Function>
auto throws_message(Function&& function, std::string_view expected)
    -> ::testing::AssertionResult {
  try {
    std::forward<Function>(function)();
  } catch (const Exception& error) {
    if (std::string_view{error.what()} == expected)
      return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure()
           << "Expected message: " << std::string{expected}
           << "; actual: " << error.what();
  } catch (...) {
    return ::testing::AssertionFailure() << "Wrong exception type";
  }
  return ::testing::AssertionFailure() << "No exception thrown";
}
}  // namespace tjg::test

#define EXPECT_THROW_MSG(statement, exception, message) \
  EXPECT_TRUE(                                          \
      (tjg::test::throws_message<exception>([&] { statement; }, message)))

#define EXPECT_WRITE(file, statement, expected)        \
  do {                                                 \
    tjg::test::output_redirect redirect{file};         \
    statement;                                         \
    const auto captured = redirect.restore_and_read(); \
    EXPECT_EQ(captured, expected);                     \
  } while (false)

#endif  // TJG_TEST_SUPPORT_H_
