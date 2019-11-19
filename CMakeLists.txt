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
        COMMAND protobuf::protoc --cpp_out ${CMAKE_CURRENT_BINARY_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/protobuf.proto
        COMMENT "Generating ${generated_files} from ${proto_file}"
        VERBATIM
)

add_library(protobuf ${CMAKE_CURRENT_BINARY_DIR}/protobuf.pb.cc)
target_link_libraries(protobuf protobuf::libprotobuf)
target_include_directories(protobuf PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/..)


add_library(member STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/member.cpp
)
target_include_directories(member
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/
)
target_link_libraries(member
  PUBLIC Boost::system
  PUBLIC protobuf
)

add_library(table STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/sources/table.cpp
)
target_include_directories(table
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(table
  PUBLIC member
)


enable_testing()
add_test(NAME unit_tests COMMAND tests)
