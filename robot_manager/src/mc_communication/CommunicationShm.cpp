#include <mc_communication/CommunicationShm.h>

#include <mc_rtc/Configuration.h>

// #include <type_traits>

namespace mc_communication
{

CommunicationShm::CommunicationShm(const mc_rtc::Configuration & com_config) : Communication(com_config)
{
  mc_rtc::log::success("com shm start");

  /* Initialize shared memory blocks*/
  const char * homeDir = std::getenv("HOME");
  key_path_ = std::string(homeDir) + Communication::ip();
  {
    auto [id, ptr] = createShmBlock<MessageConfigShm>(key_path_, Communication::ports()["config"]);
    shm_ids_.push_back(id);
    shm_map_["config"] = ptr;
  }
  {
    auto [id, ptr] = createShmBlock<MessageStateShm>(key_path_, Communication::ports()["state"]);
    shm_ids_.push_back(id);
    shm_map_["state"] = ptr;
  }
  {
    auto [id, ptr] = createShmBlock<MessageCommandShm>(key_path_, Communication::ports()["command"]);
    shm_ids_.push_back(id);
    shm_map_["command"] = ptr;
  }

  mc_rtc::log::info("com shm done");
};

CommunicationShm::~CommunicationShm()
{
  mc_rtc::log::info("Destructor called, cleaning up {} segments", shm_ids_.size());

  for(auto & [type, ptr] : shm_map_)
  {
    if(ptr && ptr != (void *)-1)
    {
      if(shmdt(ptr) == -1)
      {
        perror("shmdt failed");
      }
      else
      {
        mc_rtc::log::info("Detached shmptr {}", ptr);
      }
    }
  }

  for(int id : shm_ids_)
  {
    if(shmctl(id, IPC_RMID, nullptr) == -1)
    {
      perror("shmctl failed"); // This will tell you exactly WHY it failed
    }
    else
    {
      mc_rtc::log::info("Marked shmid {} for destruction", id);
    }
  }
}

bool CommunicationShm::sendMessage(const MessageConfig & msg)
{
  mc_rtc::log::success("com shm sendMessage config start");

  /* Convert to shared memory compatible message */
  MessageConfigShm msgShm;

  size_t nameLen = msg.name.size();
  if(nameLen > 255)
  {
    mc_rtc::log::info("name: {}", msg.name);
    mc_rtc::log::error_and_throw("Name is longer than 255 characters");
  }
  msgShm.name_size = static_cast<uint8_t>(nameLen);
  std::memcpy(msgShm.name, msg.name.c_str(), nameLen);
  msgShm.name[nameLen] = '\0';

  mc_rtc::log::info("com shm sendMessage config 1");

  std::string configStr = msg.config.dump();
  size_t configLen = configStr.size();
  if(configLen > 4095)
  {
    mc_rtc::log::info("config: {}", configStr);
    mc_rtc::log::error_and_throw("Config is longer than 4095 characters");
  }
  msgShm.config_size = static_cast<uint16_t>(configLen);
  std::memcpy(msgShm.config, configStr.c_str(), configLen);
  msgShm.config[configLen] = '\0';

  mc_rtc::log::info("com shm sendMessage config 2");

  msgShm.read = msg.read;

  /* Send message */
  mc_rtc::log::info("com shm sendMessage config done");
  return sendMessageImpl(msgShm, "config");
}

bool CommunicationShm::sendMessage(const MessageState & msg)
{
  MessageStateShm msgShm;
  return sendMessageImpl(msgShm, "state");
}

bool CommunicationShm::sendMessage(const MessageCommand & msg)
{
  MessageCommandShm msgShm;
  return sendMessageImpl(msgShm, "command");
}

bool CommunicationShm::receiveMessage(MessageConfig & msg)
{
  mc_rtc::log::success("com shm sendMessage config start");

  /* Receive message */
  MessageConfigShm msgShm;
  bool status = receiveMessageImpl(msgShm, "config");
  if(!status) return status;

  /* Convert to standard message */
  msg.name = std::string(msgShm.name, msgShm.name_size);

  std::string config_data(msgShm.config, msgShm.config_size);
  msg.config = mc_rtc::Configuration::fromData(config_data);
  if(!msg.config.has("controller") || !msg.config.has("communication")) return false;

  msg.read = msgShm.read;

  return status;
}

bool CommunicationShm::receiveMessage(MessageState & msg)
{
  MessageStateShm msgShm;
  return receiveMessageImpl(msgShm, "state");
}

bool CommunicationShm::receiveMessage(MessageCommand & msg)
{
  MessageCommandShm msgShm;
  return receiveMessageImpl(msgShm, "command");
}

} // namespace mc_communication
