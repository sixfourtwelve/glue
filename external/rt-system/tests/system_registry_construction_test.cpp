#include "test_support.h"

#include <system.h>

#include <stdexcept>

namespace {

  int constructed_count = 0;
  int destroyed_count = 0;
  int initialized_count = 0;
  int shutdown_count = 0;

  class constructed_system final : public rt::system {
  public:
    DECLARE_SYSTEM(constructed_system);

    constructed_system() {
      ++constructed_count;
    }

    ~constructed_system() override {
      ++destroyed_count;
    }

    result init(rt::system_registry&) override {
      ++initialized_count;
      return {};
    }

    void deinit() noexcept override {
      ++shutdown_count;
    }
  };

  class throwing_constructor_system final : public rt::system {
  public:
    DECLARE_SYSTEM(throwing_constructor_system);

    throwing_constructor_system() {
      throw std::runtime_error{"expected construction exception"};
    }
  };

  REGISTER_SYSTEM_PRIORITY(constructed_system, 0)
  REGISTER_SYSTEM_PRIORITY(throwing_constructor_system, 10)

} // namespace

int main() {
  rt::system_registry registry;
  registry.set_logging_enabled(false);

  const auto result = registry.init_all();
  TEST_CHECK(!result.has_value());
  TEST_CHECK(result.error().code == rt::system_error_code::construction_failed);
  TEST_CHECK(result.error().message.find("expected construction exception") != std::string::npos);
  TEST_CHECK(!registry.initialized());
  TEST_CHECK(registry.size() == 0);
  TEST_CHECK(constructed_count == 1);
  TEST_CHECK(destroyed_count == 1);
  TEST_CHECK(initialized_count == 0);
  TEST_CHECK(shutdown_count == 0);

  return test_support::finish();
}
