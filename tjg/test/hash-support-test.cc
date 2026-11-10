#include "tjg/hash_support.h"

#include "test-support.h"

struct hash_input {
  std::size_t value;
};

struct throwing_hash_input {
  bool should_throw;
};

namespace std {
template <> struct hash<hash_input> {
  auto operator()(hash_input value) const noexcept -> size_t {
    return value.value;
  }
};
template <> struct hash<throwing_hash_input> {
  auto operator()(throwing_hash_input value) const -> size_t {
    if (value.should_throw) throw 42;
    return 0;
  }
};
}  // namespace std

TEST(hash_support_test, boost_mixers) {
  static_assert(tjg::detail::hash_mix(uint32_t{0}) == 0);
  static_assert(tjg::detail::hash_mix(uint32_t{1}) == 0x86d2fa73);
  static_assert(tjg::detail::hash_mix(uint64_t{0}) == 0);
  static_assert(tjg::detail::hash_mix(uint64_t{1}) == 0x3c0aad46f555e0b9);
}

TEST(hash_support_test, boost_combine_vectors) {
  std::size_t seed = 0;
  static_assert(noexcept(tjg::hash_combine(seed, hash_input{})));
  const uint64_t expected64[] = {
      0xa55db391e20904c2, 0xda4fe80ec08343ba, 0x21de69012360be33};
  const uint32_t expected32[] = {0x64625032, 0x9e3e5f5c, 0x1a5ae45e};
  const hash_input inputs[] = {{0}, {42}, {123}};
  for (std::size_t i = 0; i != 3; ++i) {
    tjg::hash_combine(seed, inputs[i]);
    EXPECT_EQ(seed, sizeof(seed) == 8 ? expected64[i] : expected32[i]);
  }
}

TEST(hash_support_test, propagates_hasher_exception) {
  std::size_t seed = 123;
  static_assert(!noexcept(tjg::hash_combine(seed, throwing_hash_input{})));
  // Keep both paths reachable when the compiler inlines the hasher.
  volatile bool should_throw = true;
  EXPECT_THROW(tjg::hash_combine(seed, throwing_hash_input{should_throw}), int);
  EXPECT_EQ(seed, 123u);
}
