#include <glue/message_system.h>
#include <future>
#include <spdlog/spdlog.h>

#include "system.h"

namespace glue {
  rt::system::result message_system::init(rt::system_registry& registry) {
    m_tasks = &registry.require<task_system>();
    return rt::system::ok();
  }

  rt::system::result message_system::update() noexcept {
    return rt::system::ok();
  }

  void message_system::deinit() noexcept {}

  std::future<void> message_system::disperse() {
    return m_tasks->async([]() {
      spdlog::info("Disperse called");
    });
  }

  REGISTER_SYSTEM_PRIORITY(glue::message_system, 10)

} // namespace glue
