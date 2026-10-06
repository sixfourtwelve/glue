#include "test_support.h"

#include <system.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

  bool fail_system_initialization = false;

  class quiet_system final : public rt::system {
  public:
    DECLARE_SYSTEM(quiet_system);

    result init(rt::system_registry&) override {
      if (fail_system_initialization) {
        return fail("Failed to fetch message_system");
      }

      return {};
    }
  };

  REGISTER_SYSTEM(quiet_system)

  class test_application final : public rt::registry_application {
  public:
    rt::system::result init(rt::system_registry&) override {
      return {};
    }

    rt::system::result update(rt::system_registry&) override {
      return {};
    }

    void deinit(rt::system_registry&) noexcept override {}
  };

  class clog_capture final {
  public:
    clog_capture() : m_previous_buffer(std::clog.rdbuf(m_output.rdbuf())) {}

    ~clog_capture() {
      std::clog.rdbuf(m_previous_buffer);
    }

    clog_capture(const clog_capture&) = delete;
    clog_capture& operator=(const clog_capture&) = delete;

    [[nodiscard]] std::string output() const {
      return m_output.str();
    }

    void clear() {
      m_output.str({});
      m_output.clear();
    }

  private:
    std::ostringstream m_output;
    std::streambuf* m_previous_buffer;
  };

} // namespace

int main() {
  clog_capture capture;
  rt::system_registry registry;
  registry.set_logging_enabled(false);

  TEST_CHECK(registry.init_all().has_value());
  TEST_CHECK(registry.update_all().has_value());
  registry.deinit_all();
  TEST_CHECK(capture.output().empty());

  rt::log_error("Expected failure", "error detail");
  TEST_CHECK(capture.output().find("error") != std::string::npos);
  TEST_CHECK(capture.output().find("Expected failure: error detail") != std::string::npos);

  capture.clear();
  registry.set_logging_enabled(true);
  TEST_CHECK(registry.init_all().has_value());
  TEST_CHECK(registry.update_all().has_value());
  registry.deinit_all();
  TEST_CHECK(capture.output().find("Initializing system: quiet_system") != std::string::npos);
  TEST_CHECK(capture.output().find("Updating system: quiet_system") != std::string::npos);
  TEST_CHECK(capture.output().find("Deinitializing system: quiet_system") != std::string::npos);

  capture.clear();
  fail_system_initialization = true;
  test_application application;
  TEST_CHECK(rt::run_registry_application(application) == EXIT_FAILURE);
  TEST_CHECK(capture.output().find("Failed to initialize system 'quiet_system': Failed to fetch "
                                   "message_system") != std::string::npos);

  return test_support::finish();
}
