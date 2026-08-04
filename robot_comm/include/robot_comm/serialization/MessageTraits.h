#pragma once

#include "Messages.h"
#include <iostream>

namespace robot_comm
{

template<typename T>
struct MessageTraits;

template<>
struct MessageTraits<State>
{
  static constexpr MessageType type = MessageType::STATE;
};

template<>
struct MessageTraits<Command>
{
  static constexpr MessageType type = MessageType::COMMAND;
};

template<>
struct MessageTraits<Envelope>
{
  static constexpr MessageType type = MessageType::ENVELOPE;
};

template<>
struct MessageTraits<std::string>
{
  static constexpr MessageType type = MessageType::CONFIG;
};

} // namespace robot_comm
