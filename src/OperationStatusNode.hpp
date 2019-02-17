/**
 * Homie Node for Dallas sensors.
 *
 */

#pragma once

#include <Homie.hpp>

class OperationStatusNode : public HomieNode {

public:
  OperationStatusNode(const char* id);

  int getStatu() const { return status; }

protected:
  void setup() override;
  void loop() override;

private:
  // suggested rate is 1/60Hz (1m)
  static const int MIN_INTERVAL         = 60;  // in seconds
  static const int MEASUREMENT_INTERVAL = 300;
  const char*      cCaption             = "• Operation Status:";
  const char*      cIndent              = "  ◦ ";

  const char* cStatus          = "status";
  const char* cStatusName      = "Status";
  int status = 0;

  void printCaption();
};
