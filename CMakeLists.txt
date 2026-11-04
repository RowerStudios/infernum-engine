cmake_minimum_required(VERSION 3.24)
project(infernum LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(infernum_project "inferno" CACHE STRING "Project folder to build")

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
find_package(SDL3_image REQUIRED CONFIG)

find_library(JSONCPP_LIBRARY NAMES jsoncpp REQUIRED)
find_path(JSONCPP_INCLUDE_DIR NAMES json/json.h REQUIRED)

if(APPLE)
    set(inf_platform "macos")
elseif(WIN32)
    set(inf_platform "windows")
else()
    set(inf_platform "linux")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${inf_platform})

add_executable(Inferno
    src/cpp/main.cpp
    src/cpp/game.cpp
    src/cpp/sounds.cpp
    src/cpp/engine.cpp
    src/cpp/tween.cpp
)

target_include_directories(Inferno PRIVATE src/header)

target_link_libraries(Inferno PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image)

file(GLOB infernum_scene_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${infernum_project}/scenes/*/*.cpp")

foreach(scene_source IN LISTS infernum_scene_sources)
    get_filename_component(scene_name "${scene_source}" NAME_WE)
    set(scene_output_dir "${CMAKE_BINARY_DIR}/${inf_platform}/scenes/${scene_name}")

    add_library(${scene_name} SHARED
        ${scene_source}
        src/cpp/sounds.cpp
        src/cpp/engine.cpp
        src/cpp/tween.cpp
        src/cpp/ui.cpp
    )

    target_include_directories(${scene_name} PRIVATE src/header ${JSONCPP_INCLUDE_DIR})

    target_link_libraries(${scene_name} PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf SDL3_image::SDL3_image ${JSONCPP_LIBRARY})

    set_target_properties(${scene_name} PROPERTIES
        PREFIX ""
        LIBRARY_OUTPUT_DIRECTORY ${scene_output_dir}
        RUNTIME_OUTPUT_DIRECTORY ${scene_output_dir}
    )

    if(APPLE)
        target_link_options(${scene_name} PRIVATE -Wl,-rpath,@executable_path)
    elseif(UNIX)
        target_link_options(${scene_name} PRIVATE "-Wl,-rpath,\$ORIGIN:\$ORIGIN/..:\$ORIGIN/../..")
    endif()

    if(UNIX)
        add_custom_command(TARGET ${scene_name} POST_BUILD
            COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/src/dlls.sh
                ${CMAKE_BINARY_DIR}/${inf_platform}
                $<TARGET_FILE:${scene_name}>
        )
    endif()
endforeach()

file(GLOB infernum_scene_properties "${CMAKE_CURRENT_SOURCE_DIR}/${infernum_project}/scenes/*/properties.json")
set(scene_property_commands)
foreach(scene_property IN LISTS infernum_scene_properties)
    get_filename_component(scene_dir "${scene_property}" DIRECTORY)
    get_filename_component(scene_name "${scene_dir}" NAME)
    set(dest_dir "${CMAKE_BINARY_DIR}/${inf_platform}/scenes/${scene_name}")
    list(APPEND scene_property_commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${dest_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${scene_property}"
            "${dest_dir}/properties.json")
endforeach()
if(scene_property_commands)
    add_custom_target(scene_properties_copy ALL ${scene_property_commands})
endif()

if(APPLE)
    target_link_options(Inferno PRIVATE -Wl,-rpath,@executable_path -Wl,-export_dynamic)
elseif(UNIX)
    target_link_options(Inferno PRIVATE "-Wl,-rpath,\$ORIGIN" "-Wl,--export-dynamic")
endif()

add_custom_target(assets ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/${inf_platform}/fonts
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/${infernum_project}/fonts/debug.ttf
        ${CMAKE_BINARY_DIR}/${inf_platform}/fonts/debug.ttf
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/${infernum_project}/sounds
        ${CMAKE_BINARY_DIR}/${inf_platform}/sounds
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/${infernum_project}/assets
        ${CMAKE_BINARY_DIR}/${inf_platform}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/${infernum_project}/game_properties.json
        ${CMAKE_BINARY_DIR}/${inf_platform}/game_properties.json
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/${infernum_project}/VERSION
        ${CMAKE_BINARY_DIR}/${inf_platform}/VERSION
)

if(UNIX)
    add_custom_command(TARGET Inferno POST_BUILD
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/src/dlls.sh
            ${CMAKE_BINARY_DIR}/${inf_platform}
            $<TARGET_FILE:Inferno>
    )
endif()
