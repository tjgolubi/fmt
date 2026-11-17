#include "tjg/inline_buffer.hpp"
#include "test-support.h"

#include <string>
#include <string_view>

#include <algorithm>
#include <iterator>

TEST(inline_buffer_test, inline_storage_and_reuse) {
  tjg::basic_inline_buffer<char, 4> buffer;
  const auto storage = buffer.data();
  EXPECT_TRUE(buffer.empty());
  for (char c : std::string_view{"abcd"}) buffer.push_back(c);
  EXPECT_EQ(buffer.data(), storage);
  EXPECT_EQ(buffer.view(), "abcd");

  auto copy = buffer;
  EXPECT_NE(copy.data(), storage);
  EXPECT_EQ(copy.view(), buffer.view());
  buffer.clear();
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(buffer.data(), storage);
  buffer.push_back('z');
  EXPECT_EQ(buffer.view(), "z");
  EXPECT_EQ(copy.view(), "abcd");
}

TEST(inline_buffer_test, pointer_ranges_and_embedded_nulls) {
  tjg::inline_buffer<> buffer;
  buffer.append<char>(nullptr, nullptr);
  EXPECT_EQ(buffer.size(), 0u);
  const auto text = std::string{"a\0b", 3};
  buffer.append(text.data(), text.data() + text.size());
  const auto suffix = std::string_view{"tail"};
  buffer.append(suffix);
  const auto& const_buffer = buffer;
  EXPECT_EQ(const_buffer.view(), text + "tail");
}

TEST(inline_buffer_test, repeated_external_append) {
  tjg::basic_inline_buffer<char, 9> buffer;
  const char text[] = "abc";
  buffer.append(text, text + 3);
  buffer.append(text, text + 3);
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "abcabc");
  buffer.append(text, text + 3);
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "abcabcabc");
}

TEST(inline_buffer_test, resize) {
  tjg::basic_inline_buffer<char, 16> buffer;
  buffer.push_back('a');
  const auto storage = buffer.data();
  EXPECT_EQ(buffer.size(), 1u);
  EXPECT_EQ(storage[0], 'a');
  buffer.resize(16);
  EXPECT_EQ(buffer.size(), 16u);
  EXPECT_EQ(buffer.data(), storage);
  EXPECT_EQ(buffer.data()[0], 'a');
  std::fill(buffer.data() + 1, buffer.data() + buffer.size(), 'x');
  buffer.resize(2);
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "ax");
  buffer.clear();
  EXPECT_EQ(buffer.data(), storage);
}

TEST(inline_buffer_test, wide_text_and_output_iterators) {
  tjg::basic_inline_buffer<wchar_t, 10> buffer;
  const auto text = std::wstring{L"wide\0text", 9};
  std::copy(text.begin(), text.end(), std::back_inserter(buffer));
  EXPECT_EQ(buffer.view(), text);
  buffer.append(std::string_view{"\xff"});
  EXPECT_EQ(buffer.view().back(), wchar_t{255});
}

TEST(inline_buffer_test, constexpr_storage) {
  static_assert([] {
    tjg::basic_inline_buffer<char, 3> buffer;
    buffer.push_back('a');
    buffer.resize(3);
    buffer.data()[1] = 'b';
    buffer.data()[2] = 'c';
    buffer.resize(2);
    return buffer.size() == 2 && buffer.data()[0] == 'a' &&
           buffer.data()[1] == 'b';
  }());
  static_assert([] {
    tjg::basic_inline_buffer<char, 4> buffer;
    buffer.push_back('a');
    const char tail[] = {'\0', 'b'};
    buffer.append(tail, tail + 2);
    return buffer.size() == 3 && buffer.data()[0] == 'a' &&
           buffer.data()[1] == '\0' && buffer.data()[2] == 'b';
  }());
  static_assert([] {
    tjg::basic_inline_buffer<char, 4> buffer;
    buffer.push_back('a');
    buffer.push_back('b');
    const char text[] = {'a', 'b'};
    buffer.append(text, text + 2);
    const auto matches = buffer.size() == 4 && buffer.data()[3] == 'b';
    buffer.clear();
    return matches && buffer.size() == 0;
  }());
}
