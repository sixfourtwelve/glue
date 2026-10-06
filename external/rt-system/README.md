# rt-system

`rt-system` is a small, RTTI-free C++ system registry with deterministic lifecycle ordering, failure rollback, and application-controlled updates.

## Requirements

- CMake 3.25 or newer
- A C++26 compiler
- Ninja is recommended for local builds

## Add it to a project

Add the repository as a submodule:

```sh
git submodule add git@github.com:sixfourtwelve/rt-system.git external/rt-system
git submodule update --init --recursive
```

Then link the CMake target:

```cmake
add_subdirectory(external/rt-system)
target_link_libraries(your_application PRIVATE rt::system)
```

## Define and register a system

Declare the type in a header:

```cpp
#pragma once

#include <system.h>

class renderer final : public rt::system {
public:
  DECLARE_SYSTEM(renderer);

  result init(rt::system_registry& registry) override;
  result update() override;
  void deinit() noexcept override;
};
```

Register it once in a `.cpp` file. Lower priorities initialize and update first.

```cpp
#include "renderer.h"

REGISTER_SYSTEM_PRIORITY(renderer, 100)
```

Registration macros support qualified names:

```cpp
REGISTER_SYSTEM(app::renderer)
```

Do not place `REGISTER_SYSTEM` in a header. Doing so creates one registration object in every translation unit that includes it, and duplicate registrations are rejected.

## Registry lifecycle

```cpp
rt::system_registry registry;

if (auto result = registry.init_all(); !result) {
  // Initialization failures automatically roll back every system whose
  // initialization started, including the system that failed.
  return EXIT_FAILURE;
}

if (auto result = registry.update_all(); !result) {
  registry.deinit_all();
  return EXIT_FAILURE;
}

registry.deinit_all();
```

Return `fail()` from a system lifecycle method to report an error:

```cpp
rt::system::result audio_system::init(rt::system_registry&) {
  return fail("Failed to initialize audio");
}
```

The registry follows these rules:

- Systems are constructed first, then initialized by priority and name.
- During initialization, `find<T>()` only exposes systems that completed initialization.
- Deinitialization runs in reverse initialization order.
- A partially initialized system is included in failure rollback.
- `update_all()` performs exactly one update pass.
- The application decides whether and when to call `update_all()`.
- Duplicate type or name registrations fail initialization.
- Type lookup does not require C++ RTTI.

## Required and optional systems

Use `find<T>()` when a system is optional:

```cpp
if (auto* audio = registry.find<audio_system>()) {
  audio->play();
}
```

Use `require<T>()` when absence indicates an invalid configuration or initialization order:

```cpp
auto& renderer_system = registry.require<renderer>();
```

`require<T>()` returns a mutable or const reference when the system is initialized. It throws
`std::logic_error` when the system is not registered or has not completed initialization. During
system initialization, that exception is converted into an initialization failure and normal
rollback still occurs.

## Application helper

Implement `rt::registry_application` and use `REGISTRY_MAIN` for a simple executable entry point:

```cpp
class application final : public rt::registry_application {
public:
  rt::system::result init(rt::system_registry&) override {
    return {};
  }

  rt::system::result update(rt::system_registry& registry) override {
    // A game could call this once per frame. A CLI may omit it entirely.
    return registry.update_all();
  }

  void deinit(rt::system_registry&) noexcept override {}
};

REGISTRY_MAIN(application)
```

`run_registry_application()` initializes the registry and application, calls the application once, and then deinitializes both. It does not call `update_all()` automatically and does not provide an event or frame loop.

## Logging

Registry lifecycle logging is enabled by default. Application and lifecycle failures use the same
timestamped format with an `error` level. Lifecycle messages can be disabled when configuring the
target:

```cmake
target_compile_definitions(rt-system PRIVATE SYSTEM_ENABLE_LOGGING=0)
```

Set `SYSTEM_ENABLE_LOG_COLORS=0` to keep logging while disabling ANSI colors. The `NO_COLOR` environment variable is also respected at runtime.

An application can replace or disable the registry's lifecycle event handler directly:

```cpp
registry.set_event_handler(my_event_handler);
registry.set_logging_enabled(false);
```

`set_logging_enabled(false)` only disables lifecycle messages emitted by the registry. It cannot
suppress logs written directly by a system through another logger such as spdlog. Failure messages
also remain enabled so errors are not silently discarded.

## Build and test

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```
