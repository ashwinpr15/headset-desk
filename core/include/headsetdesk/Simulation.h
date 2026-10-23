#pragma once
#include "DeviceDiscovery.h"
#include "ReadOnlyTransport.h"
#include "sony/protocol/SemanticTypes.h"
#include <functional>

namespace headsetdesk {
enum class SimulationFailure { None, IgnoreSettings, DisconnectOnSetting, NoSettingAck };
Candidate simulatedCandidate(Model model);
// pressNoiseButton, when given, receives a function that acts like the headset's NC/Ambient
// button: the simulated headset switches mode by itself and sends its 69 17 notification.
std::unique_ptr<ReadOnlyTransport> makeSimulatedTransport(Model model, PacketSink packets = {},
    SimulationFailure failure = SimulationFailure::None,
    std::function<void(sony::protocol::NoiseControlMode)>* pressNoiseButton = nullptr);
} // namespace headsetdesk
