cmake_minimum_required(VERSION 3.17)

project(ServerTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(TESTS "data-tests DataTests" "session-data-factory-tests SessionDataFactoryTests" "player-tests PlayerTests" "stats-manager-tests StatsManagerTests" "game-tests GameTests")

foreach(TEST ${TESTS})
    message(STATUS "Entering test : ${TEST}")

    string(REPLACE " " ";" INFOS ${TEST})
    list(GET INFOS 0 EXEC)
    list(GET INFOS 1 NAME)

    add_executable(${EXEC} ${NAME}.cpp Common.cpp Common.hpp)
    target_include_directories(${EXEC} PUBLIC ${Boost_INCLUDE_DIR})
    target_link_libraries(${EXEC} PUBLIC Boost::unit_test_framework rbo spdlog::spdlog)
endforeach()

target_link_libraries(session-data-factory-tests PUBLIC nlohmann_json::nlohmann_json)
