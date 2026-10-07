#pragma once

#include "glue/task_system.h"
#include "system.h"

namespace glue {
  class example_system final : public rt::system {
  public:
    DECLARE_SYSTEM(example_system);

    example_system() = default;
    ~example_system() override = default;

    rt::system::result init(rt::system_registry&) override;
    rt::system::result update() noexcept override;
    void deinit() noexcept override;

    std::future<void> disperse();

  private:
    task_system* m_tasks{nullptr};
  };
} // namespace glue
