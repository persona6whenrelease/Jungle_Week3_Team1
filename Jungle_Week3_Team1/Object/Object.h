#pragma once
#include "../Types/Class.h"

class UObject
{
public:
	UObject() = default;
	virtual ~UObject() = default;

public:
	FString UUID = { 0 };
};

extern TArray<UObject*> GUObjectArray;