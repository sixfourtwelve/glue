#include <glue/example_system.h>
#include <future>
#include <spdlog/spdlog.h>

#include "system.h"

namespace glue {
  rt::system::result example_system::init(rt::system_registry& registry) {
    m_tasks = &registry.require<task_system>();
    return rt::system::ok();
  }

  rt::system::result example_system::update() noexcept {
    return rt::system::ok();
  }

  void example_system::deinit() noexcept {}

  std::future<void> example_system::disperse() {
    return m_tasks->async([]() {
      spdlog::info("Disperse called");
    });
  }

  REGISTER_SYSTEM_PRIORITY(glue::example_system, 10)

} // namespace glue
