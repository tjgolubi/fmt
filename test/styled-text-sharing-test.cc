// Isolated allocation regression: replacing global new must not affect other tests.
#include "fmt/styled-text.h"

#include <cstdio>
#include <cstdlib>
#include <new>

namespace {
bool count_allocations = false;
size_t allocated_bytes = 0;
size_t allocation_count = 0;
}

#if defined(__GNUC__)
#  define FMTCOLOR_TEST_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#  define FMTCOLOR_TEST_NOINLINE __declspec(noinline)
#else
#  define FMTCOLOR_TEST_NOINLINE
#endif

FMTCOLOR_TEST_NOINLINE void* operator new(size_t size) {
  if (count_allocations) {
    allocated_bytes += size;
    ++allocation_count;
  }
  void* ptr = std::malloc(size == 0 ? 1 : size);
  if (!ptr) FMT_THROW(std::bad_alloc());
  return ptr;
}
FMTCOLOR_TEST_NOINLINE void operator delete(void* ptr) noexcept {
  std::free(ptr);
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete[](void* ptr) noexcept { ::operator delete(ptr); }
#if defined(__cpp_sized_deallocation) && __cpp_sized_deallocation >= 201309L
void operator delete(void* ptr, size_t) noexcept { ::operator delete(ptr); }
void operator delete[](void* ptr, size_t) noexcept { ::operator delete(ptr); }
#endif

namespace {
void start_counting() {
  allocated_bytes = 0;
  allocation_count = 0;
  count_allocations = true;
}

template<typename Char>
bool check_sharing() {
  using node = fmt::basic_styled_text<Char>;
  const auto large = std::basic_string<Char>(100000, static_cast<Char>('x'));
  const auto source = node(large);
  start_counting();
  auto copy = source;
  auto assigned = node{};
  assigned = source;
  count_allocations = false;
  if (allocation_count != 0 || copy != source || assigned != source) return false;

  const auto branch = source + source;
  start_counting();
  auto branch_copy = branch;
  count_allocations = false;
  // Structure may allocate; copying either large character buffer must not.
  if (allocated_bytes >= large.size() * sizeof(Char) || branch_copy != branch)
    return false;

  start_counting();
  auto empty = node{};
  auto explicit_empty = node(std::basic_string<Char>{});
  auto moved = node(std::move(copy));
  count_allocations = false;
  return allocation_count == 0 && empty.empty() && explicit_empty.empty() &&
         copy.empty() && moved == source;
}
} // namespace

int main() {
  if (check_sharing<char>() && check_sharing<wchar_t>()) return 0;
  std::fputs("styled_text sharing allocation regression failed\n", stderr);
  return 1;
}
