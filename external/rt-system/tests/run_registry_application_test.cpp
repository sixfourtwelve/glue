#include "test_support.h"

#include <system.h>

#include <cstdlib>

namespace {

  int lifecycle_step = 0;
  int system_update_count = 0;

  class application_system final : public rt::system {
  public:
    DECLARE_SYSTEM(application_system);

    result init(rt::system_registry&) override {
      TEST_CHECK(++lifecycle_step == 1);
      return {};
    }

    result update() override {
      ++system_update_count;
      return {};
    }

    void deinit() noexcept override {
      TEST_CHECK(++lifecycle_step == 5);
    }
  };

  REGISTER_SYSTEM(application_system)

  class application final : public rt::registry_application {
  public:
    explicit application(bool fail_update) noexcept : m_fail_update(fail_update) {}

    rt::system::result init(rt::system_registry& registry) override {
      TEST_CHECK(++lifecycle_step == 2);
      TEST_CHECK(registry.find<application_system>() != nullptr);
      return {};
    }

    rt::system::result update(rt::system_registry& registry) override {
      TEST_CHECK(++lifecycle_step == 3);
      TEST_CHECK(registry.find<application_system>() != nullptr);
      TEST_CHECK(system_update_count == 0);

      if (m_fail_update) {
        return std::unexpected(rt::system::error{"expected application update failure"});
      }

      return {};
    }

    void deinit(rt::system_registry& registry) noexcept override {
      TEST_CHECK(++lifecycle_step == 4);
      TEST_CHECK(registry.find<application_system>() != nullptr);
      TEST_CHECK(system_update_count == 0);
    }

  private:
    bool m_fail_update;
  };

  void run_case(bool fail_update) {
    lifecycle_step = 0;
    system_update_count = 0;

    application app{fail_update};
    const auto result = rt::run_registry_application(app);

    TEST_CHECK(result == (fail_update ? EXIT_FAILURE : EXIT_SUCCESS));
    TEST_CHECK(lifecycle_step == 5);
    TEST_CHECK(system_update_count == 0);
  }

} // namespace

int main() {
  run_case(false);
  run_case(true);
  return test_support::finish();
}
