#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <optional>
#include <intrin.h>

std::string GetModulePath(HMODULE hModule);
bool IsAddressInModule(HMODULE hModule, LPVOID _lpAddress);
BOOL PatchMemory(LPVOID lpAddress, LPVOID lpPatch, SIZE_T patchSize);

template<typename T>
bool IsAddressInModule(HMODULE hModule, T _lpAddress)
{
	return IsAddressInModule(hModule, (LPVOID)_lpAddress);
}

template<typename T, typename K>
BOOL Patch(T addr, const K& obj)
{
	return PatchMemory((LPVOID)addr, (LPVOID)&obj, sizeof(K));
}

template<typename AddrT = uint64_t>
std::optional<AddrT> LibraryOffsetAddress(const char* libName, size_t offset)
{
	auto hLib = GetModuleHandleA(libName);

	if (hLib == 0)
		return {};

	return (AddrT)((uint64_t)hLib + offset);
}

template<typename AddrT = uint64_t>
std::optional<AddrT> LibrarySymbolAddress(const char* libName, const char* symbolName)
{
	auto hLib = GetModuleHandleA(libName);

	if (hLib == 0)
		return {};

	if (FARPROC sym = GetProcAddress(hLib, symbolName))
		return (AddrT)sym;

	return {};
}

#define CALLER_IN_MODULE(hMod) IsAddressInModule(hMod, _ReturnAddress())