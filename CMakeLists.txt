cmake_minimum_required(VERSION 3.20)
project(MedicalDeckbuilder CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release if no build type is specified (silences raylib's warning)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(FetchContent)
include(CTest)

# Fetch raylib 4.5.0
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/4.5.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# raylib is third-party C code; silence all warnings so only our warnings show
if(TARGET raylib)
    target_compile_options(raylib PRIVATE
        $<$<C_COMPILER_ID:GNU,Clang>:-w>
        $<$<C_COMPILER_ID:MSVC>:/W0>
    )
endif()

# Fetch nlohmann/json 3.11.2 (full source so the CMake target is available)
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# Main executable
add_executable(medical_deckbuilder
    src/main.cpp
    src/app/AppSettings.cpp
    src/audio/CardAudio.cpp

    src/content/EnemyFactory.cpp
    src/content/MapData.cpp

    src/gameplay/CardEffect.cpp
    src/gameplay/MapRunState.cpp
    src/gameplay/CombatResolver.cpp
    src/gameplay/StatusCollection.cpp

    src/core/Card.cpp
    src/core/CardDatabase.cpp
    src/core/Deck.cpp
    src/core/Enemy.cpp
    src/core/GameState.cpp
    src/core/Player.cpp

    src/ui/CardFaceCache.cpp
    src/ui/DissolveEffect.cpp
    src/ui/EnemySprite.cpp
    src/ui/HitEffect.cpp
    src/ui/GameScreen.cpp
    src/ui/InputHandler.cpp
    src/ui/PlayerSprite.cpp
    src/ui/ScreenTransition.cpp
    src/ui/UIState.cpp
    src/ui/screens/GameScreenMenus.cpp
    src/ui/screens/GameScreenOverlays.cpp
)

target_include_directories(medical_deckbuilder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(medical_deckbuilder
    raylib
    nlohmann_json::nlohmann_json
)

# Always refresh assets after every build, even if no source files changed.
# add_custom_target(ALL) runs unconditionally on every cmake --build invocation.
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        $<TARGET_FILE_DIR:medical_deckbuilder>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:medical_deckbuilder>/assets
    DEPENDS medical_deckbuilder
    COMMENT "Refreshing assets in build output directory"
)

if(BUILD_TESTING)
    add_executable(medical_deckbuilder_tests
        tests/core_tests.cpp
        src/content/EnemyFactory.cpp
        src/gameplay/CardEffect.cpp
        src/gameplay/CombatResolver.cpp
        src/gameplay/StatusCollection.cpp
        src/core/Card.cpp
        src/core/CardDatabase.cpp
        src/core/Deck.cpp
        src/core/Enemy.cpp
        src/core/GameState.cpp
        src/core/Player.cpp
    )

    target_include_directories(medical_deckbuilder_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    target_link_libraries(medical_deckbuilder_tests
        nlohmann_json::nlohmann_json
    )

    add_test(NAME medical_deckbuilder_tests COMMAND medical_deckbuilder_tests)
    set_tests_properties(medical_deckbuilder_tests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
