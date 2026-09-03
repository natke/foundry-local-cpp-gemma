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

if(NOT EXISTS "${FOUNDRY_LOCAL_ROOT}/include/ort_genai.h")
    message(FATAL_ERROR
        "ORT GenAI headers were not found under ${FOUNDRY_LOCAL_ROOT}. "
        "Run scripts/bootstrap.sh or scripts/bootstrap.ps1 again.")
endif()

add_library(foundry_local SHARED IMPORTED GLOBAL)
add_library(onnxruntime_genai SHARED IMPORTED GLOBAL)
add_library(onnxruntime SHARED IMPORTED GLOBAL)

if(WIN32)
    set_target_properties(foundry_local PROPERTIES
        IMPORTED_IMPLIB "${FOUNDRY_LOCAL_ROOT}/lib/foundry_local.lib"
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/foundry_local.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
    set_target_properties(onnxruntime_genai PROPERTIES
        IMPORTED_IMPLIB "${FOUNDRY_LOCAL_ROOT}/lib/onnxruntime-genai.lib"
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/onnxruntime-genai.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_IMPLIB "${FOUNDRY_LOCAL_ROOT}/lib/onnxruntime.lib"
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/onnxruntime.dll")
elseif(APPLE)
    set_target_properties(foundry_local PROPERTIES
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/libfoundry_local.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
    set_target_properties(onnxruntime_genai PROPERTIES
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/libonnxruntime-genai.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/libonnxruntime.dylib")
else()
    set_target_properties(foundry_local PROPERTIES
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/libfoundry_local.so"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
    set_target_properties(onnxruntime_genai PROPERTIES
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/libonnxruntime-genai.so"
        INTERFACE_INCLUDE_DIRECTORIES "${FOUNDRY_LOCAL_ROOT}/include")
    set_target_properties(onnxruntime PROPERTIES
        IMPORTED_LOCATION "${FOUNDRY_LOCAL_ROOT}/lib/libonnxruntime.so")
endif()

add_executable(foundry_local_gemma main.cc)
target_link_libraries(foundry_local_gemma PRIVATE foundry_local)

add_executable(raw_ort_genai raw_ort_genai.cc)
target_link_libraries(raw_ort_genai PRIVATE onnxruntime_genai onnxruntime)

if(WIN32)
    foreach(target foundry_local_gemma raw_ort_genai)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${FOUNDRY_LOCAL_ROOT}/lib"
                    "$<TARGET_FILE_DIR:${target}>")
    endforeach()
else()
    set_target_properties(foundry_local_gemma raw_ort_genai PROPERTIES
        BUILD_RPATH "${FOUNDRY_LOCAL_ROOT}/lib")
endif()
