#include <catch2/catch_test_macros.hpp>
#include "headsetdesk/DeviceSession.h"
#include "headsetdesk/Simulation.h"
#include "sony/transport/FakeTransport.h"
#include "WindowsBluetoothConnector.h"
#include "ReplyingFakeTransport.h"
#include "sony/protocol/FrameCodec.h"

using namespace headsetdesk;
using namespace sony::transport;
using namespace sony::protocol;

namespace {
Candidate candidate(Model model, std::string address, std::optional<bool> paired, std::optional<bool> active) {
    return {model, {.name = std::string(profileFor(model).name), .address = DeviceAddress(std::move(address)), .paired = paired, .connected = active}};
}
}

TEST_CASE("Only exact supported names can select a protocol profile", "[profile][safety]") {
    CHECK(identifyModel("WH-1000XM5") == Model::Xm5);
    CHECK(identifyModel("wh-ch720n") == Model::Ch720n);
    for (auto name : {"", "WH-1000XM4", "WH-1000XM6", "XM5", "WH-1000XM5 clone", "Sony WH-CH720N", "Renamed headset"})
        CHECK_FALSE(identifyModel(name).has_value());
}

TEST_CASE("Reviewed hardware reads have independent evidence and all writes stay off", "[profile][safety]") {
    for (auto model : {Model::Xm5, Model::Ch720n}) {
        const auto& profile = profileFor(model);
        REQUIRE(profile.capabilities.size() == 6);
        for (const auto& capability : profile.capabilities) {
            CHECK_FALSE(capability.writesEnabled);
            CHECK(capability.readConfidence == Confidence::Verified);
            CHECK(capability.evidence == (model == Model::Ch720n ? Evidence::OurCh720nHardware : Evidence::OurXm5Hardware));
            CHECK(capability.testedFirmware == (model == Model::Ch720n ? "1.1.4" : "2.5.1"));
            if (capability.feature == Feature::NoiseControl || capability.feature == Feature::Equalizer)
                CHECK(capability.writeConfidence == Confidence::Unknown);
        }
    }
    CHECK(profileFor(Model::Xm5).supportedCodecs.size() == 3);
    CHECK(profileFor(Model::Ch720n).supportedCodecs.size() == 2);
}

TEST_CASE("Paired disconnected headphones remain available for manual connect", "[discovery]") {
    FakeDeviceDiscovery discovery;
    auto xm5 = candidate(Model::Xm5, "00:00:00:00:00:01", true, false);
    auto ch = candidate(Model::Ch720n, "00:00:00:00:00:02", true, true);
    discovery.setDevices({xm5.device, ch.device, xm5.device,
        {.name = "WH-1000XM4", .address = "00:00:00:00:00:03", .paired = true, .connected = true},
        {.name = "WH-1000XM5", .address = "00:00:00:00:00:04", .paired = std::nullopt, .connected = true},
        {.name = "WH-CH720N", .address = "00:00:00:00:00:05", .paired = false, .connected = true}});
    const auto devices = supportedPairedDevices(discovery);
    REQUIRE(devices.size() == 2);
    CHECK(devices[0].device.connected == false);
    auto automatic = startupTarget(devices);
    REQUIRE(automatic.has_value());
    CHECK(automatic->model == Model::Ch720n);
}

TEST_CASE("Launch selection idles when active state is unknown or false", "[discovery]") {
    CHECK_FALSE(startupTarget({}).has_value());
    const auto unknown = candidate(Model::Xm5, "00:00:00:00:00:01", true, std::nullopt);
    const auto off = candidate(Model::Ch720n, "00:00:00:00:00:02", true, false);
    CHECK_FALSE(startupTarget({unknown, off}).has_value());
    auto xm5 = unknown; xm5.device.connected = true;
    auto ch = off; ch.device.connected = true;
    CHECK(startupTarget({ch, xm5})->model == Model::Xm5);
    CHECK(startupTarget({xm5, ch}, ch.device.address.str())->model == Model::Ch720n);
}

TEST_CASE("Native Windows search policy reads cached paired devices without inquiry", "[windows][offline]") {
    const auto params = WindowsBluetoothConnector::pairedDeviceSearchParams(nullptr);
    CHECK(params.dwSize == sizeof(params));
    CHECK(params.fReturnAuthenticated == TRUE);
    CHECK(params.fReturnRemembered == TRUE);
    CHECK(params.fReturnConnected == TRUE);
    CHECK(params.fReturnUnknown == FALSE);
    CHECK(params.fIssueInquiry == FALSE);
}

TEST_CASE("False and zero telemetry are known values; missing values are Unknown", "[state]") {
    Telemetry<bool> charging;
    CHECK(charging.availability == Availability::Unknown);
    CHECK_FALSE(charging.value.has_value());
    charging.known(false);
    CHECK(charging.availability == Availability::Known);
    REQUIRE(charging.value.has_value());
    CHECK(*charging.value == false);
    charging.unknown("No response");
    CHECK_FALSE(charging.value.has_value());
    CHECK_FALSE(charging.observedAt.has_value());
    Telemetry<int> battery;
    battery.known(0);
    REQUIRE(battery.value.has_value());
    CHECK(*battery.value == 0);
}

TEST_CASE("Simulation uses the shared binary protocol core on both profiles", "[core][offline]") {
    for (auto model : {Model::Xm5, Model::Ch720n}) {
        DeviceSession session(makeSimulatedTransport(model));
        CHECK(session.state().connection == ConnectionState::Disconnected);
        session.connect(simulatedCandidate(model));
        CHECK_FALSE(session.state().battery.value.has_value());
        session.refresh();
        const auto snapshot = session.state();
        CHECK(snapshot.connection == ConnectionState::Connected);
        CHECK(snapshot.model == model);
        CHECK(snapshot.battery.value == 80);
        CHECK(snapshot.charging.value == false);
        CHECK(snapshot.firmware.value == (model == Model::Xm5 ? "2.5.1" : "1.1.4"));
        CHECK(snapshot.activeCodec.value == "AAC");
        REQUIRE(snapshot.equalizer.value.has_value());
        CHECK(snapshot.equalizer.value->bands.size() == 5);
        session.disconnect();
        CHECK_FALSE(session.state().firmware.value.has_value());
        CHECK_FALSE(session.state().model.has_value());
        session.connect(simulatedCandidate(model));
        session.refresh();
        CHECK(session.state().battery.value == 80);
        // Successful synthetic reads never change capability confidence.
        if (model == Model::Ch720n) CHECK(profileFor(model).capabilities[0].evidence == Evidence::OurCh720nHardware);
    }
}

TEST_CASE("Unknown or unpaired candidates are rejected before opening the transport", "[core][safety]") {
    auto raw = std::make_unique<FakeTransport>();
    auto* inspect = raw.get();
    DeviceSession session(std::make_unique<ReadOnlyTransport>(std::move(raw), [] { return true; }));
    auto target = simulatedCandidate(Model::Xm5);
    SECTION("Renamed device") { target.device.name = "Other headphone"; }
    SECTION("Unpaired") { target.device.paired = false; }
    SECTION("Mismatched profile") { target.model = Model::Ch720n; }
    REQUIRE_THROWS_AS(session.connect(target), sony::SonyException);
    CHECK_FALSE(inspect->isConnected());
    CHECK(inspect->sentCount() == 0);
    CHECK(inspect->connectedAddress().str().empty());
    CHECK(session.state().connection == ConnectionState::Error);
}

TEST_CASE("Slow refresh sends only battery GET and preserves other observations", "[core][offline]") {
    std::vector<std::vector<std::uint8_t>> requests;
    DeviceSession session(makeSimulatedTransport(Model::Xm5,
        [&](std::string_view direction, std::span<const std::byte> bytes) {
            if (direction != "TX") return;
            auto frame = FrameCodec::decode({reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()});
            if (frame.type == DataType::DataMdr) requests.push_back(frame.payload);
        }));
    session.connect(simulatedCandidate(Model::Xm5));
    session.refresh();
    const auto before = session.state();
    requests.clear();
    session.refreshBattery();
    REQUIRE(requests.size() == 1);
    CHECK(requests.front() == std::vector<std::uint8_t>{0x22, 0x00});
    CHECK(session.state().firmware.observedAt == before.firmware.observedAt);
    CHECK(session.state().noise.observedAt == before.noise.observedAt);
    CHECK(session.state().equalizer.observedAt == before.equalizer.observedAt);
    CHECK(session.state().battery.value == 80);
}

TEST_CASE("A lost connection clears previously known telemetry", "[core][state]") {
    auto raw = std::make_unique<sony::test::ReplyingFakeTransport>();
    auto* inspect = raw.get();
    inspect->queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x01, 0x00}}});
    inspect->queueReply({{.type = DataType::DataMdr, .sequence = 1, .payload = {0x23, 0x00, 80, 0}}});
    inspect->queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x05, 0x02, 0, '2', '.', '5', '.', '1'}}});
    inspect->queueReply({{.type = DataType::DataMdr, .sequence = 1, .payload = {0x13, 0x02, 0x02}}});
    inspect->queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x67, 0x17, 1, 1, 0, 0, 1}}});
    inspect->queueReply({{.type = DataType::DataMdr, .sequence = 1, .payload = {0x57, 0x00, 0xa0, 6, 10, 10, 10, 10, 10, 10}}});
    DeviceSession session(std::make_unique<ReadOnlyTransport>(std::move(raw), [] { return true; }));
    session.connect(simulatedCandidate(Model::Xm5));
    session.refresh();
    REQUIRE(session.state().battery.value == 80);
    REQUIRE(session.state().firmware.value == "2.5.1");
    inspect->simulateDisconnect();
    session.refresh();
    CHECK(session.state().connection == ConnectionState::Unavailable);
    CHECK_FALSE(session.state().battery.value.has_value());
    CHECK_FALSE(session.state().charging.value.has_value());
    session.disconnect();
    CHECK(session.state().connection == ConnectionState::Disconnected);
}
