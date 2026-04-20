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
  keyPath_ = std::string(homeDir) + NetworkInterface::ip();
  createShmBlock<MessageConfigShm>(NetworkInterface::port("config"));
  createShmBlock<MessageStateShm>(NetworkInterface::port("state"));
  createShmBlock<MessageCommandShm>(NetworkInterface::port("command"));

  mc_rtc::log::info("network shm done");
};

NetworkInterfaceShm::~NetworkInterfaceShm()
{
  for(void * ptr : shmptr_)
  {
    if(ptr && ptr != (void *)-1)
    {
      shmdt(ptr);
    }
  }

  for(int id : shmid_)
  {
    shmctl(id, IPC_RMID, nullptr);
  }
}

template<typename msg>
void NetworkInterfaceShm::createShmBlock(uint8_t port)
{
  key_t key = ftok(keyPath_.c_str(), port);
  int id = shmget(key, sizeof(msg), 0666 | IPC_CREAT);
  if(id < 0)
  {
    perror("shmget");
    throw std::runtime_error("shmget failed");
  }

  void * ptr = shmat(id, nullptr, 0);
  if(ptr == (void *)-1)
  {
    shmctl(id, IPC_RMID, nullptr);
    // TODO: error
  }

  shmid_.push_back(id);
  shmptr_.push_back(ptr);

  mc_rtc::log::success("Shared memory created: port={}, shmid={}", port, id);
};

void NetworkInterfaceShm::sendMessage(const MessageConfigShm & msg)
{
  sendMessageImpl(msg, NetworkInterface::port("config"));
}

void NetworkInterfaceShm::sendMessage(const MessageStateShm & msg)
{
  sendMessageImpl(msg, NetworkInterface::port("state"));
}

void NetworkInterfaceShm::sendMessage(const MessageCommandShm & msg)
{
  sendMessageImpl(msg, NetworkInterface::port("command"));
}

template<typename msg>
void NetworkInterfaceShm::sendMessageImpl(const msg & message, uint8_t port)
{
  msg * dest = static_cast<msg *>(shmptr_[port]);
  if(dest)
  {
    *dest = message;
  }
}

bool NetworkInterfaceShm::receiveMessage(MessageConfigShm & msg)
{
  return receiveMessageImpl(msg, NetworkInterface::port("config"));
}

bool NetworkInterfaceShm::receiveMessage(MessageStateShm & msg)
{
  return receiveMessageImpl(msg, NetworkInterface::port("state"));
}

bool NetworkInterfaceShm::receiveMessage(MessageCommandShm & msg)
{
  return receiveMessageImpl(msg, NetworkInterface::port("command"));
}

template<typename msg>
bool NetworkInterfaceShm::receiveMessageImpl(msg & message, uint8_t port)
{
  const msg * src = static_cast<const msg *>(shmptr_[port]);
  if(src)
  {
    message = *src;
    return true;
  }
}

} // namespace mc_network
