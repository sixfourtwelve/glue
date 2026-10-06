#include "test_support.h"

#include <system.h>

#include <stdexcept>

namespace {

  int shutdown_step = 0;

  class initialized_system final : public rt::system {
  public:
    DECLARE_SYSTEM(initialized_system);

    void deinit() noexcept override {
      TEST_CHECK(++shutdown_step == 2);
    }
  };

  class throwing_system final : public rt::system {
  public:
    DECLARE_SYSTEM(throwing_system);

    result init(rt::system_registry& registry) override {
      TEST_CHECK(registry.find<initialized_system>() != nullptr);
      TEST_CHECK(registry.find<throwing_system>() == nullptr);
      throw std::runtime_error{"expected initialization exception"};
    }

    void deinit() noexcept override {
      TEST_CHECK(++shutdown_step == 1);
    }
  };

  REGISTER_SYSTEM_PRIORITY(initialized_system, 0)
  REGISTER_SYSTEM_PRIORITY(throwing_system, 10)

} // namespace

int main() {
  rt::system_registry registry;
  registry.set_logging_enabled(false);

  const auto result = registry.init_all();
  TEST_CHECK(!result.has_value());
  TEST_CHECK(result.error().code == rt::system_error_code::initialization_failed);
  TEST_CHECK(result.error().system_name == "throwing_system");
  TEST_CHECK(result.error().message == "expected initialization exception");
  TEST_CHECK(!registry.initialized());
  TEST_CHECK(registry.size() == 0);
  TEST_CHECK(shutdown_step == 2);

  return test_support::finish();
}
