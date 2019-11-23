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


hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

hunter_add_package(nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)

hunter_add_package(Boost COMPONENTS system)
find_package(Boost CONFIG REQUIRED system)

hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)


set(generated_files protobuf.pb.cc protobuf.pb.h)

add_custom_command(
        OUTPUT ${generated_files}
        COMMAND protobuf::protoc --cpp_out ${CMAKE_CURRENT_BINARY_DIR}
                                 -I ${CMAKE_CURRENT_SOURCE_DIR}/protobuf ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/protobuf.proto
        COMMENT "Generating ${generated_files} from ${proto_file}"
        VERBATIM
)

add_library(protobuf ${CMAKE_CURRENT_BINARY_DIR}/protobuf.pb.cc)
target_link_libraries(protobuf protobuf::libprotobuf)
target_include_directories(protobuf PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/..)


add_library(types STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/member.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/gossip.cpp
)
add_library(behavior
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/behavior.cpp
)
add_executable(daemon
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/daemon.cpp
)


target_include_directories(types
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(behavior
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(daemon
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)


target_link_libraries(types
  PUBLIC Boost::system
  PUBLIC protobuf
)
target_link_libraries(behavior
  PUBLIC types
)
target_link_libraries(daemon
  PUBLIC behavior
        -pthread
)


enable_testing()
add_test(NAME unit_tests COMMAND tests)
