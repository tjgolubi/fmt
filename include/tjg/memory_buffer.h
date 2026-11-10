// Minimal contiguous storage.
#ifndef TJG_MEMORY_BUFFER_H_
#define TJG_MEMORY_BUFFER_H_
#pragma once

#include "error_support.h"

#include <stdexcept>
#include <algorithm>
#include <memory>

#include <type_traits>
#include <utility>
#include <limits>

#include <cstddef>

namespace tjg {

/// A growable character buffer with inline storage for `InitSize` elements.
/// Output occupies `data()[0..size())`; no null terminator is added.
/// Growth invalidates pointers into the buffer.
/// `clear` retains allocated storage.
/// This buffer is neither copyable nor movable.
template <typename Char, std::size_t InitSize = 256>
class basic_memory_buffer {
  static_assert(InitSize > 0);
  static_assert(std::is_trivially_copyable_v<Char>);

  Char _inline_storage[InitSize]{};
  std::unique_ptr<Char[]> _storage{};
  std::size_t _size{0};
  std::size_t _capacity{InitSize};

  static constexpr auto max_size =
                      (std::numeric_limits<std::size_t>::max)() / sizeof(Char);

 public:
  using value_type = Char;

  constexpr basic_memory_buffer() noexcept = default;
  basic_memory_buffer(const basic_memory_buffer&) = delete;
  void operator=(const basic_memory_buffer&) = delete;

  /// Returns contiguous output storage, without a null terminator.
  constexpr auto data() noexcept -> Char*
  { return _storage ? _storage.get() : _inline_storage; }

  constexpr auto data() const noexcept -> const Char*
  { return _storage ? _storage.get() : _inline_storage; }

  /// Returns the number of output characters.
  constexpr auto size() const noexcept -> std::size_t { return _size; }

  /// Discards output while retaining storage for reuse.
  constexpr void clear() noexcept { _size = 0; }

private:

  constexpr void unchecked_reserve(std::size_t rsv_size)
      TJG_NOEXCEPT_UNLESS_BAD_ALLOC {
    if (rsv_size <= _capacity) return;
    const auto grown = (_capacity > max_size - _capacity / 2)
                     ? max_size : _capacity + _capacity / 2;
    const auto capacity = (std::max)(rsv_size, grown);
    auto storage = std::make_unique_for_overwrite<Char[]>(capacity);
    std::copy_n(data(), _size, storage.get());
    _storage  = std::move(storage);
    _capacity = capacity;
  } // unchecked_reserve

public:
  constexpr void reserve(std::size_t rsv_size) {
    if (rsv_size > max_size)
      TJG_THROW(std::length_error{"tjg::memory_buffer: buffer is too large"});
    unchecked_reserve(rsv_size);
  } // reserve

  /// Resizes the buffer, without fill.
  constexpr void resize(std::size_t new_size) {
    reserve(new_size);
    _size = new_size;
  }

  /// Appends a pointer range that must not refer to this buffer's storage.
  constexpr void append(const Char* first, const Char* last) {
    if (first == last) return;
    const auto count = static_cast<std::size_t>(last - first);
    if (count > max_size - _size)
      TJG_THROW(std::length_error{"tjg::memory_buffer: buffer is too large"});
    auto required = _size + count;
    unchecked_reserve(required);
    std::copy(first, last, data() + _size);
    _size = required;
  } // append

  /// Appends one character.
  constexpr void push_back(Char value) {
    if (_size == _capacity)
      append(&value, &value + 1);
    else
      data()[_size++] = value;
  }
}; // basic_memory_buffer

using memory_buffer = basic_memory_buffer<char>;

} // tjg

#endif  // TJG_MEMORY_BUFFER_H_
