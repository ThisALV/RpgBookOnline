set(SERVER_HEADERS_DIR ${RBO_INCLUDE_DIR}/Rbo/Server)

set(SERVER_SRC Main.cpp Lobby.cpp Executor.cpp LobbyDataFactory.cpp LocalGameBuilder.cpp InstructionsProvider.cpp GameplayAPI.cpp TablesLock.cpp ContainersAPI.cpp GameAPI.cpp)
set(SERVER_HEADERS ${SERVER_HEADERS_DIR}/Common.hpp ${SERVER_HEADERS_DIR}/Lobby.hpp ${SERVER_HEADERS_DIR}/Executor.hpp ${SERVER_HEADERS_DIR}/LobbyDataFactory.hpp ${SERVER_HEADERS_DIR}/LocalGameBuilder.hpp ${SERVER_HEADERS_DIR}/InstructionsProvider.hpp ${SERVER_HEADERS_DIR}/TablesLock.hpp)

add_executable(server ${SERVER_SRC} ${SERVER_HEADERS})

find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(server PRIVATE rbo nlohmann_json::nlohmann_json ${LUA_LIBRARIES} sol2::sol2)
