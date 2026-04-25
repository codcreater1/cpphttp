cmake_minimum_required(VERSION 3.16)
project(cpphttp VERSION 1.0.0 LANGUAGES CXX)

# ── C++17 ─────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler warnings ─────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wno-unused-parameter)
endif()

# ── Library ───────────────────────────────────────────────────────────────────
add_library(cpphttp
    src/server.cpp
    src/request.cpp
    src/response.cpp
    src/router.cpp
    src/logger.cpp
)

target_include_directories(cpphttp PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# pthreads for std::thread
find_package(Threads REQUIRED)
target_link_libraries(cpphttp PUBLIC Threads::Threads)

# ── Examples ──────────────────────────────────────────────────────────────────
option(BUILD_EXAMPLES "Build example binaries" ON)

if(BUILD_EXAMPLES)
    add_executable(hello_world examples/hello_world.cpp)
    target_link_libraries(hello_world PRIVATE cpphttp)

    add_executable(todo_api examples/todo_api.cpp)
    target_link_libraries(todo_api PRIVATE cpphttp)
endif()

# ── Tests (optional) ──────────────────────────────────────────────────────────
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS cpphttp EXPORT cpphttpTargets)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT cpphttpTargets
    FILE cpphttpTargets.cmake
    NAMESPACE cpphttp::
    DESTINATION lib/cmake/cpphttp
)
