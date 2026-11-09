cmake_minimum_required(VERSION 3.21)
project(JackalExampleModule LANGUAGES CXX)
if(NOT WIN32)
    message(FATAL_ERROR "Jackal extensions require Windows")
endif()
find_package(nlohmann_json CONFIG REQUIRED)
add_library(example-module SHARED example.cpp example.def)
target_compile_features(example-module PRIVATE cxx_std_17)
target_include_directories(example-module PRIVATE ../JackalExtensionSDK)
target_link_libraries(example-module PRIVATE nlohmann_json::nlohmann_json)
set_property(TARGET example-module PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(EXTENSION_ARCH x64)
else()
    set(EXTENSION_ARCH x86)
endif()
set_target_properties(example-module PROPERTIES
    PREFIX ""
    OUTPUT_NAME "example-module-${EXTENSION_ARCH}"
    RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_SOURCE_DIR}>"
)
