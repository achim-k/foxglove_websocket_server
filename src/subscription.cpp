#include "foxglove_websocket_server/subscription.h"

#include <cassert>

namespace foxglove_websocket_server {

void Subscription::addClient() {
  std::lock_guard<std::recursive_mutex> guard(*mutex_);
  if (++num_subscribed_clients_ == 1) {
    this->subscribe();
  }
}

void Subscription::removeClient() {
  std::lock_guard<std::recursive_mutex> guard(*mutex_);
  if (--num_subscribed_clients_ == 0) {
    this->unsubscribe();
  }
  assert((void("Negative number of clients!"), num_subscribed_clients_ >= 0));
}

void Subscription::setMessageCallback(
    std::function<void(uint64_t, std::string_view)> msg_callback) {
  std::lock_guard<std::recursive_mutex> guard(*mutex_);
  msg_callback_ = msg_callback;
}

void Subscription::subscribe() {
  std::lock_guard<std::recursive_mutex> guard(*mutex_);
  if (subscriber_) {
    RCLCPP_WARN(node_->get_logger(), "Already subscribed to topic '%s'", topic_.c_str());
    return;
  }

  rclcpp::SubscriptionEventCallbacks event_callbacks;
  event_callbacks.incompatible_qos_callback = [&](const rclcpp::QOSRequestedIncompatibleQoSInfo&) {
    RCLCPP_ERROR(node_->get_logger(), "Incompatible subscriber QOS settings for topic '%s'",
                 topic_.c_str());
  };

  rclcpp::SubscriptionOptions subscription_options;
  subscription_options.event_callbacks = event_callbacks;
  subscription_options.callback_group = callback_group_;

  const size_t queue_length = 10;
  try {
    subscriber_ = node_->create_generic_subscription(
        topic_, type_, queue_length,
        [&](std::shared_ptr<rclcpp::SerializedMessage> msg) {
          msg_callback_(node_->now().nanoseconds(),
                        std::string_view(
                            reinterpret_cast<const char*>(msg->get_rcl_serialized_message().buffer),
                            msg->get_rcl_serialized_message().buffer_length));
        },
        subscription_options);
    RCLCPP_INFO(node_->get_logger(), "Subscribed to topic '%s'", topic_.c_str());
  } catch (const std::exception& ex) {
    RCLCPP_ERROR(node_->get_logger(), "Error when subscribing to topic '%s': %s", topic_.c_str(),
                 ex.what());
    subscriber_.reset();
  }
}

void Subscription::unsubscribe() {
  subscriber_.reset();
  RCLCPP_INFO(node_->get_logger(), "Unsubscribed from topic '%s'", topic_.c_str());
}

}  // namespace foxglove_websocket_server