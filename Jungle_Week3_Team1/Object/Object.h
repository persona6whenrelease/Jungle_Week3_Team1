#pragma once
#include "../Types/Class.h"


class UObject
{
public:
	UObject() = default;
	virtual ~UObject() = default;

public:
	uint32 UUID = { 0 };
};

extern TArray<UObject*> GUObjectArray;