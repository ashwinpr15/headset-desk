#pragma once
#include "DeviceDiscovery.h"
#include "ReadOnlyTransport.h"

namespace headsetdesk {
enum class SimulationFailure { None, IgnoreSettings, DisconnectOnSetting, NoSettingAck };
Candidate simulatedCandidate(Model model);
std::unique_ptr<ReadOnlyTransport> makeSimulatedTransport(Model model, PacketSink packets = {},
    SimulationFailure failure = SimulationFailure::None);
} // namespace headsetdesk
