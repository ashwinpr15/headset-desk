#pragma once

#include "sony/transport/ITransport.h"
#include <atomic>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <mutex>
#include <optional>
#include <vector>

namespace headsetdesk {
using PacketSink = std::function<void(std::string_view direction, std::span<const std::byte> bytes)>;

// Default read-only outbound boundary. Only DeviceSession can grant one exact
// setting payload. Querying battery requires a confirmed V2 service.
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
    friend class DeviceSession;
    // The core may grant one exact, validated setting frame for an opted-in
    // session. Diagnostics and imported setters cannot grant themselves access.
    void permitSetting(std::vector<std::uint8_t> payload);
    void revokeSetting() noexcept;
    // Speak-to-Chat and DSEE are read with two extra exact GETs, only after the user has opted in.
    void allowExtraQueries(bool allowed) noexcept { extraQueries_ = allowed; }
    std::atomic<bool> extraQueries_{false};
    std::mutex permitMutex_;
    std::optional<std::vector<std::uint8_t>> settingPermit_;
    std::unique_ptr<sony::transport::ITransport> inner_;
    ServiceCheck isV2Service_;
    PacketSink packets_;
    std::atomic<bool> serviceVerified_{false};
};

std::unique_ptr<ReadOnlyTransport> makeWindowsReadOnlyTransport(PacketSink packets = {});
} // namespace headsetdesk
