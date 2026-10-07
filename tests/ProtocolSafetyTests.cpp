#include <catch2/catch_test_macros.hpp>
#include "headsetdesk/ReadOnlyTransport.h"
#include "sony/protocol/ProtocolV1.h"
#include "sony/protocol/ProtocolV2.h"
#include "sony/transport/FakeTransport.h"
#include "ReplyingFakeTransport.h"
#include <condition_variable>
#include <mutex>

using namespace sony;
using namespace sony::protocol;
using namespace sony::transport;
using namespace headsetdesk;

namespace {
std::size_t send(ReadOnlyTransport& transport, SonyFrame frame) {
    const auto bytes = FrameCodec::encode(frame);
    return transport.send({reinterpret_cast<const std::byte*>(bytes.data()), bytes.size()});
}
}

TEST_CASE("A V1 service cannot receive a V2 battery opcode", "[safety][generation]") {
    auto fake = std::make_unique<FakeTransport>();
    auto* inspect = fake.get();
    ReadOnlyTransport guard(std::move(fake), [] { return false; });
    REQUIRE_THROWS_AS(guard.connect("00:00:00:00:00:01"), SonyException);
    CHECK_FALSE(inspect->isConnected());
    CHECK(inspect->sentCount() == 0);
    CHECK_FALSE(guard.isConnected());
    REQUIRE_THROWS_AS(send(guard, {.type = DataType::DataMdr, .payload = {0x22, 0x00}}), SonyException);
    CHECK(inspect->sentCount() == 0);
}

TEST_CASE("The outbound guard rejects every setter and nonallowlisted query", "[safety][writes]") {
    auto fake = std::make_unique<FakeTransport>();
    auto* inspect = fake.get();
    ReadOnlyTransport guard(std::move(fake), [] { return true; });
    guard.connect("00:00:00:00:00:01");
    for (auto payload : std::vector<std::vector<uint8_t>>{
        {0x68, 0x17, 1, 1, 0, 0, 1}, {0x58, 0, 0xa0, 6, 10, 10, 10, 10, 10, 10},
        {0xe8, 1, 1}, {0x28, 5, 0x11, 0}, {0x22, 0x09}, {0x22, 0x0a}, {0x22},
        {0x22, 0, 0}, {0xff, 0}, {0x10, 0}, {0x66, 0x02}, {0x56, 0x04}}) {
        CHECK_THROWS_AS(send(guard, {.type = DataType::DataMdr, .payload = payload}), SonyException);
    }
    CHECK_THROWS_AS(send(guard, {.type = DataType::Data, .payload = {0x22, 0}}), SonyException);
    CHECK_THROWS_AS(send(guard, {.type = DataType::Ack, .payload = {0x22, 0}}), SonyException);
    CHECK_THROWS_AS(send(guard, {.type = DataType::DataMdr, .sequence = 2, .payload = {0x22, 0}}), SonyException);
    CHECK(inspect->sentCount() == 0);
}

TEST_CASE("Only exact reviewed GETs and ACKs pass the guard", "[safety][reads]") {
    auto fake = std::make_unique<FakeTransport>();
    auto* inspect = fake.get();
    ReadOnlyTransport guard(std::move(fake), [] { return true; });
    guard.connect("00:00:00:00:00:01");
    for (auto payload : std::vector<std::vector<uint8_t>>{{0, 0}, {0x22, 0}, {4, 2}, {0x12, 2}, {0x66, 0x17}, {0x56, 0}})
        CHECK(send(guard, {.type = DataType::DataMdr, .payload = payload}) > 0);
    CHECK(send(guard, {.type = DataType::Ack, .sequence = 1}) > 0);
    CHECK(inspect->sentCount() == 7);
}

TEST_CASE("Upstream setters are blocked even if called beneath the app core", "[safety][writes]") {
    auto fake = std::make_unique<FakeTransport>();
    auto* inspect = fake.get();
    ReadOnlyTransport guard(std::move(fake), [] { return true; });
    SonyProtocolSession session(&guard);
    session.connect("00:00:00:00:00:01");
    ProtocolV2 protocol(session, false, true);
    CHECK_THROWS_AS(protocol.setNoiseControl({.mode = NoiseControlMode::NoiseCancelling}), SonyException);
    CHECK_THROWS_AS(protocol.setEqualizerCustom(0, {0, 0, 0, 0, 0}), SonyException);
    CHECK_THROWS_AS(protocol.setDsee(true), SonyException);
    CHECK(inspect->sentCount() == 0);
}

TEST_CASE("Over-ear battery diagnostics never probe earbud subtypes", "[safety][battery]") {
    sony::test::ReplyingFakeTransport fake;
    SonyProtocolSession session(&fake);
    session.connect("00:00:00:00:00:01");
    ProtocolV2 protocol(session, false, true);
    SECTION("Short response") {
        fake.queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x23, 0x00, 80}}});
        CHECK_THROWS_AS(protocol.getBattery(), SonyException);
    }
    SECTION("Unknown charging byte") {
        fake.queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x23, 0x00, 80, 9}}});
        const auto battery = protocol.getBattery();
        CHECK(battery.main == 80);
        CHECK_FALSE(battery.charging.has_value());
    }
    SECTION("Invalid percent") {
        fake.queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x23, 0x00, 255, 0}}});
        const auto battery = protocol.getBattery();
        CHECK_FALSE(battery.main.has_value());
        CHECK(battery.charging == false);
    }
    SECTION("Timeout") { CHECK_THROWS_AS(protocol.getBattery(), SonyException); }
    std::size_t requests = 0;
    for (const auto& bytes : fake.sentFrames()) {
        const auto frame = FrameCodec::decode(bytes);
        if (frame.type != DataType::DataMdr) continue;
        CHECK(frame.payload == std::vector<uint8_t>{0x22, 0});
        ++requests;
    }
    CHECK(requests == 1);
}

TEST_CASE("Strict decoder rejects bytes beyond the declared payload", "[safety][frame]") {
    auto wire = FrameCodec::encode({.type = DataType::DataMdr, .payload = {0x22, 0}});
    wire.insert(wire.end() - 1, 0x11);
    CHECK_THROWS_AS(FrameCodec::decode(wire), SonyException);
}

TEST_CASE("Late cached responses cannot supply a future request", "[safety][freshness]") {
    FakeTransport fake;
    SonyProtocolSession session(&fake);
    session.connect("00:00:00:00:00:01");
    std::mutex mutex;
    std::condition_variable ready;
    bool dispatched = false;
    session.onNotification([&](const SonyFrame&) { std::lock_guard lock(mutex); dispatched = true; ready.notify_all(); });
    fake.queueIncoming(FrameCodec::encode({.type = DataType::DataMdr, .sequence = 0, .payload = {0x23, 0, 90, 0}}));
    {
        std::unique_lock lock(mutex);
        REQUIRE(ready.wait_for(lock, std::chrono::seconds(1), [&] { return dispatched; }));
    }
    try {
        session.sendAndAwaitResponse({.type = DataType::DataMdr, .payload = {0x22, 0}}, 0x23, 0, std::chrono::milliseconds(30));
        FAIL("An unsolicited old value must not satisfy a new query");
    } catch (const SonyException& error) { CHECK(error.code() == SonyErrorCode::Timeout); }
}

TEST_CASE("Truncated stream chunks are assembled before a response is accepted", "[safety][stream]") {
    sony::test::ReplyingFakeTransport fake;
    fake.setMaxReceiveChunkSize(1);
    SonyProtocolSession session(&fake);
    session.connect("00:00:00:00:00:01");
    fake.queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x23, 0, 0x3e, 0}}});
    ProtocolV2 protocol(session, false, true);
    CHECK(protocol.getBattery().main == 62);
}

TEST_CASE("ACKs alone cannot complete a read request", "[safety][ack]") {
    sony::test::ReplyingFakeTransport fake;
    SonyProtocolSession session(&fake);
    session.connect("00:00:00:00:00:01");
    fake.queueReply({{.type = DataType::Ack, .sequence = 1}});
    try {
        session.sendAndAwaitResponse({.type = DataType::DataMdr, .payload = {0x22, 0}}, 0x23, 0, std::chrono::milliseconds(30));
        FAIL("An ACK is not battery telemetry");
    } catch (const SonyException& error) { CHECK(error.code() == SonyErrorCode::Timeout); }
}

TEST_CASE("Firmware responses must match the queried subtype", "[safety][matching]") {
    sony::test::ReplyingFakeTransport fake;
    SonyProtocolSession session(&fake);
    session.connect("00:00:00:00:00:01");
    fake.queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x05, 0x03, 0, '2', '.', '5', '.', '1'}}});
    ProtocolV2 protocol(session);
    CHECK_THROWS_AS(protocol.getFirmwareVersion(), SonyException);
}

TEST_CASE("Unknown codec codes are never replaced by a supported codec", "[safety][codec]") {
    sony::test::ReplyingFakeTransport fake;
    SonyProtocolSession session(&fake);
    session.connect("00:00:00:00:00:01");
    fake.queueReply({{.type = DataType::DataMdr, .sequence = 0, .payload = {0x13, 0x02, 0xff}}});
    ProtocolV2 protocol(session);
    CHECK(protocol.getCodec().empty());
}

TEST_CASE("Partial native sends cannot bypass complete-frame validation", "[safety][transport]") {
    class PartialTransport final : public FakeTransport {
    public:
        std::vector<std::byte> sent;
        std::size_t send(std::span<const std::byte> bytes) override {
            const auto count = std::min<std::size_t>(3, bytes.size());
            sent.insert(sent.end(), bytes.begin(), bytes.begin() + count);
            return count;
        }
    };
    auto partial = std::make_unique<PartialTransport>();
    auto* inspect = partial.get();
    ReadOnlyTransport guard(std::move(partial), [] { return true; });
    guard.connect("00:00:00:00:00:01");
    const auto expected = FrameCodec::encode({.type = DataType::DataMdr, .payload = {0x22, 0}});
    CHECK(send(guard, {.type = DataType::DataMdr, .payload = {0x22, 0}}) == expected.size());
    REQUIRE(inspect->sent.size() == expected.size());
    CHECK(std::equal(expected.begin(), expected.end(), inspect->sent.begin(),
        [](auto a, auto b) { return a == std::to_integer<uint8_t>(b); }));
}
