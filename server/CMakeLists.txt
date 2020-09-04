set(SERVER_HEADERS_DIR ${RBO_INCLUDE_DIR}/Rbo/Server)

set(SERVER_SRC main.cpp Lobby.cpp LobbyExecutor.cpp LobbyDataFactory.cpp LocalGameBuilder.cpp InstructionsProvider.cpp LuaBuiltinsInitialization.cpp TablesLock.cpp)
set(SERVER_HEADERS ${SERVER_HEADERS_DIR}/Lobby.hpp ${SERVER_HEADERS_DIR}/LobbyExecutor.hpp ${SERVER_HEADERS_DIR}/LobbyDataFactory.hpp ${SERVER_HEADERS_DIR}/LocalGameBuilder.hpp ${SERVER_HEADERS_DIR}/InstructionsProvider.hpp ${SERVER_HEADERS_DIR}/GameJsonCast.inl ${SERVER_HEADERS_DIR}/TablesLock.hpp)

add_executable(server ${SERVER_SRC} ${SERVER_HEADERS})

find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_include_directories(server PRIVATE "${RBO_INCLUDE_DIR}")
target_link_libraries(server PRIVATE rbo nlohmann_json::nlohmann_json ${LUA_LIBRARIES} sol2::sol2)
