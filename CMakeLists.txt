cmake_minimum_required(VERSION 3.5)
project(foxglove_websocket_server)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosapi_msgs REQUIRED)
find_package(std_srvs REQUIRED)

#find_package(nlohmann_json REQUIRED)
find_package(websocketpp REQUIRED)
find_package(Boost REQUIRED)

include_directories(
  include
  ${Boost_INCLUDE_DIRS}
  ${WEBSOCKETPP_INCLUDE_DIRS}
)

add_library(foxlove_server_ros src/subscription.cpp)
ament_target_dependencies(foxlove_server_ros
  rclcpp
)

add_executable(server src/node.cpp)
ament_target_dependencies(server
  rclcpp
  rosapi_msgs
  std_srvs
)
target_link_libraries(server
  foxlove_server_ros
)

install(TARGETS foxlove_server_ros
  DESTINATION lib/${PROJECT_NAME})

install(TARGETS server
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY include/
  DESTINATION include)
ament_export_include_directories(include)

install(FILES
  launch/foxglove_server_launch.xml
  DESTINATION share/${PROJECT_NAME}/launch
)

ament_package()