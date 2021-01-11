find_package(Boost REQUIRED COMPONENTS unit_test_framework)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(RBO_TESTS "data-tests DataTests" "session-data-factory-tests SessionDataFactoryTests" "player-tests PlayerTests" "stats-manager-tests StatsManagerTests" "game-tests GameTests" "enemy-tests EnemyTests")

foreach(TEST ${RBO_TESTS})
    message(STATUS "Entering test : ${TEST}")

    string(REPLACE " " ";" INFOS ${TEST})
    list(GET INFOS 0 EXEC)
    list(GET INFOS 1 NAME)

    add_executable(${EXEC} ${NAME}.cpp)
    target_include_directories(${EXEC} PRIVATE ${Boost_INCLUDE_DIR})
    target_link_libraries(${EXEC} PRIVATE rbo Boost::unit_test_framework spdlog::spdlog)
endforeach()

target_link_libraries(session-data-factory-tests PRIVATE nlohmann_json::nlohmann_json)
