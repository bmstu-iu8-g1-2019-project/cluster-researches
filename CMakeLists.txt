cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(
        HUNTER_CACHE_SERVERS
        "https://github.com/bmstu-iu8-cpp-sem-3/hunter-cache"
        CACHE STRING "Default cache server"
)

include("tools/gate/cmake/HunterGate.cmake")

huntergate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.34.tar.gz"
        SHA1 "70287b1ffa810ee4e952052a9adff9b4856d0d54"
)

project(gossip-daemon)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

hunter_add_package(nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)

hunter_add_package(Boost COMPONENTS system)
find_package(Boost CONFIG REQUIRED system)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread")


add_library(types STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/types.cpp
)
target_include_directories(types
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_link_libraries(types
        PUBLIC nlohmann_json::nlohmann_json
        PUBLIC Boost::system
)


add_library(buffer STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/buffer.cpp
)
target_include_directories(buffer
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
)


add_library(behavior STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/behavior.cpp
)
target_include_directories(behavior
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_link_libraries(behavior
        PUBLIC types buffer ${CMAKE_THREAD_LIBS_INIT}
)


add_executable(type_translation_unittests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/types_unittests.cpp
)
target_include_directories(type_translation_unittests
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_link_libraries(type_translation_unittests
        PUBLIC GTest::main types buffer
)


add_executable(${CMAKE_PROJECT_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/daemon.cpp
)
target_include_directories(${CMAKE_PROJECT_NAME}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_link_libraries(${CMAKE_PROJECT_NAME}
        PUBLIC behavior buffer ${CMAKE_THREAD_LIBS_INIT}
)


add_executable(gossip_receiving_test
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/gossip_receiving_test.cpp
)
target_include_directories(gossip_receiving_test
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers
)
target_link_libraries(gossip_receiving_test
        PUBLIC behavior
)

enable_testing()
add_test(NAME unit_tests COMMAND tests)
