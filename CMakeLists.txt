cmake_minimum_required(VERSION 3.22)
project(Vajra VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -Wextra -Wpedantic -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# ---------------------------------------------------------------- dependencies
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

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

# GLAD is vendored (generator output) — see ThirdParty/glad
add_library(glad STATIC ThirdParty/glad/src/glad.c)
target_include_directories(glad PUBLIC ThirdParty/glad/include)

# ---------------------------------------------------------------- engine target
file(GLOB_RECURSE VAJRA_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
add_executable(Vajra ${VAJRA_SOURCES})

target_include_directories(Vajra PRIVATE src)
target_link_libraries(Vajra PRIVATE
    SDL2::SDL2 OpenGL::GL glad glm::glm EnTT::EnTT
    nlohmann_json::nlohmann_json Threads::Threads ${CMAKE_DL_LIBS})

target_compile_definitions(Vajra PRIVATE
    VAJRA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
    # GLM 1.x gates gtx/ headers (norm, quaternion, component_wise) behind this.
    # Required, or the build fails the moment glm/gtx/norm.hpp is included.
    GLM_ENABLE_EXPERIMENTAL)

# ------------------------------------------------------------ headless tests
# Physics, navigation and level loading need no GL context, so they can be
# verified in CI without a display. Run with: ctest --test-dir build
enable_testing()
add_executable(vajra_tests
    tests/smoke.cpp
    src/Physics/Collision.cpp
    src/AI/Navigation.cpp
    src/Scene/Level.cpp)
target_include_directories(vajra_tests PRIVATE src)
target_link_libraries(vajra_tests PRIVATE glm::glm nlohmann_json::nlohmann_json)
target_compile_definitions(vajra_tests PRIVATE
    VAJRA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
    GLM_ENABLE_EXPERIMENTAL)
add_test(NAME smoke COMMAND vajra_tests)

# copy assets next to the binary so the game runs from the build directory
add_custom_command(TARGET Vajra POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:Vajra>/assets)

# ------------------------------------------------------------------- install
# Needed by the AppImage and Flatpak packaging scripts under packaging/.
include(GNUInstallDirs)
set_target_properties(Vajra PROPERTIES OUTPUT_NAME vajra)
install(TARGETS Vajra RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/vajra/assets)
