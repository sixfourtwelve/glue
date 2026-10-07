#pragma once

#include "glue/task_system.h"
#include "system.h"

namespace glue {
  class message_system final : public rt::system {
  public:
    DECLARE_SYSTEM(message_system);

    message_system() = default;
    ~message_system() override = default;

    rt::system::result init(rt::system_registry&) override;
    rt::system::result update() noexcept override;
    void deinit() noexcept override;

    std::future<void> disperse();

  private:
    task_system* m_tasks{nullptr};
  };
} // namespace glue
