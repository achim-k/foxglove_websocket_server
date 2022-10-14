# Foxglove WebSocket Server

A simple websocket server that exposes ROS topics using the [Foxglove websocket protocol](https://github.com/foxglove/ws-protocol), providing a smooth integration with [Foxglove Studio](https://studio.foxglove.dev/).

C++ was chosen as the implementation language for the following reasons:
  - True multi threading support unlike Python (Global interpreter lock)
  - The node can be deployed as a nodelet

## Launch

Use the provided launch file that also spins up the `rosapi` node which is required to get available topics and their message definitions.

```bash
roslaunch foxglove_websocket_server foxglove_server.launch
```

## Topic discovery

By default, topic discovery is performed periodically by calling the `rosapi/topics_and_raw_types` service. This behavior can be disabled by setting the `topic_discovery_period` parameter to `0`. In this case, you will have to manually trigger topic discovery using the provided `discover_topics` service.