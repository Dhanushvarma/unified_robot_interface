#pragma once

#include <mc_network_interface/NetworkInterface.h>

#include <mc_rtc/logging.h>

#include <iostream>
#include <string>

namespace mc_network
{

struct MessageConfigShm
{
  uint8_t name_size;
  char name[256];
  uint16_t config_size;
  char config[4096];

  bool read = true;
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

  void sendMessage(const MessageConfig & msg) override;
  void sendMessage(const MessageState & msg) override;
  void sendMessage(const MessageCommand & msg) override;

  bool receiveMessage(MessageConfig & msg) override {}
  bool receiveMessage(MessageState & msg) override {}
  bool receiveMessage(MessageCommand & msg) override {}

private:
  void sendMessage(const MessageConfigShm & msg);
  void sendMessage(const MessageStateShm & msg);
  void sendMessage(const MessageCommandShm & msg);

  bool receiveMessage(MessageConfigShm & msg);
  bool receiveMessage(MessageStateShm & msg);
  bool receiveMessage(MessageCommandShm & msg);

  template<typename msg>
  std::pair<int, void *> createShmBlock(const std::string & path, const int & id);

  template<typename msg>
  void sendMessageImpl(const msg & message, const std::string & type);

  template<typename msg>
  bool receiveMessageImpl(msg & message, uint8_t port);

  std::string key_path_;
  std::vector<int> shm_ids_;
  std::unordered_map<std::string, void *> shm_map_;
};

template<typename msg>
std::pair<int, void *> NetworkInterfaceShm::createShmBlock(const std::string & path, const int & id)
{
  mc_rtc::log::success("network shm createShmBlock start");
  mc_rtc::log::info("id {} size {}", id, sizeof(msg));
  key_t key = ftok(path.c_str(), id);

  mc_rtc::log::info("network shm createShmBlock 1");

  int shm_id = shmget(key, sizeof(msg), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id < 0)
  {
    if(errno == EEXIST)
    {
      shm_id = shmget(key, sizeof(msg), 0666);
      mc_rtc::log::warning("Shared memory already exists for id = {}", shm_id);
    }
    else
    {
      perror("shmget");
      throw std::runtime_error("shmget failed");
    }
  }

  mc_rtc::log::info("network shm createShmBlock 2");

  void * shm_ptr = shmat(shm_id, nullptr, 0);
  if(shm_ptr == (void *)-1)
  {
    shmctl(shm_id, IPC_RMID, nullptr);
    throw std::runtime_error("shmat failed");
  }

  mc_rtc::log::info("network shm createShmBlock done");

  return {shm_id, shm_ptr};
};

template<typename MessageType>
void NetworkInterfaceShm::sendMessageImpl(const MessageType & msg, const std::string & type)
{
  mc_rtc::log::success("network shm sendMessageImpl start for type: {}", type);

  MessageType * dest = static_cast<MessageType *>(shm_map_[type]);

  if(dest)
  {
    *dest = msg;
    mc_rtc::log::info("Successfully copied message to SHM for type: {}", type);
  }
  else
  {
    mc_rtc::log::error("Destination pointer for {} is null", type);
  }

  mc_rtc::log::info("network shm sendMessageImpl done");
}

template<typename msg>
bool NetworkInterfaceShm::receiveMessageImpl(msg & message, uint8_t port)
{
  // const msg * src = static_cast<const msg *>(shmptr_[port]);
  // if(src)
  // {
  //   message = *src;
  //   return true;
  // }
  return false;
}

} // namespace mc_network
