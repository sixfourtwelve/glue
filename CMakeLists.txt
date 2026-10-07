cmake_minimum_required(VERSION 3.25)

set(PROJECT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/project.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_CONFIG_FILE}")
file(READ "${PROJECT_CONFIG_FILE}" PROJECT_CONFIG)
string(JSON PROJECT_SLUG GET "${PROJECT_CONFIG}" name)

project("${PROJECT_SLUG}" VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/rt-system)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Taskflow CONFIG REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
)

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/*.h"
)

add_executable("${PROJECT_NAME}"
  ${PROJECT_SOURCES}
  ${PROJECT_HEADERS}
)

target_include_directories("${PROJECT_NAME}" PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_26)

if(MSVC)
    target_compile_options("${PROJECT_NAME}" PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
        /Zc:preprocessor
        /GR-
        /EHsc
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
    target_compile_options("${PROJECT_NAME}" PRIVATE
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

target_link_libraries("${PROJECT_NAME}" PRIVATE
  rt::system
  nlohmann_json::nlohmann_json
  spdlog::spdlog
  Taskflow::Taskflow
)
set_target_properties("${PROJECT_NAME}" PROPERTIES
  CXX_EXTENSIONS OFF
)
