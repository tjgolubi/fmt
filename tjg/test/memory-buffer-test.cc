#include "tjg/memory_buffer.h"
#include "test-support.h"

#include <string>

#include <algorithm>
#include <iterator>

TEST(memory_buffer_test, inline_storage_and_growth) {
  tjg::basic_memory_buffer<char, 4> buffer;
  const auto inline_data = buffer.data();
  EXPECT_EQ(buffer.size(), 0u);
  for (char c : std::string{"abcd"}) buffer.push_back(c);
  EXPECT_EQ(buffer.data(), inline_data);
  buffer.push_back('e');
  EXPECT_NE(buffer.data(), inline_data);
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "abcde");

  const auto heap_data = buffer.data();
  buffer.clear();
  EXPECT_EQ(buffer.size(), 0u);
  EXPECT_EQ(buffer.data(), heap_data);
  buffer.push_back('z');
  EXPECT_EQ(buffer.data(), heap_data);
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "z");
}

TEST(memory_buffer_test, pointer_ranges_and_embedded_nulls) {
  tjg::memory_buffer buffer;
  buffer.append(nullptr, nullptr);
  EXPECT_EQ(buffer.size(), 0u);
  const auto text = std::string{"a\0b", 3};
  buffer.append(text.data(), text.data() + text.size());
  const auto large = std::string(10000, 'x');
  buffer.append(large.data(), large.data() + large.size());
  const auto& const_buffer = buffer;
  EXPECT_EQ(std::string(const_buffer.data(), const_buffer.size()), text + large);
}

TEST(memory_buffer_test, external_append_across_growth) {
  tjg::basic_memory_buffer<char, 4> buffer;
  const char text[] = "abc";
  buffer.append(text, text + 3);
  buffer.append(text, text + 3);
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "abcabc");
  buffer.append(text, text + 3);
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "abcabcabc");
}

TEST(memory_buffer_test, reserve_and_resize) {
  tjg::basic_memory_buffer<char, 4> buffer;
  buffer.push_back('a');
  buffer.reserve(32);
  const auto storage = buffer.data();
  EXPECT_EQ(buffer.size(), 1u);
  EXPECT_EQ(storage[0], 'a');
  buffer.reserve(2);
  EXPECT_EQ(buffer.data(), storage);
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

TEST(memory_buffer_test, wide_text_and_output_iterators) {
  tjg::basic_memory_buffer<wchar_t, 2> buffer;
  const auto text = std::wstring{L"wide\0text", 9};
  std::copy(text.begin(), text.end(), std::back_inserter(buffer));
  EXPECT_EQ(std::wstring(buffer.data(), buffer.size()), text);
}

TEST(memory_buffer_test, constexpr_storage) {
  static_assert([] {
    tjg::basic_memory_buffer<char, 2> buffer;
    buffer.push_back('a');
    buffer.reserve(8);
    buffer.resize(3);
    buffer.data()[1] = 'b';
    buffer.data()[2] = 'c';
    buffer.resize(2);
    return buffer.size() == 2 && buffer.data()[0] == 'a' &&
           buffer.data()[1] == 'b';
  }());
  static_assert([] {
    tjg::basic_memory_buffer<char, 4> buffer;
    buffer.push_back('a');
    const char tail[] = {'\0', 'b'};
    buffer.append(tail, tail + 2);
    return buffer.size() == 3 && buffer.data()[0] == 'a' &&
           buffer.data()[1] == '\0' && buffer.data()[2] == 'b';
  }());
  static_assert([] {
    tjg::basic_memory_buffer<char, 1> buffer;
    buffer.push_back('a');
    buffer.push_back('b');
    const char text[] = {'a', 'b'};
    buffer.append(text, text + 2);
    const auto matches = buffer.size() == 4 && buffer.data()[3] == 'b';
    buffer.clear();
    return matches && buffer.size() == 0;
  }());
}
