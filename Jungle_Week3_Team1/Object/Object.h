#pragma once

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

public:
	uint32 UUID = { 0 };
};

extern TArray<UObject*> GUObjectArray;