#pragma once

#include "system.h"
#include <taskflow/taskflow.hpp>

namespace glue {
  class message_system final : public rt::system {
  public:
    DECLARE_SYSTEM(message_system);

    message_system() = default;
    ~message_system() override = default;

    rt::system::result init(rt::system_registry&) override;
    rt::system::result update() noexcept override;
    void deinit() noexcept override;

    void disperse();

  private:
    tf::Executor m_executor;
    tf::Taskflow m_taskflow;
  };
} // namespace glue
