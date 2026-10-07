#pragma once

#include <taskflow/taskflow.hpp>
#include <utility>
#include <system.h>

namespace glue {
  class task_system final : public rt::system {
  public:
    DECLARE_SYSTEM(task_system);
    task_system() = default;
    ~task_system() override = default;

    template <typename Function> [[nodiscard]] auto async(Function&& function) {
      return m_executor.async(std::forward<Function>(function));
    }

    template <typename Function> [[nodiscard]] auto async_silent(Function&& function) {
      return m_executor.silent_async(std::forward<Function>(function));
    }

    void deinit() noexcept override {
      m_executor.wait_for_all();
    }

  private:
    tf::Executor m_executor;
  };
} // namespace glue
