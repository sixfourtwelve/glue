#include <glue/base_system.h>

#include "system.h"

namespace glue {
  rt::system::result base_system::init(rt::system_registry& registry) {
    m_message_system = &registry.require<message_system>();

    if (not m_message_system)
      return rt::system::fail("Failed to fetch message_system");

    m_message_system->disperse();

    return rt::system::ok();
  }

  rt::system::result base_system::update() noexcept {
    return rt::system::ok();
  }

  void base_system::deinit() noexcept {}

  REGISTER_SYSTEM_PRIORITY(glue::base_system, 100);

} // namespace glue
