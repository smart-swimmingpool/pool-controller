
#include "RuleManu.hpp"
#include "Homie.hpp"

/**
 *
 */
RuleManu::RuleManu() {}

/**
 *
 */
void RuleManu::loop() {
  // no ruling if manual
  Homie.getLogger() << F("  ◦ § RuleManu: loop") << endl;
  return;
}
