#include "system.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

#if defined(_WIN32)
  #include <io.h>
#elif defined(__unix__) || defined(__APPLE__)
  #include <unistd.h>
#endif

#include <algorithm>
#include <exception>
#include <format>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace rt {
  namespace detail {
    namespace {

      system_registration*& registration_head() noexcept {
        static system_registration* head = nullptr;
        return head;
      }

    } // namespace

    system_registration::system_registration(std::string_view name, std::int32_t priority,
                                             const void* type_id, system_factory factory,
                                             std::source_location location) noexcept
        : m_name(name),
          m_priority(priority),
          m_type_id(type_id),
          m_factory(factory),
          m_location(location),
          m_next(registration_head()) {
      registration_head() = this;
    }

    const system_registration* first_system_registration() noexcept {
      return registration_head();
    }

  } // namespace detail
  namespace {

    void write_log_message(std::string_view level, std::string_view color, std::string_view message,
                           std::string_view detail = {}) {
      const auto now = std::chrono::system_clock::now();
      const auto whole_seconds = std::chrono::floor<std::chrono::seconds>(now);
      const auto milliseconds =
          std::chrono::duration_cast<std::chrono::milliseconds>(now - whole_seconds).count();
      const auto time = std::chrono::system_clock::to_time_t(now);
      std::tm local_time{};

#if defined(_WIN32)
      static_cast<void>(::localtime_s(&local_time, &time));
#else
      static_cast<void>(::localtime_r(&time, &local_time));
#endif

      bool use_color = false;

#if SYSTEM_ENABLE_LOG_COLORS
      if (std::getenv("NO_COLOR") == nullptr) {
  #if defined(_WIN32)
        use_color = ::_isatty(::_fileno(stderr)) != 0;
  #elif defined(__unix__) || defined(__APPLE__)
        use_color = ::isatty(::fileno(stderr)) != 0;
  #endif
      }
#else
      static_cast<void>(color);
#endif

      std::ostringstream output;
      output << '[' << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0')
             << std::setw(3) << milliseconds << "] [";

      if (use_color) {
        output << color;
      }

      output << level;

      if (use_color) {
        output << "\033[0m";
      }

      output << "] " << message;

      if (!detail.empty()) {
        output << ": " << detail;
      }

      output << '\n';

      static std::mutex output_mutex;
      const std::lock_guard lock{output_mutex};
      std::clog << output.str();
    }

    [[nodiscard]] system_error
    make_system_error(system_error_code code, std::string_view system_name, std::string message) {
      return {
          .code = code,
          .system_name = std::string(system_name),
          .message = std::move(message),
      };
    }

    [[nodiscard]] std::string
    system_registration_location(const detail::system_registration& registration) {
      return std::format("{}:{}", registration.location().file_name(),
                         registration.location().line());
    }

  } // namespace

  void log_error(std::string_view message, std::string_view detail) noexcept {
    try {
      write_log_message("error", "\033[31m", message, detail);
    } catch (...) {
      static_cast<void>(std::fwrite(message.data(), sizeof(char), message.size(), stderr));
      if (!detail.empty()) {
        static constexpr std::string_view separator{": "};
        static_cast<void>(std::fwrite(separator.data(), sizeof(char), separator.size(), stderr));
        static_cast<void>(std::fwrite(detail.data(), sizeof(char), detail.size(), stderr));
      }
      static_cast<void>(std::fputc('\n', stderr));
    }
  }

  void log_system_event(system_event event, std::string_view system_name) {
#if SYSTEM_ENABLE_LOGGING
    std::string_view action;

    switch (event) {
    case system_event::initializing:
      action = "Initializing system";
      break;
    case system_event::updating:
      action = "Updating system";
      break;
    case system_event::deinitializing:
      action = "Deinitializing system";
      break;
    }

    write_log_message("info", "\033[32m", action, system_name);
#else
    static_cast<void>(event);
    static_cast<void>(system_name);
#endif
  }

  system_registry::~system_registry() {
    deinit_all();
  }

  int run_registry_application(registry_application& application) {
    system_registry registry;

    if (auto result = registry.init_all(); !result) {
      const auto& error = result.error();

      if (error.code == system_error_code::initialization_failed && !error.system_name.empty()) {
        log_error(std::format("Failed to initialize system '{}'", error.system_name),
                  error.message);
      } else {
        log_error("Failed to initialize system registry", error.message);
      }

      return EXIT_FAILURE;
    }

    const auto deinit = [&application, &registry] {
      application.deinit(registry);
      registry.deinit_all();
    };

    try {
      if (auto result = application.init(registry); !result) {
        log_error("Failed to initialize application", result.error().message);
        deinit();
        return EXIT_FAILURE;
      }
    } catch (const std::exception& exception) {
      log_error("Failed to initialize application", exception.what());
      deinit();
      return EXIT_FAILURE;
    } catch (...) {
      log_error("Failed to initialize application with an unknown exception");
      deinit();
      return EXIT_FAILURE;
    }

    try {
      if (auto result = application.update(registry); !result) {
        log_error("Failed to update application", result.error().message);
        deinit();
        return EXIT_FAILURE;
      }
    } catch (const std::exception& exception) {
      log_error("Failed to update application", exception.what());
      deinit();
      return EXIT_FAILURE;
    } catch (...) {
      log_error("Failed to update application with an unknown exception");
      deinit();
      return EXIT_FAILURE;
    }

    deinit();
    return EXIT_SUCCESS;
  }

  std::expected<void, system_error> system_registry::init_all() {
    if (m_initialized) {
      return std::unexpected(make_system_error(system_error_code::already_initialized, {},
                                               "The system registry is already initialized"));
    }

    m_entries.clear();
    m_deinit_count = 0;

    std::vector<const detail::system_registration*> registrations;
    for (auto* registration = detail::first_system_registration(); registration != nullptr;
         registration = registration->next()) {
      registrations.push_back(registration);
    }

    std::ranges::sort(registrations, [](const auto* left, const auto* right) {
      return std::tuple{left->priority(), left->name()} <
             std::tuple{right->priority(), right->name()};
    });

    std::vector<const detail::system_registration*> unique_registrations;
    unique_registrations.reserve(registrations.size());

    for (const auto* registration : registrations) {
      const auto same_type =
          std::ranges::find_if(unique_registrations, [registration](const auto* candidate) {
            return candidate->type_id() == registration->type_id();
          });

      if (same_type != unique_registrations.end()) {
        return std::unexpected(make_system_error(
            system_error_code::duplicate_registration, registration->name(),
            std::format("System type '{}' is registered more than once ({} and {})",
                        registration->name(), system_registration_location(**same_type),
                        system_registration_location(*registration))));
      }

      const auto same_name =
          std::ranges::find_if(unique_registrations, [registration](const auto* candidate) {
            return candidate->name() == registration->name();
          });

      if (same_name != unique_registrations.end()) {
        return std::unexpected(make_system_error(
            system_error_code::duplicate_registration, registration->name(),
            std::format("Different system types use the name '{}' ({} and {})",
                        registration->name(), system_registration_location(**same_name),
                        system_registration_location(*registration))));
      }

      unique_registrations.push_back(registration);
    }

    registrations = std::move(unique_registrations);
    m_entries.reserve(registrations.size());

    for (const auto* registration : registrations) {
      try {
        m_entries.push_back({
            .registration = registration,
            .instance = registration->factory()(),
        });
      } catch (const std::exception& exception) {
        m_entries.clear();
        return std::unexpected(
            make_system_error(system_error_code::construction_failed, registration->name(),
                              std::format("Failed to construct system '{}': {}",
                                          registration->name(), exception.what())));
      } catch (...) {
        m_entries.clear();
        return std::unexpected(make_system_error(
            system_error_code::construction_failed, registration->name(),
            std::format("Failed to construct system '{}' with an unknown exception",
                        registration->name())));
      }
    }

    for (auto& entry : m_entries) {
      notify(system_event::initializing, entry.registration->name());
      entry.lifecycle = entry_state::initializing;
      ++m_deinit_count;

      try {
        if (auto result = entry.instance->init(*this); !result) {
          const auto error =
              make_system_error(system_error_code::initialization_failed,
                                entry.registration->name(), std::move(result.error().message));
          rollback_initialization();
          return std::unexpected(error);
        }
      } catch (const std::exception& exception) {
        const auto error = make_system_error(system_error_code::initialization_failed,
                                             entry.registration->name(), exception.what());
        rollback_initialization();
        return std::unexpected(error);
      } catch (...) {
        const auto error = make_system_error(system_error_code::initialization_failed,
                                             entry.registration->name(), "Unknown exception");
        rollback_initialization();
        return std::unexpected(error);
      }

      entry.lifecycle = entry_state::initialized;
    }

    m_initialized = true;
    return {};
  }

  std::expected<void, system_error> system_registry::update_all() {
    if (!m_initialized) {
      return std::unexpected(make_system_error(system_error_code::not_initialized, {},
                                               "The system registry is not initialized"));
    }

    for (auto& entry : m_entries) {
      notify(system_event::updating, entry.registration->name());

      try {
        if (auto result = entry.instance->update(); !result) {
          return std::unexpected(
              make_system_error(system_error_code::update_failed, entry.registration->name(),
                                std::format("System '{}' failed to update: {}",
                                            entry.registration->name(), result.error().message)));
        }
      } catch (const std::exception& exception) {
        return std::unexpected(
            make_system_error(system_error_code::update_failed, entry.registration->name(),
                              std::format("Failed to update system '{}': {}",
                                          entry.registration->name(), exception.what())));
      } catch (...) {
        return std::unexpected(
            make_system_error(system_error_code::update_failed, entry.registration->name(),
                              std::format("Failed to update system '{}' with an unknown exception",
                                          entry.registration->name())));
      }
    }

    return {};
  }

  void system_registry::deinit_all() noexcept {
    rollback_initialization();
  }

  void system_registry::throw_required_system_unavailable(std::string_view system_name) {
    throw std::logic_error(std::format(
        "Required system '{}' is unavailable because it is not registered or has not completed "
        "initialization",
        system_name));
  }

  void system_registry::notify(system_event event, std::string_view system_name) const noexcept {
    if (m_event_handler == nullptr) {
      return;
    }

    try {
      m_event_handler(event, system_name);
    } catch (...) {
      // Diagnostics must never interrupt system lifecycle management.
    }
  }

  void system_registry::rollback_initialization() noexcept {
    while (m_deinit_count > 0) {
      --m_deinit_count;
      const auto& entry = m_entries[m_deinit_count];
      notify(system_event::deinitializing, entry.registration->name());
      entry.instance->deinit();
    }

    m_initialized = false;
    m_entries.clear();
  }

} // namespace rt
