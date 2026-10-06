#include "test_support.h"

#include <system.h>

namespace duplicate_fixture {

  inline int construction_count = 0;

  class duplicate_system final : public rt::system {
  public:
    DECLARE_SYSTEM(duplicate_system);

    duplicate_system() {
      ++construction_count;
    }
  };

} // namespace duplicate_fixture

REGISTER_SYSTEM(duplicate_fixture::duplicate_system)
REGISTER_SYSTEM(duplicate_fixture::duplicate_system)

int main() {
  rt::system_registry registry;
  registry.set_logging_enabled(false);

  const auto result = registry.init_all();
  TEST_CHECK(!result.has_value());
  TEST_CHECK(result.error().code == rt::system_error_code::duplicate_registration);
  TEST_CHECK(result.error().system_name == "duplicate_fixture::duplicate_system");
  TEST_CHECK(result.error().message.find("registered more than once") != std::string::npos);
  TEST_CHECK(duplicate_fixture::construction_count == 0);
  TEST_CHECK(!registry.initialized());
  TEST_CHECK(registry.size() == 0);

  return test_support::finish();
}
