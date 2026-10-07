#pragma once
#include "DeviceDiscovery.h"
#include "ReadOnlyTransport.h"

namespace headsetdesk {
Candidate simulatedCandidate(Model model);
std::unique_ptr<ReadOnlyTransport> makeSimulatedTransport(Model model, PacketSink packets = {});
} // namespace headsetdesk
