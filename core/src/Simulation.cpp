#include "headsetdesk/Simulation.h"
#include "sony/protocol/FrameCodec.h"
#include "sony/transport/FakeTransport.h"

namespace headsetdesk {
namespace {
class SimulatedHeadset final : public sony::transport::FakeTransport {
public:
    explicit SimulatedHeadset(Model model, SimulationFailure failure) : model_(model), failure_(failure) {}
    std::size_t send(std::span<const std::byte> bytes) override {
        const auto written = FakeTransport::send(bytes);
        const auto frame = sony::protocol::FrameCodec::decode({reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size()});
        if (frame.type != sony::protocol::DataType::DataMdr) return written;
        const auto& request = frame.payload;
        const bool setting = !request.empty() && (request[0] == 0x68 || request[0] == 0x58);
        if (setting && failure_ == SimulationFailure::DisconnectOnSetting) { disconnect(); return written; }
        if (setting && failure_ != SimulationFailure::IgnoreSettings) {
            if (request[0] == 0x68) { noise_ = request; noise_[0] = 0x67; }
            else { eq_ = request; eq_[0] = 0x57; }
        }
        if (setting && failure_ == SimulationFailure::NoSettingAck) return written;
        std::vector<std::uint8_t> reply;
        if (request == std::vector<std::uint8_t>{0x00, 0x00}) reply = {0x01, 0x00};
        else if (request == std::vector<std::uint8_t>{0x22, 0x00}) reply = {0x23, 0x00, 80, 0};
        else if (request == std::vector<std::uint8_t>{0x04, 0x02}) {
            reply = {0x05, 0x02, 0x00};
            const std::string version = model_ == Model::Xm5 ? "2.5.1" : "1.1.4";
            reply.insert(reply.end(), version.begin(), version.end());
        } else if (request == std::vector<std::uint8_t>{0x12, 0x02}) reply = {0x13, 0x02, 0x02};
        else if (request == std::vector<std::uint8_t>{0x66, 0x17}) reply = noise_;
        else if (request == std::vector<std::uint8_t>{0x56, 0x00}) reply = eq_;
        queueIncoming(sony::protocol::FrameCodec::encode({.type = sony::protocol::DataType::Ack,
            .sequence = static_cast<std::uint8_t>(1 - (frame.sequence & 1))}));
        if (!reply.empty()) {
            queueIncoming(sony::protocol::FrameCodec::encode({.type = sony::protocol::DataType::DataMdr,
                .sequence = responseSequence_, .payload = std::move(reply)}));
            responseSequence_ ^= 1;
        }
        return written;
    }
private:
    Model model_;
    SimulationFailure failure_;
    std::vector<std::uint8_t> noise_{0x67, 0x17, 1, 1, 0, 0, 1};
    std::vector<std::uint8_t> eq_{0x57, 0, 0xa0, 6, 10, 10, 10, 10, 10, 10};
    std::uint8_t responseSequence_{0};
};
}

Candidate simulatedCandidate(Model model) {
    // Synthetic address used only in memory. It is never logged or passed to Windows.
    return {model, {.name = std::string(profileFor(model).name),
        .address = model == Model::Xm5 ? "00:00:00:00:00:01" : "00:00:00:00:00:02", .paired = true, .connected = true}};
}
std::unique_ptr<ReadOnlyTransport> makeSimulatedTransport(Model model, PacketSink packets, SimulationFailure failure) {
    return std::make_unique<ReadOnlyTransport>(std::make_unique<SimulatedHeadset>(model, failure), [] { return true; }, std::move(packets));
}
} // namespace headsetdesk
