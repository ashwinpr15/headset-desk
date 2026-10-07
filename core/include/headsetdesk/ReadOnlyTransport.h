#pragma once

#include "sony/transport/ITransport.h"
#include <atomic>
#include <functional>
#include <memory>
#include <span>
#include <string_view>

namespace headsetdesk {
using PacketSink = std::function<void(std::string_view direction, std::span<const std::byte> bytes)>;

// The final outbound boundary. Upstream setters exist in the imported library
// but cannot cross this transport. Querying battery requires a V2 service.
class ReadOnlyTransport final : public sony::transport::ITransport {
public:
    using ServiceCheck = std::function<bool()>;
    ReadOnlyTransport(std::unique_ptr<sony::transport::ITransport> inner,
                      ServiceCheck isV2Service, PacketSink packets = {});
    ~ReadOnlyTransport() override;
    void connect(const sony::transport::DeviceAddress& address) override;
    void disconnect() noexcept override;
    bool isConnected() const noexcept override;
    std::size_t send(std::span<const std::byte> data) override;
    std::size_t receive(std::span<std::byte> buffer) override;

private:
    std::unique_ptr<sony::transport::ITransport> inner_;
    ServiceCheck isV2Service_;
    PacketSink packets_;
    std::atomic<bool> serviceVerified_{false};
};

std::unique_ptr<ReadOnlyTransport> makeWindowsReadOnlyTransport(PacketSink packets = {});
} // namespace headsetdesk
