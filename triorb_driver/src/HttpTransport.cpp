#include <triorb_driver/HttpTransport.h>

#include <httplib.h>

#include <stdexcept>
#include <utility>

namespace triorb_driver
{

namespace
{

class CppHttpTransport final : public HttpTransport
{
public:
  CppHttpTransport(std::string host, uint16_t port, std::chrono::milliseconds timeout) : client_(std::move(host), port)
  {
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);

    const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout - seconds);

    client_.set_connection_timeout(seconds.count(), microseconds.count());

    client_.set_read_timeout(seconds.count(), microseconds.count());

    client_.set_write_timeout(seconds.count(), microseconds.count());
  }

  HttpResponse get(const std::string & path) override
  {
    const auto result = client_.Get(path);

    if(!result)
    {
      throw std::runtime_error("TriOrb HTTP GET failed for " + path);
    }

    return {result->status, result->body};
  }

  HttpResponse post(const std::string & path, const std::string & body) override
  {
    const auto result = client_.Post(path, body, "application/json");

    if(!result)
    {
      throw std::runtime_error("TriOrb HTTP POST failed for " + path);
    }

    return {result->status, result->body};
  }

private:
  httplib::Client client_;
};

} // namespace

std::unique_ptr<HttpTransport> makeHttpTransport(const std::string & host,
                                                 uint16_t port,
                                                 std::chrono::milliseconds timeout)
{
  return std::make_unique<CppHttpTransport>(host, port, timeout);
}

} // namespace triorb_driver
