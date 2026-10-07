#include <catch2/catch_test_macros.hpp>
#include "sony/protocol/DeviceState.h"
using namespace sony::protocol;

TEST_CASE("DeviceState default initialization and immutability", "[device_state]") {
    DeviceState state;

    CHECK_FALSE(state.battery.main.has_value());
    CHECK_FALSE(state.battery.left.has_value());
    CHECK_FALSE(state.battery.right.has_value());
    CHECK_FALSE(state.battery.caseBattery.has_value());
    CHECK_FALSE(state.battery.charging.has_value());

    CHECK(state.noiseControl.mode == NoiseControlMode::Off);
    CHECK(state.noiseControl.ambientLevel == 0);
    CHECK_FALSE(state.noiseControl.focusOnVoice);

    CHECK(state.equalizer.preset == 0);
    CHECK(state.equalizer.clearBass == 0);
    for (int band : state.equalizer.bands) {
        CHECK(band == 0);
    }

    CHECK_FALSE(state.dsee);
    CHECK(state.firmware.empty());
    CHECK(state.codec.empty());
    CHECK(state.autoPowerOff == 0);
    CHECK_FALSE(state.speakToChat);
    CHECK_FALSE(state.adaptiveVolume);

    state.battery.main = 85;
    state.battery.charging = true;
    state.noiseControl.mode = NoiseControlMode::NoiseCancelling;
    state.firmware = "2.0.1";

    DeviceStateSnapshot snapshot = std::make_shared<const DeviceState>(state);
    CHECK(snapshot->battery.main.value() == 85);
    CHECK(snapshot->battery.charging == true);
    CHECK(snapshot->noiseControl.mode == NoiseControlMode::NoiseCancelling);
    CHECK(snapshot->firmware == "2.0.1");

    // Modify local state and verify snapshot is unaffected
    state.battery.main = 40;
    CHECK(snapshot->battery.main.value() == 85);
}

