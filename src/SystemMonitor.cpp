#include "SystemMonitor.hpp"

namespace PoolController {

// Static member initialization
unsigned long SystemMonitor::lastMemoryCheck = 0;
uint32_t SystemMonitor::minFreeHeap = 0;
bool SystemMonitor::lowMemoryWarning = false;

} // namespace PoolController
