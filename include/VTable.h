#pragma once

#include <cstdint>
#include <unordered_map>

class VTable {
public:
	VTable(void* vtableEntry);
	VTable(VTable&& other) noexcept = default;
	VTable(VTable& other) = delete;

	template<typename T>
	void Apply(T& to)
	{
		Apply<T>(&to);
	}

	template<typename T>
	void Apply(T* to)
	{
		*(void**)to = mRawEntry;
	}

	template<typename T>
	static VTable From(T* obj)
	{
		return VTable(VtblFrom(obj));
	}

	template<typename T>
	static void* VtblFrom(T* obj)
	{
		return *(void**)obj;
	}

	template<typename M>
	bool AlreadyReplaced(M methodId)
	{
		return mBackupEntries.find((size_t)methodId) != mBackupEntries.end();
	}

	template<typename M, typename R, typename T>
	bool Replace(M methodId, R with, T& outBackup)
	{
		if (AlreadyReplaced(methodId))
			Restore(methodId);

		mBackupEntries[methodId] = (void*)(outBackup = (T) * ((void**)mRawEntry + (size_t)methodId));

		return Patch((void**)mRawEntry + (size_t)methodId, with);
	}

	template<typename M>
	bool Restore(M methodId)
	{
		if (mBackupEntries.find((size_t)methodId) == mBackupEntries.end())
			return false;

		BOOL res = Patch((void**)mRawEntry + (size_t)methodId, mBackupEntries[(size_t)methodId]);

		mBackupEntries.erase((size_t)methodId);

		return res;
	}

	void* mRawEntry;
	std::unordered_map<size_t, void*> mBackupEntries;
};