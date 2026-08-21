#pragma once

#include <chrono>
#include <memory>
#include <string>

namespace triorb_driver
{

struct HttpResponse
{
  int status = 0;
  std::string body;
};

class HttpTransport
{
public:
  virtual ~HttpTransport() = default;

  virtual HttpResponse get(const std::string & path) = 0;

  virtual HttpResponse post(const std::string & path, const std::string & body) = 0;
};

std::unique_ptr<HttpTransport> makeHttpTransport(const std::string & host,
                                                 uint16_t port,
                                                 std::chrono::milliseconds timeout);

} // namespace triorb_driver
