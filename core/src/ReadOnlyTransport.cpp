#include "headsetdesk/ReadOnlyTransport.h"
#include "sony/protocol/FrameCodec.h"
#include "sony/transport/BluetoothConnectorTransport.h"
#include "sony/transport/SonyError.h"
#include "WindowsBluetoothConnector.h"
#include <array>
#include <algorithm>

namespace headsetdesk {
using sony::SonyErrorCode;
using sony::SonyException;

ReadOnlyTransport::ReadOnlyTransport(std::unique_ptr<sony::transport::ITransport> inner,
                                    ServiceCheck isV2Service, PacketSink packets)
    : inner_(std::move(inner)), isV2Service_(std::move(isV2Service)), packets_(std::move(packets)) {
    if (!inner_ || !isV2Service_) throw std::invalid_argument("Transport and service check are required");
}

ReadOnlyTransport::~ReadOnlyTransport() { disconnect(); }

void ReadOnlyTransport::connect(const sony::transport::DeviceAddress& address) {
    disconnect();
    try {
        inner_->connect(address);
        if (!isV2Service_()) {
            throw SonyException(SonyErrorCode::ProtocolViolation,
                "The Sony V2 control service was not confirmed. No headphone queries were sent.");
        }
        serviceVerified_ = true;
    } catch (...) {
        inner_->disconnect();
        throw;
    }
}

void ReadOnlyTransport::disconnect() noexcept {
    serviceVerified_ = false;
    inner_->disconnect();
}

bool ReadOnlyTransport::isConnected() const noexcept {
    return serviceVerified_ && inner_->isConnected();
}

std::size_t ReadOnlyTransport::send(std::span<const std::byte> bytes) {
    if (!isConnected()) throw SonyException(SonyErrorCode::Disconnected);
    auto data = std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
    const auto frame = sony::protocol::FrameCodec::decode(data);
    if (frame.sequence > 1) throw SonyException(SonyErrorCode::ProtocolViolation, "Invalid sequence bit");
    bool permitted = frame.type == sony::protocol::DataType::Ack && frame.payload.empty();
    // These exact GET payloads come from ProtocolV2.cpp. No arbitrary raw
    // commands, setters, earbud probes, power controls, or firmware updates.
    constexpr std::array<std::array<std::uint8_t, 2>, 6> queries{{
        {{0x00, 0x00}}, {{0x22, 0x00}}, {{0x04, 0x02}},
        {{0x12, 0x02}}, {{0x66, 0x17}}, {{0x56, 0x00}}
    }};
    if (frame.type == sony::protocol::DataType::DataMdr && frame.payload.size() == 2) {
        permitted = std::any_of(queries.begin(), queries.end(), [&](const auto& query) {
            return std::equal(query.begin(), query.end(), frame.payload.begin());
        });
    }
    if (!permitted) throw SonyException(SonyErrorCode::Unsupported,
        "This diagnostic build permits only the reviewed V2 read queries and protocol ACKs.");
    if (packets_) packets_("TX", bytes);
    std::size_t written = 0;
    while (written < bytes.size()) {
        const auto count = inner_->send(bytes.subspan(written));
        if (count == 0 || count > bytes.size() - written)
            throw SonyException(SonyErrorCode::TransportFailure, "Invalid transport send length");
        written += count;
    }
    return written;
}

std::size_t ReadOnlyTransport::receive(std::span<std::byte> buffer) {
    if (!isConnected()) throw SonyException(SonyErrorCode::Disconnected);
    const auto count = inner_->receive(buffer);
    if (count > buffer.size()) throw SonyException(SonyErrorCode::TransportFailure, "Invalid receive length");
    if (packets_ && count > 0) packets_("RX", buffer.first(count));
    return count;
}

std::unique_ptr<ReadOnlyTransport> makeWindowsReadOnlyTransport(PacketSink packets) {
    auto connector = std::make_unique<WindowsBluetoothConnector>(SonyProtocolVersion::V2);
    auto* native = connector.get();
    auto transport = std::make_unique<sony::transport::BluetoothConnectorTransport>(std::move(connector));
    return std::make_unique<ReadOnlyTransport>(std::move(transport),
        [native] { return native->getProtocolVersion() == SonyProtocolVersion::V2; }, std::move(packets));
}
} // namespace headsetdesk
