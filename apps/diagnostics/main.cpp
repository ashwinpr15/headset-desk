#include "headsetdesk/DeviceSession.h"
#include "headsetdesk/Simulation.h"
#include "sony/transport/PlatformTransport.h"
#include "sony/transport/Logger.h"
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string_view>

namespace {
using namespace headsetdesk;
void usage() {
    std::cout << "Headset Desk: internal read-only diagnostics (Phase 2)\n"
        "  --simulate xm5|ch720n [--dump FILE]   Offline synthetic data\n"
        "  --devices                           List supported paired headphones\n"
        "  --device-index N --read-only [--dump FILE]\n"
        "                                      Open that device for reviewed queries\n"
        "No arguments: shows this help. No Bluetooth connection is opened.\n";
}
template<class T> void field(std::string_view name, const Telemetry<T>& data) {
    std::cout << name << ": ";
    if (data.availability == Availability::Known && data.value) std::cout << std::boolalpha << *data.value;
    else std::cout << (data.availability == Availability::Unsupported ? "Unsupported" : "Unknown");
    std::cout << '\n';
    if (!data.error.empty()) std::cout << "  Details: " << data.error << '\n';
}
void show(const Snapshot& snapshot, bool simulated) {
    std::cout << (simulated ? "SIMULATION - synthetic values, no hardware evidence\n" : "READ-ONLY HARDWARE DIAGNOSTICS\n");
    std::cout << "Model (paired Windows name): " << (snapshot.model ? profileFor(*snapshot.model).name : "Unknown") << '\n';
    field("Battery %", snapshot.battery); field("Charging", snapshot.charging);
    field("Firmware", snapshot.firmware); field("Reported active codec", snapshot.activeCodec);
    std::cout << "Noise control: ";
    if (snapshot.noise.value) {
        const auto& noise = *snapshot.noise.value;
        std::cout << (noise.mode == sony::protocol::NoiseControlMode::NoiseCancelling ? "NC" :
                      noise.mode == sony::protocol::NoiseControlMode::Ambient ? "Ambient" : "Off");
        if (noise.mode == sony::protocol::NoiseControlMode::Ambient) std::cout << " " << noise.ambientLevel;
    } else std::cout << "Unknown";
    std::cout << "\nEQ: ";
    if (snapshot.equalizer.value) {
        for (const auto value : snapshot.equalizer.value->bands) std::cout << value << ' ';
        std::cout << "Clear Bass " << snapshot.equalizer.value->clearBass;
    } else std::cout << "Unknown";
    std::cout << "\nWrites: disabled\n";
    if (!snapshot.connectionError.empty()) std::cout << "Connection: " << snapshot.connectionError << '\n';
    if (snapshot.model) std::cout << "Read evidence: reviewed hardware capture on firmware "
        << profileFor(*snapshot.model).capabilities.front().testedFirmware
        << "; observed states only. This run does not update verification status.\n";
}
}

int main(int argc, char** argv) {
    using namespace headsetdesk;
    try {
        sony::Logger::setLogLevel(sony::LogLevel::Off);
        bool list = false, readOnly = false;
        std::optional<Model> simulation;
        std::optional<std::size_t> index;
        std::filesystem::path dumpPath;
        for (int i = 1; i < argc; ++i) {
            const std::string_view arg = argv[i];
            auto argument = [&]() -> std::string_view {
                if (++i >= argc) throw std::invalid_argument("Missing argument");
                return argv[i];
            };
            if (arg == "--help") { usage(); return 0; }
            if (arg == "--devices") list = true;
            else if (arg == "--read-only") readOnly = true;
            else if (arg == "--simulate") {
                const auto name = argument();
                if (name == "xm5") simulation = Model::Xm5;
                else if (name == "ch720n") simulation = Model::Ch720n;
                else throw std::invalid_argument("Simulation model must be xm5 or ch720n");
            } else if (arg == "--device-index") {
                const auto number = argument();
                std::size_t value = 0;
                const auto result = std::from_chars(number.data(), number.data() + number.size(), value);
                if (result.ec != std::errc{} || result.ptr != number.data() + number.size() || value == 0)
                    throw std::invalid_argument("Device index must be a positive whole number");
                index = value;
            } else if (arg == "--dump") dumpPath = std::filesystem::path(std::string(argument()));
            else throw std::invalid_argument("Unrecognized diagnostic option");
        }
        if (argc == 1) { usage(); return 0; }
        const int modes = static_cast<int>(list) + static_cast<int>(index.has_value()) + static_cast<int>(simulation.has_value());
        if (modes != 1 || (list && (readOnly || !dumpPath.empty())) || (simulation && readOnly))
            throw std::invalid_argument("Choose exactly one mode: devices, simulation, or read-only device-index");
        if (index && !readOnly) throw std::invalid_argument("Hardware queries require --read-only");
        std::vector<Candidate> candidates;
        if (!simulation) {
            auto discovery = sony::transport::createPlatformDiscovery();
            candidates = supportedPairedDevices(*discovery);
        }
        if (list) {
            if (candidates.empty()) std::cout << "No supported paired headphones found. Pair them in Windows Bluetooth settings.\n";
            for (std::size_t n = 0; n < candidates.size(); ++n) {
                std::cout << n + 1 << ". " << profileFor(candidates[n].model).name << " - "
                    << (candidates[n].device.connected == true ? "active" : "paired; not active") << '\n';
            }
            return 0;
        }
        if (index && *index > candidates.size()) throw std::invalid_argument("Device index was not found; run --devices again");
        Candidate candidate = simulation ? simulatedCandidate(*simulation) : candidates[*index - 1];
        std::ofstream capture;
        std::mutex captureMutex;
        PacketSink sink;
        if (!dumpPath.empty()) {
            if (std::filesystem::exists(dumpPath)) throw std::invalid_argument("Capture file already exists; use a new file name");
            capture.open(dumpPath, std::ios::out);
            if (!capture) throw std::runtime_error("Could not create packet capture");
            capture << "Headset Desk Phase 2\nmode=" << (simulation ? "SIMULATION" : "READ_ONLY_HARDWARE")
                << " model=" << profileFor(candidate.model).name << "\n"
                << "Addresses and machine/account information are omitted. RX records are stream chunks.\n"
                << "TX records are validated send attempts; receipt by hardware is not implied.\n";
            const auto start = std::chrono::steady_clock::now();
            sink = [&, start](std::string_view direction, std::span<const std::byte> bytes) {
                std::lock_guard lock(captureMutex);
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
                capture << std::dec << elapsed << "ms " << direction << ' ' << bytes.size() << "B ";
                for (auto b : bytes) capture << std::hex << std::setw(2) << std::setfill('0') << std::to_integer<unsigned int>(b) << ' ';
                capture << '\n'; capture.flush();
                if (!capture) throw std::runtime_error("Packet capture failed");
            };
        }
        DeviceSession device(simulation ? makeSimulatedTransport(*simulation, sink) : makeWindowsReadOnlyTransport(sink));
        device.connect(candidate);
        device.refresh();
        const auto snapshot = device.state();
        show(snapshot, simulation.has_value());
        device.disconnect();
        return snapshot.connection == ConnectionState::Connected ? 0 : 2;
    } catch (const std::exception& error) {
        std::cerr << "Headset Desk diagnostics: " << error.what() << '\n';
        return 1;
    }
}
