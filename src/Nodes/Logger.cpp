// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
#include "Logger.hpp"

namespace PoolController {
namespace Nodes {

// Logger is now a plain struct with no HomieNode constructor.
// All Homie-specific initialisation (advertise, setDatatype, etc.) has
// been removed as part of the full migration to standalone MQTT via
// MqttPublisher + NetworkManager.

}  // namespace Nodes
}  // namespace PoolController
