#pragma once

#include <mc_network_interface/NetworkInterface.h>

#include <mc_rtc/logging.h>

#include <iostream>
#include <string>

namespace mc_network
{

struct MessageConfigShm
{
  bool read = true;
  uint8_t name_size;
  char name[256];
  uint16_t config_size;
  char config[4096];
};

struct MessageStateShm
{
  uint8_t size;
  double state[256];
};

struct MessageCommandShm
{
  uint8_t size;
  double command[256];
};

class NetworkInterfaceShm : public NetworkInterface
{
public:
  NetworkInterfaceShm();
  NetworkInterfaceShm(const mc_rtc::Configuration & network_config);
  ~NetworkInterfaceShm();

  void sendMessage(const MessageConfigShm & msg);
  void sendMessage(const MessageStateShm & msg);
  void sendMessage(const MessageCommandShm & msg);

  void receiveMessage(const MessageConfigShm & msg);
  void receiveMessage(const MessageStateShm & msg);
  void receiveMessage(const MessageCommandShm & msg);

private:
  template<typename msg>
  void createShmBlock(uint8_t port);

  std::string keyPath_;
  std::vector<int> shmid_;
  std::vector<void *> shmptr_;
};

// struct ShmMessageState
// {
//   uint32_t size;    // Actual number of elements used
//   double data[100]; // Fixed capacity

//   // Helper to convert from User to Shm
//   void fromUser(const UserMessageState & user)
//   {
//     size = std::min((int)user.state.size(), 100);
//     std::copy(user.state.begin(), user.state.begin() + size, data);
//   }

//   // Helper to convert from Shm to User
//   UserMessageState toUser() const
//   {
//     UserMessageState user;
//     user.state.assign(data, data + size);
//     return user;
//   }
// };

} // namespace mc_network
