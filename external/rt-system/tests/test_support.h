#pragma once

#include <cstdio>
#include <source_location>
#include <string_view>

namespace test_support {

  inline int failure_count = 0;

  inline void check(bool condition, std::string_view expression,
                    const std::source_location location = std::source_location::current()) {
    if (condition) {
      return;
    }

    ++failure_count;
    std::fprintf(stderr, "%s:%u: check failed: %.*s\n", location.file_name(), location.line(),
                 static_cast<int>(expression.size()), expression.data());
  }

  [[nodiscard]] inline int finish() noexcept {
    return failure_count == 0 ? 0 : 1;
  }

} // namespace test_support

#define TEST_CHECK(Expression) ::test_support::check(static_cast<bool>(Expression), #Expression)
