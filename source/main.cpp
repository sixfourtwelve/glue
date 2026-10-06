#include <system.h>

class application final : public rt::registry_application {
public:
  rt::system::result init(rt::system_registry&) override {
    return rt::system::ok();
  }

  rt::system::result update(rt::system_registry& registry) noexcept override {
    registry.set_logging_enabled(false);
    if (auto result = registry.update_all(); not result)
      return rt::system::fail("Failed to update a service");
    registry.set_logging_enabled(true);

    return rt::system::ok();
  }

  void deinit(rt::system_registry&) noexcept override {}
};

REGISTRY_MAIN(application)
