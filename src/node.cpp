
#include <ros/ros.h>
#include <rosapi/TopicsAndRawTypes.h>
#include <std_srvs/Trigger.h>

#include <functional>
#include <thread>

#include "foxglove/websocket/server.hpp"
#include "foxglove_websocket_server/subscription.h"

using foxglove_websocket_server::Subscription;

constexpr char CHANNEL_ENCODING[] = "ros1";

class FosgloveServerRos {
 public:
  FosgloveServerRos(int port)
      : _nh(),
        _server(std::make_shared<foxglove::websocket::Server>(port, "foxglove_websocket_server")),
        _srv_get_topics(
            _nh.serviceClient<rosapi::TopicsAndRawTypes>("rosapi/topics_and_raw_types")) {
    _server->setSubscribeHandler([&](foxglove::websocket::ChannelId channel_id) {
      try {
        std::lock_guard<std::mutex> lock(_mutex);
        _subscriptions.at(channel_id).addClient();
      } catch (const std::out_of_range&) {
        ROS_ERROR_STREAM("Unknown channel: " << channel_id);
      }
    });

    _server->setUnsubscribeHandler([&](foxglove::websocket::ChannelId channel_id) {
      try {
        std::lock_guard<std::mutex> lock(_mutex);
        _subscriptions.at(channel_id).removeClient();
      } catch (const std::out_of_range&) {
        ROS_ERROR_STREAM("Unknown channel: " << channel_id);
      }
    });
  }

  void run() { _server->run(); }
  void stop() { _server->stop(); }
  ~FosgloveServerRos() { this->stop(); }

  void discoverTopics() {
    rosapi::TopicsAndRawTypes srv;
    if (_srv_get_topics.call(srv)) {
      std::lock_guard<std::mutex> lock(_mutex);
      for (std::size_t i = 0; i < srv.response.topics.size(); i++) {
        const std::string& topic = srv.response.topics[i];
        const std::string& type = srv.response.types[i];
        const std::string& definition_text = srv.response.typedefs_full_text[i];

        if (std::find_if(_subscriptions.begin(), _subscriptions.end(),
                         [&topic](const auto& t) -> bool {
                           return t.second.getTopic() == topic;
                         }) == _subscriptions.end()) {
          // Not found, add it.
          Subscription subscription(_nh, topic);

          const auto channel_id = _server->addChannel({
              topic,
              CHANNEL_ENCODING,
              type,
              definition_text,
          });
          subscription.setMessageCallback(std::bind(&foxglove::websocket::Server::sendMessage,
                                                    _server.get(), channel_id,
                                                    std::placeholders::_1, std::placeholders::_2));
          _subscriptions.insert({channel_id, std::move(subscription)});
        }
      }
    } else {
      ROS_WARN_STREAM("Failed to discover available ROS topics.");
    }
  }

 private:
  ros::NodeHandle _nh;
  std::shared_ptr<foxglove::websocket::Server> _server;
  ros::ServiceClient _srv_get_topics;
  std::mutex _mutex;
  std::map<foxglove::websocket::ChannelId, Subscription> _subscriptions;
};

int main(int argc, char** argv) {
  ros::init(argc, argv, "foxglove_websocket_server");
  ros::NodeHandle nh;
  ros::NodeHandle nhp("~");
  const int port = nhp.param<int>("port", 8765);

  FosgloveServerRos foxglove_server(port);

  // Optional: Periodically discover topics.
  ros::Timer topic_discovery_timer;
  const float topic_discovery_period = nhp.param<float>("topic_discovery_period", 5.0);
  if (topic_discovery_period > 0) {
    topic_discovery_timer =
        nh.createTimer(ros::Duration(topic_discovery_period),
                       std::bind(&FosgloveServerRos::discoverTopics, &foxglove_server));
    topic_discovery_timer.start();
  }

  // Provide a service to trigger topic discovery manually.
  ros::ServiceServer service =
      nh.advertiseService<std_srvs::Trigger::Request, std_srvs::Trigger::Response>(
          "discover_topics", [&foxglove_server](auto&, auto& response) -> bool {
            foxglove_server.discoverTopics();
            response.success = true;
            return true;
          });

  int num_threads = nhp.param<int>("num_threads", 0);
  if (num_threads <= 0) {
    num_threads = std::max(1u, std::thread::hardware_concurrency());
  }
  ROS_INFO("Launching spinner with %d threads.", num_threads);
  ros::AsyncSpinner spinner(num_threads);
  spinner.start();
  foxglove_server.run();

  return EXIT_SUCCESS;
}