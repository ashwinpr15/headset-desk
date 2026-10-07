#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "headsetdesk/ReadOnlyTransport.h"
#include "sony/protocol/ProtocolV2.h"
#include "sony/protocol/EqualizerPresets.h"
#include "sony/transport/FakeTransport.h"
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

using namespace sony::protocol;
using namespace sony::transport;
using namespace headsetdesk;

namespace {
struct Exchange { SonyFrame request; std::vector<uint8_t> received; };
struct Capture { std::vector<Exchange> exchanges; std::size_t records{0}; };

Capture loadCapture(const char* filename) {
    std::ifstream input(std::filesystem::path(HEADSET_DESK_FIXTURE_DIR) / filename);
    REQUIRE(input.good());
    Capture capture;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream record(line);
        std::string timestamp, direction, length;
        record >> timestamp >> direction >> length;
        if (direction != "TX" && direction != "RX") continue;
        std::vector<uint8_t> wire;
        std::string hex;
        while (record >> hex) wire.push_back(static_cast<uint8_t>(std::stoul(hex, nullptr, 16)));
        REQUIRE(wire.size() == std::stoul(length));
        // Every chunk in these two reviewed captures contains exactly one frame.
        // Decode the original wire bytes so checksums and declared lengths are tested.
        const auto frame = FrameCodec::decode(wire);
        ++capture.records;
        if (direction == "TX") {
            if (frame.type == DataType::DataMdr) capture.exchanges.push_back({frame, {}});
            else { REQUIRE(frame.type == DataType::Ack); REQUIRE(frame.payload.empty()); }
        } else {
            REQUIRE_FALSE(capture.exchanges.empty());
            auto& received = capture.exchanges.back().received;
            received.insert(received.end(), wire.begin(), wire.end());
        }
    }
    return capture;
}

class CaptureTransport final : public FakeTransport {
public:
    explicit CaptureTransport(Capture capture) : capture_(std::move(capture)) {}
    std::size_t send(std::span<const std::byte> bytes) override {
        const auto frame = FrameCodec::decode({reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size()});
        const auto count = FakeTransport::send(bytes);
        if (frame.type == DataType::DataMdr) {
            if (next_ >= capture_.exchanges.size()) throw std::runtime_error("Unexpected replay query");
            const auto& exchange = capture_.exchanges[next_++];
            if (frame.payload != exchange.request.payload || frame.sequence != exchange.request.sequence)
                throw std::runtime_error("Replay query differs from hardware capture");
            queueIncoming(exchange.received);
        }
        return count;
    }
    std::size_t queries() const { return next_; }
private:
    Capture capture_;
    std::size_t next_{0};
};
}

TEST_CASE("Reviewed physical captures replay through the guarded V2 read path", "[hardware-fixture][offline]") {
    const bool xm5 = GENERATE(true, false);
    const auto capture = loadCapture(xm5 ? "xm5-readonly.txt" : "ch720n-readonly.txt");
    REQUIRE(capture.records == (xm5 ? 28 : 24));
    REQUIRE(capture.exchanges.size() == 6);
    auto raw = std::make_unique<CaptureTransport>(capture);
    auto* inspect = raw.get();
    ReadOnlyTransport guard(std::move(raw), [] { return true; });
    SonyProtocolSession session(&guard);
    std::mutex mutex;
    std::condition_variable received;
    std::vector<SonyFrame> notifications;
    session.onNotification([&](const SonyFrame& frame) {
        std::lock_guard lock(mutex);
        notifications.push_back(frame);
        received.notify_all();
    });
    session.connect("00:00:00:00:00:01"); // Synthetic address; no native transport.
    ProtocolV2 protocol(session, false, true);
    REQUIRE_NOTHROW(protocol.initDevice());
    const auto battery = protocol.getBattery();
    CHECK(battery.main == (xm5 ? 44 : 99));
    CHECK(battery.charging == false);
    CHECK(protocol.getFirmwareVersion() == (xm5 ? "2.5.1" : "1.1.4"));
    CHECK(protocol.getCodec() == "AAC");
    CHECK(protocol.getNoiseControl().mode == NoiseControlMode::Off);
    const auto eq = protocol.getEqualizer();
    CHECK(eq.clearBass == (xm5 ? 3 : -1));
    CHECK(eq.bands == (xm5 ? std::vector<int>{-1, 0, 2, 2, 4} : std::vector<int>{0, 5, 7, 7, 9}));
    CHECK(eq.preset == (xm5 ? 0xa1 : 0x10));
    CHECK(equalizerPresetId(eq.preset) == (xm5 ? "" : "bright"));
    if (xm5) {
        std::unique_lock lock(mutex);
        REQUIRE(received.wait_for(lock, std::chrono::seconds(2), [&] { return notifications.size() == 2; }));
        for (const auto& frame : notifications) CHECK(frame.payload.front() == 0xc9);
    }
    session.disconnect();
    CHECK(inspect->queries() == 6);
    std::size_t acks = 0;
    for (const auto& bytes : inspect->sentFrames()) {
        const auto frame = FrameCodec::decode(bytes);
        if (frame.type == DataType::Ack) { CHECK(frame.payload.empty()); ++acks; }
    }
    CHECK(acks == (xm5 ? 8 : 6));
}
