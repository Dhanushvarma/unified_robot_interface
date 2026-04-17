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
  shmid_.push_back(id);

  void * ptr = shmat(id, NULL, 0);
  if(ptr == (void *)-1)
  {
    perror("shmat");
  }
  shmptr_.push_back(ptr);

  mc_rtc::log::success("Shared memory successfully created port {} id {}", port, id);
};

void NetworkInterfaceShm::sendMessage(const MessageConfigShm & msg)
{
  MessageConfigShm * dest = static_cast<MessageConfigShm *>(shmptr_[0]);
  if(dest)
  {
    *dest = msg;
  }
}

} // namespace mc_network
