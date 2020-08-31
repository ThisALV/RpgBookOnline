cmake_minimum_required(VERSION 3.17)

project(Server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VERBOSE_MAKEFILE OFF)

find_package(spdlog CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)

set(EXEC server)
set(LIB rbo)
set(INCLUDE include/Rbo)

set(SOURCES Common.cpp AsioCommon.cpp StatsManager.cpp SessionDataFactory.cpp Data.cpp Session.cpp ReplyHandler.cpp Game.cpp Enemy.cpp Player.cpp Gameplay.cpp)
set(HEADERS Common.hpp AsioCommon.hpp StatsManager.hpp SessionDataFactory.hpp Data.hpp Session.hpp ReplyHandler.hpp Game.hpp Enemy.hpp GameBuilder.hpp Player.hpp Gameplay.hpp)

set(SERV_SOURCES Lobby.cpp LobbyDataFactory.cpp LocalGameBuilder.cpp InstructionsProvider.cpp LuaBuiltinsInitialization.cpp TablesLock.cpp main.cpp LobbyExecutor.cpp GameJsonCast.inl)
set(SERV_HEADERS Lobby.hpp LobbyDataFactory.hpp LocalGameBuilder.hpp InstructionsProvider.hpp TablesLock.hpp LobbyExecutor.hpp)

add_executable(${EXEC} ${SERV_SOURCES} ${SERV_HEADERS})
add_library(${LIB} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${LIB} PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(${LIB} PUBLIC nlohmann_json::nlohmann_json)

target_include_directories(${EXEC} PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(${EXEC} PUBLIC ${LIB} spdlog::spdlog ${LUA_LIBRARIES} sol2::sol2 nlohmann_json::nlohmann_json)

target_compile_definitions(${EXEC} PUBLIC SOL_ALL_SAFETIES_ON SOL_PRINT_ERRORS=0)

install(FILES ${HEADERS} DESTINATION ${INCLUDE})
install(FILES ${SERV_HEADERS} DESTINATION ${INCLUDE})
install(TARGETS ${LIB} LIBRARY)
