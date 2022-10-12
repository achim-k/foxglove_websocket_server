cmake_minimum_required(VERSION 3.0.2)
project(foxglove_websocket_ros)

set(CMAKE_BUILD_TYPE "Release")

add_compile_options(-std=c++17)

find_package(catkin REQUIRED COMPONENTS roscpp topic_tools)

#find_package(nlohmann_json REQUIRED)
find_package(websocketpp REQUIRED)

catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES foxglove_subscription_tests
#  CATKIN_DEPENDS other_catkin_pkg
#  DEPENDS system_lib
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${WEBSOCKETPP_INCLUDE_DIRS}
)

add_executable(server src/node.cpp)

target_link_libraries(server
  ${catkin_LIBRARIES}
)
