#pragma once

#include "DeviceDiscovery.h"
#include "ReadOnlyTransport.h"
#include "sony/protocol/ProtocolV2.h"
#include <chrono>
#include <mutex>
#include <optional>
#include <string>

namespace headsetdesk {
enum class Availability { Known, Unknown, Unsupported };
enum class ConnectionState { Disconnected, Connecting, Connected, Unavailable, Error };

template<class T> struct Telemetry {
    Availability availability{Availability::Unknown};
    std::optional<T> value;
    std::string error;
    std::optional<std::chrono::system_clock::time_point> observedAt;
    void known(T next) {
        value = std::move(next);
        availability = Availability::Known;
        error.clear();
        observedAt = std::chrono::system_clock::now();
    }
    void unknown(std::string reason = {}) {
        availability = Availability::Unknown;
        value.reset();
        observedAt.reset();
        error = std::move(reason);
    }
};

struct Snapshot {
    bool controlsEnabled{false}; // Explicit experimental opt-in, cleared on disconnect/loss.
    ConnectionState connection{ConnectionState::Disconnected};
    std::optional<Model> model; // Paired Windows name, not a live Sony identity response.
    Telemetry<int> battery;
    Telemetry<bool> charging;
    Telemetry<std::string> firmware;
    Telemetry<std::string> activeCodec;
    Telemetry<sony::protocol::NoiseControlState> noise;
    Telemetry<sony::protocol::EqualizerState> equalizer;
    std::string connectionError;
};

// Synchronous core shared by the GUI worker and internal CLI. Calls are
// serialized; state() returns a copy. Settings require session opt-in and
// exact validated payloads. No public raw packet API.
class DeviceSession {
public:
    explicit DeviceSession(std::unique_ptr<ReadOnlyTransport> transport);
    ~DeviceSession();
    DeviceSession(const DeviceSession&) = delete;
    DeviceSession& operator=(const DeviceSession&) = delete;
    void connect(const Candidate& candidate);
    void disconnect();
    void refresh();
    void refreshBattery();
    void enableControls(bool enabled);
    void setNoise(sony::protocol::NoiseControlMode mode, int ambientLevel = -1);
    void setEqualizer(int clearBass, const std::vector<int>& bands);
    Snapshot state() const;

private:
    void clearTelemetry();
    void resetLink();
    Snapshot requireControls() const;
    void commandFailed();
    mutable std::mutex operationMutex_;
    mutable std::mutex stateMutex_;
    std::unique_ptr<ReadOnlyTransport> transport_;
    std::unique_ptr<sony::protocol::SonyProtocolSession> session_;
    std::unique_ptr<sony::protocol::ProtocolV2> protocol_;
    Snapshot state_;
};
} // namespace headsetdesk
