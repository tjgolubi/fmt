/// @file
/// Fixed-capacity character buffers with inline storage.

#pragma once

#include <tjg/color/bits/error_support.hpp>
#include "utility.hpp"

#include <string_view>
#include <algorithm>
#include <array>

#include <concepts>
#include <utility>

namespace tjg {

/**
 * A fixed-capacity character buffer that starts empty.
 * The output is the range `[data(), data() + size())`. A terminating null
 * character is not guaranteed. Copies have independent storage and copy
 * only the source's output characters.
 *
 * A `wchar_t` buffer also accepts `char` input. Each input byte is widened
 * through `unsigned char`, preserving its numeric byte value.
 *
 * @tparam Char The stored character type, `char` or `wchar_t`.
 * @tparam Size The positive capacity in characters. Choose a capacity large
 *              enough for every write. Capacity violations fail `TJG_EXPECTS`.
 */
template<CharType Char, std::size_t Size = 256>
  requires (Size > 0)
class basic_inline_buffer {
  std::array<Char, Size> _storage;
  std::size_t _size{0};

  static constexpr std::size_t _capacity{Size};

 public:
  /// The stored character type.
  using value_type = Char;

  /// Constructs an empty buffer. At runtime, storage is left uninitialized.
  constexpr basic_inline_buffer() noexcept {
    if consteval { _storage = {}; }
  }

  /// Copies the source's output. At runtime, the remaining storage is
  /// uninitialized.
  constexpr basic_inline_buffer(const basic_inline_buffer& other) noexcept
    : _size{other._size}
  {
    // A constexpr object needs initialized storage beyond its output.
    if consteval { _storage = {}; }
    std::copy(other.begin(), other.end(), data());
  }

  /// Copies the source's output and leaves the remaining storage intact.
  constexpr auto operator=(const basic_inline_buffer& other) noexcept
    -> basic_inline_buffer&
  {
    if (this != &other) {
      std::copy(other.begin(), other.end(), data());
      _size = other._size;
    }
    return *this;
  }

  /// Returns whether the buffer contains no output characters.
  constexpr auto empty() const noexcept -> bool { return (_size == 0); }

  /// Returns a writable pointer to the start of the contiguous storage.
  constexpr auto data() noexcept -> Char* { return _storage.data(); }

  /// Returns a read-only pointer to the start of the contiguous storage.
  constexpr auto data() const noexcept -> const Char*
  { return _storage.data(); }

  /// Returns the number of output characters.
  constexpr auto size() const noexcept -> std::size_t { return _size; }

  /// Returns a view that borrows the storage and captures the current size.
  constexpr auto view() const noexcept -> std::basic_string_view<Char>
  { return {data(), size()}; }

  /// Returns a read-only iterator to the start of the output.
  constexpr auto begin() const noexcept -> const Char* { return data(); }

  /// Returns a read-only iterator one past the end of the output.
  constexpr auto end() const noexcept -> const Char* { return data() + size(); }

  /// Resets the size to zero. Stored character values are left intact.
  constexpr void clear() noexcept { _size = 0; }

  /// Changes the output size. Growing fills newly exposed characters with
  /// `fill`, which defaults to zero.
  /// Shrinking leaves the stored character values intact.
  /// @pre `new_size` is at most `Size`.
  constexpr void resize(std::size_t new_size, Char fill = Char{0}) noexcept {
    TJG_EXPECTS(new_size <= _capacity, "tjg::inline_buffer::resize: too large");
    if (new_size > _size)
      std::fill_n(_storage.data() + _size, new_size - _size, fill);
    _size = new_size;
  }

  /// Lets `op` write up to limit characters directly into the buffer.
  /// @pre `limit` is at most the buffer's capacity.
  /// @pre `op` does not throw and returns a `std::size_t <= limit`.
  /// @pre On return from `op`, every character in the reported prefix is
  ///      initialized.
  template<typename Operation>
  constexpr void resize_and_overwrite(std::size_t limit, Operation op) noexcept
  {
    TJG_EXPECTS(limit <= _capacity,
            "tjg::inline_buffer::resize_and_overwrite: limit exceeds capacity");

    const std::size_t written = std::move(op)(data(), limit);
    TJG_EXPECTS(written <= limit,
              "tjg::inline_buffer::resize_and_overwrite: result exceeds limit");

    _size = written;
  }

  /// Appends one character.
  /// @pre At least one character fits in the remaining capacity.
  template<CharType C>
    requires (std::same_as<C, Char>
          || (std::same_as<Char, wchar_t> && std::same_as<C, char>))
  constexpr void push_back(C value) noexcept {
    TJG_EXPECTS(_size < _capacity, "tjg::inline_buffer::push_back: too large");
    if constexpr (std::same_as<C, Char>)
      _storage[_size] = value;
    else
      _storage[_size] = static_cast<wchar_t>(static_cast<unsigned char>(value));
    ++_size;
  }

  /// Appends the characters in the half-open range `[first, last)`.
  /// @pre The range is valid and fits in the remaining capacity. For a
  ///      same-type copy, the destination start is outside the source range.
  template<CharType C>
    requires (std::same_as<C, Char>
          || (std::same_as<Char, wchar_t> && std::same_as<C, char>))
  constexpr void append(const C* first, const C* last) noexcept {
    if (first == last) return;

    const auto count = static_cast<std::size_t>(last - first);
    TJG_EXPECTS(count <= _capacity - _size,
                "tjg::inline_buffer::append: too large");

    auto dest = data() + _size;
    if constexpr (std::same_as<C, Char>) {
      std::copy(first, last, dest);
    } else {
      std::transform(first, last, dest, [](char ch) {
        return static_cast<wchar_t>(static_cast<unsigned char>(ch));
      });
    }
    _size += count;
  }

  /// Appends every character in the view, including embedded null characters.
  /// @pre The view fits in the remaining capacity. A nonempty view refers to
  ///      external storage.
  template<CharType C, typename Traits>
    requires (std::same_as<C, Char>
          || (std::same_as<Char, wchar_t> && std::same_as<C, char>))
  constexpr void append(std::basic_string_view<C, Traits> text) noexcept {
    if (text.empty()) return;
    append(text.data(), text.data() + text.size());
  }

  /// Appends the characters of a null-terminated string up to its terminator.
  /// @pre `cp` points to a valid null-terminated string in external storage.
  ///      The characters preceding the terminator fit in the remaining capacity.
  template<CharType C>
    requires (std::same_as<C, Char>
          || (std::same_as<Char, wchar_t> && std::same_as<C, char>))
  constexpr void append(const C* cp) noexcept {
    TJG_EXPECTS(cp != nullptr, "tjg::inline_buffer::append: null string");
    append(std::basic_string_view<C>{cp});
  }

  /// Appends a narrow view to a wide buffer, accepting implicit conversions
  /// to `std::string_view`.
  /// @pre The view fits in the remaining capacity. A nonempty view refers to
  ///      external storage.
  constexpr void append(std::string_view text) noexcept
    requires std::same_as<Char, wchar_t>
  { append<char, std::char_traits<char>>(text); }

}; // basic_inline_buffer

/// A narrow character buffer with the default capacity of 256 characters.
template<std::size_t N = 256>
using inline_buffer = basic_inline_buffer<char, N>;

} // tjg
