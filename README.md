# Foxglove WebSocket Server

A simple websocket server that exposes ROS topics using the [Foxglove websocket protocol](https://github.com/foxglove/ws-protocol), providing a smooth integration with [Foxglove Studio](https://studio.foxglove.dev/).

C++ was chosen as the implementation language for the following reasons:
  - True multi threading support unlike Python (Global interpreter lock)
  - The node can be deployed as a nodelet
    - Note that on ROS2, [rclcpp::GenericSubscriptions](https://docs.ros2.org/galactic/api/rclcpp/classrclcpp_1_1GenericSubscription.html) do not support intra-process handling

## Launch

Use the provided launch file to launch the server. This additionally spins up the `rosapi` node, which is required to discover available topics and their message definitions.

```bash
ros2 launch foxglove_websocket_server foxglove_server_launch.xml
```

You can now visualize ROS topics in Foxglove Studio by navigating with your browser to https://studio.foxglove.dev/?ds=foxglove-websocket&ds.url=ws://localhost:8765


## Topic discovery

By default, topic discovery is performed periodically by calling the `rosapi/topics_and_raw_types` service. This behavior can be disabled by setting the `topic_discovery_period` parameter to `0`. In this case, you will have to manually trigger topic discovery using the provided `discover_topics` service.