#include <catch2/catch_test_macros.hpp>
#include "headsetdesk/DeviceSession.h"
#include "headsetdesk/Simulation.h"
#include "sony/protocol/FrameCodec.h"
#include <algorithm>

using namespace headsetdesk;
using namespace sony::protocol;

namespace {
auto recorder(std::vector<std::vector<std::uint8_t>>& requests) {
    return [&requests](std::string_view direction, std::span<const std::byte> bytes) {
        if (direction != "TX") return;
        const auto frame = FrameCodec::decode({reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()});
        if (frame.type == DataType::DataMdr) requests.push_back(frame.payload);
    };
}
int writes(const std::vector<std::vector<std::uint8_t>>& requests) {
    return std::count_if(requests.begin(), requests.end(), [](const auto& p) { return p[0] == 0x68 || p[0] == 0x58; });
}
}

TEST_CASE("Experimental controls require explicit connection opt-in", "[controls][safety]") {
    for (auto model : {Model::Xm5, Model::Ch720n}) {
        std::vector<std::vector<std::uint8_t>> requests;
        DeviceSession session(makeSimulatedTransport(model, recorder(requests)));
        REQUIRE_THROWS(session.enableControls(true));
        session.connect(simulatedCandidate(model));
        REQUIRE_THROWS(session.enableControls(true));
        session.refresh();
        REQUIRE_THROWS(session.setNoise(NoiseControlMode::Off));
        REQUIRE_THROWS(session.setEqualizer(0, {0,0,0,0,0}));
        CHECK(writes(requests) == 0);
        session.enableControls(true);
        session.refreshBattery();
        CHECK(session.state().controlsEnabled);
        session.disconnect();
        CHECK_FALSE(session.state().controlsEnabled);
        session.connect(simulatedCandidate(model)); session.refresh();
        CHECK_FALSE(session.state().controlsEnabled);
    }
}

TEST_CASE("Both model profiles confirm bounded noise and EQ settings by readback", "[controls][offline]") {
    for (auto model : {Model::Xm5, Model::Ch720n}) {
        std::vector<std::vector<std::uint8_t>> requests;
        DeviceSession session(makeSimulatedTransport(model, recorder(requests)));
        session.connect(simulatedCandidate(model)); session.refresh(); session.enableControls(true);
        for (const auto mode : {NoiseControlMode::Off, NoiseControlMode::NoiseCancelling, NoiseControlMode::Ambient}) {
            for (int level : {1, 20}) {
                requests.clear(); session.setNoise(mode, level);
                CHECK(session.state().noise.value->mode == mode);
                if (mode == NoiseControlMode::Ambient) CHECK(session.state().noise.value->ambientLevel == level);
                REQUIRE(requests.size() == 2);
                CHECK(requests[0] == std::vector<std::uint8_t>{0x68,0x17,1,static_cast<std::uint8_t>(mode != NoiseControlMode::Off),static_cast<std::uint8_t>(mode == NoiseControlMode::Ambient),0,static_cast<std::uint8_t>(level)});
                CHECK(requests[1] == std::vector<std::uint8_t>{0x66,0x17});
            }
        }
        requests.clear(); session.setEqualizer(-10, {-10,-5,0,5,10});
        CHECK(session.state().equalizer.value->clearBass == -10);
        CHECK(session.state().equalizer.value->bands == std::vector<int>{-10,-5,0,5,10});
        REQUIRE(requests.size() == 2);
        CHECK(requests[0] == std::vector<std::uint8_t>{0x58,0,0xa0,6,0,0,5,10,15,20});
        CHECK(requests[1] == std::vector<std::uint8_t>{0x56,0});
        session.setEqualizer(10, {10,10,10,10,10});
        session.refresh(); CHECK(session.state().controlsEnabled);
        for (const auto& capability : profileFor(model).capabilities) {
            CHECK_FALSE(capability.writesEnabled);
            if (capability.feature == Feature::NoiseControl || capability.feature == Feature::Equalizer)
                CHECK(capability.writeConfidence == Confidence::Unknown);
        }
    }
}

TEST_CASE("Invalid controls fail before any setting reaches the wire", "[controls][safety]") {
    std::vector<std::vector<std::uint8_t>> requests;
    DeviceSession session(makeSimulatedTransport(Model::Xm5, recorder(requests)));
    session.connect(simulatedCandidate(Model::Xm5)); session.refresh(); session.enableControls(true);
    requests.clear();
    REQUIRE_THROWS(session.setNoise(static_cast<NoiseControlMode>(9)));
    REQUIRE_THROWS(session.setNoise(NoiseControlMode::Ambient, 0));
    REQUIRE_THROWS(session.setNoise(NoiseControlMode::Ambient, 21));
    REQUIRE_THROWS(session.setEqualizer(11, {0,0,0,0,0}));
    REQUIRE_THROWS(session.setEqualizer(0, {0,0,0,0}));
    REQUIRE_THROWS(session.setEqualizer(0, {0,0,0,0,-11}));
    CHECK(requests.empty());
}

TEST_CASE("Unconfirmed settings disable controls without a write retry", "[controls][failure]") {
    for (auto model : {Model::Xm5, Model::Ch720n}) {
        for (auto failure : {SimulationFailure::IgnoreSettings, SimulationFailure::NoSettingAck, SimulationFailure::DisconnectOnSetting}) {
            for (bool eq : {false, true}) {
                std::vector<std::vector<std::uint8_t>> requests;
                DeviceSession session(makeSimulatedTransport(model, recorder(requests), failure));
                session.connect(simulatedCandidate(model)); session.refresh(); session.enableControls(true);
                requests.clear();
                if (eq) REQUIRE_THROWS(session.setEqualizer(3, {1,2,3,4,5}));
                else REQUIRE_THROWS(session.setNoise(NoiseControlMode::Ambient, 20));
                CHECK_FALSE(session.state().controlsEnabled);
                CHECK(writes(requests) == 1);
                session.refresh();
                CHECK_FALSE(session.state().controlsEnabled);
                CHECK(writes(requests) == 1);
                if (failure == SimulationFailure::DisconnectOnSetting) {
                    CHECK(session.state().connection == ConnectionState::Unavailable);
                    CHECK_FALSE(session.state().equalizer.value);
                }
            }
        }
    }
}
