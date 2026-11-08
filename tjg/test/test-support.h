// Test helpers for TJG; independent of the fmt engine and its test harness.
#ifndef TJG_TEST_SUPPORT_H_
#define TJG_TEST_SUPPORT_H_

#include "gtest/gtest.h"

#include <string>

#include <cstdio>

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

}  // namespace tjg::test

#define EXPECT_WRITE(file, statement, expected)        \
  do {                                                 \
    tjg::test::output_redirect redirect{file};         \
    statement;                                         \
    const auto captured = redirect.restore_and_read(); \
    EXPECT_EQ(captured, expected);                     \
  } while (false)

#endif  // TJG_TEST_SUPPORT_H_
