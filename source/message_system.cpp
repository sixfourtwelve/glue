#include <glue/message_system.h>
#include <spdlog/spdlog.h>

#include "system.h"

namespace glue {
  rt::system::result message_system::init(rt::system_registry&) {
    return rt::system::ok();
  }

  rt::system::result message_system::update() noexcept {
    return {};
  }

  void message_system::deinit() noexcept {}

  void message_system::disperse() {
    spdlog::info("Disperse called");
  }

  REGISTER_SYSTEM_PRIORITY(glue::message_system, 10);

} // namespace glue
