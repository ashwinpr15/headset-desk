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
    return std::count_if(requests.begin(), requests.end(), [](const auto& p) { return p[0] == 0x68 || p[0] == 0x58 || p[0] == 0xe8 || p[0] == 0xf8; });
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
                CHECK(requests[0] == std::vector<std::uint8_t>{0x68,0x17,1,static_cast<std::uint8_t>(mode != NoiseControlMode::Off),static_cast<std::uint8_t>((mode == NoiseControlMode::Ambient) != (model == Model::Ch720n)),0,static_cast<std::uint8_t>(level)});
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

TEST_CASE("Speak-to-Chat, DSEE and voice passthrough need opt-in and confirm by readback", "[controls][features]") {
    for (auto model : {Model::Xm5, Model::Ch720n}) {
        std::vector<std::vector<std::uint8_t>> requests;
        DeviceSession session(makeSimulatedTransport(model, recorder(requests)));
        session.connect(simulatedCandidate(model)); session.refresh();
        // Nothing extra is queried or readable before the opt-in.
        CHECK_FALSE(session.state().dsee.value); CHECK_FALSE(session.state().speakToChat.value);
        for (const auto& p : requests) CHECK((p[0] != 0xe6 && p[0] != 0xf6));
        REQUIRE_THROWS(session.setDsee(true));
        session.enableControls(true);
        REQUIRE(session.state().dsee.value.has_value());
        CHECK(*session.state().dsee.value == false);
        CHECK(session.state().speakToChat.value.has_value() == (model == Model::Xm5));
        requests.clear(); session.setDsee(true);
        REQUIRE(requests.size() == 2);
        CHECK(requests[0] == std::vector<std::uint8_t>{0xe8, 0x01, 0x01});
        CHECK(requests[1] == std::vector<std::uint8_t>{0xe6, 0x01});
        CHECK(*session.state().dsee.value);
        if (model == Model::Xm5) {
            requests.clear(); session.setSpeakToChat(true);
            CHECK(requests[0] == std::vector<std::uint8_t>{0xf8, 0x0c, 0x00, 0x01});
            CHECK(*session.state().speakToChat.value);
            session.setSpeakToChat(false);
            CHECK_FALSE(*session.state().speakToChat.value);
        } else REQUIRE_THROWS(session.setSpeakToChat(true));
        session.setNoise(NoiseControlMode::Ambient, 5, 1);
        CHECK(session.state().noise.value->focusOnVoice);
        session.setNoise(NoiseControlMode::Ambient, 5, 0);
        CHECK_FALSE(session.state().noise.value->focusOnVoice);
        session.enableControls(false);
        CHECK_FALSE(session.state().dsee.value);
        requests.clear(); session.refresh();
        for (const auto& p : requests) CHECK((p[0] != 0xe6 && p[0] != 0xf6));
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

TEST_CASE("CH720N reads and writes the ambient bit the other way round", "[controls][ch720n]") {
    // Owner heard noise cancelling when ambient was requested with the upstream bit order.
    std::vector<std::vector<std::uint8_t>> requests;
    DeviceSession session(makeSimulatedTransport(Model::Ch720n, recorder(requests)));
    session.connect(simulatedCandidate(Model::Ch720n)); session.refresh(); session.enableControls(true);
    CHECK(session.state().noise.value->mode == NoiseControlMode::NoiseCancelling);
    requests.clear(); session.setNoise(NoiseControlMode::Ambient, 7);
    CHECK(requests[0] == std::vector<std::uint8_t>{0x68,0x17,1,1,0,0,7}); // upstream order would send 1
    CHECK(session.state().noise.value->mode == NoiseControlMode::Ambient);
    CHECK(session.state().noise.value->ambientLevel == 7);
    requests.clear(); session.setNoise(NoiseControlMode::NoiseCancelling);
    CHECK(requests[0][4] == 1);
    CHECK(session.state().noise.value->mode == NoiseControlMode::NoiseCancelling);
    CHECK(session.state().lastChange.find("confirmed") != std::string::npos);
}

TEST_CASE("A readback that never matches keeps controls on and sends nothing twice", "[controls][failure]") {
    for (auto model : {Model::Xm5, Model::Ch720n}) {
        for (bool eq : {false, true}) {
            std::vector<std::vector<std::uint8_t>> requests;
            DeviceSession session(makeSimulatedTransport(model, recorder(requests), SimulationFailure::IgnoreSettings));
            session.connect(simulatedCandidate(model)); session.refresh(); session.enableControls(true);
            requests.clear();
            if (eq) REQUIRE_THROWS_AS(session.setEqualizer(3, {1,2,3,4,5}), SettingNotConfirmed);
            else REQUIRE_THROWS_AS(session.setNoise(NoiseControlMode::Ambient, 20), SettingNotConfirmed);
            CHECK(session.state().controlsEnabled);
            CHECK(writes(requests) == 1);
            CHECK_FALSE(session.state().lastChange.empty());
        }
    }
}

TEST_CASE("Unconfirmed settings disable controls without a write retry", "[controls][failure]") {
    for (auto model : {Model::Xm5, Model::Ch720n}) {
        for (auto failure : {SimulationFailure::NoSettingAck, SimulationFailure::DisconnectOnSetting}) {
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
