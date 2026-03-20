#pragma once
#include "../CoreMinimal.h"

class UObject : public UObjectBase
{
	DECLARE_OBJECT(UObject, UObjectBase)

public:
	UObject() = default;
	virtual ~UObject() = default;

	bool IsA(UClass* TargetClass) const
	{
		UClass* cur = GetClass();
		while (cur)
		{
			if (cur == TargetClass) return true;
			cur = cur->Parent;
		}
		return false;
	}

	virtual void Release() {}

public:
	uint32 UUID = { 0 };
	bool bPendingKill = false;
};

extern TArray<UObject*> GUObjectArray;