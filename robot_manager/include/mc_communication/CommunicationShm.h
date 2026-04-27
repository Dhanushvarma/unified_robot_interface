#pragma once

#include <mc_communication/Communication.h>

#include <mc_rtc/logging.h>

#include <iostream>
#include <string>

namespace mc_communication
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

class CommunicationShm : public Communication
{
public:
  CommunicationShm();
  CommunicationShm(const mc_rtc::Configuration & com_config);
  ~CommunicationShm();

  bool sendMessage(const MessageConfig & message) override;
  bool sendMessage(const MessageState & message) override;
  bool sendMessage(const MessageCommand & message) override;

  bool receiveMessage(MessageConfig & message) override;
  bool receiveMessage(MessageState & message) override;
  bool receiveMessage(MessageCommand & message) override;

private:
  template<typename MessageType>
  std::pair<int, void *> createShmBlock(const std::string & path, const int & id);

  template<typename MessageType>
  bool sendMessageImpl(const MessageType & message, const std::string & type);

  template<typename MessageType>
  bool receiveMessageImpl(MessageType & message, const std::string & type);

  std::string key_path_;
  std::vector<int> shm_ids_;
  std::unordered_map<std::string, void *> shm_map_;
};

template<typename MessageType>
std::pair<int, void *> CommunicationShm::createShmBlock(const std::string & path, const int & id)
{
  mc_rtc::log::success("com shm createShmBlock start");
  mc_rtc::log::info("id {} size {}", id, sizeof(MessageType));
  key_t key = ftok(path.c_str(), id);

  mc_rtc::log::info("com shm createShmBlock 1");

  int shm_id = shmget(key, sizeof(MessageType), 0666 | IPC_CREAT | IPC_EXCL);
  if(shm_id < 0)
  {
    if(errno == EEXIST)
    {
      shm_id = shmget(key, sizeof(MessageType), 0666);
      mc_rtc::log::warning("Shared memory already exists for id = {}", shm_id);
    }
    else
    {
      perror("shmget");
      throw std::runtime_error("shmget failed");
    }
  }

  mc_rtc::log::info("com shm createShmBlock 2");

  void * shm_ptr = shmat(shm_id, nullptr, 0);
  if(shm_ptr == (void *)-1)
  {
    shmctl(shm_id, IPC_RMID, nullptr);
    throw std::runtime_error("shmat failed");
  }

  mc_rtc::log::info("com shm createShmBlock done");

  return {shm_id, shm_ptr};
};

template<typename MessageType>
bool CommunicationShm::sendMessageImpl(const MessageType & message, const std::string & type)
{
  mc_rtc::log::success("com shm sendMessageImpl start for type: {}", type);

  MessageType * dest = static_cast<MessageType *>(shm_map_[type]);

  if(dest)
  {
    *dest = message;
    mc_rtc::log::info("Copied message to SHM for type: {}", type);
    return true;
  }

  mc_rtc::log::info("com shm sendMessageImpl done");
  return false;
}

template<typename MessageType>
bool CommunicationShm::receiveMessageImpl(MessageType & message, const std::string & type)
{
  mc_rtc::log::success("com shm receiveMessageImpl start for type: {}", type);

  MessageType * src = static_cast<MessageType *>(shm_map_[type]);

  if(src)
  {
    message = *src;
    mc_rtc::log::info("Coppied message from SHM for type: {}", type);
    return true;
  }

  return false;
}

} // namespace mc_communication
