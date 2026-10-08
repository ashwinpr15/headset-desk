#include "headsetdesk/DeviceSession.h"
#include "sony/transport/SonyError.h"
#include <algorithm>
#include <thread>

namespace headsetdesk {
namespace {
std::string describe(sony::protocol::NoiseControlMode mode) {
    using sony::protocol::NoiseControlMode;
    return mode == NoiseControlMode::NoiseCancelling ? "Noise Cancelling" : mode == NoiseControlMode::Ambient ? "Ambient" : "Off";
}
}
DeviceSession::DeviceSession(std::unique_ptr<ReadOnlyTransport> transport) : transport_(std::move(transport)) {
    if (!transport_) throw std::invalid_argument("Read-only transport required");
}
DeviceSession::~DeviceSession() { disconnect(); }

void DeviceSession::clearTelemetry() {
    state_.controlsEnabled = false;
    state_.lastChange.clear();
    state_.battery.unknown(); state_.charging.unknown(); state_.firmware.unknown();
    state_.activeCodec.unknown(); state_.noise.unknown(); state_.equalizer.unknown();
    state_.speakToChat.unknown(); state_.dsee.unknown();
}

void DeviceSession::resetLink() {
    protocol_.reset();
    session_.reset(); // Stops the reader before native transport destruction.
    transport_->disconnect();
}

void DeviceSession::connect(const Candidate& candidate) {
    std::lock_guard operation(operationMutex_);
    resetLink();
    {
        std::lock_guard state(stateMutex_);
        state_ = {};
        state_.connection = ConnectionState::Connecting;
    }
    try {
        auto model = identifyModel(candidate.device.name);
        if (!model || *model != candidate.model || candidate.device.paired != true)
            throw sony::SonyException(sony::SonyErrorCode::Unsupported, "Select a paired WH-1000XM5 or WH-CH720N.");
        session_ = std::make_unique<sony::protocol::SonyProtocolSession>(transport_.get());
        session_->connect(candidate.device.address);
        ncAmbientSwapped_ = *model == Model::Ch720n;
        protocol_ = std::make_unique<sony::protocol::ProtocolV2>(*session_, false, true, ncAmbientSwapped_);
        protocol_->initDevice(); // Reviewed GET handshake; no feature setters.
        if (!session_->isConnected()) throw sony::SonyException(sony::SonyErrorCode::Disconnected);
        std::lock_guard state(stateMutex_);
        state_.model = *model;
        state_.connection = ConnectionState::Connected;
    } catch (const std::exception& error) {
        resetLink();
        std::lock_guard state(stateMutex_);
        state_.connection = ConnectionState::Error;
        state_.connectionError = error.what();
        throw;
    }
}

void DeviceSession::disconnect() {
    std::lock_guard operation(operationMutex_);
    resetLink();
    std::lock_guard state(stateMutex_);
    clearTelemetry();
    state_.model.reset();
    state_.connection = ConnectionState::Disconnected;
    state_.connectionError.clear();
}

void DeviceSession::refresh() {
    std::lock_guard operation(operationMutex_);
    if (!session_ || !session_->isConnected()) {
        std::lock_guard state(stateMutex_);
        clearTelemetry();
        state_.connection = ConnectionState::Unavailable;
        state_.connectionError = "The headphone control connection is unavailable.";
        return;
    }
    Snapshot next;
    {
        std::lock_guard state(stateMutex_);
        next.model = state_.model;
        next.controlsEnabled = state_.controlsEnabled;
        next.lastChange = state_.lastChange;
    }
    next.connection = ConnectionState::Connected;
    try {
        auto battery = protocol_->getBattery();
        if (battery.main && *battery.main >= 0 && *battery.main <= 100) next.battery.known(*battery.main);
        else next.battery.unknown("Battery percentage was not reported.");
        if (battery.charging.has_value()) next.charging.known(*battery.charging);
        else next.charging.unknown("Charging state was not reported.");
    } catch (const std::exception& error) {
        next.battery.unknown(error.what()); next.charging.unknown(error.what());
    }
    try {
        auto firmware = protocol_->getFirmwareVersion();
        if (firmware.empty() || firmware.size() > 64 || !std::all_of(firmware.begin(), firmware.end(),
            [](unsigned char byte) { return byte >= 32 && byte < 127; }))
            throw sony::SonyException(sony::SonyErrorCode::InvalidResponse, "Firmware version was not reported.");
        next.firmware.known(std::move(firmware));
    } catch (const std::exception& error) { next.firmware.unknown(error.what()); }
    try {
        auto codec = protocol_->getCodec();
        if (codec.empty()) throw sony::SonyException(sony::SonyErrorCode::InvalidResponse, "Active codec was not reported.");
        // Report only the protocol response. A static codec list is never a fallback.
        next.activeCodec.known(std::move(codec));
    } catch (const std::exception& error) { next.activeCodec.unknown(error.what()); }
    try {
        auto noise = protocol_->getNoiseControl();
        if (noise.mode == sony::protocol::NoiseControlMode::Ambient &&
            (noise.ambientLevel < 1 || noise.ambientLevel > 20))
            throw sony::SonyException(sony::SonyErrorCode::InvalidResponse, "Invalid ambient level.");
        next.noise.known(noise);
    } catch (const std::exception& error) { next.noise.unknown(error.what()); }
    try {
        auto eq = protocol_->getEqualizer();
        if (eq.bands.size() != 5 || eq.clearBass < -10 || eq.clearBass > 10 ||
            !std::all_of(eq.bands.begin(), eq.bands.end(), [](int v) { return v >= -10 && v <= 10; }))
            throw sony::SonyException(sony::SonyErrorCode::InvalidResponse, "Invalid five-band equalizer response.");
        next.equalizer.known(std::move(eq));
    } catch (const std::exception& error) { next.equalizer.unknown(error.what()); }
    if (!session_->isConnected()) {
        next.controlsEnabled = false;
        next.connection = ConnectionState::Unavailable;
        next.battery.unknown(); next.charging.unknown(); next.firmware.unknown();
        next.activeCodec.unknown(); next.noise.unknown(); next.equalizer.unknown();
        next.connectionError = "The headphone control connection was lost.";
    }
    if (!next.noise.value || !next.equalizer.value || !next.firmware.value) next.controlsEnabled = false;
    if (next.controlsEnabled && next.connection == ConnectionState::Connected) readExtras(next);
    std::lock_guard state(stateMutex_);
    state_ = std::move(next);
}

void DeviceSession::readExtras(Snapshot& into) {
    // Two extra exact GETs, only while Allow changes is on. A feature the headphones don't
    // answer simply stays unknown and its control stays unavailable.
    if (!session_ || !session_->isConnected() || !into.model) return;
    if (*into.model == Model::Xm5) {
        try { into.speakToChat.known(protocol_->getSpeakToChat()); }
        catch (const std::exception& error) { into.speakToChat.unknown(error.what()); }
    } else into.speakToChat.unknown("Speak-to-Chat isn't part of this model.");
    try { into.dsee.known(protocol_->getDsee()); }
    catch (const std::exception& error) { into.dsee.unknown(error.what()); }
}

void DeviceSession::enableControls(bool enabled) {
    std::lock_guard operation(operationMutex_);
    const auto snapshot = state();
    if (enabled && (!session_ || !session_->isConnected() || !snapshot.model ||
        !snapshot.noise.value || !snapshot.equalizer.value || !snapshot.firmware.value))
        throw sony::SonyException(sony::SonyErrorCode::Unsupported, "Refresh the headphone readings before enabling controls.");
    {
        std::lock_guard state(stateMutex_);
        state_.controlsEnabled = enabled;
        if (!enabled) { state_.speakToChat.unknown(); state_.dsee.unknown(); }
    }
    transport_->allowExtraQueries(enabled);
    if (enabled) {
        Snapshot extras;
        extras.model = snapshot.model;
        readExtras(extras);
        std::lock_guard state(stateMutex_);
        state_.speakToChat = extras.speakToChat; state_.dsee = extras.dsee;
    }
}

Snapshot DeviceSession::requireControls() const {
    auto snapshot = state();
    if (!session_ || !session_->isConnected() || !snapshot.controlsEnabled || !snapshot.model)
        throw sony::SonyException(sony::SonyErrorCode::Unsupported, "Setting controls are disabled for this session.");
    return snapshot;
}

void DeviceSession::commandFailed() {
    transport_->revokeSetting();
    transport_->allowExtraQueries(false);
    std::lock_guard state(stateMutex_);
    state_.controlsEnabled = false;
    state_.speakToChat.unknown(); state_.dsee.unknown();
}

void DeviceSession::setNoise(sony::protocol::NoiseControlMode mode, int ambientLevel, int voice) {
    std::lock_guard operation(operationMutex_);
    const auto snapshot = requireControls();
    using sony::protocol::NoiseControlMode;
    if (!snapshot.noise.value || (mode != NoiseControlMode::Off && mode != NoiseControlMode::NoiseCancelling && mode != NoiseControlMode::Ambient))
        throw sony::SonyException(sony::SonyErrorCode::InvalidResponse, "Invalid noise-control request.");
    auto desired = *snapshot.noise.value;
    desired.mode = mode;
    if (voice < -1 || voice > 1) throw sony::SonyException(sony::SonyErrorCode::InvalidResponse, "Invalid noise-control request.");
    if (voice >= 0) desired.focusOnVoice = voice == 1;
    desired.ambientLevel = ambientLevel == -1 ? std::max(1, desired.ambientLevel) : ambientLevel;
    if (desired.ambientLevel < 1 || desired.ambientLevel > 20)
        throw sony::SonyException(sony::SonyErrorCode::InvalidResponse, "Ambient level must be 1–20.");
    std::vector<std::uint8_t> payload{0x68, 0x17, 1,
        static_cast<std::uint8_t>(mode != NoiseControlMode::Off),
        static_cast<std::uint8_t>((mode == NoiseControlMode::Ambient) != ncAmbientSwapped_),
        static_cast<std::uint8_t>(desired.focusOnVoice), static_cast<std::uint8_t>(desired.ambientLevel)};
    try {
        transport_->permitSetting(std::move(payload));
        protocol_->setNoiseControl(desired); // Waits for ACK; never retries a setting.
        transport_->revokeSetting();
        // The headphones can take a moment to apply a change, so poll for up to ~2.5 s.
        sony::protocol::NoiseControlState actual;
        for (int attempt = 0; attempt < 12; ++attempt) {
            actual = protocol_->getNoiseControl();
            {
                std::lock_guard state(stateMutex_);
                state_.noise.known(actual);
            }
            if (actual.mode == mode) { // Ambient level is a bonus check, not a reason to fail the mode.
                std::lock_guard state(stateMutex_);
                state_.lastChange = "Noise: " + describe(mode) + (mode == NoiseControlMode::Ambient ? " level " + std::to_string(actual.ambientLevel) : "") + " confirmed";
                return;
            }
            if (attempt < 11) std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::lock_guard state(stateMutex_);
        state_.lastChange = "Noise: asked for " + describe(mode) + ", headphones reported " + describe(actual.mode);
        throw SettingNotConfirmed("The headphones reported " + describe(actual.mode) + " instead of " + describe(mode) + ".");
    } catch (const SettingNotConfirmed&) { throw;
    } catch (...) { commandFailed(); throw; }
}

void DeviceSession::setEqualizer(int clearBass, const std::vector<int>& bands) {
    std::lock_guard operation(operationMutex_);
    const auto snapshot = requireControls();
    if (!snapshot.equalizer.value || bands.size() != 5 || clearBass < -10 || clearBass > 10 ||
        !std::all_of(bands.begin(), bands.end(), [](int value) { return value >= -10 && value <= 10; }))
        throw sony::SonyException(sony::SonyErrorCode::InvalidResponse, "EQ needs five bands and Clear Bass, each from -10 to +10.");
    std::vector<std::uint8_t> payload{0x58, 0, 0xa0, 6, static_cast<std::uint8_t>(clearBass + 10)};
    for (int value : bands) payload.push_back(static_cast<std::uint8_t>(value + 10));
    try {
        transport_->permitSetting(std::move(payload));
        protocol_->setEqualizerCustom(clearBass, bands);
        transport_->revokeSetting();
        sony::protocol::EqualizerState actual;
        for (int attempt = 0; attempt < 12; ++attempt) {
            actual = protocol_->getEqualizer();
            const bool matches = actual.clearBass == clearBass && actual.bands == bands;
            {
                std::lock_guard state(stateMutex_);
                state_.equalizer.known(actual);
                if (matches) state_.lastChange = "EQ confirmed";
            }
            if (matches) return;
            if (attempt < 11) std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::lock_guard state(stateMutex_);
        state_.lastChange = "EQ: headphones reported different values than requested";
        throw SettingNotConfirmed("The headphones reported different EQ values than requested.");
    } catch (const SettingNotConfirmed&) { throw;
    } catch (...) { commandFailed(); throw; }
}

void DeviceSession::setSpeakToChat(bool enabled) { setToggle(false, enabled); }
void DeviceSession::setDsee(bool enabled) { setToggle(true, enabled); }

void DeviceSession::setToggle(bool dsee, bool enabled) {
    std::lock_guard operation(operationMutex_);
    const auto snapshot = requireControls();
    const auto& current = dsee ? snapshot.dsee : snapshot.speakToChat;
    if (!current.value || (!dsee && snapshot.model != Model::Xm5))
        throw sony::SonyException(sony::SonyErrorCode::Unsupported, "The headphones did not report this feature.");
    const std::string name = dsee ? (snapshot.model == Model::Xm5 ? "DSEE Extreme" : "DSEE") : "Speak-to-Chat";
    std::vector<std::uint8_t> payload = dsee
        ? std::vector<std::uint8_t>{0xe8, 0x01, static_cast<std::uint8_t>(enabled)}
        : std::vector<std::uint8_t>{0xf8, 0x0c, static_cast<std::uint8_t>(enabled ? 0 : 1), 0x01};
    try {
        transport_->permitSetting(std::move(payload));
        if (dsee) protocol_->setDsee(enabled); else protocol_->setSpeakToChat(enabled); // Waits for ACK; never retries.
        transport_->revokeSetting();
        for (int attempt = 0; attempt < 12; ++attempt) {
            const bool actual = dsee ? protocol_->getDsee() : protocol_->getSpeakToChat();
            {
                std::lock_guard state(stateMutex_);
                (dsee ? state_.dsee : state_.speakToChat).known(actual);
                if (actual == enabled) { state_.lastChange = name + (enabled ? " on" : " off") + " confirmed"; return; }
            }
            if (attempt < 11) std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        std::lock_guard state(stateMutex_);
        state_.lastChange = name + ": headphones reported the opposite setting";
        throw SettingNotConfirmed("The headphones kept " + name + " " + (enabled ? "off" : "on") + ".");
    } catch (const SettingNotConfirmed&) { throw;
    } catch (...) { commandFailed(); throw; }
}

Snapshot DeviceSession::state() const {
    std::lock_guard state(stateMutex_);
    return state_;
}

void DeviceSession::refreshBattery() {
    std::lock_guard operation(operationMutex_);
    Snapshot next = state();
    if (!session_ || !session_->isConnected()) {
        std::lock_guard state(stateMutex_);
        clearTelemetry();
        state_.connection = ConnectionState::Unavailable;
        state_.connectionError = "The headphone control connection was lost.";
        return;
    }
    try {
        auto battery = protocol_->getBattery();
        if (battery.main && *battery.main >= 0 && *battery.main <= 100) next.battery.known(*battery.main);
        else next.battery.unknown("Battery percentage was not reported.");
        if (battery.charging.has_value()) next.charging.known(*battery.charging);
        else next.charging.unknown("Charging state was not reported.");
    } catch (const std::exception& error) {
        next.battery.unknown(error.what()); next.charging.unknown(error.what());
    }
    std::lock_guard state(stateMutex_);
    state_ = std::move(next);
    if (!session_->isConnected()) {
        clearTelemetry();
        state_.connection = ConnectionState::Unavailable;
        state_.connectionError = "The headphone control connection was lost.";
    }
}
} // namespace headsetdesk
