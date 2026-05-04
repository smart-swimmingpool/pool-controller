/**
 * Unit tests for RuleAuto
 */

#include <gtest/gtest.h>
#include "RuleAuto.hpp"
#include "../mocks/MockRelayController.hpp"

class RuleAutoTest : public ::testing::Test {
protected:
    void SetUp() override {
        solarRelay = new MockRelayController();
        poolRelay = new MockRelayController();

        // Create a mock RelayModuleNode wrapper for testing
        // Note: This is a simplified approach. In a real scenario, you'd need
        // to create a proper mock that inherits from RelayModuleNode
        rule = new RuleAuto(
            reinterpret_cast<RelayModuleNode*>(solarRelay),
            reinterpret_cast<RelayModuleNode*>(poolRelay)
        );

        // Set up default temperatures
        rule->setPoolTemperature(25.0f);
        rule->setSolarTemperature(40.0f);
        rule->setPoolMaxTemperature(30.0f);
        rule->setSolarMinTemperature(35.0f);
        rule->setTemperatureHysteresis(1.0f);
    }

    void TearDown() override {
        delete rule;
        delete solarRelay;
        delete poolRelay;
    }

    RuleAuto* rule;
    MockRelayController* solarRelay;
    MockRelayController* poolRelay;
};

TEST_F(RuleAutoTest, GetModeReturnsAuto) {
    EXPECT_STREQ(rule->getMode(), "auto");
}

TEST_F(RuleAutoTest, InitialState) {
    EXPECT_EQ(rule->getPoolTemperature(), 25.0f);
    EXPECT_EQ(rule->getSolarTemperature(), 40.0f);
    EXPECT_EQ(rule->getPoolMaxTemperature(), 30.0f);
    EXPECT_EQ(rule->getSolarMinTemperature(), 35.0f);
    EXPECT_EQ(rule->getTemperatureHysteresis(), 1.0f);
}

TEST_F(RuleAutoTest, SetTemperatures) {
    rule->setPoolTemperature(28.0f);
    rule->setSolarTemperature(45.0f);

    EXPECT_EQ(rule->getPoolTemperature(), 28.0f);
    EXPECT_EQ(rule->getSolarTemperature(), 45.0f);
}

TEST_F(RuleAutoTest, SetThresholds) {
    rule->setPoolMaxTemperature(35.0f);
    rule->setSolarMinTemperature(40.0f);
    rule->setTemperatureHysteresis(2.0f);

    EXPECT_EQ(rule->getPoolMaxTemperature(), 35.0f);
    EXPECT_EQ(rule->getSolarMinTemperature(), 40.0f);
    EXPECT_EQ(rule->getTemperatureHysteresis(), 2.0f);
}
