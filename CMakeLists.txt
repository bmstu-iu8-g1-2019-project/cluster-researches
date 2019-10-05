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

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread")

add_library(table STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/membertable.cpp)
target_include_directories(table
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers/)
target_link_libraries(table PUBLIC nlohmann_json::nlohmann_json)


add_library(behavior STATIC
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/behavior.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/gossiping.cpp)
target_include_directories(behavior
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers/)
target_link_libraries(behavior PUBLIC table ${CMAKE_THREAD_LIBS_INIT})


add_executable(tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/membertable_unittests.cpp)
target_include_directories(tests
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers/)
target_link_libraries(tests PUBLIC GTest::main table)


add_executable(${CMAKE_PROJECT_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/sources/daemon.cpp)
target_include_directories(${CMAKE_PROJECT_NAME}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/headers/)
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC behavior ${CMAKE_THREAD_LIBS_INIT})


enable_testing()
add_test(NAME unit_tests COMMAND tests)
