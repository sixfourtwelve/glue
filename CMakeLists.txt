cmake_minimum_required(VERSION 3.25)

project(glue VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/rt-system)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)

file(GLOB_RECURSE GLUE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

file(GLOB_RECURSE GLUE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/glue/*.h"
)

add_executable(glue
  ${GLUE_SOURCES}
  ${GLUE_HEADERS}
)

target_include_directories(glue PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(glue PRIVATE cxx_std_26)

if(MSVC)
    target_compile_options(glue PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
        /Zc:preprocessor
        /GR-
        /EHsc
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
    target_compile_options(glue PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
        -Wshadow
        -Wnon-virtual-dtor
        -Woverloaded-virtual
        -Wformat=2
        -Wundef
        -Werror=return-type
        -fno-rtti
        # Retain useful call stacks for profilers and crash reports.
        -fno-omit-frame-pointer
    )
else()
    message(FATAL_ERROR "Disabling RTTI is not configured for ${CMAKE_CXX_COMPILER_ID}")
endif()

target_link_libraries(glue PRIVATE
  rt::system
  nlohmann_json::nlohmann_json
  spdlog::spdlog
  Taskflow::Taskflow
)
set_target_properties(glue PROPERTIES
  CXX_EXTENSIONS OFF
)
