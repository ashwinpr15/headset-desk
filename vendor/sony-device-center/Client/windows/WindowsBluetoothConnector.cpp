// Adapted from sony-device-center (MIT). See vendor LICENSE and IMPORT-MANIFEST.json.
#include "WindowsBluetoothConnector.h"
#include <limits>
#include <memory>

namespace {
struct CloseRadio { void operator()(void* h) const noexcept { if (h) CloseHandle(h); } };
struct CloseRadioSearch { void operator()(void* h) const noexcept { if (h) BluetoothFindRadioClose(h); } };
struct CloseDeviceSearch { void operator()(void* h) const noexcept { if (h) BluetoothFindDeviceClose(h); } };
struct CloseSocket {
    SOCKET value;
    ~CloseSocket() { if (value != INVALID_SOCKET) closesocket(value); }
};
}

WindowsBluetoothConnector::WindowsBluetoothConnector(std::optional<SonyProtocolVersion> requiredVersion)
    : _requiredVersion(requiredVersion) {
    WSADATA data{};
    const int error = WSAStartup(MAKEWORD(2, 2), &data);
    if (error) throw std::runtime_error("WSAStartup failed: " + std::to_string(error));
}
WindowsBluetoothConnector::~WindowsBluetoothConnector() { disconnect(); WSACleanup(); }

bool WindowsBluetoothConnector::_tryConnect(const char* uuid, SOCKADDR_BTH& address) {
    const auto result = UuidFromStringA(reinterpret_cast<RPC_CSTR>(const_cast<char*>(uuid)), &address.serviceClassId);
    if (result != RPC_S_OK) throw std::runtime_error("Invalid Sony service UUID: " + std::to_string(result));
    u_long nonblocking = 1;
    if (ioctlsocket(_socket, FIONBIO, &nonblocking)) throw std::runtime_error("Could not configure Bluetooth socket");
    int error = 0;
    if (::connect(_socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
            fd_set writable, failures;
            FD_ZERO(&writable); FD_ZERO(&failures);
            FD_SET(_socket, &writable); FD_SET(_socket, &failures);
            timeval deadline{8, 0};
            const int ready = select(0, nullptr, &writable, &failures, &deadline);
            if (!ready) error = WSAETIMEDOUT;
            else if (ready == SOCKET_ERROR) error = WSAGetLastError();
            else {
                int length = sizeof(error);
                if (getsockopt(_socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &length)) error = WSAGetLastError();
            }
        }
    }
    nonblocking = 0;
    if (ioctlsocket(_socket, FIONBIO, &nonblocking)) throw std::runtime_error("Could not restore blocking Bluetooth socket");
    WSASetLastError(error);
    return error == 0;
}

void WindowsBluetoothConnector::connect(const std::string& addressText) {
    disconnect();
    _protocolVersion = SonyProtocolVersion::V1;
    SOCKADDR_BTH address{};
    address.addressFamily = AF_BTH;
    address.btAddr = MACStringToLong(addressText);
    try {
        // Headset Desk supplies V2 explicitly. Generic upstream adapters keep
        // their V1-then-V2 fallback, but known models never attempt V1 here.
        if (!_requiredVersion || *_requiredVersion == SonyProtocolVersion::V1) {
            _initSocket();
            if (_tryConnect(SONY_UUID, address)) { _connected = true; return; }
            disconnect();
        }
        if (!_requiredVersion || *_requiredVersion == SonyProtocolVersion::V2) {
            _initSocket();
            if (_tryConnect(SONY_UUID_V2, address)) {
                _protocolVersion = SonyProtocolVersion::V2;
                _connected = true;
                return;
            }
        }
        const auto error = WSAGetLastError();
        throw RecoverableException("The Sony control service could not be reached (Windows error " +
            std::to_string(error) + "). Make sure the paired headphones are powered on and connected.", true);
    } catch (...) { disconnect(); throw; }
}

int WindowsBluetoothConnector::send(char* bytes, size_t length) {
    if (length > static_cast<size_t>(std::numeric_limits<int>::max())) throw std::runtime_error("Send buffer too large");
    const auto result = ::send(_socket, bytes, static_cast<int>(length), 0);
    if (result == SOCKET_ERROR) throw RecoverableException("Bluetooth send failed (Windows error " + std::to_string(WSAGetLastError()) + ")", true);
    return result;
}
int WindowsBluetoothConnector::recv(char* bytes, size_t length) {
    if (length > static_cast<size_t>(std::numeric_limits<int>::max())) throw std::runtime_error("Receive buffer too large");
    const auto result = ::recv(_socket, bytes, static_cast<int>(length), 0);
    if (result == SOCKET_ERROR) {
        const int error = WSAGetLastError();
        if (error == WSAETIMEDOUT) throw RecoverableException("Bluetooth receive timed out", false);
        throw RecoverableException("Bluetooth receive failed (Windows error " + std::to_string(error) + ")", true);
    }
    return result;
}
void WindowsBluetoothConnector::disconnect() noexcept {
    _connected = false;
    if (_socket != INVALID_SOCKET) {
        shutdown(_socket, SD_BOTH); closesocket(_socket); _socket = INVALID_SOCKET;
    }
}
bool WindowsBluetoothConnector::isConnected() noexcept { return _connected; }
SonyProtocolVersion WindowsBluetoothConnector::getProtocolVersion() noexcept { return _protocolVersion; }

void WindowsBluetoothConnector::_initSocket() {
    CloseSocket socket{::socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM)};
    if (socket.value == INVALID_SOCKET) throw std::runtime_error("Could not create Bluetooth socket: " + std::to_string(WSAGetLastError()));
    ULONG enabled = TRUE;
    for (const int option : {static_cast<int>(SO_BTH_AUTHENTICATE), static_cast<int>(SO_BTH_ENCRYPT)}) {
        if (setsockopt(socket.value, SOL_RFCOMM, option, reinterpret_cast<char*>(&enabled), sizeof(enabled)))
            throw std::runtime_error("Could not configure Bluetooth security: " + std::to_string(WSAGetLastError()));
    }
    const DWORD receiveDeadline = 2500, sendDeadline = 8000;
    if (setsockopt(socket.value, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&receiveDeadline), sizeof(receiveDeadline)) ||
        setsockopt(socket.value, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sendDeadline), sizeof(sendDeadline)))
        throw std::runtime_error("Could not configure Bluetooth deadlines: " + std::to_string(WSAGetLastError()));
    _socket = socket.value; socket.value = INVALID_SOCKET;
}

BLUETOOTH_DEVICE_SEARCH_PARAMS WindowsBluetoothConnector::pairedDeviceSearchParams(HANDLE radio) {
    BLUETOOTH_DEVICE_SEARCH_PARAMS p{};
    p.dwSize = sizeof(p);
    p.fReturnAuthenticated = TRUE;
    p.fReturnRemembered = TRUE;
    p.fReturnUnknown = FALSE;
    p.fReturnConnected = TRUE;
    p.fIssueInquiry = FALSE;
    p.hRadio = radio;
    return p;
}
std::vector<BluetoothDevice> WindowsBluetoothConnector::getConnectedDevices() {
    // Adapter method name retained; includes paired-but-disconnected devices.
    BLUETOOTH_FIND_RADIO_PARAMS parameters{sizeof(parameters)};
    HANDLE rawRadio = nullptr;
    auto* rawSearch = BluetoothFindFirstRadio(&parameters, &rawRadio);
    if (!rawSearch) {
        const auto error = GetLastError();
        if (error == ERROR_NO_MORE_ITEMS) return {};
        throw std::runtime_error("Bluetooth adapter enumeration failed: " + std::to_string(error));
    }
    std::unique_ptr<void, CloseRadioSearch> search(rawSearch);
    std::vector<BluetoothDevice> result;
    do {
        std::unique_ptr<void, CloseRadio> radio(rawRadio);
        auto deviceParameters = pairedDeviceSearchParams(rawRadio);
        auto devices = _findDevicesInRadio(&deviceParameters);
        result.insert(result.end(), devices.begin(), devices.end());
    } while (BluetoothFindNextRadio(rawSearch, &rawRadio));
    const auto error = GetLastError();
    if (error != ERROR_NO_MORE_ITEMS && error != ERROR_SUCCESS)
        throw std::runtime_error("Bluetooth adapter enumeration failed: " + std::to_string(error));
    return result;
}
std::vector<BluetoothDevice> WindowsBluetoothConnector::_findDevicesInRadio(BLUETOOTH_DEVICE_SEARCH_PARAMS* parameters) {
    BLUETOOTH_DEVICE_INFO info{};
    info.dwSize = sizeof(info);
    auto* rawSearch = BluetoothFindFirstDevice(parameters, &info);
    if (!rawSearch) {
        const auto error = GetLastError();
        if (error == ERROR_NO_MORE_ITEMS) return {};
        throw std::runtime_error("Paired headphone enumeration failed: " + std::to_string(error));
    }
    std::unique_ptr<void, CloseDeviceSearch> search(rawSearch);
    std::vector<BluetoothDevice> result;
    do {
        result.push_back({_wstringToUtf8(info.szName), MACBytesToString(info.Address.rgBytes),
            info.fAuthenticated != FALSE, info.fConnected != FALSE});
    } while (BluetoothFindNextDevice(rawSearch, &info));
    const auto error = GetLastError();
    if (error != ERROR_NO_MORE_ITEMS && error != ERROR_SUCCESS)
        throw std::runtime_error("Paired headphone enumeration failed: " + std::to_string(error));
    return result;
}
std::string WindowsBluetoothConnector::_wstringToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (!count) throw std::runtime_error("Could not read headphone name");
    std::string result(count, '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), count, nullptr, nullptr);
    return result;
}
