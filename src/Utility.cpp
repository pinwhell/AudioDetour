#include <Utility.h>
#include <Psapi.h>

std::string GetModulePath(HMODULE hModule) {
	char path[MAX_PATH];
	DWORD length = GetModuleFileNameA(hModule, path, MAX_PATH);
	if (length == 0) {
		// Handle error (e.g., call GetLastError())
		return "";
	}
	return std::string(path, length);
}

bool IsAddressInModule(HMODULE hModule, LPVOID _lpAddress) {

	LPCVOID lpAddress = (LPCVOID)_lpAddress;

	if (hModule == NULL || lpAddress == NULL) {
		return false;
	}

	// Get module information
	MODULEINFO moduleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo))) {
		return false;
	}

	// Calculate the module's address range
	BYTE* moduleBase = static_cast<BYTE*>(moduleInfo.lpBaseOfDll);
	BYTE* moduleEnd = moduleBase + moduleInfo.SizeOfImage;

	// Check if the address is within the module's range
	BYTE* address = static_cast<BYTE*>(const_cast<LPVOID>(lpAddress));
	return address >= moduleBase && address < moduleEnd;
}

BOOL PatchMemory(LPVOID lpAddress, LPVOID lpPatch, SIZE_T patchSize) {
	DWORD oldProtect;

	// Change the protection to allow write access
	if (!VirtualProtect(lpAddress, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		printf("Failed to change memory protection. Error: %lu\n", GetLastError());
		return FALSE;
	}

	// Copy the patch into the memory region
	memcpy(lpAddress, lpPatch, patchSize);

	// Restore the original protection
	if (!VirtualProtect(lpAddress, patchSize, oldProtect, &oldProtect)) {
		printf("Failed to restore memory protection. Error: %lu\n", GetLastError());
		return FALSE;
	}

	return TRUE;
}