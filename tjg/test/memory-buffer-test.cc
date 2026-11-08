#include "tjg/memory_buffer.h"
#include "test-support.h"

#include <string>

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

TEST(memory_buffer_test, self_append_across_growth) {
  tjg::basic_memory_buffer<char, 4> buffer;
  const char initial[] = "abc";
  buffer.append(initial, initial + 3);
  buffer.append(buffer.data(), buffer.data() + buffer.size());
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "abcabc");
  buffer.append(buffer.data(), buffer.data() + buffer.size());
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "abcabcabcabc");
  buffer.clear();
  buffer.append(initial, initial + 3);
  buffer.append(buffer.data(), buffer.data() + 2);
  EXPECT_EQ(std::string(buffer.data(), buffer.size()), "abcab");
}

TEST(memory_buffer_test, wide_text_and_output_iterators) {
  tjg::basic_memory_buffer<wchar_t, 2> buffer;
  const auto text = std::wstring{L"wide\0text", 9};
  std::copy(text.begin(), text.end(), std::back_inserter(buffer));
  EXPECT_EQ(std::wstring(buffer.data(), buffer.size()), text);
}

TEST(memory_buffer_test, constexpr_storage) {
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
    buffer.append(buffer.data(), buffer.data() + 2);
    const auto matches = buffer.size() == 4 && buffer.data()[3] == 'b';
    buffer.clear();
    return matches && buffer.size() == 0;
  }());
}
