#pragma once
#include "../CoreMinimal.h"

struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FMeshData
{
	TArray<FVertex> Vertices;
	TArray<uint32> Indices;
};
