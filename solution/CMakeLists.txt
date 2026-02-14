cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Добавляем библиотеку, указывая, что она статическая.
add_library(MyLib STATIC 
	src/domain_model/geom.h
	src/domain_model/collision_detector.h
	src/domain_model/collision_detector.cpp
	src/domain_model/model_env.h
	src/domain_model/model_env.cpp
	src/domain_model/model_game.h
	src/domain_model/model_game.cpp
	src/domain_model/tagged.h
	src/domain_model/loot_generator.cpp
	src/domain_model/loot_generator.h
	src/application_model/game.h
	src/application_model/game.cpp
	src/application_model/model_app.h
	src/application_model/model_app.cpp
	src/application_model/player_tokens.h
	src/serialization/model_serialization.h
	src/serialization/model_serialization.cpp
	src/application_model/game_server.h
	src/application_model/game_server.cpp
)

# Добавляем сторонние библиотеки. Указываем видимость PUBLIC, т. к. 
# они должны быть видны и в библиотеке MyLib, и в зависимостях.
target_include_directories(MyLib PUBLIC ${ZLIB_INCLUDES})
target_link_libraries(MyLib PUBLIC ${BOOST_LIB} ${ZLIB_LIB})

# Добавляем зависимость целей от статической библиотеки
add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/http_handler/request_handler.cpp
	src/http_handler/request_handler.h
	src/http_handler/api_handler.h
	src/http_handler/api_handler.cpp
	src/command_line.h
	src/ticker.h
)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost MyLib)

add_executable(game_server_tests
    tests/model_serialization.cpp
)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost Threads::Threads MyLib) 