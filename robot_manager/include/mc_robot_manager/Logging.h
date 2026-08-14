#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mc_fleet
{

namespace log
{

template<typename... Args>
inline void info(Args &&... args)
{
  ((std::cout << std::forward<Args>(args)), ...);
  std::cout << std::endl;
}

template<typename... Args>
inline void warning(Args &&... args)
{
  std::cerr << "[WARNING] ";
  ((std::cerr << std::forward<Args>(args)), ...);
  std::cerr << std::endl;
}

template<typename... Args>
inline void error(Args &&... args)
{
  std::cerr << "[ERROR] ";
  ((std::cerr << std::forward<Args>(args)), ...);
  std::cerr << std::endl;
}

template<typename... Args>
[[noreturn]] inline void errorAndThrow(Args &&... args)
{
  std::ostringstream os;
  ((os << std::forward<Args>(args)), ...);

  std::cerr << "[ERROR] " << os.str() << std::endl;

  throw std::runtime_error(os.str());
}

} // namespace log

} // namespace mc_fleet
