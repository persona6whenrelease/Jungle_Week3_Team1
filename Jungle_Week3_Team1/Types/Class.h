#pragma once
#include "../CoreMinimal.h"

class UClass
{
public:
	FString Name;
	uint64 ClassSize;
	UClass* Parent;
};
