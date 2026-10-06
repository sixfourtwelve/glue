#include "test_support.h"

#include <system.h>

namespace {

  class ready_system;
  class failing_system;
  class unstarted_system;

  int initialize_step = 0;
  int shutdown_step = 0;
  bool unstarted_system_initialized = false;

  class ready_system final : public rt::system {
  public:
    DECLARE_SYSTEM(ready_system);

    result init(rt::system_registry&) override {
      TEST_CHECK(++initialize_step == 1);
      return {};
    }

    void deinit() noexcept override {
      TEST_CHECK(++shutdown_step == 2);
    }
  };

  class failing_system final : public rt::system {
  public:
    DECLARE_SYSTEM(failing_system);

    result init(rt::system_registry& registry) override;

    void deinit() noexcept override {
      TEST_CHECK(++shutdown_step == 1);
    }
  };

  class unstarted_system final : public rt::system {
  public:
    DECLARE_SYSTEM(unstarted_system);

    result init(rt::system_registry&) override {
      unstarted_system_initialized = true;
      return {};
    }
  };

  rt::system::result failing_system::init(rt::system_registry& registry) {
    TEST_CHECK(++initialize_step == 2);
    TEST_CHECK(registry.find<ready_system>() != nullptr);
    TEST_CHECK(registry.find<failing_system>() == nullptr);
    TEST_CHECK(registry.find<unstarted_system>() == nullptr);
    return fail("expected initialization failure");
  }

  REGISTER_SYSTEM_PRIORITY(ready_system, 0)
  REGISTER_SYSTEM_PRIORITY(failing_system, 10)
  REGISTER_SYSTEM_PRIORITY(unstarted_system, 20)

} // namespace

int main() {
  rt::system_registry registry;
  registry.set_logging_enabled(false);

  const auto result = registry.init_all();
  TEST_CHECK(!result.has_value());
  TEST_CHECK(result.error().code == rt::system_error_code::initialization_failed);
  TEST_CHECK(result.error().system_name == "failing_system");
  TEST_CHECK(result.error().message == "expected initialization failure");
  TEST_CHECK(!registry.initialized());
  TEST_CHECK(registry.size() == 0);
  TEST_CHECK(initialize_step == 2);
  TEST_CHECK(shutdown_step == 2);
  TEST_CHECK(!unstarted_system_initialized);
  TEST_CHECK(registry.find<ready_system>() == nullptr);
  TEST_CHECK(registry.find<failing_system>() == nullptr);
  TEST_CHECK(registry.find<unstarted_system>() == nullptr);

  registry.deinit_all();
  TEST_CHECK(shutdown_step == 2);

  return test_support::finish();
}
