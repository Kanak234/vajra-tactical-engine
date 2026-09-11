cmake_minimum_required(VERSION 3.22)
project(Vajra VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------- Sanitizers
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_COVERAGE "Enable code coverage profiling" OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

if(ENABLE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# ---------------------------------------------------------------- dependencies
find_package(SDL2 QUIET)
find_package(OpenGL QUIET)
find_package(Threads REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET ON)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1)
FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.2)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3)
FetchContent_MakeAvailable(glm entt json)

# ---------------------------------------------------------------- headless engine library
# Core logic, physics, AI navigation, and gameplay state without OpenGL/SDL dependencies
add_library(vajra_core STATIC
    src/Physics/Collision.cpp
    src/AI/Navigation.cpp
    src/Scene/Level.cpp
    src/Gameplay/Weapon.cpp
    src/Gameplay/MissionSystem.cpp
)
target_include_directories(vajra_core PUBLIC src)
target_link_libraries(vajra_core PUBLIC
    glm::glm
    EnTT::EnTT
    nlohmann_json::nlohmann_json
    Threads::Threads)
target_compile_definitions(vajra_core PUBLIC
    VAJRA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
    GLM_ENABLE_EXPERIMENTAL)

# ---------------------------------------------------------------- engine executable (when GUI available)
if(SDL2_FOUND AND OPENGL_FOUND)
    message(STATUS "SDL2 and OpenGL detected: configuring game executable target 'Vajra'")
    # GLAD is vendored (generator output) — see ThirdParty/glad
    add_library(glad STATIC ThirdParty/glad/src/glad.c)
    target_include_directories(glad PUBLIC ThirdParty/glad/include)

    file(GLOB_RECURSE VAJRA_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
    add_executable(Vajra ${VAJRA_SOURCES})

    target_include_directories(Vajra PRIVATE src)
    target_link_libraries(Vajra PRIVATE
        vajra_core
        SDL2::SDL2 OpenGL::GL glad
        ${CMAKE_DL_LIBS})

    target_compile_definitions(Vajra PRIVATE
        VAJRA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
        GLM_ENABLE_EXPERIMENTAL)

    # copy assets next to the binary so the game runs from the build directory
    add_custom_command(TARGET Vajra POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Vajra>/assets)

    # ------------------------------------------------------------------- install
    include(GNUInstallDirs)
    set_target_properties(Vajra PROPERTIES OUTPUT_NAME vajra)
    install(TARGETS Vajra RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY assets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/vajra/assets)
else()
    message(STATUS "SDL2 or OpenGL development packages not found: building headless vajra_core and test targets")
endif()

# ------------------------------------------------------------ test suite target
enable_testing()
add_executable(vajra_tests
    tests/smoke.cpp
    tests/test_collision.cpp
    tests/test_navigation.cpp
    tests/test_gameplay.cpp
    tests/test_level.cpp
)
target_link_libraries(vajra_tests PRIVATE vajra_core)
target_compile_definitions(vajra_tests PRIVATE
    VAJRA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
    GLM_ENABLE_EXPERIMENTAL)
add_test(NAME all_tests COMMAND vajra_tests)

# ------------------------------------------------------------ CPack packaging
set(CPACK_PACKAGE_NAME "vajra-tactical-engine")
set(CPACK_PACKAGE_VENDOR "Kanak Prabhakar")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Tactical infiltration game and custom engine in C++20")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "TGZ;ZIP")
include(CPack)
