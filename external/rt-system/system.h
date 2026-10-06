#pragma once

#ifndef SYSTEM_ENABLE_LOGGING
  #define SYSTEM_ENABLE_LOGGING 1
#endif

#ifndef SYSTEM_ENABLE_LOG_COLORS
  #define SYSTEM_ENABLE_LOG_COLORS 1
#endif

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <expected>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace rt {
  namespace detail {

    template <typename T> [[nodiscard]] inline const void* system_type_id() noexcept {
      static const char id{};
      return &id;
    }

  } // namespace detail

  class system_registry;

  class system {
  public:
    struct error {
      std::string message;
    };

    using result = std::expected<void, error>;
    using this_class = system;

    [[nodiscard]] static result fail(std::string message) {
      return std::unexpected(error{
          .message = std::move(message),
      });
    }

    // Optional, you can just return {} for success, but this is a convenience function to make it
    // explicit.
    [[nodiscard]] static result ok() {
      return {};
    }

    virtual ~system() = default;

    [[nodiscard]] static constexpr const char* get_class_name() noexcept {
      return "system";
    }

    [[nodiscard]] static const void* get_class_id() noexcept {
      return detail::system_type_id<this_class>();
    }

    template <typename T> [[nodiscard]] bool is_instance_of() const noexcept {
      return is_type(detail::system_type_id<T>());
    }

    template <typename T> [[nodiscard]] static constexpr bool is_derived_from() noexcept {
      return std::is_base_of_v<T, this_class>;
    }

    [[nodiscard]] virtual bool is_type(const void* id) const noexcept {
      return id == get_class_id();
    }

    [[nodiscard]] virtual result init(system_registry&) {
      return {};
    }

    [[nodiscard]] virtual result update() {
      return {};
    }

    virtual void deinit() noexcept {}
  };

  // Compatibility aliases for code using the original lifecycle-specific names.
  using system_init_error = system::error;
  using system_init_result = system::result;
  using system_update_error = system::error;
  using system_update_result = system::result;

  enum class system_event : std::uint8_t {
    initializing,
    updating,
    deinitializing,
  };

  using system_event_handler = void (*)(system_event event, std::string_view system_name);

  void log_error(std::string_view message, std::string_view detail = {}) noexcept;
  void log_system_event(system_event event, std::string_view system_name);

  enum class system_error_code : std::uint8_t {
    already_initialized,
    not_initialized,
    duplicate_registration,
    construction_failed,
    initialization_failed,
    update_failed,
  };

  struct system_error {
    system_error_code code;
    std::string system_name;
    std::string message;
  };

  namespace detail {

    using system_factory = std::unique_ptr<system> (*)();

    class system_registration {
    public:
      system_registration(std::string_view name, std::int32_t priority, const void* type_id,
                          system_factory factory,
                          std::source_location location = std::source_location::current()) noexcept;

      system_registration(const system_registration&) = delete;
      system_registration& operator=(const system_registration&) = delete;
      system_registration(system_registration&&) = delete;
      system_registration& operator=(system_registration&&) = delete;

      [[nodiscard]] std::string_view name() const noexcept {
        return m_name;
      }

      [[nodiscard]] std::int32_t priority() const noexcept {
        return m_priority;
      }

      [[nodiscard]] const void* type_id() const noexcept {
        return m_type_id;
      }

      [[nodiscard]] system_factory factory() const noexcept {
        return m_factory;
      }

      [[nodiscard]] const std::source_location& location() const noexcept {
        return m_location;
      }

      [[nodiscard]] const system_registration* next() const noexcept {
        return m_next;
      }

    private:
      std::string_view m_name;
      std::int32_t m_priority;
      const void* m_type_id;
      system_factory m_factory;
      std::source_location m_location;
      system_registration* m_next{nullptr};
    };

    [[nodiscard]] const system_registration* first_system_registration() noexcept;

    template <typename T>
      requires std::derived_from<T, system> && std::default_initializable<T>
    class registered_system final : public system_registration {
    public:
      registered_system(std::string_view name, std::int32_t priority,
                        std::source_location location = std::source_location::current()) noexcept
          : system_registration(name, priority, system_type_id<T>(), &create, location) {}

    private:
      [[nodiscard]] static std::unique_ptr<system> create() {
        return std::make_unique<T>();
      }
    };

  } // namespace detail

  // The systems registry, you can use this to set the event handler
  // access other systems via find<T>() or require<T>()
  class system_registry final {
  public:
    system_registry() = default;
    ~system_registry();

    system_registry(const system_registry&) = delete;
    system_registry& operator=(const system_registry&) = delete;
    system_registry(system_registry&&) = delete;
    system_registry& operator=(system_registry&&) = delete;

    [[nodiscard]] std::expected<void, system_error> init_all();
    [[nodiscard]] std::expected<void, system_error> update_all();
    void deinit_all() noexcept;

    void set_event_handler(system_event_handler handler) noexcept {
      m_event_handler = handler;
    }

    // disable logging of system events (initializing, updating, deinitializing),
    // handy for loops that call update_all() frequently, because otherwise its
    // really fucking annoying
    void set_logging_enabled(bool enabled) noexcept {
      m_event_handler = enabled ? &log_system_event : nullptr;
    }

    [[nodiscard]] bool initialized() const noexcept {
      return m_initialized;
    }

    [[nodiscard]] std::size_t size() const noexcept {
      return m_entries.size();
    }

    template <typename T>
      requires std::derived_from<T, system>
    [[nodiscard]] T* find() noexcept {
      for (auto& entry : m_entries) {
        if (entry.lifecycle == entry_state::initialized && entry.instance->is_instance_of<T>()) {
          return static_cast<T*>(entry.instance.get());
        }
      }

      return nullptr;
    }

    // Use required<T> if you want to throw an exception when the system is not found.
    // This one is more lazy and will return nullptr if the system is not found.
    template <typename T>
      requires std::derived_from<T, system>
    [[nodiscard]] const T* find() const noexcept {
      for (const auto& entry : m_entries) {
        if (entry.lifecycle == entry_state::initialized && entry.instance->is_instance_of<T>()) {
          return static_cast<const T*>(entry.instance.get());
        }
      }

      return nullptr;
    }

    template <typename T>
      requires std::derived_from<T, system>
    [[nodiscard]] T& require() {
      if (auto* instance = find<T>(); instance != nullptr) {
        return *instance;
      }

      throw_required_system_unavailable(T::get_class_name());
    }

    template <typename T>
      requires std::derived_from<T, system>
    [[nodiscard]] const T& require() const {
      if (const auto* instance = find<T>(); instance != nullptr) {
        return *instance;
      }

      throw_required_system_unavailable(T::get_class_name());
    }

  private:
    enum class entry_state : std::uint8_t {
      constructed,
      initializing,
      initialized,
    };

    struct entry {
      const detail::system_registration* registration;
      std::unique_ptr<system> instance;
      entry_state lifecycle{entry_state::constructed};
    };

    [[noreturn]] static void throw_required_system_unavailable(std::string_view system_name);
    void notify(system_event event, std::string_view system_name) const noexcept;
    void rollback_initialization() noexcept;

    std::vector<entry> m_entries;
    std::size_t m_deinit_count{0};
    system_event_handler m_event_handler{&log_system_event};
    bool m_initialized{false};
  };

  // The programs entrypoint, which inits everything for you
  // used with REGISTRY_MAIN(my_application_or_whatever)
  class registry_application {
  public:
    virtual ~registry_application() = default;

    [[nodiscard]] virtual system::result init(system_registry& registry) = 0;
    [[nodiscard]] virtual system::result update(system_registry& registry) = 0;
    virtual void deinit(system_registry& registry) noexcept = 0;
  };

  [[nodiscard]] int run_registry_application(registry_application& application);

  namespace detail {

    template <typename T>
      requires std::derived_from<T, registry_application> && std::default_initializable<T>
    [[nodiscard]] int registry_main() noexcept {
      std::unique_ptr<T> application;

      try {
        application = std::make_unique<T>();
      } catch (const std::exception& exception) {
        log_error("Failed to construct application", exception.what());
        return EXIT_FAILURE;
      } catch (...) {
        log_error("Failed to construct application with an unknown exception");
        return EXIT_FAILURE;
      }

      try {
        return run_registry_application(*application);
      } catch (const std::exception& exception) {
        log_error("Application failed with an unexpected exception", exception.what());
      } catch (...) {
        log_error("Application failed with an unknown exception");
      }

      return EXIT_FAILURE;
    }

  } // namespace detail

} // namespace rt

// poor mans RTTI
#define DECLARE_SYSTEM(Class)                                                                      \
  using this_class = Class;                                                                        \
  using base_class = ::rt::system;                                                                 \
                                                                                                   \
  [[nodiscard]] static constexpr const char* get_class_name() noexcept {                           \
    return #Class;                                                                                 \
  }                                                                                                \
                                                                                                   \
  [[nodiscard]] static const void* get_class_id() noexcept {                                       \
    return ::rt::detail::system_type_id<this_class>();                                             \
  }                                                                                                \
                                                                                                   \
  template <typename T> [[nodiscard]] static constexpr bool is_derived_from() noexcept {           \
    return std::is_base_of_v<T, this_class>;                                                       \
  }                                                                                                \
                                                                                                   \
  [[nodiscard]] bool is_type(const void* id) const noexcept override {                             \
    return id == get_class_id() || base_class::is_type(id);                                        \
  }

#define SYSTEM_DETAIL_CONCAT_IMPL(Left, Right) Left##Right
#define SYSTEM_DETAIL_CONCAT(Left, Right) SYSTEM_DETAIL_CONCAT_IMPL(Left, Right)
#define SYSTEM_DETAIL_REGISTRATION_NAME(Index) SYSTEM_DETAIL_CONCAT(system_registration_, Index)

#define SYSTEM_DETAIL_REGISTER_SYSTEM(Class, Priority, Index)                                      \
  namespace {                                                                                      \
    [[maybe_unused]] const ::rt::detail::registered_system<Class>                                  \
        SYSTEM_DETAIL_REGISTRATION_NAME(Index){#Class, Priority};                                  \
  }

// Register a system with a specific priority. Lower numbers are initialized first.
// Handy if in a game, you want to init the window first, then the input, then ui, then renderer
#define REGISTER_SYSTEM_PRIORITY(Class, Priority)                                                  \
  SYSTEM_DETAIL_REGISTER_SYSTEM(Class, Priority, __COUNTER__)

#define REGISTER_SYSTEM(Class) REGISTER_SYSTEM_PRIORITY(Class, 0)

#define REGISTRY_MAIN(Application)                                                                 \
  int main() {                                                                                     \
    return ::rt::detail::registry_main<Application>();                                             \
  }
