#pragma once

#include <mc_rtc/Configuration.h>

#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>

namespace mc_network
{

struct InitMessage
{
  bool read = true;
  char name[100];
  char config[4096];
};

struct Message
{
  pthread_mutex_t mutex;
  bool read = true;
  double command[100];
};

class NetworkInterface
{
public:
  NetworkInterface(std::string name, const mc_rtc::Configuration & config)
  : name_(std::move(name)), ip_(config("network")("ip")), port_(config("network")("port"))
  {
    const char * homeDir = std::getenv("HOME");
    std::string keyPath = std::string(homeDir) + "/workspace/sandbox/mc_robot_manager/CMakeLists.txt";
    key_t key = ftok(keyPath.c_str(), 99);
    shmid_ = shmget(key, sizeof(mc_network::InitMessage), 0666 | IPC_CREAT);
    if(shmid_ == -1)
    {
      mc_rtc::log::info("keyPath {}", keyPath);
      std::string error_msg = "shmget failed: " + std::string(strerror(errno));
      mc_rtc::log::error_and_throw("Error creating shared memory");
    }
    mc_rtc::log::success("Shared memory successfully created shmid_ {}", shmid_);

    InitMessage * init_message = (InitMessage *)shmat(shmid_, NULL, 0);
    if(init_message == (void *)-1)
    {
      mc_rtc::log::error_and_throw("Error attaching shared memory");
    }

    strncpy(init_message->name, name_.c_str(), sizeof(init_message->name) - 1);
    std::string dump = config.dump();
    strncpy(init_message->config, dump.c_str(), sizeof(init_message->config) - 1);
    init_message->read = false;

    shmdt(init_message);
  }

  ~NetworkInterface()
  {
    shmctl(shmid_, IPC_RMID, nullptr);
  }

protected:
  // Accessors for derived classes
  [[nodiscard]] const std::string & name() const
  {
    return name_;
  }
  [[nodiscard]] const std::string & ip() const
  {
    return ip_;
  }
  [[nodiscard]] uint16_t port() const
  {
    return port_;
  }

private:
  int shmid_;

  const std::string name_;
  const std::string ip_;
  uint16_t port_;
};

} // namespace mc_network
