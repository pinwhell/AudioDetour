#include <Windows.h>
#include <Utility.h>
#include <dll_injector.h>
#include <MinHook.h>
#include <thread>
//#include <MMDetours.h>

HMODULE hThisMod;

HRESULT(*oCoCreateInstance)(
	REFCLSID  rclsid,
	LPUNKNOWN pUnkOuter,
	DWORD     dwClsContext,
	REFIID    riid,
	LPVOID* ppv
	);

HRESULT WINAPI hCoCreateInstance(
	REFCLSID  rclsid,
	LPUNKNOWN pUnkOuter,
	DWORD     dwClsContext,
	REFIID    riid,
	LPVOID* ppv
)
{
	HRESULT hRes = oCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

	if (CALLER_IN_MODULE(hThisMod) || FAILED(hRes))
		return hRes;

	/*if (IsEqualGUID(__uuidof(MMDeviceEnumerator), rclsid))
	{
		*ppv = new DetourMultiMediaDevEnumer((IMMDeviceEnumerator*)*ppv);
		return S_OK;
	}*/

	return S_OK;
}

NTSTATUS(NTAPI* oZwCreateuserProcess)(
	PHANDLE ProcessHandle,
	PHANDLE ThreadHandle,
	ACCESS_MASK ProcessDesiredAccess,
	ACCESS_MASK ThreadDesiredAccess,
	LPVOID ProcessObjectAttributes,
	LPVOID ThreadObjectAttributes,
	ULONG ProcessFlags,
	ULONG ThreadFlags,
	LPVOID ProcessParameters,
	PVOID CreateInfo,
	PVOID AttributeList
	);

decltype(oZwCreateuserProcess) ZwCreateuserProcess;

NTSTATUS NTAPI hZwCreateuserProcess(
	PHANDLE ProcessHandle,
	PHANDLE ThreadHandle,
	ACCESS_MASK ProcessDesiredAccess,
	ACCESS_MASK ThreadDesiredAccess,
	LPVOID ProcessObjectAttributes,
	LPVOID ThreadObjectAttributes,
	ULONG ProcessFlags,
	ULONG ThreadFlags,
	LPVOID ProcessParameters,
	PVOID CreateInfo,
	PVOID AttributeList
)
{
	NTSTATUS res = oZwCreateuserProcess(
		ProcessHandle,
		ThreadHandle,
		ProcessDesiredAccess,
		ThreadDesiredAccess,
		ProcessObjectAttributes,
		ThreadObjectAttributes,
		ProcessFlags,
		ThreadFlags,
		ProcessParameters,
		CreateInfo,
		AttributeList
	);

	DWORD pid = GetProcessId(*ProcessHandle);

	std::thread([pid] {
		Sleep(250);
		injectDLL(pid, GetModulePath(hThisMod).c_str());
		}).detach();

	return res;
}

//HRESULT (STDMETHODCALLTYPE* oActivate)(
//	IMMDevice* thiz,
//	REFIID iid,
//	DWORD dwClsCtx,
//	PROPVARIANT* pActivationParams,
//	void** ppInterface);
//
//
//HRESULT STDMETHODCALLTYPE hActivate(
//	IMMDevice* thiz,
//	REFIID iid,
//	DWORD dwClsCtx,
//	PROPVARIANT* pActivationParams,
//	void** ppInterface)
//{
//	MessageBoxA(NULL, ("hActivate" + std::to_string((int)GetCurrentProcessId())).c_str(), NULL, MB_OK);
//
//	const auto activate = [&](IMMDevice* dev = nullptr) {
//		return oActivate(dev ? dev : thiz, iid, dwClsCtx, pActivationParams, ppInterface);
//		};
//
//	return activate(AskUserMultiMediaDevice());
//}

BOOL CALLBACK DllMain(HMODULE hMod, DWORD loadReason)
{
	//static VTable immDeviceVTBL = IMMDeviceVTableGet().value();

	hThisMod = hMod;

	if (loadReason == DLL_PROCESS_ATTACH)
	{
		MH_Initialize();

		// Hook
		ZwCreateuserProcess = LibrarySymbolAddress<decltype(ZwCreateuserProcess)>("ntdll.dll", "ZwCreateUserProcess").value();
		MH_CreateHook(CoCreateInstance, hCoCreateInstance, (LPVOID*)&oCoCreateInstance);
		MH_EnableHook(CoCreateInstance);
		MH_CreateHook(ZwCreateuserProcess, hZwCreateuserProcess, (LPVOID*)&oZwCreateuserProcess);
		MH_EnableHook(ZwCreateuserProcess);

		//immDeviceVTBL.Replace(EVTBLIMMDevice::Activate, hActivate, oActivate);
	}

	if (loadReason == DLL_PROCESS_DETACH)
	{
		//immDeviceVTBL.Restore(EVTBLIMMDevice::Activate);

		MH_DisableHook(ZwCreateuserProcess);
		MH_DisableHook(CoCreateInstance);

		// Unhook

		MH_Uninitialize();
	}

	return TRUE;
}