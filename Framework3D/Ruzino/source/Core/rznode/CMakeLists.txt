cmake_minimum_required(VERSION 3.29.6)
project(rznode VERSION 0.1.0 LANGUAGES CXX)

set(RZNODES_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "RZNodes directory" FORCE)
message(STATUS "RZNODES_DIR: ${RZNODES_DIR}")

set(BUILD_SHARED_LIBS ON)

if (NOT OUT_BINARY_DIR)

set(OUT_BINARY_DIR ${CMAKE_SOURCE_DIR}/Binaries/${CMAKE_BUILD_TYPE})
set(OUTPUT_DIR
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUT_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${OUT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${OUT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUT_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${OUT_BINARY_DIR}"
)

endif()

if(NOT Python_FOUND)
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    if(NOT Python_EXECUTABLE)
        set(Python_EXECUTABLE ${Python_INTERPRETER})
    endif()
endif()

if(NOT Python3_FOUND)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    if(NOT Python3_EXECUTABLE)
        set(Python3_EXECUTABLE ${Python3_INTERPRETER})
    endif()
endif()

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT)
if(nanobind_ROOT)
    find_package(nanobind CONFIG REQUIRED)
endif()

include(cmake/AddLibrary.cmake)
include(cmake/AddNodes.cmake)
enable_testing()

if (NOT TARGET gtest)
add_subdirectory(ext/googletest)

set_target_properties(gtest PROPERTIES FOLDER "ThirdParty")
set_target_properties(gtest_main PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock_main PROPERTIES FOLDER "ThirdParty")

set_target_properties(gtest PROPERTIES ${OUTPUT_DIR})
set_target_properties(gtest_main PROPERTIES ${OUTPUT_DIR})

endif()

add_subdirectory(ext/entt)
add_subdirectory(ext/spdlog)

set_target_properties(spdlog PROPERTIES ${OUTPUT_DIR})

add_subdirectory(core)
add_subdirectory(system)

option(NODE_IMGUI_ENABLED "Enable ImGui UI Nodes" OFF)
if(NODE_IMGUI_ENABLED)
    add_subdirectory(ui_imgui)
endif()

