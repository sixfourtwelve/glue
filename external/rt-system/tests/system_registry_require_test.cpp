#include "test_support.h"

#include <system.h>

#include <stdexcept>
#include <string_view>

namespace {

  class first_system;
  class second_system;
  class absent_system;

  template <typename T> void expect_unavailable(rt::system_registry& registry) {
    try {
      static_cast<void>(registry.require<T>());
      TEST_CHECK(false);
    } catch (const std::logic_error& exception) {
      TEST_CHECK(std::string_view{exception.what()}.find(T::get_class_name()) !=
                 std::string_view::npos);
    } catch (...) {
      TEST_CHECK(false);
    }
  }

  class first_system final : public rt::system {
  public:
    DECLARE_SYSTEM(first_system);

    result init(rt::system_registry& registry) override;
  };

  class second_system final : public rt::system {
  public:
    DECLARE_SYSTEM(second_system);

    result init(rt::system_registry& registry) override;
  };

  class absent_system final : public rt::system {
  public:
    DECLARE_SYSTEM(absent_system);
  };

  rt::system::result first_system::init(rt::system_registry& registry) {
    expect_unavailable<first_system>(registry);
    expect_unavailable<second_system>(registry);
    expect_unavailable<absent_system>(registry);
    return {};
  }

  rt::system::result second_system::init(rt::system_registry& registry) {
    TEST_CHECK(&registry.require<first_system>() == registry.find<first_system>());
    expect_unavailable<second_system>(registry);
    expect_unavailable<absent_system>(registry);
    return {};
  }

  REGISTER_SYSTEM_PRIORITY(first_system, 0)
  REGISTER_SYSTEM_PRIORITY(second_system, 10)

} // namespace

int main() {
  rt::system_registry registry;
  registry.set_logging_enabled(false);

  const auto initialization = registry.init_all();
  TEST_CHECK(initialization.has_value());
  TEST_CHECK(&registry.require<first_system>() == registry.find<first_system>());
  TEST_CHECK(&registry.require<second_system>() == registry.find<second_system>());
  expect_unavailable<absent_system>(registry);

  const auto& const_registry = registry;
  TEST_CHECK(&const_registry.require<first_system>() == const_registry.find<first_system>());
  TEST_CHECK(&const_registry.require<second_system>() == const_registry.find<second_system>());

  registry.deinit_all();
  expect_unavailable<first_system>(registry);
  expect_unavailable<second_system>(registry);

  return test_support::finish();
}
