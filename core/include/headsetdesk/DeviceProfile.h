#pragma once

#include <array>
#include <optional>
#include <span>
#include <string_view>

namespace headsetdesk {

enum class Model { Xm5, Ch720n };
enum class Confidence { Verified, Probable, Unknown, Unsupported };
enum class Evidence { OurXm5Hardware, OurCh720nHardware, UpstreamHardware, UpstreamCode, StaticSpecification, None };
enum class Feature { Battery, Charging, Firmware, ActiveCodec, NoiseControl, Equalizer };

struct Capability {
    Feature feature;
    Confidence readConfidence;
    Evidence evidence;
    std::string_view reference;
    std::string_view testedFirmware;
    // Hardware write evidence is independent of read evidence and runtime
    // experimental opt-in. The default policy never enables setters.
    Confidence writeConfidence;
    bool writesEnabled{false};
};

struct DeviceProfile {
    Model model;
    std::string_view name;
    std::span<const std::string_view> supportedCodecs;
    std::span<const Capability> capabilities;
};

const DeviceProfile& profileFor(Model model);
std::optional<Model> identifyModel(std::string_view pairedDeviceName);
std::string_view toString(Confidence confidence);
std::string_view toString(Evidence evidence);

} // namespace headsetdesk
