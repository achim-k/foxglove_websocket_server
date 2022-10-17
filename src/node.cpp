
#include <functional>
#include <rclcpp/rclcpp.hpp>
#include <rosapi_msgs/srv/topics_and_raw_types.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <thread>

#include "foxglove/websocket/server.hpp"
#include "foxglove_websocket_server/subscription.h"

using foxglove_websocket_server::Subscription;

constexpr char CHANNEL_ENCODING[] = "cdr";

class FosgloveServerRos {
 public:
  FosgloveServerRos(std::shared_ptr<rclcpp::Node> node, int port)
      : _node(node),
        _server(std::make_shared<foxglove::websocket::Server>(port, "foxglove_websocket_server")) {
    _server->setSubscribeHandler([&](foxglove::websocket::ChannelId channel_id) {
      try {
        _subscriptions.at(channel_id).addClient();
      } catch (const std::out_of_range&) {
        RCLCPP_ERROR_STREAM(_node->get_logger(), "Unknown channel: " << channel_id);
      }
    });

    _server->setUnsubscribeHandler([&](foxglove::websocket::ChannelId channel_id) {
      try {
        _subscriptions.at(channel_id).removeClient();
      } catch (const std::out_of_range&) {
        RCLCPP_ERROR_STREAM(_node->get_logger(), "Unknown channel: " << channel_id);
      }
    });
  }

  void run() { _server->run(); }
  void stop() { _server->stop(); }
  ~FosgloveServerRos() { this->stop(); }

  void discoverTopics(
      std::shared_future<rosapi_msgs::srv::TopicsAndRawTypes::Response::SharedPtr> result) {
    if (!result.valid()) {
      RCLCPP_ERROR_STREAM(_node->get_logger(), "Failed to discover topics.");
      return;
    }

    const auto response = result.get();
    for (std::size_t i = 0; i < response->topics.size(); i++) {
      const std::string& topic = response->topics[i];
      const std::string& type = response->types[i];
      const std::string& definition_text = response->typedefs_full_text[i];

      if (std::find_if(_subscriptions.begin(), _subscriptions.end(),
                       [&topic](const auto& t) -> bool { return t.second.getTopic() == topic; }) ==
          _subscriptions.end()) {
        // Not found, add it.
        Subscription subscription(_node, topic, type);

        const auto channel_id = _server->addChannel({
            topic,
            CHANNEL_ENCODING,
            type,
            definition_text,
        });
        subscription.setMessageCallback(std::bind(&foxglove::websocket::Server::sendMessage,
                                                  _server.get(), channel_id, std::placeholders::_1,
                                                  std::placeholders::_2));
        _subscriptions.insert({channel_id, std::move(subscription)});
      }
    }
  }

 private:
  std::shared_ptr<rclcpp::Node> _node;
  std::shared_ptr<foxglove::websocket::Server> _server;
  std::map<foxglove::websocket::ChannelId, Subscription> _subscriptions;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("foxglove_websocket_server");
  node->declare_parameter<int>("port", 8765);
  node->declare_parameter<int>("num_threads", 0);
  node->declare_parameter<double>("topic_discovery_period", 5.0);

  const int port = node->get_parameter("port").as_int();
  FosgloveServerRos foxglove_server(node, port);

  auto srv_get_topics =
      node->create_client<rosapi_msgs::srv::TopicsAndRawTypes>("rosapi/topics_and_raw_types");
  auto topicsSrvCb =
      std::bind(&FosgloveServerRos::discoverTopics, &foxglove_server, std::placeholders::_1);

  // Provide a service to trigger topic discovery manually.
  const auto service = node->create_service<std_srvs::srv::Trigger>(
      "discover_topics",
      [&](const std::shared_ptr<std_srvs::srv::Trigger::Request>,
          std::shared_ptr<std_srvs::srv::Trigger::Response> response) -> void {
        auto request = std::make_shared<rosapi_msgs::srv::TopicsAndRawTypes::Request>();
        srv_get_topics->async_send_request(request, topicsSrvCb);
        response->success = true;
      });

  // Optional: Periodically discover topics.
  rclcpp::TimerBase::SharedPtr topic_discovery_timer;
  const double topic_discovery_period = node->get_parameter("topic_discovery_period").as_double();
  if (topic_discovery_period > 0) {
    topic_discovery_timer =
        node->create_wall_timer(std::chrono::duration<double>(topic_discovery_period), [&]() {
          auto request = std::make_shared<rosapi_msgs::srv::TopicsAndRawTypes::Request>();
          srv_get_topics->async_send_request(request, topicsSrvCb);
        });
  }

  const size_t num_threads = node->get_parameter("num_threads").as_int();
  using rclcpp::executors::MultiThreadedExecutor;
  MultiThreadedExecutor executor(rclcpp::ExecutorOptions(), num_threads);
  executor.add_node(node);
  std::thread executor_thread(std::bind(&MultiThreadedExecutor::spin, &executor));

  foxglove_server.run();

  return EXIT_SUCCESS;
}