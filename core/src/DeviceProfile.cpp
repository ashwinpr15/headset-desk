#include "headsetdesk/DeviceProfile.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace headsetdesk {
namespace {
constexpr std::string_view xm5Capture = "tests/fixtures/xm5-readonly.txt; observed states only; docs/hardware-validation.md";
constexpr std::string_view chCapture = "tests/fixtures/ch720n-readonly.txt; observed states only; docs/hardware-validation.md";
constexpr std::array<std::string_view, 3> xm5Codecs{"SBC", "AAC", "LDAC"};
constexpr std::array<std::string_view, 2> chCodecs{"SBC", "AAC"};
constexpr std::array xm5Capabilities{
    Capability{Feature::Battery, Confidence::Verified, Evidence::OurXm5Hardware, xm5Capture, "2.5.1", Confidence::Unsupported},
    Capability{Feature::Charging, Confidence::Verified, Evidence::OurXm5Hardware, xm5Capture, "2.5.1", Confidence::Unsupported},
    Capability{Feature::Firmware, Confidence::Verified, Evidence::OurXm5Hardware, xm5Capture, "2.5.1", Confidence::Unsupported},
    Capability{Feature::ActiveCodec, Confidence::Verified, Evidence::OurXm5Hardware, xm5Capture, "2.5.1", Confidence::Unsupported},
    Capability{Feature::NoiseControl, Confidence::Verified, Evidence::OurXm5Hardware, xm5Capture, "2.5.1", Confidence::Unknown},
    Capability{Feature::Equalizer, Confidence::Verified, Evidence::OurXm5Hardware, xm5Capture, "2.5.1", Confidence::Unknown},
};
constexpr std::array chCapabilities{
    Capability{Feature::Battery, Confidence::Verified, Evidence::OurCh720nHardware, chCapture, "1.1.4", Confidence::Unsupported},
    Capability{Feature::Charging, Confidence::Verified, Evidence::OurCh720nHardware, chCapture, "1.1.4", Confidence::Unsupported},
    Capability{Feature::Firmware, Confidence::Verified, Evidence::OurCh720nHardware, chCapture, "1.1.4", Confidence::Unsupported},
    Capability{Feature::ActiveCodec, Confidence::Verified, Evidence::OurCh720nHardware, chCapture, "1.1.4", Confidence::Unsupported},
    Capability{Feature::NoiseControl, Confidence::Verified, Evidence::OurCh720nHardware, chCapture, "1.1.4", Confidence::Unknown},
    Capability{Feature::Equalizer, Confidence::Verified, Evidence::OurCh720nHardware, chCapture, "1.1.4", Confidence::Unknown},
};
const DeviceProfile xm5{Model::Xm5, "WH-1000XM5", xm5Codecs, xm5Capabilities};
const DeviceProfile ch720n{Model::Ch720n, "WH-CH720N", chCodecs, chCapabilities};
} // namespace

const DeviceProfile& profileFor(Model model) {
    return model == Model::Xm5 ? xm5 : ch720n;
}

std::optional<Model> identifyModel(std::string_view name) {
    std::string normalized(name);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    // Exact names: a substring or a familiar MAC prefix is not sufficient to
    // choose a protocol generation. Renamed/unknown devices stay idle.
    if (normalized == "WH-1000XM5") return Model::Xm5;
    if (normalized == "WH-CH720N") return Model::Ch720n;
    return std::nullopt;
}

std::string_view toString(Confidence value) {
    switch (value) {
    case Confidence::Verified: return "VERIFIED";
    case Confidence::Probable: return "PROBABLE";
    case Confidence::Unknown: return "UNKNOWN";
    case Confidence::Unsupported: return "UNSUPPORTED";
    }
    return "UNKNOWN";
}

std::string_view toString(Evidence value) {
    switch (value) {
    case Evidence::OurXm5Hardware: return "OUR-XM5-HARDWARE-VERIFIED";
    case Evidence::OurCh720nHardware: return "OUR-CH720N-HARDWARE-VERIFIED";
    case Evidence::UpstreamHardware: return "UPSTREAM-HARDWARE-VERIFIED";
    case Evidence::UpstreamCode: return "UPSTREAM-CODE-ONLY";
    case Evidence::StaticSpecification: return "STATIC-SPECIFICATION";
    case Evidence::None: return "NONE";
    }
    return "NONE";
}
} // namespace headsetdesk
