#pragma once
#include <stdio.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "Bthprops.lib")
#endif
#include <winsock2.h>
#include <ws2bth.h>
#include <BluetoothAPIs.h>
#include <iostream>
#include <rpc.h>
#include "IBluetoothConnector.h"
#include <string>
#include "ByteMagic.h"
#include <atomic>

class WindowsBluetoothConnector final : public IBluetoothConnector
{
public:
	explicit WindowsBluetoothConnector(std::optional<SonyProtocolVersion> requiredVersion = std::nullopt);

	~WindowsBluetoothConnector();

	virtual void connect(const std::string& addrStr) noexcept(false);
	virtual int send(char* buf, size_t length) noexcept(false);
	virtual int recv(char* buf, size_t length) noexcept(false);
	virtual void disconnect() noexcept;
	virtual bool isConnected() noexcept;

	virtual std::vector<BluetoothDevice> getConnectedDevices() noexcept(false);
	virtual SonyProtocolVersion getProtocolVersion() noexcept;

	// Cached devices; no inquiry or pairing. Verified by offline policy tests.
	static BLUETOOTH_DEVICE_SEARCH_PARAMS pairedDeviceSearchParams(HANDLE radio);

private:
	std::vector<BluetoothDevice> _findDevicesInRadio(BLUETOOTH_DEVICE_SEARCH_PARAMS* searchParams);
	std::string _wstringToUtf8(const std::wstring& wstr);
	bool _tryConnect(const char* uuid, SOCKADDR_BTH& sab);

	SOCKET _socket = INVALID_SOCKET;
	std::atomic<bool> _connected = false;
	SonyProtocolVersion _protocolVersion = SonyProtocolVersion::V1;
	std::optional<SonyProtocolVersion> _requiredVersion;
	void _initSocket();
};
