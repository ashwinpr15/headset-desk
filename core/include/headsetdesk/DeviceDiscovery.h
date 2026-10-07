#pragma once

#include "DeviceProfile.h"
#include "sony/transport/IDeviceDiscovery.h"
#include <optional>
#include <vector>

namespace headsetdesk {
struct Candidate {
    Model model;
    sony::transport::DiscoveredDevice device;
};

std::vector<Candidate> supportedPairedDevices(sony::transport::IDeviceDiscovery& discovery);
// No active device => no connection attempt. Two active devices => preferred
// address if supplied, otherwise XM5 first, with stable address ordering.
std::optional<Candidate> startupTarget(const std::vector<Candidate>& candidates,
                                      std::string_view preferredAddress = {});
} // namespace headsetdesk
