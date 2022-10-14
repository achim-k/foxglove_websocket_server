#include "foxglove_websocket_server/subscription.h"

#include <ros/message_event.h>

#include <cassert>
#include <ros_type_introspection/utils/shape_shifter.hpp>

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
    ROS_WARN_STREAM("Already subscribed to topic " << topic_);
    return;
  }

  const uint32_t queue_length = 10;
  subscriber_ = std::make_unique<ros::Subscriber>(nh_.subscribe<RosIntrospection::ShapeShifter>(
      topic_, queue_length,
      [&](const ros::MessageEvent<RosIntrospection::ShapeShifter const>& msgEvent) -> void {
        const auto& msg = msgEvent.getConstMessage();
        msg_callback_(
            msgEvent.getReceiptTime().toNSec(),
            std::string_view(reinterpret_cast<const char*>(msg->raw_data()), msg->size()));
      }));
  ROS_INFO_STREAM("Subscribed to topic " << topic_);
}

void Subscription::unsubscribe() {
  subscriber_.reset();
  ROS_INFO_STREAM("Unsubscribed from topic " << topic_);
}

}  // namespace foxglove_websocket_server