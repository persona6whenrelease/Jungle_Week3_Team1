#pragma once
#include "../CoreMinimal.h"

class UObject
{
public:
	UObject() = default;
	virtual ~UObject() = default;

public:
	FString UUID = { 0 };
};

extern TArray<UObject*> GUObjectArray;