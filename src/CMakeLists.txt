set(LIB_HEADERS_DIR ${RBO_INCLUDE_DIR}/Rbo)

set(RBO_SRC AsioCommon.cpp Common.cpp Data.cpp Enemy.cpp Game.cpp Gameplay.cpp Player.cpp ReplyHandler.cpp Session.cpp SessionDataFactory.cpp StatsManager.cpp)
set(RBO_HEADERS ${LIB_HEADERS_DIR}/AsioCommon.hpp ${LIB_HEADERS_DIR}/Common.hpp ${LIB_HEADERS_DIR}/Data.hpp ${LIB_HEADERS_DIR}/Enemy.hpp ${LIB_HEADERS_DIR}/Game.hpp ${LIB_HEADERS_DIR}/Gameplay.hpp ${LIB_HEADERS_DIR}/Player.hpp ${LIB_HEADERS_DIR}/ReplyHandler.hpp ${LIB_HEADERS_DIR}/Session.hpp ${LIB_HEADERS_DIR}/SessionDataFactory.hpp ${LIB_HEADERS_DIR}/StatsManager.hpp ${LIB_HEADERS_DIR}/GameBuilder.hpp)

add_library(rbo STATIC ${RBO_SRC} ${RBO_HEADERS})

find_package(Boost REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_include_directories(rbo PUBLIC "${RBO_INCLUDE_DIR}" "${Boost_INCLUDE_DIRS}")
target_link_libraries(rbo PUBLIC spdlog::spdlog PRIVATE nlohmann_json::nlohmann_json)

