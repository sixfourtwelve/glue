#include <spdlog/spdlog.h>
#include <system.h>
#include <glue/task_system.h>

class application final : public rt::registry_application {
public:
  rt::system::result init(rt::system_registry& registry) override {
    m_tasks = &registry.require<glue::task_system>();

    (m_tasks->async([]() {
      spdlog::info("Hello world!");
    })).get();

    return rt::system::ok();
  }

  rt::system::result update(rt::system_registry& registry) noexcept override {
    if (auto result = registry.update_all(); not result)
      return rt::system::fail("Failed to update a service");

    return rt::system::ok();
  }

  void deinit(rt::system_registry&) noexcept override {}

private:
  glue::task_system* m_tasks{nullptr};
};

REGISTRY_MAIN(application)
