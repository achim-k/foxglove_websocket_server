cmake_minimum_required(VERSION 3.0.2)
project(foxglove_websocket_server)

add_compile_options(-std=c++17)

find_package(catkin REQUIRED COMPONENTS
  ros_type_introspection
  rosapi
  roscpp
  std_srvs
)

#find_package(nlohmann_json REQUIRED)
find_package(websocketpp REQUIRED)

catkin_package(
  INCLUDE_DIRS include
  LIBRARIES foxlove_server_ros
  CATKIN_DEPENDS
    ros_type_introspection
    rosapi
    roscpp
    std_srvs
  # DEPENDS websocketpp
)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${WEBSOCKETPP_INCLUDE_DIRS}
)

add_library(foxlove_server_ros src/subscription.cpp)
target_link_libraries(foxlove_server_ros
  ${catkin_LIBRARIES}
)

add_executable(server src/node.cpp)
target_link_libraries(server
  foxlove_server_ros
  ${catkin_LIBRARIES}
)

install(TARGETS foxlove_server_ros
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
)

install(TARGETS server
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
)

install(DIRECTORY launch/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
)