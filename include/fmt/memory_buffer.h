// Minimal contiguous storage for the tjg extension.
#ifndef TJG_MEMORY_BUFFER_H_
#define TJG_MEMORY_BUFFER_H_

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace tjg {

/**
 * A growable character buffer with inline storage for SIZE elements.
 * Output occupies data()[0..size()); no null terminator is added.
 * Growth invalidates pointers into the buffer. clear retains allocated storage.
 * This buffer is neither copyable nor movable.
 */
template <typename Char, std::size_t SIZE = 500> class basic_memory_buffer {
  static_assert(SIZE > 0);
  static_assert(std::is_trivially_copyable_v<Char>);

  Char _inline[SIZE]{};
  std::unique_ptr<Char[]> _storage{};
  std::size_t _size{0};
  std::size_t _capacity{SIZE};

 public:
  using value_type = Char;

  constexpr basic_memory_buffer() noexcept = default;
  basic_memory_buffer(const basic_memory_buffer&) = delete;
  void operator=(const basic_memory_buffer&) = delete;

  /// Returns contiguous output storage, without a null terminator.
  constexpr auto data() noexcept -> Char* {
    return _storage ? _storage.get() : _inline;
  }
  constexpr auto data() const noexcept -> const Char* {
    return _storage ? _storage.get() : _inline;
  }

  /// Returns the number of output characters.
  constexpr auto size() const noexcept -> std::size_t { return _size; }

  /// Discards output while retaining storage for reuse.
  constexpr void clear() noexcept { _size = 0; }

  /// Appends a pointer range, including a range within this buffer's output.
  constexpr void append(const Char* first, const Char* last) {
    if (first == last) return;
    const auto count = static_cast<std::size_t>(last - first);
    constexpr auto max_size =
        (std::numeric_limits<std::size_t>::max)() / sizeof(Char);
    if (count > max_size - _size) {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
      throw std::length_error("memory buffer is too large");
#else
      std::abort();
#endif
    }
    const auto required = _size + count;
    if (required > _capacity) {
      const auto grown = _capacity > max_size - _capacity / 2
                             ? max_size
                             : _capacity + _capacity / 2;
      const auto capacity = (std::max)(required, grown);
      auto storage = std::make_unique_for_overwrite<Char[]>(capacity);
      std::copy_n(data(), _size, storage.get());
      // Keep old storage alive until a possibly aliased input is copied.
      std::copy(first, last, storage.get() + _size);
      _storage = std::move(storage);
      _capacity = capacity;
    } else {
      std::copy(first, last, data() + _size);
    }
    _size = required;
  }

  /// Appends one character.
  constexpr void push_back(Char value) {
    if (_size == _capacity) {
      append(&value, &value + 1);
    } else {
      data()[_size++] = value;
    }
  }
};

using memory_buffer = basic_memory_buffer<char>;

}  // namespace tjg

#endif  // TJG_MEMORY_BUFFER_H_
