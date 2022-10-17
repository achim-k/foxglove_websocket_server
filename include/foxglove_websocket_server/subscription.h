#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <string_view>

namespace foxglove_websocket_server {

class Subscription {
 public:
  explicit Subscription(std::shared_ptr<rclcpp::Node> node, const std::string& topic,
                        const std::string& type)
      : node_(node),
        topic_(topic),
        type_(type),
        num_subscribed_clients_(0),
        subscriber_(nullptr),
        callback_group_(node_->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive)),
        mutex_(std::make_unique<std::recursive_mutex>()),
        msg_callback_() {}

  const std::string& getTopic() const { return topic_; }
  void addClient();
  void removeClient();
  void setMessageCallback(std::function<void(uint64_t, std::string_view)> msg_callback);

 private:
  void subscribe();
  void unsubscribe();

  std::shared_ptr<rclcpp::Node> node_;
  std::string topic_;
  std::string type_;
  int num_subscribed_clients_;
  std::shared_ptr<rclcpp::GenericSubscription> subscriber_;
  rclcpp::CallbackGroup::SharedPtr callback_group_;
  mutable std::unique_ptr<std::recursive_mutex> mutex_;
  std::function<void(uint64_t, std::string_view)> msg_callback_;
};

}  // namespace foxglove_websocket_server