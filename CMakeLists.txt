cmake_minimum_required(VERSION 3.16)

project(LanternRunSFML VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(LANTERN_FETCH_SFML "Download SFML automatically when it is not installed" ON)

find_package(SFML 2.6 COMPONENTS graphics window system audio QUIET)

if(NOT SFML_FOUND)
    if(NOT LANTERN_FETCH_SFML)
        message(FATAL_ERROR "SFML was not found. Install SFML 2.6 or enable LANTERN_FETCH_SFML.")
    endif()

    include(FetchContent)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SFML
        URL https://github.com/SFML/SFML/archive/refs/tags/2.6.1.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SFML)
endif()

file(GLOB_RECURSE LANTERN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_executable(LanternRunSFML ${LANTERN_SOURCES})

target_link_libraries(LanternRunSFML PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)

if(WIN32)
    foreach(SFML_DLL_TARGET sfml-system sfml-window sfml-graphics sfml-audio)
        if(TARGET ${SFML_DLL_TARGET})
            add_custom_command(TARGET LanternRunSFML POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${SFML_DLL_TARGET}>
                    $<TARGET_FILE_DIR:LanternRunSFML>
            )
        endif()
    endforeach()

    if(DEFINED sfml_SOURCE_DIR AND EXISTS "${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll")
        add_custom_command(TARGET LanternRunSFML POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll"
                $<TARGET_FILE_DIR:LanternRunSFML>
        )
    endif()
endif()

add_custom_command(TARGET LanternRunSFML POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:LanternRunSFML>/assets"
)
