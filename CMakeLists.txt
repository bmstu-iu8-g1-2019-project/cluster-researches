cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(
  HUNTER_CACHE_SERVERS
  "https://github.com/bmstu-iu8-cpp-sem-3/hunter-cache"
  CACHE STRING "Default cache server"
)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++17 -pthread")

include("tools/gate/cmake/HunterGate.cmake")

huntergate(
  URL "https://github.com/ruslo/hunter/archive/v0.23.34.tar.gz"
  SHA1 "70287b1ffa810ee4e952052a9adff9b4856d0d54"
)

project(gossip-daemon)


#[[hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)]]

hunter_add_package(nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)

hunter_add_package(Boost COMPONENTS system)
find_package(Boost CONFIG REQUIRED system)

hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED cpp)


#[[set(generated_files protobuf.pb.cc protobuf.pb.h)

add_custom_command(
        OUTPUT ${generated_files}
        COMMAND protobuf::protoc --cpp_out ${CMAKE_CURRENT_BINARY_DIR}
                                 -I ${CMAKE_CURRENT_SOURCE_DIR}/protobuf ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/protobuf.proto
        COMMENT "Generating ${generated_files} from ${proto_file}"
        VERBATIM
)]]

add_library(protobuf ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/protobuf.pb.cc)
target_link_libraries(protobuf protobuf::libprotobuf)
target_include_directories(protobuf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/protobuf)


add_library(types STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/member.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/gossip.cpp
)
add_library(config STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/config.cpp
)
add_library(behavior
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/behavior.cpp
)
add_library(matrix STATIC
   ${CMAKE_CURRENT_SOURCE_DIR}/sources/matrix.cpp
)
add_executable(gossip-observer
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/observer.cpp
)
add_executable(gossip-daemon
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/daemon.cpp
)
add_executable(gossip-killer
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/killer.cpp
)
add_executable(gossip-graphs-observer
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/graphs-observer.cpp
)
add_executable(gossip-graphs-daemon
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/graphs-daemon.cpp
)


target_include_directories(types
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(config
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(behavior
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(matrix
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(gossip-observer
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(gossip-daemon
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(gossip-killer
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(gossip-graphs-observer
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(gossip-graphs-daemon
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(types
  PUBLIC Boost::system
  PUBLIC protobuf
  PUBLIC nlohmann_json::nlohmann_json
)
target_link_libraries(config
  PUBLIC types
)
target_link_libraries(behavior
  PUBLIC types
  PUBLIC nlohmann_json::nlohmann_json
)
target_link_libraries(matrix
  PUBLIC types
)
target_link_libraries(gossip-observer
  PUBLIC matrix
  PUBLIC types
  PUBLIC config
  PUBLIC behavior
  PUBLIC Boost::system
)
target_link_libraries(gossip-daemon
  PUBLIC types
  PUBLIC config
  PUBLIC behavior
  PUBLIC -pthread
)
target_link_libraries(gossip-killer
  PUBLIC types
  PUBLIC config
  PUBLIC behavior
)
target_link_libraries(gossip-graphs-observer
  PUBLIC matrix
  PUBLIC types
  PUBLIC config
  PUBLIC behavior
  PUBLIC Boost::system
)
target_link_libraries(gossip-graphs-daemon
  PUBLIC types
  PUBLIC config
  PUBLIC behavior
  PUBLIC -pthread
)



