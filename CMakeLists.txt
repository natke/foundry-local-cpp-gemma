cmake_minimum_required(VERSION 3.20)

project(foundry_local_gemma LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FOUNDRY_LOCAL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.foundry-local"
    CACHE PATH "Path to the extracted Foundry Local v2.0.1 runtime")

if(NOT EXISTS "${FOUNDRY_LOCAL_ROOT}/include/foundry_local/foundry_local_cpp.h")
    message(FATAL_ERROR
        "Foundry Local headers were not found under ${FOUNDRY_LOCAL_ROOT}. "
        "Run scripts/bootstrap.sh or scripts/bootstrap.ps1 first.")
endif()

add_library(foundry_local SHARED IMPORTED GLOBAL)

if(WIN32)
    set_target_properties(foundry_local PROPERTIES
        IMPORTED_IMPLIB "${FOUNDRY_LOCAL_ROOT}/lib/foundry_local.lib"
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/foundry_local.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
elseif(APPLE)
    set_target_properties(foundry_local PROPERTIES
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/libfoundry_local.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
else()
    set_target_properties(foundry_local PROPERTIES
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/libfoundry_local.so"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
endif()

add_executable(foundry_local_gemma main.cc)
target_link_libraries(foundry_local_gemma PRIVATE foundry_local)

if(WIN32)
    add_custom_command(TARGET foundry_local_gemma POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${FOUNDRY_LOCAL_ROOT}/lib"
                "$<TARGET_FILE_DIR:foundry_local_gemma>")
else()
    set_target_properties(foundry_local_gemma PROPERTIES
        BUILD_RPATH "${FOUNDRY_LOCAL_ROOT}/lib")
endif()
