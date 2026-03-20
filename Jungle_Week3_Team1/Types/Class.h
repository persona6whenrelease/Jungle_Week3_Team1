#pragma once
#include "../Types/String.h"

class UClass
{
public:
	FString Name;
	uint64 ClassSize;
	UClass* Parent;
};
