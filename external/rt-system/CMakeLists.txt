cmake_minimum_required(VERSION 3.25)

project(rt-system VERSION 0.1.0 LANGUAGES CXX)

function(rt_system_configure_target target)
    target_compile_features(${target} PRIVATE cxx_std_26)

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /Zc:preprocessor
            /GR-
            /EHsc
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
        target_compile_options(${target} PRIVATE
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
            -fno-omit-frame-pointer
        )
    else()
        message(FATAL_ERROR "Disabling RTTI is not configured for ${CMAKE_CXX_COMPILER_ID}")
    endif()

    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
endfunction()

add_library(rt-system system.cpp)
add_library(rt::system ALIAS rt-system)

target_include_directories(rt-system PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
target_compile_features(rt-system PUBLIC cxx_std_26)
rt_system_configure_target(rt-system)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
endif()

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    function(add_rt_system_test target)
        add_executable(${target} "tests/${target}.cpp")
        target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
        target_link_libraries(${target} PRIVATE rt::system)
        rt_system_configure_target(${target})
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    add_rt_system_test(logging_test)
    add_rt_system_test(system_registry_lifecycle_test)
    add_rt_system_test(system_registry_require_test)
    add_rt_system_test(system_registry_rollback_test)
    add_rt_system_test(system_registry_exception_test)
    add_rt_system_test(system_registry_construction_test)
    add_rt_system_test(system_registry_duplicate_test)
    add_rt_system_test(run_registry_application_test)
endif()
