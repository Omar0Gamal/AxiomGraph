cmake_minimum_required(VERSION 3.24)
project(AxiomGraph LANGUAGES CXX C CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_ARCHITECTURES "native")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(CUDAToolkit REQUIRED)
include(FetchContent)

FetchContent_Declare(nanobind GIT_REPOSITORY https://github.com/wjakob/nanobind.git GIT_TAG v1.9.2)
FetchContent_MakeAvailable(nanobind)

FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)
find_package(cuvs QUIET)
find_package(raft QUIET)

if(cuvs_FOUND AND raft_FOUND)
    set(USE_GPU_INDEX ON)
    add_compile_definitions(USE_GPU_INDEX)
    message(STATUS "cuVS and RAFT found. GPUIndex enabled.")
else()
    set(USE_GPU_INDEX OFF)
    message(STATUS "cuVS and RAFT not found. GPUIndex disabled.")
endif()

set(CORE_SOURCES
    src/core/memory_arena.cpp
    src/core/hot_graph.cpp
    src/core/vector_buffer.cpp
    src/core/csr_graph.cpp
    src/core/cpu_index.cpp
)

if(USE_GPU_INDEX)
    list(APPEND CORE_SOURCES src/core/gpu_index.cpp)
endif()

add_library(axiomgraph_core STATIC ${CORE_SOURCES})
target_include_directories(axiomgraph_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
target_link_libraries(axiomgraph_core PUBLIC CUDA::cudart)

if(USE_GPU_INDEX)
    target_link_libraries(axiomgraph_core PUBLIC cuvs::cuvs raft::raft)
endif()

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(axiomgraph_core PUBLIC OpenMP::OpenMP_CXX)
endif()

nanobind_add_module(ext src/bindings/bindings.cpp)
target_link_libraries(ext PRIVATE axiomgraph_core)
if(USE_GPU_INDEX)
    target_compile_definitions(ext PRIVATE USE_GPU_INDEX)
endif()

enable_testing()
add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE axiomgraph_core GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(test_core)

install(TARGETS ext DESTINATION axiomgraph)

find_package(Threads REQUIRED)
add_executable(server_http src/server/server_http.cpp third_party/sqlite/sqlite3.c)
target_include_directories(server_http PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite)
target_link_libraries(server_http PRIVATE axiomgraph_core Threads::Threads dl)