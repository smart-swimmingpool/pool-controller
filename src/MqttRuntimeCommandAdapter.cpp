// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
// SPDX-License-Identifier: MIT

#include "MqttRuntimeCommandAdapter.hpp"

#include <cstdlib>
#include <cstring>

#include "ControllerCommandHandler.hpp"
#include "LogCapture.hpp"
#include "MqttPublisher.hpp"
#include "NetworkManager.hpp"
#include "OperationMode.hpp"
#include "TimeClientHelper.hpp"

namespace PoolController {

namespace {

bool topicEndsWith(const char *topic, const char *suffix) {
  const std::size_t topicLength = std::strlen(topic);
  const std::size_t suffixLength = std::strlen(suffix);
  return topicLength >= suffixLength && std::memcmp(topic + topicLength - suffixLength, suffix, suffixLength) == 0;
}

bool isDigit(char value) {
  return value >= '0' && value <= '9';
}

bool parseTime(const char *value, std::size_t length, std::uint8_t &hour, std::uint8_t &minute) {
  if (length < 5 || value[2] != ':' || !isDigit(value[0]) || !isDigit(value[1]) || !isDigit(value[3]) || !isDigit(value[4])) {
    return false;
  }
  const unsigned parsedHour = static_cast<unsigned>((value[0] - '0') * 10 + (value[1] - '0'));
  const unsigned parsedMinute = static_cast<unsigned>((value[3] - '0') * 10 + (value[4] - '0'));
  if (parsedHour > 23 || parsedMinute > 59) {
    return false;
  }
  hour = static_cast<std::uint8_t>(parsedHour);
  minute = static_cast<std::uint8_t>(parsedMinute);
  return true;
}

bool parseSensorAddress(const char *value, std::size_t length, std::array<std::uint8_t, 8> &address) {
  if (length < 16) {
    return false;
  }
  for (std::size_t i = 0; i < address.size(); ++i) {
    const char hi = value[i * 2];
    const char lo = value[i * 2 + 1];
    const auto hexValue = [](char c) -> int {
      if (c >= '0' && c <= '9')
        return c - '0';
      if (c >= 'a' && c <= 'f')
        return 10 + c - 'a';
      if (c >= 'A' && c <= 'F')
        return 10 + c - 'A';
      return -1;
    };
    const int high = hexValue(hi);
    const int low = hexValue(lo);
    if (high < 0 || low < 0) {
      return false;
    }
    address[i] = static_cast<std::uint8_t>((high << 4) | low);
  }
  return true;
}

}  // namespace

void MqttRuntimeCommandAdapter::begin() {
  NetworkManager::setMqttCallback(
    [this](char *topic, char *payload, AsyncMqttClientMessageProperties properties, std::size_t len, std::size_t index,
      std::size_t total) { onMqttMessage(topic, payload, properties, len, index, total); });
}

void MqttRuntimeCommandAdapter::onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
  std::size_t len, std::size_t index, std::size_t total) {
  (void)properties;
  if (index != 0 || len != total) {
    LOG_WARN("MQTT runtime adapter: ignoring chunked message on %s\n", topic);
    return;
  }

  switch (rawQueue_.push(topic, payload, len)) {
  case MqttCommandQueue::PushResult::OK:
    break;
  case MqttCommandQueue::PushResult::FULL:
    LOG_WARN("MQTT runtime adapter: queue full — dropping %s\n", topic);
    break;
  case MqttCommandQueue::PushResult::TOO_LARGE:
    LOG_WARN("MQTT runtime adapter: message too large — dropping %s\n", topic);
    break;
  }
}

MqttRuntimeCommandAdapter::ParseResult MqttRuntimeCommandAdapter::parseRuntimeCommand(
  const MqttCommandQueue::Message &message, ControllerCommand &command) const {
  command = ControllerCommand{};
  command.source = CommandSource::MQTT;
  const char *topic = message.topic;
  const char *value = message.payload;

  if (topicEndsWith(topic, "/thermostat/preset/set")) {
    command.type = ControllerCommandType::SET_MODE;
    if (std::strcmp(value, "none") == 0)
      command.mode = OperationMode::AUTO;
    else if (std::strcmp(value, "manual") == 0)
      command.mode = OperationMode::MANUAL;
    else if (std::strcmp(value, "schedule") == 0)
      command.mode = OperationMode::TIMER;
    else if (std::strcmp(value, "boost") == 0)
      command.mode = OperationMode::BOOST;
    else
      return ParseResult::INVALID;
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/thermostat/mode/set")) {
    command.type = ControllerCommandType::SET_MODE;
    if (std::strcmp(value, "off") == 0)
      command.mode = OperationMode::MANUAL;
    else if (std::strcmp(value, "auto") == 0)
      command.mode = OperationMode::AUTO;
    else if (std::strcmp(value, "heat") == 0)
      command.mode = OperationMode::BOOST;
    else
      return ParseResult::INVALID;
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/thermostat/temperature/set") || topicEndsWith(topic, "/pool-max-temp/set")) {
    command.type = ControllerCommandType::SET_POOL_MAX_TEMPERATURE;
    command.value = std::strtof(value, nullptr);
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/solar-min-temp/set")) {
    command.type = ControllerCommandType::SET_SOLAR_MIN_TEMPERATURE;
    command.value = std::strtof(value, nullptr);
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/hysteresis/set")) {
    command.type = ControllerCommandType::SET_TEMPERATURE_HYSTERESIS;
    command.value = std::strtof(value, nullptr);
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/temp-circ-threshold/set")) {
    command.type = ControllerCommandType::SET_TEMPERATURE_CIRCULATION_THRESHOLD;
    command.value = std::strtof(value, nullptr);
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/temp-circ-factor/set")) {
    command.type = ControllerCommandType::SET_TEMPERATURE_CIRCULATION_FACTOR;
    command.integerValue = std::atoi(value);
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/temp-circ-max-runtime/set")) {
    command.type = ControllerCommandType::SET_TEMPERATURE_CIRCULATION_MAX_RUNTIME;
    command.integerValue = std::atoi(value);
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/pool-pump/set") || topicEndsWith(topic, "/solar-pump/set")) {
    if (std::strcmp(value, "ON") != 0 && std::strcmp(value, "OFF") != 0) {
      return ParseResult::INVALID;
    }
    if (topicEndsWith(topic, "/pool-pump/set")) {
      command.type = ControllerCommandType::SET_POOL_PUMP_MANUAL;
    } else {
      command.type = ControllerCommandType::SET_SOLAR_PUMP_MANUAL;
    }
    command.enabled = std::strcmp(value, "ON") == 0;
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/mode/set")) {
    command.type = ControllerCommandType::SET_MODE;
    if (!tryParseOperationMode(value, command.mode)) {
      return ParseResult::INVALID;
    }
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/timer-start/set") || topicEndsWith(topic, "/timer-end/set")) {
    if (topicEndsWith(topic, "/timer-start/set")) {
      command.type = ControllerCommandType::SET_TIMER_START;
    } else {
      command.type = ControllerCommandType::SET_TIMER_END;
    }
    if (!parseTime(value, message.payloadLen, command.hour, command.minute)) {
      return ParseResult::INVALID;
    }
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/timezone/set")) {
    const int timezoneIndex = getTimezoneIndexFromLabel(value);
    if (timezoneIndex < 0) {
      return ParseResult::INVALID;
    }
    command.type = ControllerCommandType::SET_TIMEZONE;
    command.integerValue = timezoneIndex;
    return ParseResult::VALID;
  }

  if (topicEndsWith(topic, "/solar-sensor/set") || topicEndsWith(topic, "/pool-sensor/set")) {
    command.sensorRole = topicEndsWith(topic, "/solar-sensor/set") ? SensorRole::SOLAR : SensorRole::POOL;
    if (std::strstr(value, "Not configured") != nullptr || message.payloadLen < 16) {
      command.type = ControllerCommandType::CLEAR_SENSOR_MAPPING;
      return ParseResult::VALID;
    }
    if (!parseSensorAddress(value, message.payloadLen, command.sensorAddress)) {
      return ParseResult::INVALID;
    }
    command.type = ControllerCommandType::SET_SENSOR_MAPPING;
    return ParseResult::VALID;
  }

  return ParseResult::NOT_RUNTIME;
}

void MqttRuntimeCommandAdapter::processPendingCommands() {
  MqttCommandQueue::Message message;
  for (std::size_t handled = 0; handled < kMaxCommandsPerLoop && rawQueue_.pop(message); ++handled) {
    ControllerCommand command{};
    const ParseResult result = parseRuntimeCommand(message, command);
    if (result == ParseResult::NOT_RUNTIME) {
      AsyncMqttClientMessageProperties properties{};
      MqttPublisher::handleMqttMessage(message.topic, message.payload, properties, message.payloadLen, 0, message.payloadLen);
      continue;
    }

    if (result == ParseResult::INVALID) {
      LOG_WARN("MQTT runtime adapter: invalid command on %s\n", message.topic);
      MqttPublisher::publishStates();
      continue;
    }

    if (!handler_.handle(command)) {
      LOG_WARN("MQTT runtime adapter: command rejected on %s\n", message.topic);
    }
    MqttPublisher::publishStates();
  }
}

}  // namespace PoolController
