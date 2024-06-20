#include <MMDetours.h>
#include <future>
#include <functiondiscoverykeys_devpkey.h> // for PKEY_Device_FriendlyName
#include <string>
#include <algorithm>
#include <iterator>
#include <SimpleWidget.h>
#include <atlbase.h>

IMMDeviceCollection* DetourMultiMediaDevCollection::mUserSelectedDeviceCollection = nullptr;
std::optional<size_t>	DetourMultiMediaDevCollection::mUserSelectedDeviceId = {};
std::mutex				DetourMultiMediaDevCollection::mUserSelectedDeviceIdMtx;

DetourMultiMediaDevCollection::DetourMultiMediaDevCollection(IMMDeviceCollection* orig)
	: mOriginal(orig)
	, mRef(1)
{
	std::lock_guard<std::mutex> lck(mUserSelectedDeviceIdMtx);

	if (mUserSelectedDeviceId)
		return;

	mUserSelectedDeviceId = FromDeviceCollectionAskUserChoice(mOriginal);

	if (!mUserSelectedDeviceId.has_value())
		return;

	(mUserSelectedDeviceCollection = mOriginal)->AddRef();
}

DetourMultiMediaDevCollection::~DetourMultiMediaDevCollection()
{
	mOriginal->Release();
}

DetourMultiMediaDevCollection::operator bool()
{
	return mUserSelectedDeviceId.has_value();
}

HRESULT __stdcall DetourMultiMediaDevCollection::QueryInterface(REFIID riid, void** ppvObject)
{
	return mOriginal->QueryInterface(riid, ppvObject);
}

ULONG __stdcall DetourMultiMediaDevCollection::AddRef(void)
{
	return ++mRef;
}

ULONG __stdcall DetourMultiMediaDevCollection::Release(void)
{
	if (--mRef > 0)
		return mRef;

	delete this;

	return 0;
}

HRESULT __stdcall DetourMultiMediaDevCollection::GetCount(UINT* pcDevices)
{
	if (*this)
	{
		*pcDevices = 1;
		return S_OK;
	}

	return mOriginal->GetCount(pcDevices);
}

HRESULT __stdcall DetourMultiMediaDevCollection::Item(UINT nDevice, IMMDevice** ppDevice)
{
	if (*this)
		return mUserSelectedDeviceCollection->Item(mUserSelectedDeviceId.value(), ppDevice);

	return mOriginal->Item(nDevice, ppDevice);
}

DetourMultiMediaDevEnumer::DetourMultiMediaDevEnumer(IMMDeviceEnumerator* original)
	: mOriginal(original)
	, mRef(1)
{}

DetourMultiMediaDevEnumer::~DetourMultiMediaDevEnumer()
{
	mOriginal->Release();
}

HRESULT __stdcall DetourMultiMediaDevEnumer::QueryInterface(REFIID riid, void** ppvObject)
{
	return mOriginal->QueryInterface(riid, ppvObject);
}

ULONG __stdcall DetourMultiMediaDevEnumer::AddRef(void)
{
	return ++mRef;
}

ULONG __stdcall DetourMultiMediaDevEnumer::Release(void)
{
	if (--mRef > 0)
		return mRef;

	delete this;

	return 0;
}

HRESULT __stdcall DetourMultiMediaDevEnumer::EnumAudioEndpoints(EDataFlow dataFlow, DWORD dwStateMask, IMMDeviceCollection** ppDevices)
{
	HRESULT hRes = mOriginal->EnumAudioEndpoints(dataFlow, dwStateMask, ppDevices);
	if (FAILED(hRes))
		return hRes;

	*ppDevices = new DetourMultiMediaDevCollection(*ppDevices);

	return S_OK;
}

HRESULT __stdcall DetourMultiMediaDevEnumer::GetDefaultAudioEndpoint(EDataFlow dataFlow, ERole role, IMMDevice** ppEndpoint)
{
	const auto def = [&] {
		return mOriginal->GetDefaultAudioEndpoint(dataFlow, role, ppEndpoint);
		};

	IMMDeviceCollection* pDevices;
	if (FAILED(DetourMultiMediaDevEnumer::EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &pDevices)))
		return def();

	DetourMultiMediaDevCollection* pDetour = (DetourMultiMediaDevCollection*)pDevices;

	if (!(*pDetour) || FAILED(pDetour->Item(0, ppEndpoint)))
	{
		pDetour->Release();
		return def();
	}

	pDetour->Release();

	return S_OK;
}

HRESULT __stdcall DetourMultiMediaDevEnumer::GetDevice(LPCWSTR pwstrId, IMMDevice** ppDevice)
{
	const auto def = [&] {
		return mOriginal->GetDevice(pwstrId, ppDevice);
		};

	IMMDeviceCollection* pDevices;
	if (FAILED(DetourMultiMediaDevEnumer::EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices)))
		return def();

	DetourMultiMediaDevCollection* pDetour = (DetourMultiMediaDevCollection*)pDevices;

	if (!(*pDetour) || FAILED(pDetour->Item(0, ppDevice)))
	{
		pDetour->Release();
		return def();
	}

	pDetour->Release();

	return S_OK;
}

HRESULT __stdcall DetourMultiMediaDevEnumer::RegisterEndpointNotificationCallback(IMMNotificationClient* pClient)
{
	return mOriginal->RegisterEndpointNotificationCallback(pClient);
}

HRESULT __stdcall DetourMultiMediaDevEnumer::UnregisterEndpointNotificationCallback(IMMNotificationClient* pClient)
{
	return mOriginal->UnregisterEndpointNotificationCallback(pClient);
}

std::optional<size_t> FromDeviceCollectionAskUserChoice(IMMDeviceCollection* deviceCollection)
{
	std::packaged_task<std::optional<size_t>()> tsk([deviceCollection]() -> std::optional<size_t> {
		UINT count = 0;

		if (FAILED(deviceCollection->GetCount(&count)))
			return {};

		std::vector<std::string> devNames;

		for (size_t i = 0; i < count; i++)
		{
			IMMDevice* currDev = nullptr;

			if (FAILED(deviceCollection->Item(i, &currDev)))
				continue;

			IPropertyStore* propertyStore = NULL;

			if (FAILED(currDev->OpenPropertyStore(STGM_READ, &propertyStore)))
			{
				currDev->Release();
				continue;
			}

			PROPVARIANT prop;
			PropVariantInit(&prop);

			if (FAILED(propertyStore->GetValue(PKEY_Device_FriendlyName, &prop)))
			{
				currDev->Release();
				propertyStore->Release();
				continue;
			}

			std::wstring wstr(prop.pwszVal, prop.pwszVal + lstrlenW(prop.pwszVal));
			devNames.push_back(std::to_string(i) + "|" + std::string(wstr.begin(), wstr.end()));

			PropVariantClear(&prop);
			propertyStore->Release();
			currDev->Release();
		}

		std::vector<const char*> currDevNames;
		std::transform(devNames.begin(), devNames.end(), std::back_inserter(currDevNames), [](const std::string& str) { return str.c_str(); });

		auto optionChoosed = AskChooseOption(currDevNames.data(), currDevNames.size());

		if (optionChoosed.has_value() == false)
			return {};

		std::string fullChoosenStr = devNames[optionChoosed.value()];
		return std::atoll(fullChoosenStr.substr(0, fullChoosenStr.find('|')).c_str());
		});

	std::future<std::optional<size_t>> result = tsk.get_future();
	std::thread t(std::move(tsk));
	result.wait();
	if (t.joinable())
		t.join();
	return result.get();
}

std::optional<VTable> IMMDeviceVTableGet()
{
	CComPtr<IMMDeviceEnumerator> pEnumerator = nullptr;
	CComPtr<IMMDevice> pDevice = nullptr;

	// Create device enumerator
	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator)))
		return std::nullopt;

	// Get default audio endpoint
	if (FAILED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, (IMMDevice**) &pDevice)))
		return std::nullopt;

	return VTable::From(pDevice.p);
}

IMMDevice* AskUserMultiMediaDevice()
{
	static CComPtr<IMMDevice> userSelectedDev = nullptr;

	if (userSelectedDev)
		return userSelectedDev;

	IMMDeviceEnumerator* deviceEnumer = nullptr;

	if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumer)))
		return nullptr;

	DetourMultiMediaDevEnumer mediaEnumer(deviceEnumer);

	IMMDeviceCollection* deviceCollection;

	if (FAILED(mediaEnumer.EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection)))
		return nullptr;

	DetourMultiMediaDevCollection deviceDetourCollection(deviceCollection);

	if (!deviceDetourCollection)
		return nullptr;

	if (FAILED(deviceDetourCollection.Item(0, &userSelectedDev)))
		return nullptr;

	return userSelectedDev;
}

