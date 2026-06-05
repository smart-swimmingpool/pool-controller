
#include "RuleManu.hpp"
#include <Arduino.h>

/**
 *
 */
RuleManu::RuleManu() {}

/**
 *
 */
void RuleManu::loop() {
  // no ruling if manual
  Serial.println(F("  ◦ § RuleManu: loop"));
}
