#include <mc_network_interface/NetworkInterfaceShm.h>

// #include <type_traits>

namespace mc_network
{

NetworkInterfaceShm::NetworkInterfaceShm(const mc_rtc::Configuration & network_config)
: NetworkInterface(network_config)
{
  mc_rtc::log::success("network shm start");

  /* Initialize shared memory blocks*/
  const char * homeDir = std::getenv("HOME");
  key_path_ = std::string(homeDir) + NetworkInterface::ip();
  {
    auto [id, ptr] = createShmBlock<MessageConfigShm>(key_path_, NetworkInterface::ports()["config"]);
    shm_ids_.push_back(id);
    shm_map_["config"] = ptr;
  }
  {
    auto [id, ptr] = createShmBlock<MessageStateShm>(key_path_, NetworkInterface::ports()["state"]);
    shm_ids_.push_back(id);
    shm_map_["state"] = ptr;
  }
  {
    auto [id, ptr] = createShmBlock<MessageCommandShm>(key_path_, NetworkInterface::ports()["command"]);
    shm_ids_.push_back(id);
    shm_map_["command"] = ptr;
  }

  mc_rtc::log::info("network shm done");
};

NetworkInterfaceShm::~NetworkInterfaceShm()
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

void NetworkInterfaceShm::sendMessage(const MessageConfig & msg)
{
  mc_rtc::log::success("network shm sendMessage config start");

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
  msgShm.name[nameLen] = '\0'; // Ensure null termination

  mc_rtc::log::info("network shm sendMessage config 1");

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

  mc_rtc::log::info("network shm sendMessage config 2");

  msgShm.read = msg.read;

  mc_rtc::log::info("network shm sendMessage config 3");

  sendMessageImpl(msgShm, "config");

  mc_rtc::log::info("network shm sendMessage config done");
}

void NetworkInterfaceShm::sendMessage(const MessageState & msg)
{
  sendMessageImpl(msg, "state");
}

void NetworkInterfaceShm::sendMessage(const MessageCommand & msg)
{
  sendMessageImpl(msg, "command");
}

bool NetworkInterfaceShm::receiveMessage(MessageConfigShm & msg)
{
  return receiveMessageImpl(msg, NetworkInterface::ports()["config"]);
}

bool NetworkInterfaceShm::receiveMessage(MessageStateShm & msg)
{
  return receiveMessageImpl(msg, NetworkInterface::ports()["state"]);
}

bool NetworkInterfaceShm::receiveMessage(MessageCommandShm & msg)
{
  return receiveMessageImpl(msg, NetworkInterface::ports()["command"]);
}

} // namespace mc_network
