#pragma once

#include <atomic>
#include <string>

namespace mc_fleet
{

void run(void * data, const std::atomic<bool> & interrupt);

void * init(int argc, char ** argv, uint64_t & cycle_ns, const std::atomic<bool> & interrupt);

} // namespace mc_fleet
