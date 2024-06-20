#pragma once

#include <cstdint>
#include <Windows.h>
#include <VTable.h>
#include <mmdeviceapi.h>
#include <atomic>
#include <optional>
#include <mutex>

enum EVTBLIMMDevice {
	QueryInterface,
	AddRef,
	Release,
	Activate,
	OpenPropertyStore,
	GetId,
	GetState
};

enum class EVTBLIMMDeviceCollection {
	QueryInterface,
	AddRef,
	Release,
	GetCount,
	Item
};

class DetourMultiMediaDevCollection : public IMMDeviceCollection {
public:
	DetourMultiMediaDevCollection(IMMDeviceCollection* orig);
	~DetourMultiMediaDevCollection();

	operator bool();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef(void) override;
	ULONG STDMETHODCALLTYPE Release(void) override;
	HRESULT STDMETHODCALLTYPE GetCount(UINT* pcDevices) override;
	HRESULT STDMETHODCALLTYPE Item(UINT nDevice, IMMDevice** ppDevice) override;

	IMMDeviceCollection* mOriginal;
	std::atomic_int mRef;
	static IMMDeviceCollection* mUserSelectedDeviceCollection;
	static std::optional<size_t>	mUserSelectedDeviceId;
	static std::mutex				mUserSelectedDeviceIdMtx;
};

class DetourMultiMediaDevEnumer : public IMMDeviceEnumerator {
public:
	DetourMultiMediaDevEnumer(IMMDeviceEnumerator* original);
	~DetourMultiMediaDevEnumer();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
	ULONG STDMETHODCALLTYPE AddRef(void) override;
	ULONG STDMETHODCALLTYPE Release(void) override;
	HRESULT STDMETHODCALLTYPE EnumAudioEndpoints(EDataFlow dataFlow, DWORD dwStateMask, IMMDeviceCollection** ppDevices) override;
	HRESULT STDMETHODCALLTYPE GetDefaultAudioEndpoint(EDataFlow dataFlow, ERole role, IMMDevice** ppEndpoint) override;
	HRESULT STDMETHODCALLTYPE GetDevice(LPCWSTR pwstrId, IMMDevice** ppDevice) override;
	HRESULT STDMETHODCALLTYPE RegisterEndpointNotificationCallback(IMMNotificationClient* pClient) override;
	HRESULT STDMETHODCALLTYPE UnregisterEndpointNotificationCallback(IMMNotificationClient* pClient) override;

	IMMDeviceEnumerator* mOriginal;
	std::atomic_int mRef;
};

std::optional<VTable> IMMDeviceVTableGet();
std::optional<size_t> FromDeviceCollectionAskUserChoice(IMMDeviceCollection* deviceCollection);
IMMDevice* AskUserMultiMediaDevice();