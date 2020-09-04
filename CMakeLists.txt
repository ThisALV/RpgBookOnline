cmake_minimum_required(VERSION 3.17)

project(Rbo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(RBO_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(src)
add_subdirectory(server)
add_subdirectory(tests)

target_compile_definitions(server PRIVATE SOL_ALL_SAFETIES_ON=1 SOL_PRINT_ERRORS=0)

if(WIN32)
    target_compile_definitions(rbo PUBLIC _WIN32_WINNT=0x601)
endif()

install(DIRECTORY "${RBO_INCLUDE_DIR}" TYPE INCLUDE)
install(TARGETS rbo LIBRARY)
install(TARGETS server RUNTIME)

