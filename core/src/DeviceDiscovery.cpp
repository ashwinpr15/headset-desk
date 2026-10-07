#include "headsetdesk/DeviceDiscovery.h"
#include <algorithm>
#include <set>
#include <tuple>

namespace headsetdesk {
std::vector<Candidate> supportedPairedDevices(sony::transport::IDeviceDiscovery& discovery) {
    std::vector<Candidate> result;
    std::set<std::string> addresses;
    for (const auto& device : discovery.discover()) {
        auto model = identifyModel(device.name);
        if (!model || device.paired != true || device.address.str().empty()) continue;
        if (addresses.insert(device.address.str()).second) result.push_back({*model, device});
    }
    std::sort(result.begin(), result.end(), [](const Candidate& a, const Candidate& b) {
        return std::tie(a.model, a.device.address.str()) < std::tie(b.model, b.device.address.str());
    });
    return result;
}

std::optional<Candidate> startupTarget(const std::vector<Candidate>& candidates,
                                      std::string_view preferredAddress) {
    std::optional<Candidate> selected;
    for (const auto& candidate : candidates) {
        if (candidate.device.paired != true || candidate.device.connected != true) continue;
        if (candidate.device.address.str() == preferredAddress) return candidate;
        if (!selected || std::tie(candidate.model, candidate.device.address.str()) <
                         std::tie(selected->model, selected->device.address.str())) selected = candidate;
    }
    return selected;
}
} // namespace headsetdesk
