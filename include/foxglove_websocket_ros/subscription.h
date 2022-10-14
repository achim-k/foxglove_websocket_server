#pragma once

#include <ros/node_handle.h>
#include <ros/subscriber.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace foxglove_websocket_ros {

class Subscription {
 public:
  explicit Subscription(const ros::NodeHandle& nh, const std::string& topic)
      : nh_(nh),
        topic_(topic),
        num_subscribed_clients_(0),
        subscriber_(nullptr),
        mutex_(std::make_unique<std::recursive_mutex>()),
        msg_callback_() {}

  const std::string& getTopic() const { return topic_; }
  void addClient();
  void removeClient();
  void setMessageCallback(std::function<void(uint64_t, std::string_view)> msg_callback);

 private:
  void subscribe();
  void unsubscribe();

  ros::NodeHandle nh_;
  std::string topic_;
  int num_subscribed_clients_;
  std::unique_ptr<ros::Subscriber> subscriber_;
  mutable std::unique_ptr<std::recursive_mutex> mutex_;
  std::function<void(uint64_t, std::string_view)> msg_callback_;
};

}  // namespace foxglove_websocket_ros