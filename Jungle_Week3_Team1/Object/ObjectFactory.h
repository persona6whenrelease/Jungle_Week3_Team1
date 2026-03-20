#pragma once
#include "Object.h"

class UObjectFactory
{
public:
	static uint32 GetUUID()
	{
		return NextUUID++;
	}

	static void SetUUID(uint32 UUID)
	{
		NextUUID = UUID;
	}

	template<typename T>
	static T* NewObject()
	{
		static_assert(std::is_base_of_v<UObject, T>, "T must derive from UObject");

		T* Obj = new T;
		if (!bIsLoading)
		{
			Obj->UUID = GetUUID();
		}
		GUObjectArray.push_back(Obj);

		return Obj;
	}

	static void DestroyObject(UObject* Obj);
	static void FlushPendingKill();

public:
	static uint32 NextUUID;
	static bool bIsLoading;
};

template<typename T>
T* NewObject()
{
	return UObjectFactory::NewObject<T>();
}