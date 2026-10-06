#include "test_support.h"

#include <system.h>

namespace {

  class early_system;
  class alpha_system;
  class zeta_system;

  int initialize_step = 0;
  int update_step = 0;
  int shutdown_step = 0;

  class early_system final : public rt::system {
  public:
    DECLARE_SYSTEM(early_system);

    result init(rt::system_registry& registry) override;
    result update() override;
    void deinit() noexcept override;
  };

  class alpha_system final : public rt::system {
  public:
    DECLARE_SYSTEM(alpha_system);

    result init(rt::system_registry& registry) override;
    result update() override;
    void deinit() noexcept override;
  };

  class zeta_system final : public rt::system {
  public:
    DECLARE_SYSTEM(zeta_system);

    result init(rt::system_registry& registry) override;
    result update() override;
    void deinit() noexcept override;
  };

  rt::system::result early_system::init(rt::system_registry& registry) {
    TEST_CHECK(++initialize_step == 1);
    TEST_CHECK(registry.find<early_system>() == nullptr);
    TEST_CHECK(registry.find<alpha_system>() == nullptr);
    TEST_CHECK(registry.find<zeta_system>() == nullptr);
    return {};
  }

  rt::system::result early_system::update() {
    TEST_CHECK(++update_step == 1);
    return {};
  }

  void early_system::deinit() noexcept {
    TEST_CHECK(++shutdown_step == 3);
  }

  rt::system::result alpha_system::init(rt::system_registry& registry) {
    TEST_CHECK(++initialize_step == 2);
    TEST_CHECK(registry.find<early_system>() != nullptr);
    TEST_CHECK(registry.find<alpha_system>() == nullptr);
    TEST_CHECK(registry.find<zeta_system>() == nullptr);
    return {};
  }

  rt::system::result alpha_system::update() {
    TEST_CHECK(++update_step == 2);
    return {};
  }

  void alpha_system::deinit() noexcept {
    TEST_CHECK(++shutdown_step == 2);
  }

  rt::system::result zeta_system::init(rt::system_registry& registry) {
    TEST_CHECK(++initialize_step == 3);
    TEST_CHECK(registry.find<early_system>() != nullptr);
    TEST_CHECK(registry.find<alpha_system>() != nullptr);
    TEST_CHECK(registry.find<zeta_system>() == nullptr);
    return {};
  }

  rt::system::result zeta_system::update() {
    TEST_CHECK(++update_step == 3);
    return {};
  }

  void zeta_system::deinit() noexcept {
    TEST_CHECK(++shutdown_step == 1);
  }

  REGISTER_SYSTEM_PRIORITY(zeta_system, 10)
  REGISTER_SYSTEM_PRIORITY(alpha_system, 10)
  REGISTER_SYSTEM_PRIORITY(early_system, -10)

} // namespace

int main() {
  rt::system_registry registry;
  registry.set_logging_enabled(false);

  const auto initialization = registry.init_all();
  TEST_CHECK(initialization.has_value());
  TEST_CHECK(registry.initialized());
  TEST_CHECK(registry.size() == 3);
  TEST_CHECK(initialize_step == 3);
  TEST_CHECK(registry.find<early_system>() != nullptr);
  TEST_CHECK(registry.find<alpha_system>() != nullptr);
  TEST_CHECK(registry.find<zeta_system>() != nullptr);

  const auto& const_registry = registry;
  TEST_CHECK(const_registry.find<early_system>() != nullptr);
  TEST_CHECK(const_registry.find<alpha_system>() != nullptr);
  TEST_CHECK(const_registry.find<zeta_system>() != nullptr);

  const auto update = registry.update_all();
  TEST_CHECK(update.has_value());
  TEST_CHECK(update_step == 3);

  registry.deinit_all();
  TEST_CHECK(!registry.initialized());
  TEST_CHECK(registry.size() == 0);
  TEST_CHECK(shutdown_step == 3);

  registry.deinit_all();
  TEST_CHECK(shutdown_step == 3);

  return test_support::finish();
}
