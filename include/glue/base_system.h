#pragma once

#include <glue/message_system.h>

#include "system.h"

namespace glue {
  class base_system final : public rt::system {
  public:
    DECLARE_SYSTEM(base_system);
    base_system() = default;
    ~base_system() override = default;

    rt::system::result init(rt::system_registry&) override;
    rt::system::result update() noexcept override;
    void deinit() noexcept override;

  private:
    glue::message_system* m_message_system{nullptr};
  };
} // namespace glue
